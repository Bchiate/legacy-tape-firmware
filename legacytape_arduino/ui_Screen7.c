#include "ui.h"
#include "ui_widgets.h"
#include "book.h"
#include "audio_playback.h"
#include <stdio.h>

lv_obj_t *ui_Screen7 = NULL;
lv_obj_t *ui_S7_Timer = NULL;
static lv_obj_t   *s7_status   = NULL;
static lv_obj_t   *s7_progress = NULL;
static lv_obj_t   *s7_lamp     = NULL;
static lv_obj_t   *s7_vol_lbl  = NULL;
static lv_obj_t   *s7_banner   = NULL;
static lv_timer_t *s7_ticker   = NULL;

static void s7_vol_refresh(void) {
    if (!s7_vol_lbl) return;
    char b[24];
    snprintf(b, sizeof(b), "VOL  %d%%", audio_playback_get_volume());
    lv_label_set_text(s7_vol_lbl, b);
}
static void s7_vol_down(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    audio_playback_volume_step(-10);
    s7_vol_refresh();
}
static void s7_vol_up(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    audio_playback_volume_step(+10);
    s7_vol_refresh();
}

static void s7_to_chapter(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen10, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen10_screen_init); }
static void s7_to_book(lv_event_t *e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen8,  LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen8_screen_init); }
// STOP: stop playback and return to Ready. Shared by the on-screen STOP
// button and the hardware STOP key (legacytape_arduino.ino).
void ui_Screen7_stop(void) {
    audio_playback_stop();
    _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen4_screen_init);
}
static void s7_stop(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) ui_Screen7_stop();
}

