#include "ui.h"
#include "font/lv_font.h"
#include <stdio.h>
#include <string.h>

LV_IMG_DECLARE(kitty_gif);

LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_20);
LV_FONT_DECLARE(lv_font_montserrat_28);

#define FONT_SMALL &lv_font_montserrat_14
#define FONT_MEDIUM &lv_font_montserrat_20
#define FONT_LARGE &lv_font_montserrat_28

#define COLOR_BG lv_color_hex(0x000000)
#define COLOR_CARD lv_color_hex(0x181818)
#define COLOR_TEXT_MAIN lv_color_hex(0xFFFFFF)
#define COLOR_TEXT_SEC lv_color_hex(0xA0A0A0)
#define COLOR_ACCENT lv_color_hex(0x00D1FF)

#define COLOR_TEMP lv_palette_main(LV_PALETTE_ORANGE)
#define COLOR_HUM lv_palette_main(LV_PALETTE_BLUE)
#define COLOR_GOOD lv_palette_main(LV_PALETTE_GREEN)
#define COLOR_WARN lv_palette_main(LV_PALETTE_YELLOW)
#define COLOR_BAD lv_palette_main(LV_PALETTE_RED)

static lv_obj_t *create_card(lv_obj_t *parent) {
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  return card;
}

static void style_text_title(lv_obj_t *lbl) {
  lv_obj_set_style_text_font(lbl, FONT_SMALL, 0);
  lv_obj_set_style_text_color(lbl, COLOR_TEXT_SEC, 0);
}

static void style_text_value(lv_obj_t *lbl) {
  lv_obj_set_style_text_font(lbl, FONT_MEDIUM, 0);
  lv_obj_set_style_text_color(lbl, COLOR_TEXT_MAIN, 0);
}

