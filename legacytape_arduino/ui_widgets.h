// Shared widget builders for Legacy Tape screens (Screens 5-14).
// Screen4 is kept fully inlined as the canonical reference.
#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ui.h"

// Dev mode flag (defined in the .ino). When non-zero, screens skip real side
// effects (mic, upload, playback) so the UI can be paged through for styling.
extern int g_dev_mode;

// ── Navy theme (sampled from the cassette artwork so all chrome matches it) ──
#define LT_BG          0x161C2A   // deep navy = cassette art background
#define LT_INK         0xF4EEDD   // cream primary text = cassette label
#define LT_INK_DIM     0x90A4C0   // muted slate-blue secondary text
#define LT_ACCENT      0xE26A48   // sunset orange = cassette stripe
#define LT_PANEL_DARK  0x1E2740   // slate panel (top of gradient)
#define LT_PANEL_DK2   0x18202F   // slate panel (bottom of gradient)
#define LT_RULE        0x33405C   // panel border = cassette navy edge
#define LT_BURGUNDY    0x8A3024   // (kept; re-skinned per call site)
#define LT_RED         0xE53935   // semantic: record
#define LT_RED_CTA     0xC94A2A   // (kept; re-skinned per call site)
#define LT_GREEN       0x4AC06A   // semantic: ok / play / AI
#define LT_AMBER       0xE5B03A   // semantic: warning / ready
#define LT_INK_DARK    0xF4EEDD   // button label on navy buttons -> cream
// Navy button tokens
#define LT_BTN_BG      0x263250
#define LT_BTN_PRESSED 0x35466E
#define LT_BTN_BORDER  0x46587E

// Builds the 780x44 top bar at (10,10). Returns container; also sets out_status, out_timer.
lv_obj_t *ltw_topbar(lv_obj_t *parent,
                     uint32_t lamp_color,
                     const char *status_text,
                     const char *timer_text,
                     lv_obj_t **out_lamp,
                     lv_obj_t **out_status,
                     lv_obj_t **out_timer);

// Builds the 780x74 chapter banner at (10, 396) with CHAPTER + BOOK buttons.
// Pass NULL for events to skip wiring.
lv_obj_t *ltw_chapter_banner(lv_obj_t *parent,
                             const char *chapter_num,
                             const char *chapter_title,
                             lv_event_cb_t on_chapter,
                             lv_event_cb_t on_book);

// Re-reads the active chapter (book.h) into a banner built by
// ltw_chapter_banner(). Screens are built once and cached, so they call this
// from their LV_EVENT_SCREEN_LOADED handler to avoid showing a stale chapter.
void ltw_chapter_banner_refresh(lv_obj_t *banner);

// Builds the 5-cap hw legend row for the Uxcell piano interlock buttons.
// Layout left-to-right: REC · PLAY · RWD · FF · STOP.
//
// The caps double as touch buttons, so every transport action is also
// reachable without the hardware keys. Pass a callback for each cap that
// should be tappable; pass NULL to render that cap dimmed (= not meaningful
// in the current state).
//
// Pass NULL for label to dim, NULL for callback to make it non-tappable.
void ltw_hw_legend(lv_obj_t *parent,
                   const char *lbl_rec,  lv_event_cb_t cb_rec,
                   const char *lbl_play, lv_event_cb_t cb_play,
                   const char *lbl_rwd,  lv_event_cb_t cb_rwd,
                   const char *lbl_ff,   lv_event_cb_t cb_ff,
                   const char *lbl_stop, lv_event_cb_t cb_stop);

// Builds a centered cassette panel with book title. w,h sized.
// Tag is "REC" / "OUT" or NULL to skip the VU meter on the right.
lv_obj_t *ltw_cassette(lv_obj_t *parent,
                       int w, int h, int y_offset,
                       const char *book_title,
                       const char *vu_tag);

// Turn the cassette's reels spinning on/off (call on the Recording/Playback
// screens). The reels animate only while their screen is on top.
void ltw_cassette_set_spin(lv_obj_t *cass, bool on);

// HERO cassette: the reference artwork as an embedded image with smooth
// (frame-cycled) spinning reels. `label`/`label_color` set the dynamic cassette
// label text; `spinning` turns the reels (self-gated to `parent` being on top).
// Centered at top `y`. Returns the body image object.
lv_obj_t *ltw_cassette_hero(lv_obj_t *parent, int y, const char *label,
                            uint32_t label_color, bool spinning);

// Picker header (used by Book Picker / Chapter Picker / Settings).
void ltw_picker_header(lv_obj_t *parent,
                       const char *eyebrow,
                       const char *title,
                       const char *close_label,
                       lv_event_cb_t on_close);

// Pulse a pilot lamp's opacity 255<->70 forever (period_ms full cycle).
// Use for the RECORDING red lamp + READY amber lamp. Cancel with stop.
void ltw_pulse_lamp(lv_obj_t *lamp, uint32_t period_ms);
void ltw_stop_lamp_pulse(lv_obj_t *lamp);

#ifdef __cplusplus
}
#endif
#endif
