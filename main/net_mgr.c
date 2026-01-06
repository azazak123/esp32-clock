#include "esp_dpp.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "msg.h"

#define DPP_LISTEN_CHANNEL_LIST "6"
#define DPP_DEVICE_INFO "ESP32-Clock"
#define TIMEZONE "EET-2EEST,M3.5.0/3,M10.5.0/4"
#define MAX_RETRY_NUM 3

const uint32_t SYNC_INTERVAL_MS = 4 * 3600 * 1000;

static const char *TAG = "NET_MGR";

static QueueHandle_t gui_queue = NULL;
static QueueHandle_t net_queue = NULL;
static TaskHandle_t s_dpp_task_handle = NULL;
static wifi_config_t s_dpp_wifi_config;

typedef struct {
  bool is_system_init;
  bool is_wifi_on;
  bool is_dpp_active;
  int retry_num;
} net_mgr_state_t;

static net_mgr_state_t s_state = {.is_system_init = false,
                                  .is_wifi_on = false,
                                  .is_dpp_active = false,
                                  .retry_num = 0};

typedef enum {
  DPP_STATUS_IDLE = 0,
  DPP_STATUS_CONNECTED,    // = 1
  DPP_STATUS_CONNECT_FAIL, // = 2
  DPP_STATUS_AUTH_FAIL     // = 3
} dpp_status_t;

static void sync_time(void) {
  ESP_LOGI(TAG, "Initializing SNTP...");

  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  esp_netif_sntp_init(&config);

  uint8_t retry = 0;

  while (retry <= MAX_RETRY_NUM) {
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
      ESP_LOGE(TAG, "Failed to update system time within 10s timeout");
    } else {
      setenv("TZ", TIMEZONE, 1);
      tzset();

      time_t now;
      struct tm timeinfo;
      time(&now);
      localtime_r(&now, &timeinfo);
      ESP_LOGI(TAG, "Time synced: %02d:%02d", timeinfo.tm_hour,
               timeinfo.tm_min);
      break;
    }
    retry += 1;
  }

  if (retry > MAX_RETRY_NUM) {
    ESP_LOGE(TAG, "Failed to update system time");
  }

  esp_netif_sntp_deinit();
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START: {
      if (s_state.is_dpp_active) {
        ESP_ERROR_CHECK(esp_supp_dpp_start_listen());
        ESP_LOGI(TAG, "Started listening for DPP Authentication");
      } else {
        ESP_LOGI(TAG, "WiFi config found, connecting to AP...");
        esp_wifi_connect();
      }
      break;
    }
    case WIFI_EVENT_STA_DISCONNECTED:
      if (s_state.retry_num < MAX_RETRY_NUM) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_wifi_connect();
        s_state.retry_num++;
        ESP_LOGI(TAG, "Disconnect event, retry to connect to the AP");
      } else {
        xTaskNotify(s_dpp_task_handle, DPP_STATUS_CONNECT_FAIL,
                    eSetValueWithOverwrite);
      }
      break;
    case WIFI_EVENT_STA_CONNECTED: {
      ESP_LOGI(TAG, "Successfully connected to the AP ssid : %s ",
               s_dpp_wifi_config.sta.ssid);
      break;
    }
    case WIFI_EVENT_DPP_URI_READY: {
      if (s_state.is_dpp_active) {
        wifi_event_dpp_uri_ready_t *uri_data = event_data;
        if (uri_data != NULL) {
          ESP_LOGI(TAG, "Scan below QR Code to configure the enrollee:");
          gui_msg_t msg;
          msg.type = GUI_MSG_SHOW_QR;
          msg.value.text_data = strdup(uri_data->uri);
          if (xQueueSend(gui_queue, &msg, 0) != pdTRUE) {
            ESP_LOGE(TAG, "Queue full! Dropping QR code event");
            free(msg.value.text_data);
          }
        }
      }
      break;
    }
    case WIFI_EVENT_DPP_CFG_RECVD: {
      wifi_event_dpp_config_received_t *config = event_data;
      memcpy(&s_dpp_wifi_config, &config->wifi_cfg, sizeof(s_dpp_wifi_config));
      s_state.retry_num = 0;
      esp_wifi_set_config(ESP_IF_WIFI_STA, &s_dpp_wifi_config);
      esp_wifi_connect();
      break;
    }
    case WIFI_EVENT_DPP_FAILED: {
      wifi_event_dpp_failed_t *dpp_failure = event_data;
      if (s_state.retry_num < MAX_RETRY_NUM) {
        ESP_LOGI(TAG, "DPP Auth failed (Reason: %s), retry...",
                 esp_err_to_name((int)dpp_failure->failure_reason));
        ESP_ERROR_CHECK(esp_supp_dpp_start_listen());
        s_state.retry_num++;
      } else {
        xTaskNotify(s_dpp_task_handle, DPP_STATUS_AUTH_FAIL,
                    eSetValueWithOverwrite);
      }
      break;
    }
    default:
      break;
    }
  }
  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "Got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_state.retry_num = 0;
    xTaskNotify(s_dpp_task_handle, DPP_STATUS_CONNECTED,
                eSetValueWithOverwrite);
  }
}

