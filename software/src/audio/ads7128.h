#ifndef ADS7128_H
#define ADS7128_H

#include <stdint.h>
#include <stdbool.h>

bool ads7128_init(void);

//ADC register operations
bool ads7128_write_register(uint8_t reg, uint8_t value);
bool ads7128_read_register(uint8_t reg, uint8_t *value);

bool ads7128_read_sample(uint16_t *sample, uint8_t *channel);

#endif