ui_state_t ui_setup(lv_display_t *display) {
  ui_state_t ui;

  ui.screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(ui.screen, COLOR_BG, 0);

  lv_obj_set_scrollbar_mode(ui.screen, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(ui.screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_screen_load(ui.screen);

  lv_obj_set_flex_flow(ui.screen, LV_FLEX_FLOW_COLUMN);

  lv_obj_set_style_pad_all(ui.screen, 2, 0);
  lv_obj_set_style_pad_gap(ui.screen, 4, 0);

  // ==========================================
  // ROW 1: CLOCK | DATE | BATTERY | GIF
  // ==========================================
  lv_obj_t *row_top = lv_obj_create(ui.screen);
  lv_obj_set_size(row_top, 316, 55);
  lv_obj_set_style_bg_opa(row_top, 0, 0);
  lv_obj_set_style_border_width(row_top, 0, 0);
  lv_obj_set_style_pad_all(row_top, 0, 0);

  lv_obj_set_flex_flow(row_top, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row_top, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_set_scrollbar_mode(row_top, LV_SCROLLBAR_MODE_OFF);

  // 1. CLOCK
  ui.lbl_time = lv_label_create(row_top);
  lv_label_set_text(ui.lbl_time, "--:--");
  lv_obj_set_style_text_font(ui.lbl_time, FONT_LARGE, 0);
  lv_obj_set_style_text_color(ui.lbl_time, COLOR_ACCENT, 0);
  lv_obj_set_style_pad_left(ui.lbl_time, 2, 0);

  // 2. DATE
  ui.lbl_date = lv_label_create(row_top);
  lv_label_set_text(ui.lbl_date, "---, -- ---");
  lv_obj_set_style_text_font(ui.lbl_date, FONT_SMALL, 0);
  lv_obj_set_style_text_color(ui.lbl_date, COLOR_TEXT_MAIN, 0);

  // 3. BATTERY
  ui.lbl_bat = lv_label_create(row_top);
  lv_label_set_text(ui.lbl_bat, LV_SYMBOL_BATTERY_FULL " --%");
  lv_obj_set_style_text_font(ui.lbl_bat, FONT_SMALL, 0);
  lv_obj_set_style_text_color(ui.lbl_bat, COLOR_GOOD, 0);

  // 4. GIF
  ui.gif_container = lv_obj_create(row_top);
  lv_obj_set_size(ui.gif_container, 50, 50);
  lv_obj_set_style_bg_color(ui.gif_container, lv_color_hex(0x222222), 0);
  lv_obj_set_style_radius(ui.gif_container, 8, 0);
  lv_obj_set_style_border_width(ui.gif_container, 0, 0);
  lv_obj_set_scrollbar_mode(ui.gif_container, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t *gif = lv_gif_create(ui.gif_container);
  lv_gif_set_src(gif, &kitty_gif);

  // ==========================================
  // ROW 2: TEMPERATURE | HUMIDITY
  // ==========================================
  lv_obj_t *row_mid = lv_obj_create(ui.screen);
  lv_obj_set_size(row_mid, 316, 90);
  lv_obj_set_style_bg_opa(row_mid, 0, 0);
  lv_obj_set_style_border_width(row_mid, 0, 0);
  lv_obj_set_style_pad_all(row_mid, 0, 0);
  lv_obj_set_flex_flow(row_mid, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_gap(row_mid, 4, 0);
  lv_obj_set_scrollbar_mode(row_mid, LV_SCROLLBAR_MODE_OFF);

  // 1. Temperature
  lv_obj_t *card_temp = create_card(row_mid);
  lv_obj_set_size(card_temp, 156, 90);

  lv_obj_t *lbl_t = lv_label_create(card_temp);
  lv_label_set_text(lbl_t, "Temp");
  style_text_title(lbl_t);
  lv_obj_align(lbl_t, LV_ALIGN_TOP_LEFT, 5, 5);

  ui.arc_temp = lv_arc_create(card_temp);
  lv_obj_set_size(ui.arc_temp, 60, 60);
  lv_obj_align(ui.arc_temp, LV_ALIGN_RIGHT_MID, -2, 5);
  lv_arc_set_rotation(ui.arc_temp, 135);
  lv_arc_set_bg_angles(ui.arc_temp, 0, 270);
  lv_arc_set_value(ui.arc_temp, 50);
  lv_obj_remove_style(ui.arc_temp, NULL, LV_PART_KNOB);
  lv_obj_set_style_arc_width(ui.arc_temp, 6, LV_PART_MAIN);
  lv_obj_set_style_arc_width(ui.arc_temp, 6, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(ui.arc_temp, COLOR_TEMP, LV_PART_INDICATOR);

  ui.lbl_temp_val = lv_label_create(card_temp);
  lv_label_set_text(ui.lbl_temp_val, "--");
  style_text_value(ui.lbl_temp_val);
  lv_obj_align(ui.lbl_temp_val, LV_ALIGN_LEFT_MID, 5, 5);

  // 2. Humidity
  lv_obj_t *card_hum = create_card(row_mid);
  lv_obj_set_size(card_hum, 156, 90);

  lv_obj_t *lbl_h = lv_label_create(card_hum);
  lv_label_set_text(lbl_h, "Hum");
  style_text_title(lbl_h);
  lv_obj_align(lbl_h, LV_ALIGN_TOP_LEFT, 5, 5);

  ui.lbl_hum_val = lv_label_create(card_hum);
  lv_label_set_text(ui.lbl_hum_val, "--%");
  style_text_value(ui.lbl_hum_val);
  lv_obj_set_style_text_color(ui.lbl_hum_val, COLOR_HUM, 0);
  lv_obj_align(ui.lbl_hum_val, LV_ALIGN_LEFT_MID, 5, 5);

  ui.bar_hum = lv_bar_create(card_hum);
  lv_obj_set_size(ui.bar_hum, 8, 50);
  lv_obj_align(ui.bar_hum, LV_ALIGN_RIGHT_MID, -10, 5);
  lv_obj_set_style_bg_color(ui.bar_hum, COLOR_HUM, LV_PART_INDICATOR);
  lv_bar_set_range(ui.bar_hum, 0, 100);

  // ==========================================
  // ROW 3: IAQ | CO2
  // ==========================================
  lv_obj_t *card_air = create_card(ui.screen);
  lv_obj_set_size(card_air, 316, 80);
  lv_obj_set_flex_flow(card_air, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(card_air, LV_FLEX_ALIGN_SPACE_AROUND,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // 1. IAQ
  lv_obj_t *cont_iaq = lv_obj_create(card_air);
  lv_obj_set_style_bg_opa(cont_iaq, 0, 0);
  lv_obj_set_style_border_width(cont_iaq, 0, 0);
  lv_obj_set_size(cont_iaq, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(cont_iaq, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont_iaq, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_scrollbar_mode(cont_iaq, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t *lbl_iaq_head = lv_label_create(cont_iaq);
  lv_label_set_text(lbl_iaq_head, "Air Quality");
  style_text_title(lbl_iaq_head);

  ui.lbl_iaq_val = lv_label_create(cont_iaq);
  lv_label_set_text(ui.lbl_iaq_val, "--");
  style_text_value(ui.lbl_iaq_val);

  ui.lbl_iaq_text = lv_label_create(cont_iaq);
  lv_label_set_text(ui.lbl_iaq_text, "Init...");
  lv_obj_set_style_text_font(ui.lbl_iaq_text, FONT_SMALL, 0);

  // 2. Separator
  lv_obj_t *line = lv_obj_create(card_air);
  lv_obj_set_size(line, 1, 40);
  lv_obj_set_style_bg_color(line, lv_color_hex(0x333333), 0);
  lv_obj_set_style_border_width(line, 0, 0);

  // 3. CO2
  lv_obj_t *cont_co2 = lv_obj_create(card_air);
  lv_obj_set_style_bg_opa(cont_co2, 0, 0);
  lv_obj_set_style_border_width(cont_co2, 0, 0);
  lv_obj_set_size(cont_co2, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(cont_co2, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont_co2, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_scrollbar_mode(cont_co2, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t *lbl_co2_head = lv_label_create(cont_co2);
  lv_label_set_text(lbl_co2_head, "eCO2 (ppm)");
  style_text_title(lbl_co2_head);

  ui.lbl_co2_val = lv_label_create(cont_co2);
  lv_label_set_text(ui.lbl_co2_val, "--");
  style_text_value(ui.lbl_co2_val);

  ui.lbl_press_val = lv_label_create(cont_co2);
  lv_label_set_text(ui.lbl_press_val, "-- hPa");
  lv_obj_set_style_text_font(ui.lbl_press_val, FONT_SMALL, 0);
  lv_obj_set_style_text_color(ui.lbl_press_val, COLOR_TEXT_SEC, 0);

  ui.qr_overlay = NULL;

  return ui;
}

void ui_sensors_update(ui_state_t *ui, const bme680_state_t *data) {
  if (!ui || !data)
    return;
  char buf[32];

  // Temp
  snprintf(buf, sizeof(buf), "%.1f°", data->temp);
  lv_label_set_text(ui->lbl_temp_val, buf);
  int temp_arc = (int)((data->temp / 40.0) * 100);
  if (temp_arc > 100)
    temp_arc = 100;
  if (temp_arc < 0)
    temp_arc = 0;
  lv_arc_set_value(ui->arc_temp, temp_arc);

  // Hum
  snprintf(buf, sizeof(buf), "%.0f%%", data->humidity);
  lv_label_set_text(ui->lbl_hum_val, buf);
  lv_bar_set_value(ui->bar_hum, (int)data->humidity, LV_ANIM_ON);

  // IAQ
  snprintf(buf, sizeof(buf), "%.0f", data->iaq);
  lv_label_set_text(ui->lbl_iaq_val, buf);

  lv_color_t color = COLOR_GOOD;
  const char *status = "Excellent";
  if (data->iaq > 50) {
    color = COLOR_GOOD;
    status = "Good";
  }
  if (data->iaq > 100) {
    color = COLOR_WARN;
    status = "Average";
  }
  if (data->iaq > 150) {
    color = COLOR_BAD;
    status = "Poor";
  }
  if (data->iaq > 200) {
    color = COLOR_BAD;
    status = "Bad";
  }

  lv_obj_set_style_text_color(ui->lbl_iaq_val, color, 0);
  lv_label_set_text(ui->lbl_iaq_text, status);
  lv_obj_set_style_text_color(ui->lbl_iaq_text, color, 0);

  // CO2
  snprintf(buf, sizeof(buf), "%.0f", data->co2);
  lv_label_set_text(ui->lbl_co2_val, buf);

  // Press
  snprintf(buf, sizeof(buf), "%.0f hPa", data->pressure / 100.0f);
  lv_label_set_text(ui->lbl_press_val, buf);
}

void ui_clock_update(ui_state_t *ui, const char *time_str) {
  if (ui && ui->lbl_time) {
    lv_label_set_text(ui->lbl_time, time_str);
  }
}

void ui_date_update(ui_state_t *ui, const char *date_str) {
  if (ui && ui->lbl_date) {
    lv_label_set_text(ui->lbl_date, date_str);
  }
}

void ui_battery_update(ui_state_t *ui, int level_percent, bool is_charging) {
  if (!ui || !ui->lbl_bat)
    return;

  const char *symbol = LV_SYMBOL_BATTERY_EMPTY;
  lv_color_t color = COLOR_BAD;

  if (is_charging) {
    symbol = LV_SYMBOL_CHARGE;
    color = COLOR_GOOD;
  } else {
    if (level_percent > 90) {
      symbol = LV_SYMBOL_BATTERY_FULL;
      color = COLOR_GOOD;
    } else if (level_percent > 60) {
      symbol = LV_SYMBOL_BATTERY_3;
      color = COLOR_GOOD;
    } else if (level_percent > 40) {
      symbol = LV_SYMBOL_BATTERY_2;
      color = COLOR_WARN;
    } else if (level_percent > 15) {
      symbol = LV_SYMBOL_BATTERY_1;
      color = COLOR_WARN;
    } else {
      symbol = LV_SYMBOL_BATTERY_EMPTY;
      color = COLOR_BAD;
    }
  }

  lv_label_set_text_fmt(ui->lbl_bat, "%s %d%%", symbol, level_percent);
  lv_obj_set_style_text_color(ui->lbl_bat, color, 0);
}

static void dpp_qr_close_event_cb(lv_event_t * e) {
    ui_state_t *ui = (ui_state_t *)lv_event_get_user_data(e);
    
    if (ui && ui->qr_overlay) {
        lv_obj_delete(ui->qr_overlay);
        ui->qr_overlay = NULL;
    }
}

void ui_show_dpp_qr(ui_state_t *ui, const char *uri) {
    if (!ui) return;
    if (ui->qr_overlay != NULL) return;

    ui->qr_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ui->qr_overlay, 320, 240);
    lv_obj_set_style_bg_color(ui->qr_overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(ui->qr_overlay, LV_OPA_90, 0);
    lv_obj_set_style_border_width(ui->qr_overlay, 0, 0);
    lv_obj_clear_flag(ui->qr_overlay, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_add_event_cb(ui->qr_overlay, dpp_qr_close_event_cb, LV_EVENT_CLICKED, ui);

    lv_obj_t *bg_card = lv_obj_create(ui->qr_overlay);
    lv_obj_set_size(bg_card, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(bg_card, lv_color_white(), 0);
    lv_obj_set_style_pad_all(bg_card, 10, 0);
    lv_obj_center(bg_card);
    
    lv_obj_add_flag(bg_card, LV_OBJ_FLAG_EVENT_BUBBLE); 

    lv_obj_t *qr = lv_qrcode_create(bg_card);
    
    lv_qrcode_set_size(qr, 200); 
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());

    if (uri && strlen(uri) > 0) {
        lv_result_t res = lv_qrcode_update(qr, uri, strlen(uri));
        if (res != LV_RESULT_OK) {
             lv_obj_delete(qr);
             lv_obj_t *err_lbl = lv_label_create(bg_card);
             lv_label_set_text(err_lbl, "QR Error");
             lv_obj_set_style_text_color(err_lbl, lv_color_black(), 0);
        }
    }
}

void ui_hide_dpp_qr(ui_state_t *ui) {
    if (!ui || !ui->qr_overlay) return;

    lv_obj_delete(ui->qr_overlay);
    ui->qr_overlay = NULL;
}
