#include "ui.h"
#include "ui_widgets.h"

lv_obj_t *ui_Screen14 = NULL;

static void s14_close(lv_event_t *e) { if (lv_event_get_code(e) == LV_EVENT_CLICKED) _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, &ui_Screen4_screen_init); }

typedef struct { uint32_t lampColor; const char *label; const char *value; } setting_t;

void ui_Screen14_screen_init(void) {
    ui_Screen14 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen14, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen14, lv_color_hex(LT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen14, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ltw_picker_header(ui_Screen14, "MACHINE INFO", "Settings", "CLOSE", s14_close);

    setting_t st[6] = {
        {LT_GREEN, "WIFI",        "LegacyHome_5G"},
        {LT_GREEN, "STORAGE",     "12.4 GB FREE"},
        {LT_AMBER, "INPUT LEVEL", "-14 dBFS"},
        {0x46587E, "VOLUME",      "7 / 10"},
        {0x46587E, "FIRMWARE",    "v0.9.2-beta"},
        {0x46587E, "DEVICE ID",   "LT-0042"},
    };

    for (int i = 0; i < 6; i++) {
        lv_obj_t *row = lv_obj_create(ui_Screen14);
        lv_obj_set_size(row, 748, 50);
        lv_obj_set_pos(row, 26, 108 + i * 58);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(row, lv_color_hex(LT_PANEL_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(row, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(row, lv_color_hex(LT_RULE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(row, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(row, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(row, 12, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *lamp = lv_obj_create(row);
        lv_obj_set_size(lamp, 14, 14);
        lv_obj_align(lamp, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_clear_flag(lamp, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(lamp, lv_color_hex(st[i].lampColor), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(lamp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(lamp, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(lamp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *lbl = lv_label_create(row);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 26, 0);
        lv_label_set_text(lbl, st[i].label);
        lv_obj_set_style_text_color(lbl, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lbl, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(lbl, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *val = lv_label_create(row);
        lv_obj_align(val, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_label_set_text(val, st[i].value);
        lv_obj_set_style_text_color(val, lv_color_hex(LT_INK_DIM), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(val, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(val, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void ui_Screen14_screen_destroy(void) {
    if (ui_Screen14) lv_obj_del(ui_Screen14);
    ui_Screen14 = NULL;
}
