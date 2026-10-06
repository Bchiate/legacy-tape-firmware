// Screen6 — Stopped (tape parked mid-session). Press REC to overdub, PLAY to review.
#ifndef UI_SCREEN6_H
#define UI_SCREEN6_H
#ifdef __cplusplus
extern "C" {
#endif
extern void ui_Screen6_screen_init(void);
extern void ui_Screen6_screen_destroy(void);
extern lv_obj_t *ui_Screen6;
extern lv_obj_t *ui_S6_Timer;
extern lv_obj_t *ui_S6_PilotLamp;
#ifdef __cplusplus
}
#endif
#endif
