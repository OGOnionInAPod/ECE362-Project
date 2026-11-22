#ifndef ADS7128_H
#define ADS7128_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool ads7128_init(void);

//ADC register operations
bool ads7128_write_register(uint8_t reg, uint8_t value);
bool ads7128_read_register(uint8_t reg, uint8_t *value);

bool ads7128_read_sample(uint16_t *sample, uint8_t *channel);
bool ads7128_read_samples(uint16_t *samples, uint8_t *channels, size_t count); //Experiment: see if we can read multiple samples at once w/ autonomous mode

#endif