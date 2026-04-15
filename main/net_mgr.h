#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

bool net_mgr_start(QueueHandle_t _net_queue, QueueHandle_t _gui_queue);

#endif