esp_err_t dpp_enrollee_bootstrap(void) {
  esp_err_t ret;
  char *key = NULL;

  ret = esp_supp_dpp_bootstrap_gen(DPP_LISTEN_CHANNEL_LIST,
                                   DPP_BOOTSTRAP_QR_CODE, key, DPP_DEVICE_INFO);

  if (key)
    free(key);

  return ret;
}

void stop_wifi(void) {
  if (!s_state.is_wifi_on) {
    ESP_LOGW(TAG, "Wi-Fi is already OFF");
    return;
  }

  ESP_LOGI(TAG, "Stopping Wi-Fi...");

  gui_msg_t msg;
  msg.type = GUI_MSG_HIDE_QR;
  xQueueSend(gui_queue, &msg, 0);

  if (s_state.is_dpp_active) {
    esp_supp_dpp_stop_listen();
    esp_supp_dpp_deinit();
    s_state.is_dpp_active = false;
  }

  esp_wifi_disconnect();
  esp_wifi_stop();
  esp_wifi_deinit();

  esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
  esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);

  s_state.is_wifi_on = false;
  ESP_LOGI(TAG, "Wi-Fi Stopped");
}

void start_wifi(bool use_saved_config) {
  if (s_state.is_wifi_on) {
    ESP_LOGW(TAG, "Wi-Fi is already ON, restarting...");
    stop_wifi();
  }

  xTaskNotifyStateClear(s_dpp_task_handle);

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &event_handler, NULL));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  s_state.retry_num = 0;

  wifi_config_t current_conf;
  esp_wifi_get_config(WIFI_IF_STA, &current_conf);

  if (strlen((const char *)current_conf.sta.ssid) > 0 && use_saved_config) {
    ESP_LOGI(TAG, "Config found: %s", current_conf.sta.ssid);
    s_state.is_dpp_active = false;
  } else {
    ESP_LOGW(TAG, "No config found. Starting DPP...");
    s_state.is_dpp_active = true;
    ESP_ERROR_CHECK(esp_supp_dpp_init(NULL));
    ESP_ERROR_CHECK(dpp_enrollee_bootstrap());
  }

  ESP_ERROR_CHECK(esp_wifi_start());
  s_state.is_wifi_on = true;

  uint32_t status = DPP_STATUS_IDLE;
  xTaskNotifyWait(0x00, ULONG_MAX, &status, portMAX_DELAY);

  if (status == DPP_STATUS_CONNECTED) {
    ESP_LOGI(TAG, "Wi-Fi Connected Successfully");

    if (s_state.is_dpp_active) {
      esp_supp_dpp_deinit();
      s_state.is_dpp_active = false;
      gui_msg_t msg_hide;
      msg_hide.type = GUI_MSG_HIDE_QR;
      xQueueSend(gui_queue, &msg_hide, 0);
    }

  } else {
    ESP_LOGE(TAG, "Wi-Fi Connection Failed (Status: %lu)", status);
    stop_wifi();
  }
}

void net_init(void) {
  if (s_state.is_system_init)
    return;

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  s_state.is_system_init = true;
  ESP_LOGI(TAG, "System Netif Initialized");
}

void net_mgr_task(void *param) {
  s_dpp_task_handle = xTaskGetCurrentTaskHandle();

  TickType_t last_sync_time = xTaskGetTickCount();

  net_init();

  net_msg_t init_msg;
  init_msg.type = NET_MSG_SYNC_TIME;
  xQueueSend(net_queue, &init_msg, 0);

  ESP_LOGI(TAG, "Net Manager Loop Started");

  while (true) {
    if (xTaskGetTickCount() - last_sync_time >
        pdMS_TO_TICKS(SYNC_INTERVAL_MS)) {
      net_msg_t timer_msg;
      timer_msg.type = NET_MSG_SYNC_TIME;
      xQueueSend(net_queue, &timer_msg, 0);
      last_sync_time = xTaskGetTickCount();
    }

    net_msg_t msg;
    if (xQueueReceive(net_queue, &msg, 0)) {
      ESP_LOGI(TAG, "Received MSG: %d", msg.type);

      switch (msg.type) {
      case NET_MSG_INIT_WIFI:
        if (s_state.is_wifi_on) {
          stop_wifi();
        }
        start_wifi(false);
        if (s_state.is_wifi_on) {
          sync_time();
          stop_wifi();
        }
        break;

      case NET_MSG_SYNC_TIME:
        if (s_state.is_wifi_on) {
          sync_time();
        } else {
          ESP_LOGW(TAG, "Cannot sync time: Wi-Fi is OFF. Enabling...");
          start_wifi(true);
          if (s_state.is_wifi_on) {
            sync_time();
            stop_wifi();
          }
        }
        break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

bool net_mgr_start(QueueHandle_t _net_queue, QueueHandle_t _gui_queue) {
  net_queue = _net_queue;
  gui_queue = _gui_queue;
  xTaskCreate(net_mgr_task, "net_mgr_task", 4096, NULL, tskIDLE_PRIORITY + 2,
              NULL);
  ESP_LOGI(TAG, "Task started");
  return true;
}
