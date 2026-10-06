#include "ui.h"
#include "ui_widgets.h"
#include "audio_record.h"
#include "audio_upload.h"
#include "book.h"
#include <stdio.h>

lv_obj_t *ui_Screen6 = NULL;
lv_obj_t *ui_S6_Timer = NULL;
lv_obj_t *ui_S6_PilotLamp = NULL;
static lv_obj_t *ui_S6_StatusLabel = NULL;
static lv_obj_t *ui_S6_ProgressBar = NULL;
static lv_obj_t *s6_banner         = NULL;
static lv_timer_t *s6_ticker       = NULL;

static void s6_to_chapter(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen10, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen10_screen_init);
}
static void s6_to_book(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen8, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen8_screen_init);
}
static void s6_to_rec(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen5, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen5_screen_init);
}
static void s6_to_play(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen7, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen7_screen_init);
}

static void s6_tick(lv_timer_t *t) {
    if (!ui_S6_StatusLabel) return;
    audio_state_t st = audio_record_state();
    char buf[96];
    uint32_t uploaded = audio_upload_chunks_uploaded();
    uint32_t total    = audio_record_chunks_captured();

    (void)buf;
    if (st == AUDIO_STATE_FINALIZING) {
        if (total > 0) {
            uint8_t pct = (uint8_t)((uint32_t)uploaded * 100 / total);
            lv_label_set_text(ui_S6_StatusLabel, "UPLOADING");
            if (ui_S6_ProgressBar) lv_obj_set_width(ui_S6_ProgressBar, (lv_coord_t)(780u * pct / 100u));
        } else {
            lv_label_set_text(ui_S6_StatusLabel, "SAVING");
        }
    } else if (st == AUDIO_STATE_COMPLETE) {
        // COMPLETE can mean "uploaded fine" OR "gave up with nothing uploaded".
        if (audio_upload_last_error()[0] != 0) {
            lv_label_set_text(ui_S6_StatusLabel, "NOT SENT");
            if (ui_S6_ProgressBar) lv_obj_set_width(ui_S6_ProgressBar, 0);
        } else {
            lv_label_set_text(ui_S6_StatusLabel, "UPLOADED");
            if (ui_S6_ProgressBar) lv_obj_set_width(ui_S6_ProgressBar, 780);
        }
    } else if (audio_upload_last_error()[0] != 0) {
        lv_label_set_text(ui_S6_StatusLabel, "ISSUE");
    } else {
        lv_label_set_text(ui_S6_StatusLabel, "SAVED");
    }
}

static void s6_format_duration(char *out, size_t n) {
    uint32_t s = audio_record_seconds();
    snprintf(out, n, "%02u:%02u:%02u",
             (unsigned)(s / 3600), (unsigned)((s / 60) % 60), (unsigned)(s % 60));
}

// Runs on every visit (the screen is built once and cached): show this take's
// duration, the active chapter, and the upload state right away.
static void s6_on_loaded(lv_event_t *e) {
    (void)e;
    char dur[16];
    s6_format_duration(dur, sizeof(dur));
    if (ui_S6_Timer) lv_label_set_text(ui_S6_Timer, dur);
    ltw_chapter_banner_refresh(s6_banner);
    if (ui_S6_ProgressBar) lv_obj_set_width(ui_S6_ProgressBar, 0);   // no stale bar from the last take
    s6_tick(NULL);
}

void ui_Screen6_screen_init(void) {
    ui_Screen6 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen6, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen6, lv_color_hex(0x161C2A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen6, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Show last recording duration in the timer (refreshed in s6_on_loaded)
    char dur[16];
    s6_format_duration(dur, sizeof(dur));

    ltw_topbar(ui_Screen6, LT_AMBER, "STOPPED", dur, &ui_S6_PilotLamp, NULL, &ui_S6_Timer);
    lv_obj_t *s6_cass = ltw_cassette_hero(ui_Screen6, 58, NULL, 0, false);   // static reels

    // Dynamic cassette label = short upload status, on the blank label band.
    ui_S6_StatusLabel = lv_label_create(s6_cass);
    lv_obj_set_width(ui_S6_StatusLabel, 320);
    lv_label_set_long_mode(ui_S6_StatusLabel, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(ui_S6_StatusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ui_S6_StatusLabel, LV_ALIGN_TOP_MID, 0, 31);
    lv_label_set_text(ui_S6_StatusLabel, "SAVED");
    lv_obj_set_style_text_color(ui_S6_StatusLabel, lv_color_hex(0xC87A2A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_S6_StatusLabel, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Upload progress bar along the very bottom.
    lv_obj_t *track = lv_obj_create(ui_Screen6);
    lv_obj_set_size(track, 780, 8);
    lv_obj_set_pos(track, 10, 472);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(track, lv_color_hex(0x0C1322), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(track, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(track, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(track, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(track, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_S6_ProgressBar = lv_obj_create(track);
    lv_obj_set_size(ui_S6_ProgressBar, 0, 8);
    lv_obj_set_pos(ui_S6_ProgressBar, 0, 0);
    lv_obj_set_style_bg_color(ui_S6_ProgressBar, lv_color_hex(LT_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_S6_ProgressBar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_S6_ProgressBar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_S6_ProgressBar, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ltw_hw_legend(ui_Screen6,
                  "Overdub",  s6_to_rec,
                  "Play",     s6_to_play,
                  "Rewind",   NULL,
                  "Fast Fwd", NULL,
                  NULL,       NULL);
    int s6ch = book_get_active_chapter();
    const char *s6cn = book_get_chapter_name(s6ch);
    char s6num[16]; snprintf(s6num, sizeof(s6num), "CHAPTER %02d", s6ch + 1);
    s6_banner = ltw_chapter_banner(ui_Screen6, s6num, s6cn ? s6cn : "Chapter 1", s6_to_chapter, s6_to_book);

    lv_obj_add_event_cb(ui_Screen6, s6_on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
    if (!s6_ticker) s6_ticker = lv_timer_create(s6_tick, 250, NULL);
}

void ui_Screen6_screen_destroy(void) {
    if (s6_ticker) { lv_timer_del(s6_ticker); s6_ticker = NULL; }
    if (ui_Screen6) lv_obj_del(ui_Screen6);
    ui_Screen6 = NULL;
    ui_S6_Timer = NULL;
    ui_S6_PilotLamp = NULL;
    ui_S6_StatusLabel = NULL;
    ui_S6_ProgressBar = NULL;
    s6_banner = NULL;
}
