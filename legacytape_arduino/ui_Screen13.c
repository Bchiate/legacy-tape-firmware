#include "ui.h"
#include "ui_widgets.h"
#include "book.h"

lv_obj_t *ui_Screen13 = NULL;

static void s13_dismiss(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen9, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen9_screen_init); }

void ui_Screen13_screen_init(void) {
    ui_Screen13 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen13, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen13, lv_color_hex(LT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen13, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ltw_topbar(ui_Screen13, LT_RED, "RECORDING", "00:08:31", NULL, NULL, NULL);

    // Add-to-previous pill at top
    lv_obj_t *pill = lv_btn_create(ui_Screen13);
    lv_obj_set_size(pill, 320, 36);
    lv_obj_align(pill, LV_ALIGN_TOP_MID, 0, 64);
    lv_obj_set_style_bg_color(pill, lv_color_hex(0x263250), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(pill, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(pill, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(pill, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(pill, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *pillL = lv_label_create(pill);
    lv_obj_center(pillL);
    lv_label_set_text(pillL, "<< ADD TO PREVIOUS CHAPTER");
    lv_obj_set_style_text_color(pillL, lv_color_hex(LT_INK_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(pillL, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ltw_cassette(ui_Screen13, 560, 200, -16, book_get_name(), NULL);

    // Banner with amber-tinted background to indicate chapter just advanced
    lv_obj_t *banner = lv_obj_create(ui_Screen13);
    lv_obj_set_size(banner, 780, 74);
    lv_obj_set_pos(banner, 10, 396);
    lv_obj_clear_flag(banner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(banner, lv_color_hex(LT_PANEL_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(banner, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(banner, lv_color_hex(LT_AMBER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(banner, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(banner, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(banner, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(banner, s13_dismiss, LV_EVENT_ALL, NULL);

    lv_obj_t *cn = lv_label_create(banner);
    lv_obj_set_pos(cn, 18, 10);
    lv_label_set_text(cn, ">> CHAPTER 04 - NEW");
    lv_obj_set_style_text_color(cn, lv_color_hex(LT_AMBER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cn, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(cn, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *ct = lv_label_create(banner);
    lv_obj_set_pos(ct, 18, 36);
    lv_label_set_text(ct, "NAVY DAYS");
    lv_obj_set_style_text_color(ct, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ct, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_Screen13_screen_destroy(void) {
    if (ui_Screen13) lv_obj_del(ui_Screen13);
    ui_Screen13 = NULL;
}
