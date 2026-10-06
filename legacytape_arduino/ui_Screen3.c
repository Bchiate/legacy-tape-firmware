// Screen3 — "Name your book" (navy, single clean input + full-width keyboard).
// Object names ui_TextArea1 / ui_Keyboard2 are preserved (the .ino glue
// references them).
#include "ui.h"

lv_obj_t *ui_Screen3 = NULL;
lv_obj_t *ui_WelcomeLabel3 = NULL;
lv_obj_t *ui_Keyboard2 = NULL;
lv_obj_t *ui_TextArea1 = NULL;

void ui_Screen3_screen_init(void)
{
    ui_Screen3 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen3, lv_color_hex(0x161C2A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Title + subtitle
    ui_WelcomeLabel3 = lv_label_create(ui_Screen3);
    lv_obj_align(ui_WelcomeLabel3, LV_ALIGN_TOP_MID, 0, 26);
    lv_label_set_text(ui_WelcomeLabel3, "Name your book");
    lv_obj_set_style_text_color(ui_WelcomeLabel3, lv_color_hex(0xF4EEDD), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_WelcomeLabel3, &ui_font_archivo_42, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *sub = lv_label_create(ui_Screen3);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 78);
    lv_label_set_text(sub, "This becomes the label on your first tape");
    lv_obj_set_style_text_color(sub, lv_color_hex(0x90A4C0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(sub, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Single text field (centered between the subtitle and the keyboard)
    ui_TextArea1 = lv_textarea_create(ui_Screen3);
    lv_obj_set_size(ui_TextArea1, 560, 58);
    lv_obj_align(ui_TextArea1, LV_ALIGN_TOP_MID, 0, 138);
    lv_textarea_set_one_line(ui_TextArea1, true);
    lv_textarea_set_placeholder_text(ui_TextArea1, "Your book name…");
    lv_obj_set_style_bg_color(ui_TextArea1, lv_color_hex(0x141A28), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_TextArea1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_TextArea1, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_TextArea1, lv_color_hex(0xE26A48), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(ui_TextArea1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_TextArea1, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_TextArea1, lv_color_hex(0xF4EEDD), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_TextArea1, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_TextArea1, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_TextArea1, lv_color_hex(0x90A4C0), LV_PART_TEXTAREA_PLACEHOLDER | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_TextArea1, lv_color_hex(0xE26A48), LV_PART_CURSOR | LV_STATE_DEFAULT);

    // Keyboard (full width, bottom-anchored)
    ui_Keyboard2 = lv_keyboard_create(ui_Screen3);
    lv_obj_set_size(ui_Keyboard2, 768, 224);
    lv_obj_align(ui_Keyboard2, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_keyboard_set_mode(ui_Keyboard2, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_set_style_bg_color(ui_Keyboard2, lv_color_hex(0x121826), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Keyboard2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Keyboard2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_Keyboard2, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Keyboard2, lv_color_hex(0x263250), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Keyboard2, lv_color_hex(0x35466E), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(ui_Keyboard2, lv_color_hex(0xE26A48), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(ui_Keyboard2, lv_color_hex(0x46587E), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Keyboard2, 1, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_Keyboard2, 8, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_Keyboard2, lv_color_hex(0xF4EEDD), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Keyboard2, &ui_font_Arhivo_regular_20, LV_PART_ITEMS | LV_STATE_DEFAULT);
}

void ui_Screen3_screen_destroy(void)
{
    if (ui_Screen3) lv_obj_del(ui_Screen3);
    ui_Screen3 = NULL;
    ui_WelcomeLabel3 = NULL;
    ui_Keyboard2 = NULL;
    ui_TextArea1 = NULL;
}