static void fmt_mmss(uint32_t s, char *out, size_t n) {
    snprintf(out, n, "%02u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
}

static void s7_tick(lv_timer_t *t) {
    // Created once, never destroyed — skip work unless Playback is visible so it
    // doesn't keep invalidating widgets (full-screen repaints) on hidden screens.
    if (!s7_status || lv_scr_act() != ui_Screen7) return;
    playback_state_t st = audio_playback_state();
    char buf[64];

    switch (st) {
        case PLAYBACK_FETCHING:
            lv_label_set_text(s7_status, "LOADING");
            break;
        case PLAYBACK_PLAYING: {
            uint32_t pos = audio_playback_position_sec();
            uint32_t dur = audio_playback_duration_sec();
            char a[8], b[8]; fmt_mmss(pos, a, sizeof(a)); fmt_mmss(dur, b, sizeof(b));
            snprintf(buf, sizeof(buf), "%s / %s", a, b);
            if (ui_S7_Timer) lv_label_set_text(ui_S7_Timer, buf);
            lv_label_set_text(s7_status, "PLAYING");
            if (s7_progress && dur > 0)
                lv_obj_set_width(s7_progress, (lv_coord_t)((uint32_t)pos * 780 / dur));
            break;
        }
        case PLAYBACK_DONE:
            lv_label_set_text(s7_status, "FINISHED");
            if (s7_progress) lv_obj_set_width(s7_progress, 780);
            break;
        case PLAYBACK_NONE:
            lv_label_set_text(s7_status, "NO CLIP YET");
            break;
        case PLAYBACK_ERROR:
            lv_label_set_text(s7_status, "ERROR");
            break;
        default:
            lv_label_set_text(s7_status, "");
            break;
    }
}

// Runs on every visit. The screen object is built once and cached by
// _ui_screen_change(), so starting playback cannot live in the init function.
static void s7_on_loaded(lv_event_t *e) {
    (void)e;
    playback_state_t st = audio_playback_state();
    // Playback was left running without STOP (e.g. via the CHAPTER or BOOK
    // picker): keep it going. The banner still shows the chapter that plays.
    if (st == PLAYBACK_PLAYING || st == PLAYBACK_FETCHING) return;

    // New playback of the active chapter: reset the per-playback UI.
    ltw_chapter_banner_refresh(s7_banner);
    if (ui_S7_Timer) lv_label_set_text(ui_S7_Timer, "00:00 / 00:00");
    if (s7_progress) lv_obj_set_width(s7_progress, 0);
    if (s7_status)   lv_label_set_text(s7_status, "LOADING");
    s7_vol_refresh();
    // Skipped in dev mode — the screen is shown for styling only.
    if (!g_dev_mode) audio_playback_start(book_get_active_chapter());
}

void ui_Screen7_screen_init(void) {
    ui_Screen7 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen7, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen7, lv_color_hex(0x161C2A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen7, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ltw_topbar(ui_Screen7, LT_GREEN, "PLAYBACK", "00:00 / 00:00", &s7_lamp, NULL, &ui_S7_Timer);
    lv_obj_t *s7_cass = ltw_cassette_hero(ui_Screen7, 58, NULL, 0, true);   // reels spin

    // Dynamic cassette label = short playback status, on the blank label band.
    s7_status = lv_label_create(s7_cass);
    lv_obj_set_width(s7_status, 320);
    lv_label_set_long_mode(s7_status, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s7_status, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(s7_status, LV_ALIGN_TOP_MID, 0, 31);
    lv_label_set_text(s7_status, "LOADING");
    lv_obj_set_style_text_color(s7_status, lv_color_hex(0x1E7A3A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s7_status, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Playback progress bar along the very bottom.
    lv_obj_t *track = lv_obj_create(ui_Screen7);
    lv_obj_set_size(track, 780, 8);
    lv_obj_set_pos(track, 10, 472);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(track, lv_color_hex(0x0C1322), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(track, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(track, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(track, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(track, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    s7_progress = lv_obj_create(track);
    lv_obj_set_size(s7_progress, 0, 8);
    lv_obj_set_pos(s7_progress, 0, 0);
    lv_obj_set_style_bg_color(s7_progress, lv_color_hex(LT_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s7_progress, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s7_progress, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s7_progress, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ── Transport row: VOL-  [VOL xx%]  VOL+ ............... STOP ──
    // (Replaces the 5-cap hw legend — only volume + stop matter during
    //  playback; RWD/FF aren't wired in v1.)
    const int ROW_Y = 330, BTN_H = 56;

    lv_obj_t *volDown = lv_btn_create(ui_Screen7);
    lv_obj_set_size(volDown, 96, BTN_H);
    lv_obj_set_pos(volDown, 70, ROW_Y);
    lv_obj_set_style_bg_color(volDown, lv_color_hex(0x263250), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(volDown, lv_color_hex(0x35466E), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(volDown, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(volDown, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(volDown, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *vdl = lv_label_create(volDown);
    lv_label_set_text(vdl, "VOL -");
    lv_obj_set_style_text_color(vdl, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(vdl, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(vdl);
    lv_obj_add_event_cb(volDown, s7_vol_down, LV_EVENT_CLICKED, NULL);

    s7_vol_lbl = lv_label_create(ui_Screen7);
    lv_obj_set_size(s7_vol_lbl, 150, 30);
    lv_obj_set_pos(s7_vol_lbl, 178, ROW_Y + 14);
    lv_obj_set_style_text_color(s7_vol_lbl, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(s7_vol_lbl, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(s7_vol_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    s7_vol_refresh();

    lv_obj_t *volUp = lv_btn_create(ui_Screen7);
    lv_obj_set_size(volUp, 96, BTN_H);
    lv_obj_set_pos(volUp, 338, ROW_Y);
    lv_obj_set_style_bg_color(volUp, lv_color_hex(0x263250), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(volUp, lv_color_hex(0x35466E), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(volUp, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(volUp, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(volUp, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *vul = lv_label_create(volUp);
    lv_label_set_text(vul, "VOL +");
    lv_obj_set_style_text_color(vul, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(vul, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(vul);
    lv_obj_add_event_cb(volUp, s7_vol_up, LV_EVENT_CLICKED, NULL);

    lv_obj_t *stopBtn = lv_btn_create(ui_Screen7);
    lv_obj_set_size(stopBtn, 180, BTN_H);
    lv_obj_set_pos(stopBtn, 550, ROW_Y);
    lv_obj_set_style_bg_color(stopBtn, lv_color_hex(LT_RED_CTA), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(stopBtn, lv_color_hex(LT_BURGUNDY), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_width(stopBtn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(stopBtn, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *sbl = lv_label_create(stopBtn);
    lv_label_set_text(sbl, "STOP");
    lv_obj_set_style_text_color(sbl, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(sbl, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(sbl);
    lv_obj_add_event_cb(stopBtn, s7_stop, LV_EVENT_CLICKED, NULL);

    int ach = book_get_active_chapter();
    const char *acn = book_get_chapter_name(ach);
    char chnum[16];
    snprintf(chnum, sizeof(chnum), "CHAPTER %02d", ach + 1);
    s7_banner = ltw_chapter_banner(ui_Screen7, chnum, acn ? acn : "Chapter 1", s7_to_chapter, s7_to_book);

    // Playback starts in s7_on_loaded, on every visit (including this first one).
    lv_obj_add_event_cb(ui_Screen7, s7_on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    if (!s7_ticker) s7_ticker = lv_timer_create(s7_tick, 250, NULL);
}

void ui_Screen7_screen_destroy(void) {
    if (s7_ticker) { lv_timer_del(s7_ticker); s7_ticker = NULL; }
    ltw_stop_lamp_pulse(s7_lamp);
    audio_playback_stop();
    if (ui_Screen7) lv_obj_del(ui_Screen7);
    ui_Screen7 = NULL; ui_S7_Timer = NULL;
    s7_status = NULL; s7_progress = NULL; s7_lamp = NULL; s7_vol_lbl = NULL;
    s7_banner = NULL;
}
