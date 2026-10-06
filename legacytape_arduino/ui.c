// Legacy Tape — master UI init/destroy.
// Loads Screen1 (Pairing) as the default. Firmware can swap this for
// Screen4 (Ready) once NVS confirms the device has been paired before.
#include "ui.h"
#include "ui_helpers.h"

lv_obj_t *ui____initial_actions0;

#if LV_COLOR_DEPTH != 16
    #error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP != 0
    #error "LV_COLOR_16_SWAP should be 0 to match SquareLine Studio's settings"
#endif

void ui_init(void) {
    LV_EVENT_GET_COMP_CHILD = lv_event_register_id();

    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp,
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    // Screens 1-3 are initialized eagerly (they're used on first boot).
    // Screens 4-14 init lazily via _ui_screen_change(), saving ~250 KB RAM at startup.
    ui_Screen1_screen_init();
    ui_Screen2_screen_init();
    ui_Screen3_screen_init();

    ui____initial_actions0 = lv_obj_create(NULL);
    lv_disp_load_scr(ui_Screen1);
}

void ui_destroy(void) {
    if (ui_Screen1)  ui_Screen1_screen_destroy();
    if (ui_Screen2)  ui_Screen2_screen_destroy();
    if (ui_Screen3)  ui_Screen3_screen_destroy();
    if (ui_Screen4)  ui_Screen4_screen_destroy();
    if (ui_Screen5)  ui_Screen5_screen_destroy();
    if (ui_Screen6)  ui_Screen6_screen_destroy();
    if (ui_Screen7)  ui_Screen7_screen_destroy();
    if (ui_Screen8)  ui_Screen8_screen_destroy();
    if (ui_Screen9)  ui_Screen9_screen_destroy();
    if (ui_Screen10) ui_Screen10_screen_destroy();
    if (ui_Screen11) ui_Screen11_screen_destroy();
    if (ui_Screen12) ui_Screen12_screen_destroy();
    if (ui_Screen13) ui_Screen13_screen_destroy();
    if (ui_Screen14) ui_Screen14_screen_destroy();
}
