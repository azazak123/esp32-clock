#include "driver/spi_common.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_lcd_ili9341.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_types.h"

#include "esp_lvgl_port.h"

#define TAG "LCD"

#define SPI_SCLK_PIN 12
#define SPI_MOSI_PIN 11
#define SPI_MAX_TRANS_SZ 320 * 80 * 2

#define SPI_PORT SPI2_HOST

#define DC_PIN 9
#define RESET_PIN 7

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

  return ESP_OK;

err:
  spi_bus_free(SPI_PORT);

  return ret;
}

esp_err_t lvgl_init(esp_lcd_panel_io_handle_t *io_handle,
                    esp_lcd_panel_handle_t *panel_handle,
                    lv_display_t **disp_handle) {
  const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
  ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG,
                      "LGVL port was not initialized");

  /* Add LCD screen */
  const lvgl_port_display_cfg_t disp_cfg = {
      .io_handle = *io_handle,
      .panel_handle = *panel_handle,
      .buffer_size = 320 * 16 * sizeof(uint16_t),
      .double_buffer = true,
      .hres = 320,
      .vres = 240,
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

//   lv_display_set_flush_cb(*disp_handle, swap_bytes);

  return ESP_OK;
}

esp_err_t lcd_init(lv_display_t **disp_handle) {
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_handle_t panel_handle = NULL;

  ESP_RETURN_ON_ERROR(panel_init(&io_handle, &panel_handle), TAG,
                      "Panel was not initialized");

  ESP_RETURN_ON_ERROR(lvgl_init(&io_handle, &panel_handle, disp_handle), TAG,
                      "LVGL was not initialized");

  return ESP_OK;
}