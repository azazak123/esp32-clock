#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdbool.h>

bool dashboard_app_start(QueueHandle_t queue);

#endif
