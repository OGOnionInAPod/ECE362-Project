#include "codec_i2c.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

//initialize rp2350 hardware so it can communicate w/ the ADC and DAC

// Proposed pins, pending team hardware decision
//8 and 9 belong to the same hardware controller making things much easier, prev selected 9/10 belong to separate ones
#define CODEC_SDA_PIN 8
#define CODEC_SCL_PIN 9

void codec_i2c_init(void) {

// Initialize I2C0 at 100 kHz
//done bc correlates with standard mode, 100kHz. standard mode since works with everything better
i2c_init(i2c0, 100000);

gpio_set_function(CODEC_SDA_PIN, GPIO_FUNC_I2C);
gpio_set_function(CODEC_SCL_PIN, GPIO_FUNC_I2C);

//enable pull-ups
gpio_pull_up(CODEC_SDA_PIN);
gpio_pull_up(CODEC_SCL_PIN);

}
