/*
 * Legacy Tape — main Arduino sketch
 * Target: Elecrow CrowPanel Advance 5" HMI · ESP32-S3-WROOM-1-N16R8 · 800x480 IPS · GT911 single-touch
 *
 * This file owns the boot sequence, the LVGL display/touch glue, the backlight
 * controller handshake, the transport-key poller and the main loop. Feature
 * modules sit next to it:
 *   pairing.*        device ID, pairing token, QR URL, stored WiFi credentials (NVS)
 *   pairing_ble.*    BLE GATT service the companion app uses to send WiFi credentials
 *   cloud_sync.*     polls the backend until the app reports onboarding complete
 *   audio_record.*   PDM mic capture into two 10 s PSRAM chunk buffers
 *   audio_upload.*   core-0 task that uploads chunks and finalizes each recording
 *   audio_playback.* streams a chapter's recordings from the backend to the speaker
 *   book.*           book name + chapter list (NVS)
 *   ui_*.c           LVGL screens (1-3 started in SquareLine Studio, 4-14 hand-written)
 *
 * Hardware bindings (pins also in LovyanGFX_Driver.h and the audio modules):
 *   Display:    RGB parallel (16 data + HSYNC/VSYNC/DE/PCLK), framebuffers in PSRAM.
 *   Touch:      GT911 single-touch on Wire (SDA=15, SCL=16) @ 400 kHz, addr 0x5D.
 *               Power-up sequence: GPIO 1 OUTPUT LOW 120 ms, then INPUT (selects 0x5D).
 *   I2C bus:    TCA9534 IO expander (0x18, V1.0 backlight enable) ·
 *               STC8H1K28 backlight/audio µC (0x30, V1.1/V1.2) ·
 *               MCP23017 IO expander (0x20, transport keys) · GT911 touch (0x5D).
 *   Buttons:    5-key mechanical interlock switch — REC · PLAY · RWD · FF · STOP.
 *               Wired to MCP23017 port A pins 0-4 (active LOW with internal pull-ups).
 *               Polled every 20 ms with software debounce.
 *   Mic:        PDM microphone, CLK=19, DATA=20, 16 kHz mono; unmuted through the 0x30 µC.
 *   Speaker:    I2S output to the panel's speaker amplifier (BCLK=5, LRC=6, DOUT=4).
 *   Backlight:  TCA9534 pin 1 HIGH (V1.0) AND I2C 0x30 brightness command (V1.1/V1.2). Both
 *               attempted — whichever IC is on your board responds, the other harmlessly NACKs.
 */

#include "LovyanGFX_Driver.h"
#include <lvgl.h>
#include <Wire.h>
#include <WiFi.h>
#include "ui.h"
#include "pins_config.h"
#include "pairing.h"
#include "pairing_ble.h"
#include "cloud_sync.h"
#include "book.h"
#include "audio_record.h"
#include "audio_upload.h"

// No I/O-expander libraries: every I2C peripheral (backlight µC, MCP23017) is
// driven with raw register reads/writes through Wire.

// ─── DEV MODE ────────────────────────────────────────────────────────────────
// LT_DEV_MODE=1 builds a UI-only firmware for walking every screen ON-DEVICE
// WITHOUT the app: boot skips pairing/BLE/cloud/WiFi, lands on Screen1, and
// overlays ‹ / › arrows on LVGL's top layer (so they float above every screen
// and survive screen changes) to step through all 14 screens. In dev mode
// Screen5/Screen7 do NOT start real recording/playback — pure UI so screens can
// be restyled freely. Production builds use the default (0). Enable it from the
// build instead of editing this file: `pio run -e crowpanel-dev`, or add
// -DLT_DEV_MODE=1 to the compiler flags.
#ifndef LT_DEV_MODE
#define LT_DEV_MODE 0
#endif
extern "C" { int g_dev_mode = LT_DEV_MODE; }

LGFX gfx;

// ─── LVGL plumbing ──────────────────────────────────────────────────────────
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;
static uint16_t touch_x = 0, touch_y = 0;

