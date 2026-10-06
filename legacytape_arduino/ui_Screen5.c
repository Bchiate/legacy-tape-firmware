#include "ui.h"
#include "ui_widgets.h"
#include "audio_record.h"
#include "audio_upload.h"
#include "book.h"
#include <stdio.h>

lv_obj_t *ui_Screen5 = NULL;
lv_obj_t *ui_S5_Timer = NULL;
lv_obj_t *ui_S5_PilotLamp = NULL;

static void s5_to_chapter(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen10, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen10_screen_init);
}
static void s5_to_book(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen8, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen8_screen_init);
}

// STOP: stop capturing. Upload module finishes streaming the remaining
// chunks + sends finalize. Advance to Screen6 immediately — it will show
// upload progress as the last chunks land. Shared by the on-screen STOP cap
// and the hardware STOP key (legacytape_arduino.ino).
void ui_Screen5_stop(void) {
    printf("[S5] STOP pressed - finalizing\n");
    uint32_t secs = audio_record_seconds();
    audio_upload_request_finalize(secs, book_get_active_chapter());
    audio_record_stop();
    _ui_screen_change(&ui_Screen6, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen6_screen_init);
}

static void s5_to_stopped(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) ui_Screen5_stop();
}

// Timer + live level/progress bar — runs every 60ms while Recording is visible.
#define PROG_W 780                       // progress bar inner width (px)
static lv_timer_t *s5_ticker    = NULL;
static lv_obj_t   *s5_prog_fill = NULL;  // thin live bar along the bottom
static lv_obj_t   *s5_banner    = NULL;  // chapter banner (refreshed on every visit)
static int         s5_disp      = 0;     // smoothed level 0..100

static void s5_tick(lv_timer_t *t) {
    // Created once, never destroyed -> skip work unless Recording is visible so
    // it doesn't invalidate hidden widgets (forcing full-screen repaints).
    if (lv_scr_act() != ui_Screen5) return;
    audio_state_t st = audio_record_state();

    uint32_t s = audio_record_seconds();
    char buf[16];
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
             (unsigned)(s / 3600), (unsigned)((s / 60) % 60), (unsigned)(s % 60));
    if (ui_S5_Timer) lv_label_set_text(ui_S5_Timer, buf);

    // Live mic level, smoothed (fast attack, slow release) -> drives the bar.
    int level = (st == AUDIO_STATE_RECORDING) ? audio_record_level_percent() : 0;
    if (level > s5_disp) s5_disp = level;
    else                 s5_disp += (level - s5_disp) / 3;
    if (s5_disp < 0) s5_disp = 0; if (s5_disp > 100) s5_disp = 100;

    if (s5_prog_fill) {
        lv_obj_set_width(s5_prog_fill, (lv_coord_t)(s5_disp * PROG_W / 100));
        uint32_t c = (s5_disp < 70) ? 0x4AC06A : (s5_disp < 90) ? 0xE5B03A : 0xE53935;
        lv_obj_set_style_bg_color(s5_prog_fill, lv_color_hex(c), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void s5_show_upload_status(void *unused) {
    (void)unused;
    if (lv_scr_act() == ui_Screen5)
        _ui_screen_change(&ui_Screen6, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen6_screen_init);
}

// Runs on every visit. The screen object is built once and cached by
// _ui_screen_change(), so starting a take cannot live in the init function.
static void s5_on_loaded(lv_event_t *e) {
    (void)e;
    ltw_chapter_banner_refresh(s5_banner);   // STOP files the take under the active chapter

    // In dev mode we only render the screen (no mic/upload) so it can be paged
    // through for styling without side effects.
    if (g_dev_mode) return;

    audio_state_t st = audio_record_state();
    // Back from the CHAPTER/BOOK pickers mid-take: keep recording, don't reset.
    if (st == AUDIO_STATE_RECORDING) return;
    // The previous take is still flushing or uploading. Starting now would reset
    // the buffers under it, so show its upload progress on Screen6 instead
    // (deferred: this handler runs inside the screen load).
    if (st == AUDIO_STATE_FINALIZING || audio_record_capture_active()) {
        lv_async_call(s5_show_upload_status, NULL);
        return;
    }

    // New take: reset the per-take UI, then start recording.
    s5_disp = 0;
    if (ui_S5_Timer)  lv_label_set_text(ui_S5_Timer, "00:00:00");
    if (s5_prog_fill) lv_obj_set_width(s5_prog_fill, 0);
    // Reset the uploader's per-recording counters first so last take's
    // uploaded-count/error don't leak into this session.
    audio_upload_reset();
    if (audio_record_start()) {
        printf("[S5] mic capture started\n");
    } else {
        printf("[S5] mic capture failed to start\n");
    }
}

void ui_Screen5_screen_init(void) {
    ui_Screen5 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen5, LV_OBJ_FLAG_SCROLLABLE);
    // Dark navy to match the cassette artwork's own background (seamless).
    lv_obj_set_style_bg_color(ui_Screen5, lv_color_hex(0x161C2A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ltw_topbar(ui_Screen5, LT_RED, "RECORDING", "00:00:00", &ui_S5_PilotLamp, NULL, &ui_S5_Timer);
    ltw_pulse_lamp(ui_S5_PilotLamp, 1100);

    // Hero cassette: the real reference artwork with spinning reels.
    ltw_cassette_hero(ui_Screen5, 58, "RECORDING", 0xC8202A, true);

    // Stop control (single active button; the 5-cap legend row is temporary).
    ltw_hw_legend(ui_Screen5,
                  "Recording", NULL,
                  NULL,        NULL,
                  NULL,        NULL,
                  NULL,        NULL,
                  "Stop",      s5_to_stopped);

    // Chapter banner reads the real active chapter from NVS
    int ach = book_get_active_chapter();
    const char *acn = book_get_chapter_name(ach);
    char chnum[16];
    snprintf(chnum, sizeof(chnum), "CHAPTER %02d", ach + 1);
    s5_banner = ltw_chapter_banner(ui_Screen5, chnum, acn ? acn : "Chapter 1", s5_to_chapter, s5_to_book);

    // Thin live level/progress bar along the very bottom.
    lv_obj_t *track = lv_obj_create(ui_Screen5);
    lv_obj_set_size(track, 780, 8);
    lv_obj_set_pos(track, 10, 472);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(track, lv_color_hex(0x0C1322), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(track, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(track, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(track, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(track, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    s5_prog_fill = lv_obj_create(track);
    lv_obj_set_size(s5_prog_fill, 0, 8);
    lv_obj_align(s5_prog_fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_clear_flag(s5_prog_fill, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s5_prog_fill, lv_color_hex(0x4AC06A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(s5_prog_fill, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s5_prog_fill, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(s5_prog_fill, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    s5_disp = 0;

    // Recording starts in s5_on_loaded, on every visit (including this first one).
    lv_obj_add_event_cb(ui_Screen5, s5_on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    if (!s5_ticker) s5_ticker = lv_timer_create(s5_tick, 60, NULL);
}

void ui_Screen5_screen_destroy(void) {
    if (s5_ticker) { lv_timer_del(s5_ticker); s5_ticker = NULL; }
    // Stop the lamp pulse before the object is freed so the anim doesn't
    // fire its callback on a dangling pointer.
    ltw_stop_lamp_pulse(ui_S5_PilotLamp);
    if (ui_Screen5) lv_obj_del(ui_Screen5);
    ui_Screen5 = NULL; ui_S5_Timer = NULL; ui_S5_PilotLamp = NULL;
    s5_prog_fill = NULL; s5_banner = NULL;
}
