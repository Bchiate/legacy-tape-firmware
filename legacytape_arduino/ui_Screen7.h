// Screen7 — Playback. Triggered by physical PLAY button.
#ifndef UI_SCREEN7_H
#define UI_SCREEN7_H
#ifdef __cplusplus
extern "C" {
#endif
extern void ui_Screen7_screen_init(void);
extern void ui_Screen7_screen_destroy(void);
// STOP: stop playback and show Screen4 (Ready). Used by the on-screen STOP
// button and by the hardware STOP key.
extern void ui_Screen7_stop(void);
extern lv_obj_t *ui_Screen7;
extern lv_obj_t *ui_S7_Timer;
#ifdef __cplusplus
}
#endif
#endif
