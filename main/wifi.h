#ifndef WIFI_H
#define WIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
void wifi_start(QueueHandle_t main_queue);

#endif
