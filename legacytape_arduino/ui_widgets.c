// Shared widget builders. See ui_widgets.h.
#include "ui_widgets.h"
#include "book.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static lv_obj_t *make_panel(lv_obj_t *parent, int w, int h, int x, int y) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_pos(o, x, y);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(o, lv_color_hex(LT_PANEL_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(o, lv_color_hex(LT_PANEL_DK2), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(o, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(o, lv_color_hex(LT_RULE), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(o, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(o, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(o, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return o;
}

lv_obj_t *ltw_topbar(lv_obj_t *parent, uint32_t lamp_color,
                     const char *status_text, const char *timer_text,
                     lv_obj_t **out_lamp, lv_obj_t **out_status, lv_obj_t **out_timer) {
    lv_obj_t *bar = make_panel(parent, 780, 44, 10, 10);

    lv_obj_t *lamp = lv_obj_create(bar);
    lv_obj_set_size(lamp, 14, 14);
    lv_obj_set_pos(lamp, 16, 15);
    lv_obj_clear_flag(lamp, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lamp, lv_color_hex(lamp_color), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(lamp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(lamp, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(lamp, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (out_lamp) *out_lamp = lamp;

    lv_obj_t *status = lv_label_create(bar);
    lv_obj_set_pos(status, 40, 12);
    lv_label_set_text(status, status_text);
    lv_obj_set_style_text_color(status, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(status, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(status, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (out_status) *out_status = status;

    lv_obj_t *timer = lv_label_create(bar);
    lv_obj_align(timer, LV_ALIGN_RIGHT_MID, -16, 0);
    lv_label_set_text(timer, timer_text);
    lv_obj_set_style_text_color(timer, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(timer, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(timer, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    if (out_timer) *out_timer = timer;

    return bar;
}

lv_obj_t *ltw_chapter_banner(lv_obj_t *parent,
                             const char *chapter_num,
                             const char *chapter_title,
                             lv_event_cb_t on_chapter,
                             lv_event_cb_t on_book) {
    lv_obj_t *banner = make_panel(parent, 780, 74, 10, 396);

    lv_obj_t *cn = lv_label_create(banner);
    lv_obj_set_pos(cn, 18, 10);
    lv_label_set_text(cn, chapter_num);
    lv_obj_set_style_text_color(cn, lv_color_hex(LT_ACCENT), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cn, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(cn, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *ct = lv_label_create(banner);
    lv_obj_set_pos(ct, 18, 32);
    lv_label_set_text(ct, chapter_title);
    lv_obj_set_style_text_color(ct, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ct, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);

    const char *labels[2] = {"CHAPTER", "BOOK"};
    lv_event_cb_t cbs[2] = {on_chapter, on_book};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *b = lv_btn_create(banner);
        lv_obj_set_size(b, 160, 58);
        lv_obj_align(b, LV_ALIGN_RIGHT_MID, i == 0 ? -180 : -10, 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(LT_BTN_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_color(b, lv_color_hex(LT_BTN_PRESSED), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(b, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(b, lv_color_hex(LT_BTN_PRESSED), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_border_color(b, lv_color_hex(LT_BTN_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(b, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(b, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, labels[i]);
        lv_obj_set_style_text_color(l, lv_color_hex(LT_INK_DARK), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(l, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(l);
        if (cbs[i]) lv_obj_add_event_cb(b, cbs[i], LV_EVENT_ALL, NULL);
    }
    return banner;
}

void ltw_chapter_banner_refresh(lv_obj_t *banner) {
    if (!banner) return;
    // Creation order in ltw_chapter_banner(): child 0 = number, child 1 = title.
    lv_obj_t *num   = lv_obj_get_child(banner, 0);
    lv_obj_t *title = lv_obj_get_child(banner, 1);
    int ch = book_get_active_chapter();
    const char *name = book_get_chapter_name(ch);
    char buf[16];
    snprintf(buf, sizeof(buf), "CHAPTER %02d", ch + 1);
    if (num)   lv_label_set_text(num, buf);
    if (title) lv_label_set_text(title, name ? name : "Chapter 1");
}

void ltw_hw_legend(lv_obj_t *parent,
                   const char *lbl_rec,  lv_event_cb_t cb_rec,
                   const char *lbl_play, lv_event_cb_t cb_play,
                   const char *lbl_rwd,  lv_event_cb_t cb_rwd,
                   const char *lbl_ff,   lv_event_cb_t cb_ff,
                   const char *lbl_stop, lv_event_cb_t cb_stop) {
    const char    *caps[5] = {"REC", "PLAY", "RWD", "FF", "STOP"};
    const char    *lbls[5] = {lbl_rec,  lbl_play,  lbl_rwd,  lbl_ff,  lbl_stop};
    lv_event_cb_t  cbs[5]  = {cb_rec,   cb_play,   cb_rwd,   cb_ff,   cb_stop};
    uint32_t       cs[5]   = {0xC9302A, 0x2E6B45,  0x263250, 0x263250, 0x263250};  // REC red, PLAY green, rest navy
    uint32_t       cs_p[5] = {0x8A2018, 0x1F4A30,  0x35466E, 0x35466E, 0x35466E};  // pressed
    // Bigger, more finger-friendly: 84x48 (vs old 50x26)
    const int W = 84, H = 48;
    const int y_caps = 332;
    const int spacing = (800 - 5 * W) / 6;       // even gaps between caps
    for (int i = 0; i < 5; i++) {
        int x = spacing + i * (W + spacing);
        bool active   = (lbls[i] != NULL);
        bool tappable = (cbs[i]  != NULL);

        // Use a real button if tappable so we get press feedback for free
        lv_obj_t *c = tappable ? lv_btn_create(parent) : lv_obj_create(parent);
        lv_obj_set_size(c, W, H);
        lv_obj_set_pos(c, x, y_caps);
        lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(c, lv_color_hex(cs[i]),   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(c, lv_color_hex(cs_p[i]), LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(c, active ? 255 : 90, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(c, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(c, tappable ? 2 : 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(c, lv_color_hex(LT_BTN_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(c, 120, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(c, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (tappable) lv_obj_add_event_cb(c, cbs[i], LV_EVENT_CLICKED, NULL);

        lv_obj_t *cl = lv_label_create(c);
        lv_obj_center(cl);
        lv_label_set_text(cl, caps[i]);
        lv_obj_set_style_text_color(cl, lv_color_hex(0xFFF0E0), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(cl, &ui_font_Arhivo_regular_22, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_letter_space(cl, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

// Small filled circle helper for cassette detailing (hubs, teeth, screws, holes).
static lv_obj_t *cass_circle(lv_obj_t *p, int cx, int cy, int d, uint32_t color,
                            uint32_t border_color, int border_w) {
    lv_obj_t *o = lv_obj_create(p);
    lv_obj_set_size(o, d, d);
    lv_obj_set_pos(o, cx - d / 2, cy - d / 2);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, 255, 0);
    lv_obj_set_style_radius(o, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(o, border_w, 0);
    if (border_w) lv_obj_set_style_border_color(o, lv_color_hex(border_color), 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    return o;
}

// ── Retro cassette palette (navy shell + sunset rainbow band) ──
#define CASS_SHELL    0x4A5A7E   // molded slate-blue plastic (top of gradient)
#define CASS_SHELL_D  0x394663   // (bottom of gradient)
#define CASS_EDGE     0x232E45
#define CASS_SCREW    0x2C3850
#define CASS_LABEL    0xF4EEDD   // cream J-card
#define CASS_BADGE    0x262B36   // "A" side tab
#define CASS_WINDOW   0x20232C   // tape window
#define CASS_CLUTCH   0xF1EBDA   // white reel hub
#define CASS_NOTCH    0x33405C   // clutch notches (read as spinning gaps)

// Per-cassette reel animation state. The 6 "notches" on each white clutch are
// real child objects we re-position around the hub each tick, so the reels
// look like they're physically turning. Lives in the cassette's user_data.
typedef struct {
    lv_obj_t   *notch[2][6];
    int         cx[2], cy, orbit;
    float       angle;          // radians, advances clockwise
    bool        spinning;
    lv_obj_t   *screen;         // only animate while this screen is on top
    lv_timer_t *timer;
} cass_anim_t;

static void cass_spin_cb(lv_timer_t *t) {
    cass_anim_t *a = (cass_anim_t *)t->user_data;
    if (!a || !a->spinning || lv_scr_act() != a->screen) return;
    a->angle += 0.13f;          // gentle ~0.3 rev/s; same direction both reels
    for (int r = 0; r < 2; r++) {
        for (int k = 0; k < 6; k++) {
            float ang = a->angle + k * 1.04719755f;   // 60deg spacing
            lv_obj_t *n = a->notch[r][k];
            if (!n) continue;
            lv_obj_set_pos(n, a->cx[r] + (int)(a->orbit * cosf(ang)) - lv_obj_get_width(n) / 2,
                              a->cy     + (int)(a->orbit * sinf(ang)) - lv_obj_get_height(n) / 2);
        }
    }
}

// Wound tape disc + white 6-notch clutch. tape_r = outer wound radius,
// clutch_r = hub radius. Notch objects are stored for the spin animation.
static void cass_build_reel(lv_obj_t *cass, cass_anim_t *a, int reel,
                            int cx, int cy, int tape_r, int clutch_r) {
    cass_circle(cass, cx, cy, tape_r * 2, 0x5A3A22, 0x2C1C10, 2);          // wound tape (outer)
    for (int rr = tape_r - 7; rr > clutch_r + 3; rr -= 7)                  // winding rings
        cass_circle(cass, cx, cy, rr * 2, (rr & 7) ? 0x6E4A2C : 0x4E3220, 0, 0);
    cass_circle(cass, cx, cy, clutch_r * 2, CASS_CLUTCH, 0xB8A77E, 1);     // white hub
    a->cx[reel] = cx; a->cy = cy;
    a->orbit = clutch_r * 60 / 100;
    int nd = clutch_r * 38 / 100; if (nd < 5) nd = 5;                      // notch diameter
    static const float U[6] = { 0.f, 1.0472f, 2.0944f, 3.1416f, 4.1888f, 5.2360f };
    for (int k = 0; k < 6; k++)
        a->notch[reel][k] = cass_circle(cass, cx + (int)(a->orbit * cosf(U[k])),
                                              cy + (int)(a->orbit * sinf(U[k])),
                                              nd, CASS_NOTCH, 0, 0);
    cass_circle(cass, cx, cy, clutch_r * 30 / 100, CASS_BADGE, 0, 0);      // spindle
}

lv_obj_t *ltw_cassette(lv_obj_t *parent, int w, int h, int y_offset,
                       const char *book_title, const char *vu_tag) {
    (void)vu_tag;
    int x = (800 - w) / 2;
    int y = 64 + (322 - h) / 2 + y_offset;

    // ── Shell: navy molded plastic ──
    lv_obj_t *cass = lv_obj_create(parent);
    lv_obj_set_size(cass, w, h);
    lv_obj_set_pos(cass, x, y);
    lv_obj_clear_flag(cass, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(cass, lv_color_hex(CASS_SHELL), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(cass, lv_color_hex(CASS_SHELL_D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(cass, LV_GRAD_DIR_VER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cass, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(cass, lv_color_hex(CASS_EDGE), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cass, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cass, 16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(cass, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    cass_anim_t *a = (cass_anim_t *)lv_mem_alloc(sizeof(cass_anim_t));
    if (a) { memset(a, 0, sizeof(*a)); a->screen = parent; }
    lv_obj_set_user_data(cass, a);

    // Four corner screws
    cass_circle(cass, 18,     18,     13, CASS_SCREW, 0x55648A, 1);
    cass_circle(cass, w - 18, 18,     13, CASS_SCREW, 0x55648A, 1);
    cass_circle(cass, 18,     h - 18, 13, CASS_SCREW, 0x55648A, 1);
    cass_circle(cass, w - 18, h - 18, 13, CASS_SCREW, 0x55648A, 1);

    // ── Cream label across the top, with a "Side A" tab ──
    int labelW = w - 72, labelX = 36, labelY = 14, labelH = h * 30 / 100;
    lv_obj_t *label = lv_obj_create(cass);
    lv_obj_set_size(label, labelW, labelH);
    lv_obj_set_pos(label, labelX, labelY);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(label, lv_color_hex(CASS_LABEL), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(label, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *badge = lv_obj_create(label);
    lv_obj_set_size(badge, 30, 26);
    lv_obj_set_pos(badge, 6, 6);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(badge, lv_color_hex(CASS_BADGE), 0);
    lv_obj_set_style_bg_opa(badge, 255, 0);
    lv_obj_set_style_radius(badge, 4, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_t *bl = lv_label_create(badge);
    lv_obj_center(bl);
    lv_label_set_text(bl, "A");
    lv_obj_set_style_text_color(bl, lv_color_hex(0xF6ECD4), 0);
    lv_obj_set_style_text_font(bl, &ui_font_Arhivo_regular_22, 0);

    lv_obj_t *titleL = lv_label_create(label);
    lv_obj_set_width(titleL, labelW - 80);
    lv_label_set_long_mode(titleL, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(titleL, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(titleL, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(titleL, book_title);
    lv_obj_set_style_text_color(titleL, lv_color_hex(0x2A2E3A), 0);
    lv_obj_set_style_text_font(titleL, &ui_font_Arhivo_regular_22, 0);

    // ── Sunset rainbow band behind the reels (clipped to rounded corners) ──
    static const uint32_t STRIPE[5] = { 0xF0E2BE, 0xF1C24E, 0xEC9648, 0xE26A48, 0xC8474A };
    int bandX = 26, bandY = labelY + labelH + 8;
    int bandW = w - 52, bandH = h - bandY - 30;
    lv_obj_t *rb = lv_obj_create(cass);
    lv_obj_set_size(rb, bandW, bandH);
    lv_obj_set_pos(rb, bandX, bandY);
    lv_obj_clear_flag(rb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(rb, lv_color_hex(STRIPE[0]), 0);
    lv_obj_set_style_bg_opa(rb, 255, 0);
    lv_obj_set_style_border_width(rb, 0, 0);
    lv_obj_set_style_radius(rb, 8, 0);
    lv_obj_set_style_pad_all(rb, 0, 0);
    lv_obj_set_style_clip_corner(rb, true, 0);
    int sh = bandH / 5;
    for (int s = 0; s < 5; s++) {
        lv_obj_t *st = lv_obj_create(rb);
        lv_obj_set_size(st, bandW, (s == 4) ? bandH - 4 * sh : sh);
        lv_obj_set_pos(st, 0, s * sh);
        lv_obj_clear_flag(st, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(st, lv_color_hex(STRIPE[s]), 0);
        lv_obj_set_style_bg_opa(st, 255, 0);
        lv_obj_set_style_border_width(st, 0, 0);
        lv_obj_set_style_radius(st, 0, 0);
    }

    // ── Reels + central tape window ──
    int cy      = bandY + bandH / 2;
    int leftCx  = w * 27 / 100;
    int rightCx = w * 73 / 100;
    int clutchR = bandH * 26 / 100; if (clutchR < 16) clutchR = 16;
    int bigR    = bandH * 50 / 100;             // supply reel: lots of tape
    int smallR  = clutchR + (bigR - clutchR) / 2;  // take-up reel: less tape

    // Central window with vertical tape striations
    int winW = w * 13 / 100, winH = bandH - 18;
    lv_obj_t *win = lv_obj_create(cass);
    lv_obj_set_size(win, winW, winH);
    lv_obj_set_pos(win, w / 2 - winW / 2, cy - winH / 2);
    lv_obj_clear_flag(win, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(win, lv_color_hex(CASS_WINDOW), 0);
    lv_obj_set_style_bg_opa(win, 255, 0);
    lv_obj_set_style_radius(win, 4, 0);
    lv_obj_set_style_border_width(win, 0, 0);
    lv_obj_set_style_pad_all(win, 0, 0);
    for (int v = 0; v < 4; v++) {
        lv_obj_t *ln = lv_obj_create(win);
        lv_obj_set_size(ln, 2, winH - 16);
        lv_obj_align(ln, LV_ALIGN_LEFT_MID, 8 + v * (winW - 18) / 3, 0);
        lv_obj_clear_flag(ln, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(ln, lv_color_hex(0x6E665C), 0);
        lv_obj_set_style_bg_opa(ln, 200, 0);
        lv_obj_set_style_border_width(ln, 0, 0);
        lv_obj_set_style_radius(ln, 0, 0);
    }

    if (a) {
        cass_build_reel(cass, a, 0, leftCx,  cy, bigR,   clutchR);
        cass_build_reel(cass, a, 1, rightCx, cy, smallR, clutchR);
    }

    // ── Bottom mechanism: recessed lip + capstan/sprocket holes ──
    lv_obj_t *lip = lv_obj_create(cass);
    lv_obj_set_size(lip, w * 52 / 100, 20);
    lv_obj_align(lip, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_clear_flag(lip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(lip, lv_color_hex(CASS_SHELL_D), 0);
    lv_obj_set_style_bg_opa(lip, 255, 0);
    lv_obj_set_style_border_color(lip, lv_color_hex(CASS_EDGE), 0);
    lv_obj_set_style_border_width(lip, 1, 0);
    lv_obj_set_style_radius(lip, 4, 0);
    for (int k = -2; k <= 2; k++)
        if (k != 0) cass_circle(cass, w / 2 + k * (w * 11 / 100), h - 20, 8, 0x1B2536, 0, 0);

    return cass;
}

void ltw_cassette_set_spin(lv_obj_t *cass, bool on) {
    if (!cass) return;
    cass_anim_t *a = (cass_anim_t *)lv_obj_get_user_data(cass);
    if (!a) return;
    a->spinning = on;
    // Lazily create the spin timer; it self-gates on a->spinning + active screen.
    if (on && !a->timer) a->timer = lv_timer_create(cass_spin_cb, 66, a);
}

// ── Cassette hero (real artwork image + smooth spinning reels) ──
// cassette_body is the reference cassette (477x266) with a BLANK label; the
// label text is overlaid below so it's dynamic per screen. The reels are a
// PRE-RENDERED rotation sprite-sheet (cassette_reels[]) cycled by a timer —
// pre-rendered means no runtime-rotation jaggies and no off-center wobble
// (the earlier version rotated a square crop about an imprecise pivot, which
// read as a block orbiting instead of a hub spinning).
extern const lv_img_dsc_t  cassette_body;
extern const lv_img_dsc_t *cassette_reels[];
extern const int           cassette_reel_count;
#define HERO_W    477
#define HERO_LCX  141   // reel centres in the artwork (auto-detected)
#define HERO_RCX  310
#define HERO_CY   116
#define HERO_REEL 76    // reel sprite size (white-ring diameter)

typedef struct {
    lv_obj_t   *reel[2];
    int         frame;
    bool        spinning;
    lv_obj_t   *screen;
    lv_timer_t *timer;
} hero_anim_t;

static void hero_reel_cb(lv_timer_t *t) {
    hero_anim_t *a = (hero_anim_t *)t->user_data;
    if (!a || !a->spinning || lv_scr_act() != a->screen) return;   // only when visible
    a->frame = (a->frame + 1) % cassette_reel_count;
    if (a->reel[0]) lv_img_set_src(a->reel[0], cassette_reels[a->frame]);
    if (a->reel[1]) lv_img_set_src(a->reel[1], cassette_reels[a->frame]);
}

lv_obj_t *ltw_cassette_hero(lv_obj_t *parent, int y, const char *label,
                            uint32_t label_color, bool spinning) {
    lv_obj_t *body = lv_img_create(parent);
    lv_img_set_src(body, &cassette_body);
    lv_obj_set_pos(body, (800 - HERO_W) / 2, y);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

    // Dynamic label text on the blank cassette label (offset right of the "A").
    if (label && label[0]) {
        lv_obj_t *lab = lv_label_create(body);
        lv_obj_set_width(lab, 320);
        lv_label_set_long_mode(lab, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(lab, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(lab, LV_ALIGN_TOP_MID, 0, 31);
        lv_label_set_text(lab, label);
        lv_obj_set_style_text_color(lab, lv_color_hex(label_color), 0);
        lv_obj_set_style_text_font(lab, &ui_font_Arhivo_regular_22, 0);
        lv_obj_set_style_text_letter_space(lab, 1, 0);
    }

    hero_anim_t *a = (hero_anim_t *)lv_mem_alloc(sizeof(hero_anim_t));
    if (a) { memset(a, 0, sizeof(*a)); a->screen = parent; a->spinning = spinning; }

    int cx[2] = { HERO_LCX, HERO_RCX };
    for (int i = 0; i < 2; i++) {
        lv_obj_t *reel = lv_img_create(body);
        lv_img_set_src(reel, cassette_reels[0]);
        lv_obj_set_pos(reel, cx[i] - HERO_REEL / 2, HERO_CY - HERO_REEL / 2);
        lv_obj_clear_flag(reel, LV_OBJ_FLAG_SCROLLABLE);
        if (a) a->reel[i] = reel;
    }
    if (a && spinning) a->timer = lv_timer_create(hero_reel_cb, 55, a);
    return body;
}

// ─── Pilot lamp ──────────────────────────────────────────────────────────────
// IMPORTANT: this used to run an INFINITE opacity animation to "pulse" the lamp.
// On this RGB panel the LVGL display is full_refresh=1, so every animation frame
// re-pushes the ENTIRE 800x480 framebuffer into the continuously-scanned panel
// memory — and because screens are never destroyed, the pulse kept running on
// hidden Recording/Playback screens forever, forcing a full-frame repaint
// ~30x/sec on whatever screen was actually visible. That is the on-tap/idle
// "glitch" (tearing). The lamp is now a SOLID indicator: liveness is shown by
// the topbar colour (red=rec, green=play), the live VU meter, and the timer —
// none of which churn the whole screen when idle.
void ltw_pulse_lamp(lv_obj_t *lamp, uint32_t period_ms) {
    (void)period_ms;
    if (!lamp) return;
    lv_obj_set_style_bg_opa(lamp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ltw_stop_lamp_pulse(lv_obj_t *lamp) {
    if (!lamp) return;
    lv_obj_set_style_bg_opa(lamp, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ltw_picker_header(lv_obj_t *parent,
                       const char *eyebrow, const char *title,
                       const char *close_label, lv_event_cb_t on_close) {
    lv_obj_t *eb = lv_label_create(parent);
    lv_obj_set_pos(eb, 26, 18);
    lv_label_set_text(eb, eyebrow);
    lv_obj_set_style_text_color(eb, lv_color_hex(LT_INK_DIM), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(eb, &ui_font_Arhivo_regular_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(eb, 3, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *ti = lv_label_create(parent);
    lv_obj_set_pos(ti, 26, 40);
    lv_label_set_text(ti, title);
    lv_obj_set_style_text_color(ti, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ti, &ui_font_archivo_42, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cl = lv_btn_create(parent);
    lv_obj_set_size(cl, 140, 46);
    lv_obj_align(cl, LV_ALIGN_TOP_RIGHT, -20, 16);
    lv_obj_set_style_bg_color(cl, lv_color_hex(LT_BTN_BG), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cl, lv_color_hex(LT_BTN_PRESSED), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_color(cl, lv_color_hex(LT_BTN_BORDER), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cl, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cl, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_t *cll = lv_label_create(cl);
    lv_label_set_text(cll, close_label);
    lv_obj_set_style_text_color(cll, lv_color_hex(LT_INK), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(cll, &ui_font_Arhivo_regular_18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(cll);
    // CLICKED only. Previously this also bound LV_EVENT_PRESSED, so each tap
    // fired the close/navigation callback twice (once on touch-down, once on
    // release) — a double screen-change landmine.
    if (on_close) lv_obj_add_event_cb(cl, on_close, LV_EVENT_CLICKED, NULL);
}
