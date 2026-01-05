#ifndef SENSORS_BME680_H
#define SENSORS_BME680_H

#include "freertos/idf_additions.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float iaq;        
    float temp;       
    float pressure;   
    float humidity;   
    float gas;        
    float co2;        
    uint8_t accuracy;
} bme680_state_t;

bool bme680_start(QueueHandle_t _gui_queue);

#endif
