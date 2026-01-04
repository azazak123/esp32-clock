#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dashboard.h"
#include "lcd.h"
#include "nvs_flash.h"
#include "sensors_bme680.h"
#include "msg.h"
#include "wifi.h"

// Timeout for starting up USB CDC driver
#define START_TIMEOUT_MS 5000

void app_main(void) {
  vTaskDelay(pdMS_TO_TICKS(START_TIMEOUT_MS));
  ESP_LOGI("MAIN", "System Starting...");

    esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret ==
  ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  QueueHandle_t queue = xQueueCreate(10, sizeof(msg_t));
  if (queue == NULL) {
    ESP_LOGE("MAIN", "Failed to create queue");
  }

  wifi_start(queue);

  if (!bme680_start()) {
    ESP_LOGE("MAIN", "Sensors Init Failed!");
  }

  if (!dashboard_app_start(queue)) {
    ESP_LOGE("MAIN", "Dashboard Init Failed!");
  }

  ESP_LOGI("MAIN", "All systems running.");
}
