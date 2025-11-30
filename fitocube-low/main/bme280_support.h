#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float temperature_c;
    float humidity_percent;
    float pressure_pa;
} bme280_reading_t;

esp_err_t bme280_support_init(i2c_port_t port, uint8_t address, gpio_num_t sda, gpio_num_t scl, uint32_t frequency_hz);
bool bme280_support_read(bme280_reading_t *out);

#ifdef __cplusplus
}
#endif
