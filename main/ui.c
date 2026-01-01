#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "esp_lvgl_port.h"
#include "font/lv_font.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "misc/lv_style.h"
#include "misc/lv_style_gen.h"

#include "background.c"
#include "ui.h"

static const lv_style_const_prop_t sensor_style_prop[] = {
    LV_STYLE_CONST_TEXT_ALIGN(LV_TEXT_ALIGN_CENTER),
    LV_STYLE_CONST_HEIGHT(60),
    LV_STYLE_CONST_WIDTH(60),
    LV_STYLE_CONST_RADIUS(5),
    LV_STYLE_CONST_BORDER_COLOR(LV_COLOR_MAKE(0, 255, 124)),
    LV_STYLE_CONST_BORDER_WIDTH(2),
    LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_14),
    LV_STYLE_CONST_PROPS_END,
};

static LV_STYLE_CONST_INIT(sensor_style, sensor_style_prop);

static const lv_style_const_prop_t time_style_prop[] = {
    LV_STYLE_CONST_HEIGHT(LV_PCT(40)), LV_STYLE_CONST_WIDTH(LV_PCT(100)),
    LV_STYLE_CONST_BORDER_WIDTH(0),    LV_STYLE_CONST_BG_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_PROPS_END,
};

static LV_STYLE_CONST_INIT(time_style, time_style_prop);

static const lv_style_const_prop_t time_label_style_prop[] = {
    LV_STYLE_CONST_TEXT_ALIGN(LV_TEXT_ALIGN_CENTER),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_48),
    LV_STYLE_CONST_PROPS_END,
};

static LV_STYLE_CONST_INIT(time_label_style, time_label_style_prop);

static const lv_style_const_prop_t sensors_style_prop[] = {
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

static LV_STYLE_CONST_INIT(sensors_style, sensors_style_prop);

static const lv_style_const_prop_t main_style_prop[] = {

    LV_STYLE_CONST_HEIGHT(LV_PCT(100)),
    LV_STYLE_CONST_WIDTH(LV_PCT(100)),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_FLEX_FLOW(LV_FLEX_FLOW_COLUMN),
    LV_STYLE_CONST_LAYOUT(LV_LAYOUT_FLEX),
    LV_STYLE_CONST_PROPS_END,
};

static LV_STYLE_CONST_INIT(main_style, main_style_prop);

LV_IMG_DECLARE(background)

ui_state_t ui_setup(lv_display_t *disp_handle) {
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

  lv_obj_t *iaq_label = lv_label_create(sensors);
  lv_label_set_text(iaq_label, "IAQ: N/A");
  lv_obj_add_style(iaq_label, &sensor_style, 0);

  lv_obj_t *temp_label = lv_label_create(sensors);
  lv_label_set_text(temp_label, "Temperature: N/A");
  lv_obj_add_style(temp_label, &sensor_style, 0);

  lv_obj_t *pressure_label = lv_label_create(sensors);
  lv_label_set_text(pressure_label, "Pressure: N/A");
  lv_obj_add_style(pressure_label, &sensor_style, 0);

  lv_obj_t *humidity_label = lv_label_create(sensors);
  lv_label_set_text(humidity_label, "Humidity: N/A");
  lv_obj_add_style(humidity_label, &sensor_style, 0);

  lv_obj_t *gas_label = lv_label_create(sensors);
  lv_label_set_text(gas_label, "Gas: N/A");
  lv_obj_add_style(gas_label, &sensor_style, 0);

  lv_obj_t *accuracy_label = lv_label_create(sensors);
  lv_label_set_text(accuracy_label, "Accuracy: N/A");
  lv_obj_add_style(accuracy_label, &sensor_style, 0);

  lvgl_port_unlock();

  ui_state_t state = {
      .time_label = time_label,
      .iaq_label = iaq_label,
      .temp_label = temp_label,
      .pressure_label = pressure_label,
      .humidity_label = humidity_label,
      .gas_label = gas_label,
      .accuracy_label = accuracy_label,
  };

  return state;
}

void ui_sensors_update(ui_state_t *state, bme680_state_t *bme680_state) {
  char buffer[30];

  lvgl_port_lock(0);

  sprintf(buffer, "IAQ: %.1f", bme680_state->iaq);
  lv_label_set_text(state->iaq_label, buffer);

  sprintf(buffer, "Temperature: %.1f", bme680_state->temp);
  lv_label_set_text(state->temp_label, buffer);

  sprintf(buffer, "Pressure: %.1f", bme680_state->pressure);
  lv_label_set_text(state->pressure_label, buffer);

  sprintf(buffer, "Humidity: %.1f", bme680_state->humidity);
  lv_label_set_text(state->humidity_label, buffer);

  sprintf(buffer, "Gas: %.1f", bme680_state->gas);
  lv_label_set_text(state->gas_label, buffer);

  sprintf(buffer, "Accuracy: %s", bme680_state->accuracy == 3 ? "High" : "Low");
  lv_label_set_text(state->accuracy_label, buffer);

  lvgl_port_unlock();
}