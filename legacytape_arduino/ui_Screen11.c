#include "ui.h"
#include "ui_widgets.h"

lv_obj_t *ui_Screen11 = NULL;
lv_obj_t *ui_S11_Readout = NULL;

static void s11_dismiss(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_Screen4_screen_init); }

void ui_Screen11_screen_init(void) {
    ui_Screen11 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen11, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen11, lv_color_hex(LT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen11, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Dim full-screen overlay
    lv_obj_t *dim = lv_obj_create(ui_Screen11);
    lv_obj_set_size(dim, 800, 480);
    lv_obj_set_pos(dim, 0, 0);
    lv_obj_clear_flag(dim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(dim, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(dim, 130, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dim, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(dim, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(dim, s11_dismiss, LV_EVENT_ALL, NULL);

    // Toast panel
    lv_obj_t *toast = lv_obj_create(ui_Screen11);
    lv_obj_set_size(toast, 460, 220);
    lv_obj_align(toast, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(toast, lv_color_hex(LT_PANEL_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(toast, 245, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(toast, lv_color_hex(0x33405C), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(toast, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(toast, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(toast, 24, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *title = lv_label_create(toast);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    lv_label_set_text(title, "VOLUME");
    lv_obj_set_style_text_color(title, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(title, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(title, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    // 10-segment bar
    int level = 7;
    for (int i = 0; i < 10; i++) {
        lv_obj_t *seg = lv_obj_create(toast);
        lv_obj_set_size(seg, 32, 28);
        lv_obj_set_pos(seg, i * 38, 50);
        lv_obj_clear_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(seg, lv_color_hex(i < level ? 0xE26A48 : 0x263250), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(seg, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(seg, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(seg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(seg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    ui_S11_Readout = lv_label_create(toast);
    lv_obj_align(ui_S11_Readout, LV_ALIGN_TOP_MID, 0, 96);
    lv_label_set_text(ui_S11_Readout, "7 / 10");
    lv_obj_set_style_text_color(ui_S11_Readout, lv_color_hex(LT_INK_DIM), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_S11_Readout, &ui_font_archivo_42, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *hint = lv_label_create(toast);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_label_set_text(hint, "TURN ENCODER TO ADJUST");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x90A4C0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(hint, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_Screen11_screen_destroy(void) {
    if (ui_Screen11) lv_obj_del(ui_Screen11);
    ui_Screen11 = NULL; ui_S11_Readout = NULL;
}
