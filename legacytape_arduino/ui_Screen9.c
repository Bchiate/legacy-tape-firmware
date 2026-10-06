#include "ui.h"
#include "ui_widgets.h"
#include "book.h"
#include <stdio.h>

lv_obj_t *ui_Screen9 = NULL;
lv_obj_t *ui_S9_AiCaption = NULL;

static void s9_to_chapter(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen10, LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, &ui_Screen10_screen_init); }
static void s9_to_book(lv_event_t *e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen8,  LV_SCR_LOAD_ANIM_MOVE_LEFT, 250, 0, &ui_Screen8_screen_init); }
static void s9_advance(lv_event_t *e)    { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen13, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen13_screen_init); }

void ui_Screen9_screen_init(void) {
    ui_Screen9 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen9, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen9, lv_color_hex(LT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen9, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *bar = ltw_topbar(ui_Screen9, LT_RED, "RECORDING", "00:05:12", NULL, NULL, NULL);

    // AI chip in top bar
    lv_obj_t *chip = lv_obj_create(bar);
    lv_obj_set_size(chip, 140, 22);
    lv_obj_set_pos(chip, 200, 11);
    lv_obj_clear_flag(chip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(chip, lv_color_hex(LT_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(chip, 60, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(chip, lv_color_hex(LT_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(chip, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(chip, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(chip, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *chipL = lv_label_create(chip);
    lv_obj_center(chipL);
    lv_label_set_text(chipL, "AI SPEAKING");
    lv_obj_set_style_text_color(chipL, lv_color_hex(LT_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(chipL, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cass = ltw_cassette(ui_Screen9, 560, 180, -36, book_get_name(), NULL);
    lv_obj_set_style_outline_color(cass, lv_color_hex(LT_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(cass, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_pad(cass, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(cass, 170, LV_PART_MAIN | LV_STATE_DEFAULT);

    // AI caption panel
    ui_S9_AiCaption = lv_obj_create(ui_Screen9);
    lv_obj_set_size(ui_S9_AiCaption, 780, 80);
    lv_obj_set_pos(ui_S9_AiCaption, 10, 280);
    lv_obj_clear_flag(ui_S9_AiCaption, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_S9_AiCaption, lv_color_hex(LT_PANEL_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_S9_AiCaption, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_S9_AiCaption, lv_color_hex(LT_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_S9_AiCaption, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(ui_S9_AiCaption, LV_BORDER_SIDE_TOP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_S9_AiCaption, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_S9_AiCaption, 12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_S9_AiCaption, s9_advance, LV_EVENT_ALL, NULL);

    lv_obj_t *aiE = lv_label_create(ui_S9_AiCaption);
    lv_obj_set_pos(aiE, 0, 0);
    lv_label_set_text(aiE, "LEGACY ASKS:");
    lv_obj_set_style_text_color(aiE, lv_color_hex(LT_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(aiE, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *aiT = lv_label_create(ui_S9_AiCaption);
    lv_obj_set_pos(aiT, 0, 22);
    lv_obj_set_width(aiT, 750);
    lv_label_set_text(aiT, "What was the name of the town where you grew up?");
    lv_obj_set_style_text_color(aiT, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(aiT, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    int s9ch = book_get_active_chapter();
    const char *s9cn = book_get_chapter_name(s9ch);
    char s9num[16]; snprintf(s9num, sizeof(s9num), "CHAPTER %02d", s9ch + 1);
    ltw_chapter_banner(ui_Screen9, s9num, s9cn ? s9cn : "Chapter 1", s9_to_chapter, s9_to_book);
}

void ui_Screen9_screen_destroy(void) {
    if (ui_Screen9) lv_obj_del(ui_Screen9);
    ui_Screen9 = NULL; ui_S9_AiCaption = NULL;
}
