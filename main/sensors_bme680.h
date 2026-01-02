#ifndef SENSORS_BME680_H
#define SENSORS_BME680_H

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

bool bme680_start(void);

void bme680_get_data(bme680_state_t *out_data);

#endif
