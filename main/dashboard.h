#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdbool.h>

bool dashboard_app_start(QueueHandle_t _gui_queue, QueueHandle_t _net_queue);

#endif
