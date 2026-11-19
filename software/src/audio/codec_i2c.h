#ifndef CODEC_I2C_H
#define CODEC_I2C_H

#include <stdint.h>
#include <stdbool.h>

void codec_i2c_init(void);

#include <stdint.h>
#include <stdbool.h>

void codec_i2c_init(void);

bool codec_i2c_write(uint8_t device_addr, uint8_t reg, uint8_t value);

bool codec_i2c_read(uint8_t device_addr, uint8_t reg, uint8_t *value);

#endif