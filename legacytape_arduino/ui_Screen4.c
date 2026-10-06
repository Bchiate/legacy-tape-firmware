// Screen4 — Ready (home state). Cassette static, amber pilot lamp, hw-legend visible.
#include "ui.h"
#include "ui_widgets.h"
#include "book.h"
#include <stdio.h>

lv_obj_t *ui_Screen4 = NULL;
lv_obj_t *ui_S4_Timer = NULL;
lv_obj_t *ui_S4_StatusLabel = NULL;
lv_obj_t *ui_S4_PilotLamp = NULL;
lv_obj_t *ui_S4_ChapterTitle = NULL;
lv_obj_t *ui_S4_ChapterNum = NULL;
lv_obj_t *ui_S4_BookTitle = NULL;

void ui_event_S4_btn_chapter(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen10, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen10_screen_init);
}
void ui_event_S4_btn_book(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen8, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen8_screen_init);
}
void ui_event_S4_btn_settings(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen14, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen14_screen_init);
}
// On-screen transport buttons (REC + PLAY) — wired to Ready -> Recording / Playback.
// Touch equivalents of the hardware REC / PLAY keys.
void ui_event_S4_btn_rec(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen5, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen5_screen_init);
}
void ui_event_S4_btn_play(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen7, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen7_screen_init);
}

// Runs on every visit (the screen is built once and cached): the book can be
// renamed and the active chapter changed on other screens.
static void s4_on_loaded(lv_event_t *e) {
    (void)e;
    int ch = book_get_active_chapter();
    const char *chname = book_get_chapter_name(ch);
    char chnum[16];
    snprintf(chnum, sizeof(chnum), "CHAPTER %02d", ch + 1);
    if (ui_S4_BookTitle)    lv_label_set_text(ui_S4_BookTitle, book_get_name());
    if (ui_S4_ChapterNum)   lv_label_set_text(ui_S4_ChapterNum, chnum);
    if (ui_S4_ChapterTitle) lv_label_set_text(ui_S4_ChapterTitle, chname ? chname : "Chapter 1");
}

