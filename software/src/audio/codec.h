#ifndef CODEC_H
#define CODEC_H

#include <stdbool.h>

#include "hardware/i2c.h"

bool codec_init(void);

i2c_inst_t *codec_get_i2c_inst();

#endif