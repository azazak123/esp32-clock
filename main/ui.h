#ifndef _UIH_
#define _UIH_

#include "misc/lv_types.h"

#include "bme680.h"

typedef struct ui_state_t {
  lv_obj_t *time_label;

  // sensors
  lv_obj_t *iaq_label;
  lv_obj_t *temp_label;
  lv_obj_t *pressure_label;
  lv_obj_t *humidity_label;
  lv_obj_t *gas_label;
  lv_obj_t *accuracy_label;
} ui_state_t;

ui_state_t ui_setup(lv_display_t *disp_handle);
void ui_sensors_update(ui_state_t *state, bme680_state_t *bme680_state);

#endif