void ui_Screen4_screen_init(void) {
    ui_Screen4 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen4, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen4, lv_color_hex(0x161C2A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ───── TOP STATUS BAR ─────
    lv_obj_t *topbar = lv_obj_create(ui_Screen4);
    lv_obj_set_size(topbar, 780, 44);
    lv_obj_set_pos(topbar, 10, 10);
    lv_obj_clear_flag(topbar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(topbar, lv_color_hex(0x1E2740), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(topbar, lv_color_hex(0x18202F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(topbar, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(topbar, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(topbar, lv_color_hex(0x33405C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(topbar, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(topbar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(topbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_S4_PilotLamp = lv_obj_create(topbar);
    lv_obj_set_size(ui_S4_PilotLamp, 14, 14);
    lv_obj_set_pos(ui_S4_PilotLamp, 16, 15);
    lv_obj_clear_flag(ui_S4_PilotLamp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_S4_PilotLamp, lv_color_hex(0xE5B03A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_S4_PilotLamp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_S4_PilotLamp, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_S4_PilotLamp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_S4_StatusLabel = lv_label_create(topbar);
    lv_obj_set_pos(ui_S4_StatusLabel, 40, 12);
    lv_label_set_text(ui_S4_StatusLabel, "READY");
    lv_obj_set_style_text_color(ui_S4_StatusLabel, lv_color_hex(0xF6ECD4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_S4_StatusLabel, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui_S4_StatusLabel, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_S4_Timer = lv_label_create(topbar);
    lv_obj_align(ui_S4_Timer, LV_ALIGN_RIGHT_MID, -140, 0);
    lv_label_set_text(ui_S4_Timer, "00:00:00");
    lv_obj_set_style_text_color(ui_S4_Timer, lv_color_hex(0xF6ECD4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_S4_Timer, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui_S4_Timer, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *btnSettings = lv_btn_create(topbar);
    lv_obj_set_size(btnSettings, 110, 32);
    lv_obj_align(btnSettings, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_bg_color(btnSettings, lv_color_hex(0x263250), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btnSettings, lv_color_hex(0x35466E), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btnSettings, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btnSettings, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btnSettings, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *lblS = lv_label_create(btnSettings);
    lv_label_set_text(lblS, "SETTINGS");
    lv_obj_set_style_text_color(lblS, lv_color_hex(0xE8D6A8), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lblS, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(lblS);
    lv_obj_add_event_cb(btnSettings, ui_event_S4_btn_settings, LV_EVENT_ALL, NULL);

    // ───── CASSETTE (shared hero artwork; reels static on the Ready screen) ─────
    lv_obj_t *s4_cass = ltw_cassette_hero(ui_Screen4, 58, NULL, 0, false);
    ui_S4_BookTitle = lv_label_create(s4_cass);
    lv_obj_set_width(ui_S4_BookTitle, 320);
    lv_label_set_long_mode(ui_S4_BookTitle, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(ui_S4_BookTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ui_S4_BookTitle, LV_ALIGN_TOP_MID, 0, 31);
    lv_label_set_text(ui_S4_BookTitle, book_get_name());
    lv_obj_set_style_text_color(ui_S4_BookTitle, lv_color_hex(0x2A2E3A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_S4_BookTitle, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    // ───── HW BUTTON LEGEND (tappable touch buttons until Uxcell is wired) ─────
    ltw_hw_legend(ui_Screen4,
                  "Record",   ui_event_S4_btn_rec,
                  "Play",     ui_event_S4_btn_play,
                  "Rewind",   NULL,
                  "Fast Fwd", NULL,
                  "Stop",     NULL);

    // ───── CHAPTER BANNER ─────
    lv_obj_t *banner = lv_obj_create(ui_Screen4);
    lv_obj_set_size(banner, 780, 74);
    lv_obj_set_pos(banner, 10, 396);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(banner, lv_color_hex(0x1E2740), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(banner, lv_color_hex(0x18202F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(banner, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(banner, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(banner, lv_color_hex(0x33405C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(banner, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(banner, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(banner, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_S4_ChapterNum = lv_label_create(banner);
    lv_obj_set_pos(ui_S4_ChapterNum, 18, 10);
    // Show CHAPTER NN with the active chapter's 1-based index
    char chnum[16];
    snprintf(chnum, sizeof(chnum), "CHAPTER %02d", book_get_active_chapter() + 1);
    lv_label_set_text(ui_S4_ChapterNum, chnum);
    lv_obj_set_style_text_color(ui_S4_ChapterNum, lv_color_hex(0xE26A48), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_S4_ChapterNum, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(ui_S4_ChapterNum, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_S4_ChapterTitle = lv_label_create(banner);
    lv_obj_set_pos(ui_S4_ChapterTitle, 18, 32);
    // Show the actual active chapter name from NVS
    int active = book_get_active_chapter();
    const char *chname = book_get_chapter_name(active);
    lv_label_set_text(ui_S4_ChapterTitle, chname ? chname : "Chapter 1");
    lv_obj_set_style_text_color(ui_S4_ChapterTitle, lv_color_hex(0xF6ECD4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_S4_ChapterTitle, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *btnChapter = lv_btn_create(banner);
    lv_obj_set_size(btnChapter, 160, 58);
    lv_obj_align(btnChapter, LV_ALIGN_RIGHT_MID, -180, 0);
    lv_obj_set_style_bg_color(btnChapter, lv_color_hex(0x263250), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(btnChapter, lv_color_hex(0x35466E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(btnChapter, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btnChapter, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btnChapter, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btnChapter, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btnChapter, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *btnChapterL = lv_label_create(btnChapter);
    lv_label_set_text(btnChapterL, "CHAPTER");
    lv_obj_set_style_text_color(btnChapterL, lv_color_hex(0xF4EEDD), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(btnChapterL, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(btnChapterL);
    lv_obj_add_event_cb(btnChapter, ui_event_S4_btn_chapter, LV_EVENT_ALL, NULL);

    lv_obj_t *btnBook = lv_btn_create(banner);
    lv_obj_set_size(btnBook, 160, 58);
    lv_obj_align(btnBook, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(btnBook, lv_color_hex(0x263250), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(btnBook, lv_color_hex(0x35466E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(btnBook, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btnBook, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btnBook, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btnBook, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(btnBook, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *btnBookL = lv_label_create(btnBook);
    lv_label_set_text(btnBookL, "BOOK");
    lv_obj_set_style_text_color(btnBookL, lv_color_hex(0xF4EEDD), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(btnBookL, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(btnBookL);
    lv_obj_add_event_cb(btnBook, ui_event_S4_btn_book, LV_EVENT_ALL, NULL);

    lv_obj_add_event_cb(ui_Screen4, s4_on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
}

void ui_Screen4_screen_destroy(void) {
    if (ui_Screen4) lv_obj_del(ui_Screen4);
    ui_Screen4 = NULL;
    ui_S4_Timer = NULL;
    ui_S4_StatusLabel = NULL;
    ui_S4_PilotLamp = NULL;
    ui_S4_ChapterTitle = NULL;
    ui_S4_ChapterNum = NULL;
    ui_S4_BookTitle = NULL;
}
