#include "codec.h"

#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "ads7128.h"
#include "pcm5122.h"

// I2C Bus
#define I2C_SCL_PIN 11
#define I2C_SDA_PIN 10
#define CODEC_I2C_INST i2c1

//initialize rp2350 hardware so it can communicate w/ the ADC and DAC

bool codec_init(void) {
    // Iniitialize I2C in fast mode because ADC and DAC both support it and ADC
    // sample rate is limited by the I2C protocol
    i2c_init(CODEC_I2C_INST, 400000);

    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

    if (!ads7128_init()) { return false; }
    if (!pcm5122_init()) { return false; }
    
    return true;
}

i2c_inst_t *codec_get_i2c_inst() {
    return CODEC_I2C_INST;
}