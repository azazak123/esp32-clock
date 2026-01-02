#ifndef UI_H
#define UI_H

#include "lvgl.h"
#include "bme680.h"

typedef struct {
    lv_obj_t *screen;

    // Row 1
    lv_obj_t *lbl_time;     
    lv_obj_t *lbl_date;     
    lv_obj_t *lbl_bat;
    lv_obj_t *gif_container; 

    // Row 2
    lv_obj_t *lbl_temp_val;
    lv_obj_t *arc_temp;

    // Row 3
    lv_obj_t *lbl_hum_val;
    lv_obj_t *bar_hum;

    // Row 4
    lv_obj_t *lbl_iaq_val;
    lv_obj_t *lbl_iaq_text;
    lv_obj_t *lbl_co2_val;
    lv_obj_t *lbl_press_val;

} ui_state_t;

ui_state_t ui_setup(lv_display_t *display);
void ui_sensors_update(ui_state_t *ui, const bme680_state_t *data);
void ui_clock_update(ui_state_t *ui, const char *time_str);
void ui_date_update(ui_state_t *ui, const char *date_str);
void ui_battery_update(ui_state_t *ui, int level_percent, bool is_charging);

#endif
