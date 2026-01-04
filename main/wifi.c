#include "esp_dpp.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "msg.h"

#define DPP_LISTEN_CHANNEL_LIST "6"
#define DPP_DEVICE_INFO "ESP32-Clock"
#define DPP_BOOTSTRAP_KEY NULL

#define CURVE_SEC256R1_PKEY_HEX_DIGITS 64

static QueueHandle_t queue = NULL;

static const char *TAG = "wifi dpp-enrollee";
wifi_config_t s_dpp_wifi_config;

static int s_retry_num = 0;

static TaskHandle_t s_dpp_task_handle = NULL;

typedef enum {
  DPP_STATUS_IDLE = 0,
  DPP_STATUS_CONNECTED,    // = 1
  DPP_STATUS_CONNECT_FAIL, // = 2
  DPP_STATUS_AUTH_FAIL     // = 3
} dpp_status_t;

#define WIFI_MAX_RETRY_NUM 3

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT) {
    switch (event_id) {
    case WIFI_EVENT_STA_START:
      ESP_ERROR_CHECK(esp_supp_dpp_start_listen());
      ESP_LOGI(TAG, "Started listening for DPP Authentication");
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      if (s_retry_num < WIFI_MAX_RETRY_NUM) {
        esp_wifi_connect();
        s_retry_num++;
        ESP_LOGI(TAG, "Disconnect event, retry to connect to the AP");
      } else {
        xTaskNotify(s_dpp_task_handle, DPP_STATUS_CONNECT_FAIL,
                    eSetValueWithOverwrite);
      }
      break;
    case WIFI_EVENT_STA_CONNECTED: {
      ESP_LOGI(TAG, "Successfully connected to the AP ssid : %s ",
               s_dpp_wifi_config.sta.ssid);

      msg_t msg;
      msg.type = MSG_WIFI_CONNECTED;

      if (xQueueSend(queue, &msg, 0) != pdTRUE) {
        ESP_LOGE(TAG, "Queue full! Dropping QR code event");
      }

      break;
    }
    case WIFI_EVENT_DPP_URI_READY: {
      wifi_event_dpp_uri_ready_t *uri_data = event_data;
      if (uri_data != NULL) {
        ESP_LOGI(TAG, "Scan below QR Code to configure the enrollee:");

        msg_t msg;
        msg.type = MSG_DPP_URI_READY;

        msg.data = strdup(uri_data->uri);

        if (xQueueSend(queue, &msg, 0) != pdTRUE) {
          ESP_LOGE(TAG, "Queue full! Dropping QR code event");
          free(msg.data);
        }
      }
      break;
    }
    case WIFI_EVENT_DPP_CFG_RECVD: {
      wifi_event_dpp_config_received_t *config = event_data;
      memcpy(&s_dpp_wifi_config, &config->wifi_cfg, sizeof(s_dpp_wifi_config));
      s_retry_num = 0;
      esp_wifi_set_config(ESP_IF_WIFI_STA, &s_dpp_wifi_config);
      esp_wifi_connect();
      break;
    }
    case WIFI_EVENT_DPP_FAILED: {
      wifi_event_dpp_failed_t *dpp_failure = event_data;
      if (s_retry_num < 5) {
        ESP_LOGI(TAG, "DPP Auth failed (Reason: %s), retry...",
                 esp_err_to_name((int)dpp_failure->failure_reason));
        ESP_ERROR_CHECK(esp_supp_dpp_start_listen());
        s_retry_num++;
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
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
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

void dpp_init(void *param) {

  xTaskNotifyStateClear(NULL);
  s_dpp_task_handle = xTaskGetCurrentTaskHandle();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &event_handler, NULL));

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_supp_dpp_init(NULL));
  ESP_ERROR_CHECK(dpp_enrollee_bootstrap());
  ESP_ERROR_CHECK(esp_wifi_start());

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  uint32_t status = DPP_STATUS_IDLE;

  xTaskNotifyWait(0x00, ULONG_MAX, &status, portMAX_DELAY);

  if (status == DPP_STATUS_CONNECTED) {
  } else if (status == DPP_STATUS_CONNECT_FAIL) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
             s_dpp_wifi_config.sta.ssid, s_dpp_wifi_config.sta.password);
  } else if (status == DPP_STATUS_AUTH_FAIL) {
    ESP_LOGI(TAG, "DPP Authentication failed after %d retries", s_retry_num);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }

  esp_supp_dpp_deinit();
  ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &event_handler));
  ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &event_handler));
  vTaskDelete(NULL);
}

void wifi_start(QueueHandle_t main_queue) {
  queue = main_queue;
  xTaskCreate(dpp_init, "dpp_init", 4096, NULL, tskIDLE_PRIORITY + 2, NULL);
}

