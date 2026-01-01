#include "bsec2.h"

#include "bsec_datatypes.h"
#include "esp_log.h"

#include "esp_lcd_touch.h"
#include "esp_lvgl_port.h"

#include "bme680.h"
#include "lcd.h"
#include "ui.h"

#include "bsec_iaq.h"

// Timeout for starting up USB CDC driver
#define START_TIMEOUT_MS 5000

// I2C
#define SDA_PIN 37
#define SCL_PIN 39
#define I2C_CLK_SPEED 100000
#define I2C_PORT I2C_NUM_0

// BME680
#define BME680_SAMPLE_RATE BSEC_SAMPLE_RATE_LP
#define BME680_INTERVAL_MS 1 / BME680_SAMPLE_RATE * 1000

bsec_sensor_t sensors[] = {
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_GAS_PERCENTAGE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
};

static bme680_state_t bme680_state;

void on_read_data(const bme68x_data_t data, const bsec_outputs_t outputs,
                  bsec2_t bsec2) {
  for (uint8_t i = 0; i < outputs.n_outputs; i++) {
    const bsec_data_t output = outputs.output[i];

    switch (output.sensor_id) {
    case BSEC_OUTPUT_STATIC_IAQ:
      printf("IAQ: %f\n", output.signal);
      printf("IAQ accuracy: %d\n", output.accuracy);
      bme680_state.iaq = output.signal;
      break;
    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
      printf("Temperature: %f\n", output.signal);
      bme680_state.temp = output.signal;
      break;
    case BSEC_OUTPUT_RAW_PRESSURE:
      printf("Pressure: %f\n", output.signal);
      bme680_state.pressure = output.signal;
      break;
    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
      printf("Humidity: %f\n", output.signal);
      bme680_state.humidity = output.signal;
      break;
    case BSEC_OUTPUT_GAS_PERCENTAGE:
      printf("Gas %%: %f\n", output.signal);
      printf("Gas accuracy: %d\n", output.accuracy);
      bme680_state.gas = output.signal;
      bme680_state.accuracy = output.accuracy;
      break;
    }
  }
};

bool bme680_init(i2c_bus_t *i2c, bsec2_t *bs) {
  esp_err_t err =
      i2c_bus_init(i2c, I2C_PORT, SDA_PIN, SCL_PIN, true, true, I2C_CLK_SPEED);

  if (err != ESP_OK)
    return false;

  bool result = bsec2_init(bs, i2c, BME68X_I2C_INTF);

  if (!result)
    return false;

  bsec2_set_temperature_offset(bs, 2.5f);
  bsec2_set_config(bs, bsec_config_iaq);
  
  result = bsec2_update_subscription(bs, sensors, ARRAY_LEN(sensors),
                                     BME680_SAMPLE_RATE);

  if (!result)
    return false;

  bsec2_attach_callback(bs, on_read_data);

  return true;
}

void bme680_task(void *param) {
  i2c_bus_t i2c;
  bsec2_t bs;
  bool result = bme680_init(&i2c, &bs);

  if (!result) {
    ESP_LOGE("BME680", "Init failed");
    vTaskDelete(NULL);
  }

  while (true) {
    bsec2_run(&bs);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void lcd_task(void *param) {
  lv_display_t *disp_handle = NULL;
  esp_lcd_touch_handle_t touch_handle;

  lcd_init(&disp_handle, &touch_handle);

  ui_state_t state = ui_setup(disp_handle);

  while (true) {
    ui_sensors_update(&state, &bme680_state);
    vTaskDelay(pdMS_TO_TICKS(BME680_INTERVAL_MS));
  }
}

void app_main(void) {
  // Timeout for starting up USB CDC driver
  vTaskDelay(pdMS_TO_TICKS(START_TIMEOUT_MS));

  // Run bme680 task
  xTaskCreate(bme680_task, "bme680", 4096, NULL, tskIDLE_PRIORITY + 2, NULL);

  // Run lcd task
  xTaskCreate(lcd_task, "lcd", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
}
