#ifndef _BME680H_
#define _BME680H_

#include <stdint.h>

typedef struct bme680_state_t {
  float iaq;
  float temp;
  float pressure;
  float humidity;
  float gas;
  uint8_t accuracy;
} bme680_state_t;

#endif