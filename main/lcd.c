#include "driver/spi_common.h"
#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"

#include "esp_lcd_touch_xpt2046.h"

#include "esp_lvgl_port.h"

#define TAG "LCD"

#define LCD_HRES 320
#define LCD_VRES 240

#define BUFFER_SIZE (320 * 24 * sizeof(uint16_t))
#define SPI_SCLK_PIN 12
#define SPI_MOSI_PIN 11
#define SPI_MAX_TRANS_SZ BUFFER_SIZE

#define SPI_PORT SPI2_HOST

#define DC_PIN 9
#define RESET_PIN 7

#define LCD_TOUCH_INT 3

#define BKL_PIN 5
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY          30000

static esp_err_t backlight_pwm_init(void) {
  ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_MODE,
    .timer_num        = LEDC_TIMER,
    .duty_resolution  = LEDC_DUTY_RES,
    .freq_hz          = LEDC_FREQUENCY,
    .clk_cfg          = LEDC_USE_RC_FAST_CLK
  };
  ESP_RETURN_ON_ERROR(ledc_timer_config(&ledc_timer), TAG,
                      "Ledc timer was not initialized");

  ledc_channel_config_t ledc_channel = {
    .speed_mode     = LEDC_MODE,
    .channel        = LEDC_CHANNEL,
    .timer_sel      = LEDC_TIMER,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = BKL_PIN,
    .duty           = 0,
    .hpoint         = 0,
    .sleep_mode     = LEDC_SLEEP_MODE_KEEP_ALIVE,
  };

  ESP_RETURN_ON_ERROR(ledc_channel_config(&ledc_channel), TAG,
                      "Ledc channel was not initialized");

  return ESP_OK;
}

void lcd_set_brightness(uint8_t percent) {
  if (percent > 100) percent = 100;

  uint32_t duty = (255 * percent) / 100;

  ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
  ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

esp_err_t panel_init(esp_lcd_panel_io_handle_t *io_handle,
                     esp_lcd_panel_handle_t *panel_handle) {
  ESP_LOGI(TAG, "Initialize SPI bus");
  const spi_bus_config_t bus_config = ILI9341_PANEL_BUS_SPI_CONFIG(
                                                                   SPI_SCLK_PIN, SPI_MOSI_PIN, SPI_MAX_TRANS_SZ);
  ESP_RETURN_ON_ERROR(
                      spi_bus_initialize(SPI_PORT, &bus_config, SPI_DMA_CH_AUTO), TAG,
                      "SPI bus was not initialized");

  ESP_LOGI(TAG, "Install panel IO");
  const esp_lcd_panel_io_spi_config_t io_config =
    ILI9341_PANEL_IO_SPI_CONFIG(-1, DC_PIN, NULL, NULL);
  esp_err_t ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI_PORT,
                                           &io_config, io_handle);
  ESP_GOTO_ON_ERROR(ret, err, TAG, "Panel IO was not installed");

  ESP_LOGI(TAG, "Install ILI9341 panel driver");
  const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = RESET_PIN, // Set to -1 if not use
    .rgb_endian = LCD_RGB_ENDIAN_BGR,
    .bits_per_pixel = 16,
  };
  ret = esp_lcd_new_panel_ili9341(*io_handle, &panel_config, panel_handle);
  ESP_GOTO_ON_ERROR(ret, err, TAG, "Panel driver was not installed");

  ret = esp_lcd_panel_reset(*panel_handle);
  ESP_GOTO_ON_ERROR(ret, err, TAG, "Panel was not reset");

  ret = esp_lcd_panel_init(*panel_handle);
  ESP_GOTO_ON_ERROR(ret, err, TAG, "Panel was not initialized");

  ret = esp_lcd_panel_mirror(*panel_handle, true, true);
  ESP_GOTO_ON_ERROR(ret, err, TAG, "Panel was not mirrored");

  ret = esp_lcd_panel_swap_xy(*panel_handle, true);
  ESP_GOTO_ON_ERROR(ret, err, TAG, "Panel axis ware not swapped");

  ret = esp_lcd_panel_disp_on_off(*panel_handle, true);
  ESP_GOTO_ON_ERROR(ret, err, TAG, "Panel was not turned on");

  ESP_LOGI(TAG, "Initialize PWM backlight");
  backlight_pwm_init();

  lcd_set_brightness(50);

  return ESP_OK;

 err:
  spi_bus_free(SPI_PORT);

  return ret;
}

esp_err_t lvgl_init(esp_lcd_panel_io_handle_t *io_handle,
                    esp_lcd_panel_handle_t *panel_handle,
                    lv_display_t **disp_handle,
                    esp_lcd_touch_handle_t *touch_handle) {
  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG,
                      "LGVL port was not initialized");

  /* Add LCD screen */
  const lvgl_port_display_cfg_t disp_cfg = {
    .io_handle = *io_handle,
    .panel_handle = *panel_handle,
    .buffer_size = BUFFER_SIZE,
    .double_buffer = true,
    .hres = LCD_HRES,
    .vres = LCD_VRES,
    .monochrome = false,
    .rotation =
          {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = true,
          },
    .flags =
          {
            .buff_dma = true,
            .swap_bytes = true,
          },
  };
  *disp_handle = lvgl_port_add_disp(&disp_cfg);

  const lvgl_port_touch_cfg_t touch_cfg = {
    .disp = *disp_handle,
    .handle = *touch_handle,
  };

  lvgl_port_add_touch(&touch_cfg);

  return ESP_OK;
}

esp_err_t touch_init(esp_lcd_touch_handle_t *tp) {
  ESP_LOGI(TAG, "Initialize SPI bus");
  const spi_bus_config_t bus_config = {
    .miso_io_num = 18,
    .mosi_io_num = 33,
    .sclk_io_num = 35,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 0,
  };
  ESP_RETURN_ON_ERROR(
                      spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_DISABLED), TAG,
                      "SPI bus was not initialized");

  esp_lcd_panel_io_handle_t tp_io_handle = NULL;
  esp_lcd_panel_io_spi_config_t tp_io_config =
    ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(16);
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST,
                                           &tp_io_config, &tp_io_handle));

  esp_lcd_touch_config_t tp_cfg = {
    .x_max = LCD_VRES,
    .y_max = LCD_HRES,
    .rst_gpio_num = -1,
    .int_gpio_num = LCD_TOUCH_INT,
    .flags =
          {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = true,
          },
  };

  ESP_LOGI(TAG, "Initialize touch controller XPT2046");
  return esp_lcd_touch_new_spi_xpt2046(tp_io_handle, &tp_cfg, tp);
}

esp_err_t lcd_init(lv_display_t **disp_handle,
                   esp_lcd_touch_handle_t *touch_handle) {
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_handle_t panel_handle = NULL;

  ESP_RETURN_ON_ERROR(panel_init(&io_handle, &panel_handle), TAG,
                      "Panel was not initialized");

  ESP_RETURN_ON_ERROR(touch_init(touch_handle), TAG,
                      "Touch was not initialized");

  ESP_RETURN_ON_ERROR(
                      lvgl_init(&io_handle, &panel_handle, disp_handle, touch_handle), TAG,
                      "LVGL was not initialized");

  return ESP_OK;
}
