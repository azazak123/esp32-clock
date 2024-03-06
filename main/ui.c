#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "esp_lvgl_port.h"
#include "font/lv_font.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"

#include "background.c"
#include "misc/lv_style.h"
#include "misc/lv_style_gen.h"

const lv_style_const_prop_t sensor_style_prop[] = {
    LV_STYLE_CONST_TEXT_ALIGN(LV_TEXT_ALIGN_CENTER),
    LV_STYLE_CONST_HEIGHT(60),
    LV_STYLE_CONST_WIDTH(60),
    LV_STYLE_CONST_RADIUS(5),
    LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE(0, 255, 124)),
    LV_STYLE_CONST_BORDER_WIDTH(2),
    LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_14),
    LV_STYLE_CONST_PROPS_END,
};

LV_STYLE_CONST_INIT(sensor_style, sensor_style_prop);

const lv_style_const_prop_t time_style_prop[] = {
    LV_STYLE_CONST_HEIGHT(LV_PCT(40)), LV_STYLE_CONST_WIDTH(LV_PCT(100)),
    LV_STYLE_CONST_BORDER_WIDTH(0),    LV_STYLE_CONST_BG_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_PROPS_END,
};

LV_STYLE_CONST_INIT(time_style, time_style_prop);

const lv_style_const_prop_t time_label_style_prop[] = {
    LV_STYLE_CONST_TEXT_ALIGN(LV_TEXT_ALIGN_CENTER),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_48),
    LV_STYLE_CONST_PROPS_END,
};

LV_STYLE_CONST_INIT(time_label_style, time_label_style_prop);

const lv_style_const_prop_t sensors_style_prop[] = {
    LV_STYLE_CONST_FLEX_FLOW(LV_FLEX_FLOW_ROW_WRAP),
    LV_STYLE_CONST_FLEX_MAIN_PLACE(LV_FLEX_ALIGN_SPACE_EVENLY),
    LV_STYLE_CONST_LAYOUT(LV_LAYOUT_FLEX),
    LV_STYLE_CONST_HEIGHT(LV_PCT(60)),
    LV_STYLE_CONST_WIDTH(LV_PCT(100)),
    LV_STYLE_CONST_BG_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_PAD_TOP(0),
    LV_STYLE_CONST_BORDER_WIDTH(0),
    LV_STYLE_CONST_PROPS_END,

};

LV_STYLE_CONST_INIT(sensors_style, sensors_style_prop);

const lv_style_const_prop_t main_style_prop[] = {

    LV_STYLE_CONST_HEIGHT(LV_PCT(100)),
    LV_STYLE_CONST_WIDTH(LV_PCT(100)),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_FLEX_FLOW(LV_FLEX_FLOW_COLUMN),
    LV_STYLE_CONST_LAYOUT(LV_LAYOUT_FLEX),
    LV_STYLE_CONST_PROPS_END,
};

LV_STYLE_CONST_INIT(main_style, main_style_prop);

LV_IMG_DECLARE(background);

void ui_setup(lv_display_t *disp_handle) {
  lvgl_port_lock(0);

  lv_obj_t *scr = lv_disp_get_scr_act(disp_handle);

  lv_obj_t *main = lv_img_create(scr);
  lv_img_set_src(main, &background);
  lv_obj_add_style(main, &main_style, 0);

  lv_obj_t *time = lv_obj_create(main);
  lv_obj_add_style(time, &time_style, 0);

  lv_obj_t *time_label = lv_label_create(time);
  lv_label_set_text(time_label, "12:30");
  lv_obj_add_style(time_label, &time_label_style, 0);

  lv_obj_t *sensors = lv_obj_create(main);
  lv_obj_add_style(sensors, &sensors_style, 0);

  lv_obj_t *label = lv_label_create(sensors);
  lv_label_set_text(label, "IAQ: N/A");
  lv_obj_add_style(label, &sensor_style, 0);

  label = lv_label_create(sensors);
  lv_label_set_text(label, "Temperature: N/A");
  lv_obj_add_style(label, &sensor_style, 0);

  label = lv_label_create(sensors);
  lv_label_set_text(label, "Pressure: N/A");
  lv_obj_add_style(label, &sensor_style, 0);

  label = lv_label_create(sensors);
  lv_label_set_text(label, "Humidity: N/A");
  lv_obj_add_style(label, &sensor_style, 0);

  label = lv_label_create(sensors);
  lv_label_set_text(label, "Gas: N/A");
  lv_obj_add_style(label, &sensor_style, 0);

  label = lv_label_create(sensors);
  lv_label_set_text(label, "Accuracy: N/A");
  lv_obj_add_style(label, &sensor_style, 0);

  lvgl_port_unlock();
}