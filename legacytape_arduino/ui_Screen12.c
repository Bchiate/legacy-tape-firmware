#include "ui.h"
#include "ui_widgets.h"
#include "book.h"
#include <stdio.h>

lv_obj_t *ui_Screen12 = NULL;

static void s12_to_chapter(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen10, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, &ui_Screen10_screen_init); }
static void s12_to_book(lv_event_t *e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen8,  LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, &ui_Screen8_screen_init); }
static void s12_retry(lv_event_t *e)      { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen4,  LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen4_screen_init); }

void ui_Screen12_screen_init(void) {
    ui_Screen12 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen12, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen12, lv_color_hex(LT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen12, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *bar = ltw_topbar(ui_Screen12, LT_AMBER, "READY", "00:00:00", NULL, NULL, NULL);

    // OFFLINE chip
    lv_obj_t *chip = lv_obj_create(bar);
    lv_obj_set_size(chip, 110, 22);
    lv_obj_set_pos(chip, 160, 11);
    lv_obj_clear_flag(chip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(chip, lv_color_hex(LT_AMBER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(chip, 60, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(chip, lv_color_hex(LT_AMBER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(chip, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(chip, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(chip, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *chipL = lv_label_create(chip);
    lv_obj_center(chipL);
    lv_label_set_text(chipL, "OFFLINE");
    lv_obj_set_style_text_color(chipL, lv_color_hex(LT_AMBER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(chipL, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ltw_cassette(ui_Screen12, 560, 200, -36, book_get_name(), NULL);

    // Error banner above chapter banner
    lv_obj_t *err = lv_obj_create(ui_Screen12);
    lv_obj_set_size(err, 780, 60);
    lv_obj_set_pos(err, 10, 330);
    lv_obj_clear_flag(err, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(err, lv_color_hex(0x1E2740), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(err, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(err, lv_color_hex(LT_AMBER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(err, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(err, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(err, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(err, 10, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *et = lv_label_create(err);
    lv_obj_set_pos(et, 14, 4);
    lv_label_set_text(et, "CAN'T REACH WIFI");
    lv_obj_set_style_text_color(et, lv_color_hex(LT_INK_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(et, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *es = lv_label_create(err);
    lv_obj_set_pos(es, 14, 28);
    lv_label_set_text(es, "Keep recording. We'll sync when you're back online.");
    lv_obj_set_style_text_color(es, lv_color_hex(0x90A4C0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(es, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *retry = lv_btn_create(err);
    lv_obj_set_size(retry, 100, 40);
    lv_obj_align(retry, LV_ALIGN_RIGHT_MID, -4, 0);
    lv_obj_set_style_bg_color(retry, lv_color_hex(0x263250), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(retry, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(retry, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *retryL = lv_label_create(retry);
    lv_label_set_text(retryL, "RETRY");
    lv_obj_set_style_text_color(retryL, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(retryL, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(retryL);
    lv_obj_add_event_cb(retry, s12_retry, LV_EVENT_ALL, NULL);

    int s12ch = book_get_active_chapter();
    const char *s12cn = book_get_chapter_name(s12ch);
    char s12num[16]; snprintf(s12num, sizeof(s12num), "CHAPTER %02d", s12ch + 1);
    ltw_chapter_banner(ui_Screen12, s12num, s12cn ? s12cn : "Chapter 1", s12_to_chapter, s12_to_book);
}

void ui_Screen12_screen_destroy(void) {
    if (ui_Screen12) lv_obj_del(ui_Screen12);
    ui_Screen12 = NULL;
}
