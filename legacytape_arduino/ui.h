// Legacy Tape — master UI header
// Generated screens 1-3 originate from SquareLine Studio 1.6.1 (LVGL 8.3.11).
// Screens 4-14 are hand-written but follow the same LVGL 8.3 API.
#ifndef _LEGACYTAPE_UI_H
#define _LEGACYTAPE_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined __has_include
  #if __has_include("lvgl.h")
    #include "lvgl.h"
  #elif __has_include("lvgl/lvgl.h")
    #include "lvgl/lvgl.h"
  #else
    #include "lvgl.h"
  #endif
#else
  #include "lvgl.h"
#endif

#include "ui_helpers.h"
#include "ui_comp.h"
#include "ui_comp_hook.h"
#include "ui_events.h"

// SCREENS
#include "ui_Screen1.h"
#include "ui_Screen2.h"
#include "ui_Screen3.h"
#include "ui_Screen4.h"
#include "ui_Screen5.h"
#include "ui_Screen6.h"
#include "ui_Screen7.h"
#include "ui_Screen8.h"
#include "ui_Screen9.h"
#include "ui_Screen10.h"
#include "ui_Screen11.h"
#include "ui_Screen12.h"
#include "ui_Screen13.h"
#include "ui_Screen14.h"

// EVENTS
extern lv_obj_t *ui____initial_actions0;

// IMAGES
LV_IMG_DECLARE(ui_img_qr_card_empty_2_png);
LV_IMG_DECLARE(ui_img_1240451101);

// FONTS
LV_FONT_DECLARE(ui_font_Archivobig);
LV_FONT_DECLARE(ui_font_Arhivo_regular_16);
LV_FONT_DECLARE(ui_font_Arhivo_regular_18);
LV_FONT_DECLARE(ui_font_Arhivo_regular_20);
LV_FONT_DECLARE(ui_font_Arhivo_regular_22);
LV_FONT_DECLARE(ui_font_Font1);
LV_FONT_DECLARE(ui_font_archivo_42);
LV_FONT_DECLARE(ui_font_archivo_56);
LV_FONT_DECLARE(ui_font_archivo_50);

// UI lifecycle
void ui_init(void);
void ui_destroy(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
