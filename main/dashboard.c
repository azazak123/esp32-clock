#include "dashboard.h"

#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <time.h>

#include "lcd.h"
#include "msg.h"
#include "sensors_bme680.h"
#include "ui.h"

static const char *TAG = "DASHBOARD";

static QueueHandle_t gui_queue = NULL;
static QueueHandle_t net_queue = NULL;

static void long_press_handler(lv_event_t *e) {
  ESP_LOGI(TAG, "Long press detected");

  net_msg_t msg;
  msg.type = NET_MSG_INIT_WIFI;
  
  if (xQueueSend(net_queue, &msg, 0) != pdTRUE) {
    ESP_LOGE(TAG, "ERROR: Queue is FULL or Failed to send!");
  }
}

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
    ui_state = ui_setup(disp_handle, long_press_handler);
    lvgl_port_unlock();
  } else {
    ESP_LOGE(TAG, "Failed to lock LVGL for setup");
  }

  while (true) {
    if (lvgl_port_lock(0)) {
      gui_msg_t msg;
      if (xQueueReceive(gui_queue, &msg, 0)) {
        switch (msg.type) {
        case GUI_MSG_SHOW_QR: {
          char *uri_str = (char *)msg.value.text_data;
          ui_show_dpp_qr(&ui_state, uri_str);
          free(uri_str);
          break;
        }
        case GUI_MSG_HIDE_QR:
          ui_hide_dpp_qr(&ui_state);
          break;
        case GUI_MSG_UPDATE_SENSORS: {
          bme680_state_t sensor_data = msg.value.sensor_data;
          ui_sensors_update(&ui_state, &sensor_data);
          break;
        }
        }
      }

      update_time(&ui_state);
      update_date(&ui_state);
      update_battery(&ui_state);

      lvgl_port_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

bool dashboard_app_start(QueueHandle_t _gui_queue, QueueHandle_t _net_queue) {
  gui_queue = _gui_queue;
  net_queue = _net_queue;

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
