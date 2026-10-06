#include "ui.h"
#include "ui_widgets.h"
#include "book.h"
#include <stdio.h>

lv_obj_t *ui_Screen10 = NULL;

static void s10_close(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen4_screen_init);
}

static void s10_pick(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    // The user data stashed when the row was created is the chapter index
    intptr_t idx = (intptr_t)lv_event_get_user_data(e);
    book_set_active_chapter((int)idx);
    _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen4_screen_init);
}

static lv_obj_t *s10_list = NULL;   // scrollable container for the chapter rows

// (Re)build one row per chapter. The screen itself is built once and cached,
// so this runs on every visit and after ADD to pick up new chapters and the
// active-chapter highlight. The ADD button lives outside the list, so it is
// safe to rebuild from its click handler.
static void s10_build_rows(void) {
    if (!s10_list) return;
    lv_obj_clean(s10_list);
    int active = book_get_active_chapter();
    int n = book_get_chapter_count();
    for (int i = 0; i < n; i++) {
        const char *name = book_get_chapter_name(i);
        if (!name) continue;
        bool is_active = (i == active);

        lv_obj_t *row = lv_btn_create(s10_list);
        lv_obj_set_size(row, 720, 56);
        lv_obj_set_pos(row, 0, i * 64);
        lv_obj_set_style_bg_color(row, lv_color_hex(is_active ? 0x35466E : LT_PANEL_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(row, is_active ? 255 : 200, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(row, lv_color_hex(is_active ? LT_INK : LT_RULE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(row, is_active ? 2 : 1, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(row, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        // Pass the chapter index as user data on the event callback
        lv_obj_add_event_cb(row, s10_pick, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        // Chapter number left
        char numbuf[8];
        snprintf(numbuf, sizeof(numbuf), "%02d", i + 1);
        lv_obj_t *num = lv_label_create(row);
        lv_obj_set_pos(num, 16, 16);
        lv_label_set_text(num, numbuf);
        lv_obj_set_style_text_color(num, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(num, &ui_font_archivo_42, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Chapter title
        lv_obj_t *t = lv_label_create(row);
        lv_obj_set_pos(t, 80, 19);
        lv_label_set_text(t, name);
        lv_obj_set_style_text_color(t, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(t, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (is_active) {
            lv_obj_t *a = lv_label_create(row);
            lv_obj_align(a, LV_ALIGN_RIGHT_MID, -16, 0);
            lv_label_set_text(a, "ACTIVE");
            lv_obj_set_style_text_color(a, lv_color_hex(LT_AMBER), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(a, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

static void s10_on_loaded(lv_event_t *e) {
    (void)e;
    s10_build_rows();
}

static void s10_add_chapter(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int idx = book_add_chapter();
    if (idx >= 0) {
        book_set_active_chapter(idx);
    }
    // Screen10 stays up; redraw the list with the new chapter included.
    s10_build_rows();
}

void ui_Screen10_screen_init(void) {
    ui_Screen10 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen10, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen10, lv_color_hex(LT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen10, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ltw_picker_header(ui_Screen10, "NAVIGATE / OVERDUB", "Select Chapter", "CANCEL", s10_close);

    // Scrollable list — real chapters from NVS
    lv_obj_t *list = lv_obj_create(ui_Screen10);
    lv_obj_set_size(list, 748, 280);
    lv_obj_set_pos(list, 26, 102);
    lv_obj_set_style_bg_opa(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);

    s10_list = list;
    s10_build_rows();

    // ADD CHAPTER button — pinned at the bottom, full width below the list
    lv_obj_t *addBtn = lv_btn_create(ui_Screen10);
    lv_obj_set_size(addBtn, 748, 56);
    lv_obj_set_pos(addBtn, 26, 396);
    lv_obj_set_style_bg_color(addBtn, lv_color_hex(0x263250), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(addBtn, lv_color_hex(0x35466E), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(addBtn, lv_color_hex(0x46587E), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(addBtn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(addBtn, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(addBtn, s10_add_chapter, LV_EVENT_CLICKED, NULL);

    lv_obj_t *addL = lv_label_create(addBtn);
    lv_obj_center(addL);
    lv_label_set_text(addL, "+  ADD NEW CHAPTER");
    lv_obj_set_style_text_color(addL, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(addL, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(addL, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_Screen10, s10_on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
}

void ui_Screen10_screen_destroy(void) {
    if (ui_Screen10) lv_obj_del(ui_Screen10);
    ui_Screen10 = NULL;
    s10_list = NULL;
}
