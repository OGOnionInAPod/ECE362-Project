#ifndef DAC_H
#define DAC_H

#include "pico/stdlib.h"

void dac_init();

int dac_reg_read(uint8_t reg);
int dac_reg_write(uint8_t reg, uint8_t data);

void dac_send_sample(uint32_t sample);

#endif