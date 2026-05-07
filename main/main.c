#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dashboard.h"
#include "lcd.h"
#include "nvs_flash.h"
#include "sensors_bme680.h"
#include "msg.h"
#include "net_mgr.h"
#include "esp_pm.h"

// Timeout for starting up USB CDC driver
#define START_TIMEOUT_MS 5000

void init_power_management() {
  esp_pm_config_t pm_config = {
    .max_freq_mhz = 240,
    .min_freq_mhz = 40,
    .light_sleep_enable = true
  };

  esp_err_t err = esp_pm_configure(&pm_config);
  if (err == ESP_OK) {
    ESP_LOGI("POWER", "Power management initialized successfully");
  }
}

void app_main(void) {
  vTaskDelay(pdMS_TO_TICKS(START_TIMEOUT_MS));
  ESP_LOGI("MAIN", "System Starting...");

  init_power_management();

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  QueueHandle_t gui_queue = xQueueCreate(10, sizeof(gui_msg_t));
  if (gui_queue == NULL) {
    ESP_LOGE("MAIN", "Failed to create queue");
  }

  QueueHandle_t net_mgr_queue = xQueueCreate(10, sizeof(net_msg_t));
  if (net_mgr_queue == NULL) {
    ESP_LOGE("MAIN", "Failed to create queue");
  }

  if (!net_mgr_start(net_mgr_queue, gui_queue)) {
    ESP_LOGE("MAIN", "Dashboard Init Failed!");
  }

  if (!bme680_start(gui_queue)) {
    ESP_LOGE("MAIN", "Sensors Init Failed!");
  }

  if (!dashboard_app_start(gui_queue, net_mgr_queue)) {
    ESP_LOGE("MAIN", "Net Mgr Init Failed!");
  }

  ESP_LOGI("MAIN", "All systems running.");
}
