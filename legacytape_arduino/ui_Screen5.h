// Screen5 — Recording. Triggered by physical REC button.
#ifndef UI_SCREEN5_H
#define UI_SCREEN5_H
#ifdef __cplusplus
extern "C" {
#endif
extern void ui_Screen5_screen_init(void);
extern void ui_Screen5_screen_destroy(void);
// STOP: queue finalize, stop the take, show Screen6. Used by the on-screen
// STOP cap and by the hardware STOP key.
extern void ui_Screen5_stop(void);
extern lv_obj_t *ui_Screen5;
extern lv_obj_t *ui_S5_Timer;
extern lv_obj_t *ui_S5_PilotLamp;
#ifdef __cplusplus
}
#endif
#endif
