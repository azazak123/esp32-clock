#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "sensors_bme680.h"
#include "lcd.h"
#include "dashboard.h"

// Timeout for starting up USB CDC driver
#define START_TIMEOUT_MS 5000

void app_main(void) {
    vTaskDelay(pdMS_TO_TICKS(START_TIMEOUT_MS));
    ESP_LOGI("MAIN", "System Starting...");

    if (!bme680_start()) {
        ESP_LOGE("MAIN", "Sensors Init Failed!");
    }

    if (!dashboard_app_start()) {
        ESP_LOGE("MAIN", "Dashboard Init Failed!");
    }
    
    ESP_LOGI("MAIN", "All systems running.");
}
