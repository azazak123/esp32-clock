#include "dashboard.h"

#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <time.h>

#include "lcd.h"
#include "sensors_bme680.h"
#include "ui.h"

static const char *TAG = "DASHBOARD";

static void update_time(ui_state_t *ui) {
  time_t now;
  struct tm timeinfo;
  char time_buff[16];

  time(&now);
  localtime_r(&now, &timeinfo);

  snprintf(time_buff, sizeof(time_buff), "%02d:%02d", timeinfo.tm_hour,
           timeinfo.tm_min);

  ui_clock_update(ui, time_buff);
}

static void update_date(ui_state_t *ui) {
  time_t now;
  struct tm timeinfo;
  char date_buff[32];

  time(&now);
  localtime_r(&now, &timeinfo);

  static const char *week_days[] = {"Sun", "Mon", "Tue", "Wed",
                                    "Thu", "Fri", "Sat"};

  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  snprintf(date_buff, sizeof(date_buff), "%s, %02d %s",
           week_days[timeinfo.tm_wday], timeinfo.tm_mday,
           months[timeinfo.tm_mon]);

  ui_date_update(ui, date_buff);
}

static void update_battery(ui_state_t *ui) {
  // TODO: Read real ADC value here
  // float voltage = adc_read_voltage();

  // Placeholder
  int percent = 100;
  bool charging = false;

  ui_battery_update(ui, percent, charging);
}

static void dashboard_task_loop(void *param) {
  ESP_LOGI(TAG, "Starting Dashboard Logic...");

  lv_display_t *disp_handle = NULL;
  esp_lcd_touch_handle_t touch_handle;

  lcd_init(&disp_handle, &touch_handle);

  ui_state_t ui_state;

  if (lvgl_port_lock(0)) {
    ui_state = ui_setup(disp_handle);
    ui_state = ui_setup(disp_handle);
    lvgl_port_unlock();
  } else {
    ESP_LOGE(TAG, "Failed to lock LVGL for setup");
  }

  bme680_state_t sensor_data;

  while (true) {
    bme680_get_data(&sensor_data);

    if (lvgl_port_lock(0)) {
      ui_sensors_update(&ui_state, &sensor_data);

      update_time(&ui_state);
      update_date(&ui_state);

      update_battery(&ui_state);
      lvgl_port_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

bool dashboard_app_start(void) {
  BaseType_t res = xTaskCreate(dashboard_task_loop, "dashboard", 4096, NULL,
                               tskIDLE_PRIORITY + 1, NULL);

  if (res == pdPASS) {
    ESP_LOGI(TAG, "App started successfully");
    return true;
  } else {
    ESP_LOGE(TAG, "Failed to start App task");
    return false;
  }
}
