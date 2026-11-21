#ifndef PCM5122_H
#define PCM5122_H

#include "pico/stdlib.h"

#define PCM5122_REG_IGRFLG 37
#define PCM5122_REG_I2SFMT 40
#define PCM5122_REG_PWRSTT 118

bool pcm5122_init(void);

bool pcm5122_write_register(uint8_t reg, uint8_t value);
bool pcm5122_read_register(uint8_t reg, uint8_t *value);

void pcm5122_send_sample(uint32_t left, uint32_t right);

#endif