static void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    // Factory pattern: gfx.startWrite() is held open after setup, so we must
    // close the existing transaction before pushing DMA pixels.
    if (gfx.getStartCount() > 0) {
        gfx.endWrite();
    }
    gfx.pushImageDMA(area->x1, area->y1,
                     area->x2 - area->x1 + 1,
                     area->y2 - area->y1 + 1,
                     (lgfx::rgb565_t *)&color_p->full);
    lv_disp_flush_ready(disp);
}

static void touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data) {
    bool touched = gfx.getTouch(&touch_x, &touch_y);
    if (touched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_x;
        data->point.y = touch_y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ─── Backlight init: V1.1 protocol (also works on V1.0 / V1.2) ────────────
// V1.1 requires a two-step dance:
//   1. The STC8H1K28 µC at I2C 0x30 boots slower than the ESP32.
//      We must wait for both the µC (0x30) and GT911 touch (0x5D) to ACK on I2C.
//      While waiting, we send command 0x19 to the µC to kick it into a responsive state.
//   2. Once the µC is up, send byte 0x10 to 0x30 → max brightness backlight ON.
//
// V1.0 hardware uses TCA9534 @ 0x18 for backlight rail enable — also done here harmlessly.
static void sendI2CCommand(uint8_t command) {
    Wire.beginTransmission(0x30);
    Wire.write(command);
    Wire.endTransmission();
}

static bool i2cScanForAddress(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

static void backlight_init() {
    // V1.0 path: TCA9534 IO expander — drive pins 1, 2, 4 HIGH (backlight rail + reset)
    Wire.beginTransmission(0x18); Wire.write(0x03); Wire.write(0xE1); Wire.endTransmission();
    Wire.beginTransmission(0x18); Wire.write(0x01); Wire.write(0x16); Wire.endTransmission();

    // V1.1 + V1.2 path: wake the STC8H1K28 µC, wait for it + GT911 to come online,
    // then send max-brightness command. Up to ~5 second timeout.
    int tries = 0;
    while (tries < 50) {
        if (i2cScanForAddress(0x30) && i2cScanForAddress(0x5D)) {
            Serial.println("[LegacyTape] microcontroller (0x30) + touch (0x5D) ready");
            break;
        }
        Serial.printf("[LegacyTape] waking microcontroller (try %d)\n", tries);
        sendI2CCommand(0x19);                  // wake the µC
        pinMode(1, OUTPUT);                    // GT911 reset sequence
        digitalWrite(1, LOW);
        delay(120);
        pinMode(1, INPUT);
        delay(100);
        tries++;
    }

    sendI2CCommand(0x10);                      // backlight ON, max brightness
}

// V1.1 valid range: 0x05 (off) .. 0x10 (max). V1.2 valid range: 0 (max) .. 244 (min), 245 (off).
static void backlight_set(uint8_t level) {
    sendI2CCommand(level);
}

// ─── MCP23017 IO expander (0x20) — Uxcell piano interlock buttons on PORTA pins 0-4 ──
#define MCP_ADDR     0x20
#define MCP_IODIRA   0x00
#define MCP_GPPUA    0x0C
#define MCP_GPIOA    0x12

#define BTN_REC  0x01    // MCP A0
#define BTN_PLAY 0x02    // MCP A1
#define BTN_RWD  0x04    // MCP A2
#define BTN_FF   0x08    // MCP A3
#define BTN_STOP 0x10    // MCP A4
#define BTN_MASK 0x1F    // pins 0-4

static uint8_t mcp_write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MCP_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission();
}

static uint8_t mcp_read_reg(uint8_t reg) {
    Wire.beginTransmission(MCP_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return 0xFF;   // no MCP on bus → return "no buttons"
    Wire.requestFrom((uint8_t)MCP_ADDR, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0xFF;
}

// Set once at boot: is the MCP23017 button board actually on the bus? If not,
// buttons_poll() must NOT keep hitting 0x20 — every probe is an I2C transaction
// on the SAME bus as the GT911 touch (0x5D), and that contention causes ghost /
// dropped touches that read as random "glitches" when tapping. A board wired
// up later is picked up on the next boot.
static bool g_mcp_present = false;

static void buttons_init() {
    g_mcp_present = i2cScanForAddress(MCP_ADDR);
    Serial.printf("[buttons] MCP23017 %s\n", g_mcp_present ? "present" : "not wired — polling disabled");
    if (!g_mcp_present) return;
    // Pins 0-4 as input (1 bits in IODIR = input)
    mcp_write_reg(MCP_IODIRA, BTN_MASK);
    // Internal pull-ups on the same pins
    mcp_write_reg(MCP_GPPUA, BTN_MASK);
}

// State-machine: read raw button state, debounce, dispatch transitions on the falling edge
// (i.e. the moment a button locks down — the mechanical interlock guarantees only one is held).
static lv_obj_t *current_active() {
    return lv_disp_get_scr_act(lv_disp_get_default());
}

static void on_button_pressed(uint8_t btn) {
    lv_obj_t *cur = current_active();
    switch (btn) {
        case BTN_REC:
            if (cur == ui_Screen4 || cur == ui_Screen6 || cur == ui_Screen12)
                _ui_screen_change(&ui_Screen5, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen5_screen_init);
            break;
        case BTN_PLAY:
            if (cur == ui_Screen4 || cur == ui_Screen6)
                _ui_screen_change(&ui_Screen7, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen7_screen_init);
            break;
        case BTN_STOP:
            // Same routines as the on-screen STOP buttons, so the key really
            // stops (and finalizes) a recording or stops playback.
            if (cur == ui_Screen5)
                ui_Screen5_stop();
            else if (cur == ui_Screen7)
                ui_Screen7_stop();
            else if (cur == ui_Screen9 || cur == ui_Screen13)   // dev-only mock-up screens
                _ui_screen_change(&ui_Screen6, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen6_screen_init);
            break;
        case BTN_RWD:
        case BTN_FF:
            // Meant to move the playback position; no screen change. Not implemented
            // yet: seeking needs HTTP Range requests in audio_playback (see README).
            break;
    }
}

static void buttons_poll() {
    if (!g_mcp_present) return;               // no button board → leave the I2C bus to touch
    static uint8_t last_state = BTN_MASK;     // all high = nothing pressed
    static uint32_t last_change_ms = 0;
    static uint32_t last_poll_ms = 0;
    const uint32_t DEBOUNCE_MS = 20;
    const uint32_t POLL_INTERVAL_MS = 20;     // 50 Hz, plenty for buttons

    // CRITICAL: the MCP23017 (0x20) shares the I2C bus with the GT911 touch
    // controller (0x5D) on pins 15/16. The main loop runs ~1000x/sec; if we
    // hit the MCP every iteration, that's ~1000 I2C transactions/sec — and
    // when no MCP is wired they all NACK but still occupy the bus, starving
    // the touch reads and making the screen feel dead/laggy. Rate-limit to
    // 50 Hz so touch has the bus the rest of the time.
    uint32_t now = millis();
    if (now - last_poll_ms < POLL_INTERVAL_MS) return;
    last_poll_ms = now;

    uint8_t raw = mcp_read_reg(MCP_GPIOA);
    if (raw == 0xFF) return;                  // bus error / MCP not present
    uint8_t state = raw & BTN_MASK;

    if (state == last_state) return;
    if (millis() - last_change_ms < DEBOUNCE_MS) return;
    last_change_ms = millis();

    // Detect falling edges: bits that went from 1 (released) to 0 (pressed)
    uint8_t falling = last_state & ~state;
    last_state = state;

    if (falling & BTN_REC)  on_button_pressed(BTN_REC);
    if (falling & BTN_PLAY) on_button_pressed(BTN_PLAY);
    if (falling & BTN_RWD)  on_button_pressed(BTN_RWD);
    if (falling & BTN_FF)   on_button_pressed(BTN_FF);
    if (falling & BTN_STOP) on_button_pressed(BTN_STOP);
}

// ─── Onboarding nav glue (Screen1 → Screen2 → Screen3 → Screen4) ───────────
// Wires the existing SquareLine screens without modifying the generated files.
static void s3_kb_done(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        // Capture whatever the user typed in the textarea and persist it
        // so Screen4's cassette label shows the real book name.
        if (ui_TextArea1) {
            const char *typed = lv_textarea_get_text(ui_TextArea1);
            if (typed && strlen(typed) > 0) {
                book_set_name(typed);
            }
        }
        _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, &ui_Screen4_screen_init);
    }
}

static void wire_existing_screens() {
    // NOTE: Screen1 used to have a "tap anywhere to advance" shortcut (s1_advance)
    // for dev testing. Removed — it meant picking up the device registered a
    // touch and walked it into onboarding. Screen1 now only advances via the
    // real pairing flow (cloud onboarding_complete poll -> Screen2).
    if (ui_Screen3 && ui_Keyboard2) {
        lv_keyboard_set_textarea(ui_Keyboard2, ui_TextArea1);
        lv_obj_add_event_cb(ui_Keyboard2, s3_kb_done, LV_EVENT_ALL, NULL);
    }
}

// ─── Setup / loop ──────────────────────────────────────────────────────────
// Build the cream QR pairing card on Screen1 (used by the normal first-run
// pairing flow and by dev mode so Screen1 shows its real content for styling).
static void build_pairing_card() {
    if (!ui_Screen1) return;
    // Hide the static QR-card placeholder image — we draw our own card + QR.
    if (ui_Image1) lv_obj_add_flag(ui_Image1, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *card = lv_obj_create(ui_Screen1);
    lv_obj_set_size(card, 280, 320);
    lv_obj_align(card, LV_ALIGN_RIGHT_MID, -30, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xF6ECD4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(card, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(card, lv_color_hex(0x33405C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(card, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *qr = lv_qrcode_create(card, 210,
                                    lv_color_hex(0x2A1A12),
                                    lv_color_hex(0xF6ECD4));
    lv_obj_align(qr, LV_ALIGN_TOP_MID, 0, 16);
    const char *url = pairing_get_qr_url();
    lv_qrcode_update(qr, url, strlen(url));

    lv_obj_t *cap = lv_label_create(card);
    lv_obj_align(cap, LV_ALIGN_TOP_MID, 0, 250);
    lv_label_set_text(cap, "SCAN WITH APP");
    lv_obj_set_style_text_color(cap, lv_color_hex(0x2A1A12), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cap, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(cap, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *did = lv_label_create(card);
    lv_obj_align(did, LV_ALIGN_TOP_MID, 0, 278);
    lv_label_set_text(did, pairing_get_device_id());
    lv_obj_set_style_text_color(did, lv_color_hex(0x33405C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(did, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(did, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
}

#if LT_DEV_MODE
// ─── DEV MODE on-screen navigator ────────────────────────────────────────────
// A ‹ S/N name › pill on lv_layer_top() that steps through every screen.
typedef struct { lv_obj_t **scr; void (*init)(void); const char *name; } dev_screen_t;
static const dev_screen_t DEV_SCREENS[] = {
    {&ui_Screen1,  ui_Screen1_screen_init,  "Pairing"},
    {&ui_Screen2,  ui_Screen2_screen_init,  "Setup OK"},
    {&ui_Screen3,  ui_Screen3_screen_init,  "Name Book"},
    {&ui_Screen4,  ui_Screen4_screen_init,  "Ready"},
    {&ui_Screen5,  ui_Screen5_screen_init,  "Recording"},
    {&ui_Screen6,  ui_Screen6_screen_init,  "Stopped"},
    {&ui_Screen7,  ui_Screen7_screen_init,  "Playback"},
    {&ui_Screen8,  ui_Screen8_screen_init,  "Books"},
    {&ui_Screen9,  ui_Screen9_screen_init,  "AI Notes"},
    {&ui_Screen10, ui_Screen10_screen_init, "Chapters"},
    {&ui_Screen11, ui_Screen11_screen_init, "Volume"},
    {&ui_Screen12, ui_Screen12_screen_init, "Error"},
    {&ui_Screen13, ui_Screen13_screen_init, "Chapter+"},
    {&ui_Screen14, ui_Screen14_screen_init, "Settings"},
};
static const int DEV_SCREEN_COUNT = sizeof(DEV_SCREENS) / sizeof(DEV_SCREENS[0]);
static int       g_dev_idx   = 0;
static lv_obj_t *g_dev_label = NULL;

static void dev_goto(int idx) {
    if (idx < 0) idx = DEV_SCREEN_COUNT - 1;
    if (idx >= DEV_SCREEN_COUNT) idx = 0;
    g_dev_idx = idx;
    const dev_screen_t *d = &DEV_SCREENS[idx];
    _ui_screen_change(d->scr, LV_SCR_LOAD_ANIM_NONE, 1, 0, d->init);   // spd=1 -> smooth fade (so dev stepping shows the transition)
    if (g_dev_label) {
        char buf[40];
        snprintf(buf, sizeof(buf), "S%d  %s", idx + 1, d->name);
        lv_label_set_text(g_dev_label, buf);
    }
    Serial.printf("[dev] -> Screen%d (%s)\n", idx + 1, d->name);
}

static void dev_prev_cb(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) dev_goto(g_dev_idx - 1); }
static void dev_next_cb(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) dev_goto(g_dev_idx + 1); }

static lv_obj_t *dev_make_arrow(lv_obj_t *parent, const char *sym, lv_event_cb_t cb) {
    lv_obj_t *b = lv_btn_create(parent);
    lv_obj_set_size(b, 46, 30);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x241A12), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x4A3428), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(b, lv_color_hex(0xC89060), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(b, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(b, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, sym);   // LV_SYMBOL_* uses the default font (has glyphs)
    lv_obj_set_style_text_color(l, lv_color_hex(0xF6ECD4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, NULL);
    return b;
}

static void dev_overlay_create(void) {
    // The top layer floats above every screen and persists across screen loads,
    // so the navigator stays put no matter which screen is showing.
    lv_obj_t *top = lv_layer_top();
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bar = lv_obj_create(top);
    lv_obj_set_size(bar, 250, 38);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x0E0A08), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bar, 215, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bar, lv_color_hex(0xC89060), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(bar, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *prev = dev_make_arrow(bar, LV_SYMBOL_LEFT, dev_prev_cb);
    lv_obj_align(prev, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *next = dev_make_arrow(bar, LV_SYMBOL_RIGHT, dev_next_cb);
    lv_obj_align(next, LV_ALIGN_RIGHT_MID, 0, 0);

    g_dev_label = lv_label_create(bar);
    lv_obj_align(g_dev_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(g_dev_label, lv_color_hex(0xE8C9A0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(g_dev_label, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(g_dev_label, "S1  Pairing");
}
#endif  // LT_DEV_MODE

void setup() {
    Serial.begin(115200);
    Serial.println("[LegacyTape] booting");

    // PDM mic clock pin (GPIO 19) must be set as output before I2C (factory code does this)
    pinMode(19, OUTPUT);

    // I2C up first so touch + backlight + MCP23017 can ACK.
    Wire.begin(15, 16);
    Wire.setClock(400000);
    delay(50);

    // Keep WiFi sticky: auto-reconnect if it drops, and don't sleep the radio
    // (power save interferes with both scans and staying associated). Without
    // this, a brief drop after pairing (BLE teardown / coexistence) left the
    // device offline and recordings failed to upload.
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.setSleep(false);

    // Wake the backlight µC, wait for it + GT911, then enable backlight.
    // (GT911 reset on GPIO 1 happens inside backlight_init's wait loop.)
    backlight_init();
    buttons_init();

    // Display & touch (RGB parallel + GT911 via LovyanGFX).
    gfx.init();
    gfx.initDMA();
    gfx.startWrite();
    gfx.fillScreen(TFT_BLACK);

    // LVGL init + framebuffers in PSRAM (8 MB available, 1.5 MB double buffer is fine).
    lv_init();
    buf1 = (lv_color_t *)heap_caps_malloc(LCD_H_RES * LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t *)heap_caps_malloc(LCD_H_RES * LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    if (!buf1 || !buf2) {
        Serial.println("[LegacyTape] PSRAM alloc failed! Enable OPI PSRAM in board settings.");
        while (1) delay(100);
    }
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_H_RES * LCD_V_RES);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    // RGB panels + many widgets: full_refresh pushes one complete frame per cycle
    // instead of many small DMA bursts that overlap with the panel's scanout and
    // cause visible tearing. Both buffers are full-screen sized, so this works.
    disp_drv.full_refresh = 1;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();
    wire_existing_screens();

    // Load book name + chapter list from NVS before anything renders, so
    // Screen4's cassette + Screen10's chapter list show real data.
    book_begin();

    // Allocate the recording PSRAM buffers + unmute mic. Runs once at boot.
    audio_record_begin();
    // Start the background chunk-upload task. Idle until audio_record produces chunks.
    audio_upload_begin();

    // Initialize pairing token + persist it in NVS. Render a live QR code on
    // Screen1 encoding the pairing URL. iOS app scans this and uses BLE to
    // deliver WiFi credentials back to the device.
    pairing_begin();

#if LT_DEV_MODE
    // Dev mode: skip pairing/BLE/cloud/WiFi entirely. Render Screen1 (with its
    // real QR card) and float the ‹ › navigator so every screen is reachable.
    Serial.println("[LegacyTape] DEV MODE — pairing skipped; step screens with the on-screen arrows");
    build_pairing_card();
    _ui_screen_change(&ui_Screen1, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Screen1_screen_init);
    dev_overlay_create();
#else
    if (pairing_is_complete()) {
        // Device is already set up — skip the QR/pairing/onboarding flow
        // entirely and boot straight to the Ready home screen.
        Serial.println("[LegacyTape] already paired -> booting to Ready (Screen4)");

        // CRITICAL: reconnect to WiFi using the credentials stored at pairing.
        // WiFi used to come up during the BLE pairing flow; a paired device
        // that boots straight to Ready never went through that, so it had no
        // internet — recordings captured but every chunk upload silently
        // failed and the device falsely reported "Uploaded". Reconnect here
        // (async; upload/playback/sync all wait for WL_CONNECTED).
        const char *ssid  = pairing_get_wifi_ssid();
        const char *pw    = pairing_get_wifi_pw();
        const char *ssid2 = pairing_get_wifi_ssid2();
        const char *pw2   = pairing_get_wifi_pw2();
        if (strlen(ssid) > 0) {
            WiFi.mode(WIFI_STA);
            WiFi.begin(ssid, pw);
            Serial.printf("[LegacyTape] reconnecting WiFi '%s' (secondary '%s' avail: %s)\n",
                          ssid, ssid2, strlen(ssid2) > 0 ? "yes" : "no");
            // Brief blocking wait so a record-immediately-on-boot still uploads;
            // fall back to secondary network if primary doesn't come up.
            uint32_t t0 = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) { delay(100); }
            if (WiFi.status() != WL_CONNECTED && strlen(ssid2) > 0) {
                Serial.println("[LegacyTape] primary WiFi failed, trying secondary");
                WiFi.begin(ssid2, pw2);
                t0 = millis();
                while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) { delay(100); }
            }
            Serial.printf("[LegacyTape] WiFi %s\n",
                          WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "NOT connected");
        } else {
            Serial.println("[LegacyTape] WARNING: paired but no stored WiFi SSID");
        }

        _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_Screen4_screen_init);
    } else if (ui_Screen1) {
        // First-time setup: advertise the BLE pairing service + render the QR.
        pairing_ble_begin();
        build_pairing_card();
    }
#endif  // LT_DEV_MODE

    Serial.println("[LegacyTape] ready");
}

void loop() {
    // Suspend LVGL refresh while the BLE module is doing WiFi.begin() —
    // RGB panel DMA otherwise contends with WiFi's PSRAM buffers and the
    // screen visibly glitches. Static frame stays on screen for ~5-10s.
    if (!pairing_ble_is_busy()) {
        lv_timer_handler();
    }
    buttons_poll();
    pairing_ble_loop();   // publishes WiFi scan results when ready
    cloud_sync_loop();    // polls Supabase for onboarding_complete (no-op until cloud_sync_begin)

    // pairing_mark_complete() is fired by cloud_sync when Supabase reports
    // onboarding_complete=true (after the iOS app finishes the survey), NOT by
    // BLE 0x03 status. When the event fires, advance Screen1 -> Screen2 and
    // tear down the BLE stack to free ~30 KB RAM for the rest of the UI.
    if (pairing_consume_complete_event()) {
        Serial.println("[LegacyTape] onboarding complete -> advancing to Screen2");
        cloud_sync_stop();
        pairing_ble_stop();
        // ANIM_NONE = instant cut. The MOVE_LEFT animation was visibly laggy
        // because it runs 400ms of LVGL frame redraws right after BLE deinit
        // frees ~30 KB and WiFi is still doing background work in PSRAM.
        _ui_screen_change(&ui_Screen2, LV_SCR_LOAD_ANIM_NONE, 0, 0,
                          &ui_Screen2_screen_init);
    }

    delay(1);
}
