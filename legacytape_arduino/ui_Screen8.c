#include "ui.h"
#include "ui_widgets.h"
#include "book.h"
#include <stdio.h>

lv_obj_t *ui_Screen8 = NULL;

static void s8_close(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen4_screen_init);
}

// Tap the current book → it's already active, just go back to Ready
static void s8_pick_current(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen4, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen4_screen_init);
}

// NEW BOOK → keyboard (Screen3). For v1 this overwrites the current book
// name when the user hits done — multi-book support is post-MVP.
static void s8_newbook(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
        _ui_screen_change(&ui_Screen3, LV_SCR_LOAD_ANIM_NONE, 1, 0, &ui_Screen3_screen_init);
}

static lv_obj_t *s8_tiles = NULL;   // holds the book tiles; rebuilt on every visit

// (Re)build the tiles. The screen itself is built once and cached, so this
// runs on every visit to pick up a renamed book or new chapters.
static void s8_build_tiles(void) {
    if (!s8_tiles) return;
    lv_obj_clean(s8_tiles);
    bool has_book = book_has_name();

    // Place tiles side-by-side. Current book on left, NEW BOOK on right.
    // If no book has been named yet, only show NEW BOOK centered.
    const int TILE_W = 280, TILE_H = 280;
    int total_tiles = has_book ? 2 : 1;
    int gap = 40;
    int total_w = total_tiles * TILE_W + (total_tiles - 1) * gap;
    int start_x = (800 - total_w) / 2;
    int y = 130;

    if (has_book) {
        // Existing book tile
        lv_obj_t *tile = lv_btn_create(s8_tiles);
        lv_obj_set_size(tile, TILE_W, TILE_H);
        lv_obj_set_pos(tile, start_x, y);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x1E2740), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0x35466E), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(tile, lv_color_hex(LT_AMBER), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(tile, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(tile, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(tile, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(tile, s8_pick_current, LV_EVENT_CLICKED, NULL);

        // Color band at the top of the tile (cassette label spine color)
        lv_obj_t *band = lv_obj_create(tile);
        lv_obj_set_size(band, 248, 18);
        lv_obj_set_pos(band, 0, 0);
        lv_obj_clear_flag(band, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(band, lv_color_hex(0xE26A48), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(band, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(band, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(band, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        // Book name (the actual one from NVS)
        lv_obj_t *t = lv_label_create(tile);
        lv_obj_set_pos(t, 0, 50);
        lv_obj_set_width(t, 248);
        lv_label_set_text(t, book_get_name());
        lv_obj_set_style_text_color(t, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(t, &ui_font_archivo_42, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);

        // Chapter count summary
        char info[32];
        snprintf(info, sizeof(info), "%d chapter%s",
                 book_get_chapter_count(),
                 book_get_chapter_count() == 1 ? "" : "s");
        lv_obj_t *d = lv_label_create(tile);
        lv_obj_set_pos(d, 0, 180);
        lv_label_set_text(d, info);
        lv_obj_set_style_text_color(d, lv_color_hex(0x90A4C0), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(d, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *a = lv_label_create(tile);
        lv_obj_set_pos(a, 0, 220);
        lv_label_set_text(a, "ACTIVE");
        lv_obj_set_style_text_color(a, lv_color_hex(LT_AMBER), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(a, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(a, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    // NEW BOOK tile (always present)
    int nb_x = has_book ? (start_x + TILE_W + gap) : start_x;
    lv_obj_t *nb = lv_btn_create(s8_tiles);
    lv_obj_set_size(nb, TILE_W, TILE_H);
    lv_obj_set_pos(nb, nb_x, y);
    lv_obj_set_style_bg_color(nb, lv_color_hex(LT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(nb, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(nb, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(nb, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(nb, 130, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(nb, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(nb, s8_newbook, LV_EVENT_CLICKED, NULL);

    lv_obj_t *plus = lv_label_create(nb);
    lv_obj_align(plus, LV_ALIGN_CENTER, 0, -32);
    lv_label_set_text(plus, "+");
    lv_obj_set_style_text_color(plus, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(plus, &ui_font_archivo_56, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *nbl = lv_label_create(nb);
    lv_obj_align(nbl, LV_ALIGN_CENTER, 0, 30);
    lv_label_set_text(nbl, "NEW BOOK");
    lv_obj_set_style_text_color(nbl, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(nbl, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(nbl, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void s8_on_loaded(lv_event_t *e) {
    (void)e;
    s8_build_tiles();
}

void ui_Screen8_screen_init(void) {
    ui_Screen8 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen8, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen8, lv_color_hex(LT_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen8, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ltw_picker_header(ui_Screen8, "YOUR SHELF", "Choose a Book", "CLOSE", s8_close);

    // Full-screen, transparent, non-clickable holder for the tiles, so they can
    // be rebuilt without touching the header (clicks pass through to it).
    s8_tiles = lv_obj_create(ui_Screen8);
    lv_obj_set_size(s8_tiles, 800, 480);
    lv_obj_set_pos(s8_tiles, 0, 0);
    lv_obj_clear_flag(s8_tiles, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(s8_tiles, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(s8_tiles, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(s8_tiles, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    s8_build_tiles();

    lv_obj_add_event_cb(ui_Screen8, s8_on_loaded, LV_EVENT_SCREEN_LOADED, NULL);
}

void ui_Screen8_screen_destroy(void) {
    if (ui_Screen8) lv_obj_del(ui_Screen8);
    ui_Screen8 = NULL;
    s8_tiles = NULL;
}
