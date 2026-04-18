#include "sensors_bme680.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stdio.h>

#include "bsec2.h"
#include "bsec_datatypes.h"
#include "bsec_iaq.h"
#include "msg.h"
#include "nvs.h"

static const char *TAG = "BME680";

#define SDA_PIN 37
#define SCL_PIN 39
#define I2C_PORT I2C_NUM_0
#define I2C_CLK_SPEED 100000

#define BME680_SAMPLE_RATE BSEC_SAMPLE_RATE_LP

#define STATE_SAVE_INTERVAL_MS (60 * 60 * 1000)

static QueueHandle_t gui_queue = NULL;

static bme680_state_t internal_state;
static bsec2_t bsec_instance;
static i2c_bus_t i2c_bus;
static uint8_t bsec_state_buffer[BSEC_MAX_STATE_BLOB_SIZE];

static bsec_sensor_t sensors_list[] = {
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_GAS_PERCENTAGE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_CO2_EQUIVALENT,
};

static void load_bsec_state(bsec2_t *bsec) {
  nvs_handle_t my_handle;
  esp_err_t err;

  err = nvs_open("bme680", NVS_READONLY, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Error opening NVS handle!");
    return;
  }

  size_t required_size = BSEC_MAX_STATE_BLOB_SIZE;
  err = nvs_get_blob(my_handle, "bsec_state", bsec_state_buffer, &required_size);

  if (err == ESP_OK && required_size > 0 &&
      required_size <= BSEC_MAX_STATE_BLOB_SIZE) {
    if (bsec2_set_state(bsec, bsec_state_buffer)) {
      ESP_LOGI(TAG, "BSEC state loaded from NVS successfully");
    } else {
      ESP_LOGE(TAG, "Failed to set BSEC state");
    }
  } else {
    ESP_LOGI(TAG, "No valid BSEC state found in NVS (first run?)");
  }

  nvs_close(my_handle);
}

static void save_bsec_state(bsec2_t *bsec) {
  nvs_handle_t my_handle;
  esp_err_t err;
  if (!bsec2_get_state(bsec, bsec_state_buffer)) {
    ESP_LOGE(TAG, "Failed to get state from BSEC library");
    return;
  }

  err = nvs_open("bme680", NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
    return;

  err = nvs_set_blob(my_handle, "bsec_state", bsec_state_buffer,
                     BSEC_MAX_STATE_BLOB_SIZE);
  if (err == ESP_OK) {
    err = nvs_commit(my_handle);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "BSEC state saved to NVS");
    }
  } else {
    ESP_LOGE(TAG, "Failed to write blob to NVS");
  }

  nvs_close(my_handle);
}

static void on_read_data(const bme68x_data_t data, const bsec_outputs_t outputs,
                         bsec2_t bsec2) {
  if (outputs.n_outputs == 0)
    return;

  for (uint8_t i = 0; i < outputs.n_outputs; i++) {
    const bsec_data_t output = outputs.output[i];

    switch (output.sensor_id) {
    case BSEC_OUTPUT_STATIC_IAQ:
      internal_state.iaq = output.signal;
      internal_state.accuracy = output.accuracy;
      break;
    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
      internal_state.temp = output.signal;
      break;
    case BSEC_OUTPUT_RAW_PRESSURE:
      internal_state.pressure = output.signal;
      break;
    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
      internal_state.humidity = output.signal;
      break;
    case BSEC_OUTPUT_GAS_PERCENTAGE:
      internal_state.gas = output.signal;
      break;
    case BSEC_OUTPUT_CO2_EQUIVALENT:
      internal_state.co2 = output.signal;
      break;
    }
  }

  gui_msg_t msg;

  msg.type = GUI_MSG_UPDATE_SENSORS;
  msg.value.sensor_data = internal_state;

  if (xQueueSend(gui_queue, &msg, 0) != pdTRUE) {
    ESP_LOGE(TAG, "Queue full! Dropping sensor data");
  }
}

static bool hw_init(void) {
  esp_err_t err = i2c_bus_init(&i2c_bus, I2C_PORT, SDA_PIN, SCL_PIN, true, true,
                               I2C_CLK_SPEED);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2C Init Error");
    return false;
  }

  if (!bsec2_init(&bsec_instance, &i2c_bus, BME68X_I2C_INTF)) {
    ESP_LOGE(TAG, "BSEC2 Init Error");
    return false;
  }

  // TODO: Set after testing
  // bsec2_set_temperature_offset(&bsec_instance, 3.0f);
  bsec2_set_config(&bsec_instance, bsec_config_iaq);

  load_bsec_state(&bsec_instance);

  if (!bsec2_update_subscription(&bsec_instance, sensors_list,
                                 ARRAY_LEN(sensors_list), BME680_SAMPLE_RATE)) {
    ESP_LOGE(TAG, "BSEC2 Subscription Error");
    return false;
  }

  bsec2_attach_callback(&bsec_instance, on_read_data);
  return true;
}

static void bme680_task_loop(void *param) {
  TickType_t last_save_time = xTaskGetTickCount();

  while (true) {
    bsec2_run(&bsec_instance);

    if (xTaskGetTickCount() - last_save_time >
        pdMS_TO_TICKS(STATE_SAVE_INTERVAL_MS)) {
      if (internal_state.accuracy >= 1) {
        save_bsec_state(&bsec_instance);
        last_save_time = xTaskGetTickCount();
      }
    }

    int64_t curr_time_ns = bsec2_get_time_ms(&bsec_instance) * 1000000;
    int64_t next_call_ns = bsec_instance.bme_conf.next_call;
    uint32_t delay_ms = 100;
    if (next_call_ns > curr_time_ns) {
      delay_ms = (next_call_ns - curr_time_ns) / 1000000;
    }

    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }
}

bool bme680_start(QueueHandle_t _gui_queue) {
  gui_queue = _gui_queue;

  if (!hw_init())
    return false;

  xTaskCreate(bme680_task_loop, "bme_task", 4096, NULL, tskIDLE_PRIORITY + 2,
              NULL);
  ESP_LOGI(TAG, "Task started");
  return true;
}
