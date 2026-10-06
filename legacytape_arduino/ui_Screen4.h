// Screen4 — Ready (home / default state, hardware-button driven)
#ifndef UI_SCREEN4_H
#define UI_SCREEN4_H
#ifdef __cplusplus
extern "C" {
#endif
extern void ui_Screen4_screen_init(void);
extern void ui_Screen4_screen_destroy(void);
extern lv_obj_t *ui_Screen4;
extern lv_obj_t *ui_S4_Timer;
extern lv_obj_t *ui_S4_StatusLabel;
extern lv_obj_t *ui_S4_PilotLamp;
extern lv_obj_t *ui_S4_ChapterTitle;
extern lv_obj_t *ui_S4_ChapterNum;
extern lv_obj_t *ui_S4_BookTitle;
extern void ui_event_S4_btn_chapter(lv_event_t *e);
extern void ui_event_S4_btn_book(lv_event_t *e);
extern void ui_event_S4_btn_settings(lv_event_t *e);
extern void ui_event_S4_btn_rec(lv_event_t *e);
extern void ui_event_S4_btn_play(lv_event_t *e);
#ifdef __cplusplus
}
#endif
#endif
