#include "codec.h"

#include <stddef.h> //to ensure NULL is correctly in here
#include "hardware/i2c.h"
#include "hardware/gpio.h"

#include "ads7128.h"
#include "pcm5122.h"

// I2C Bus
#define I2C_SCL_PIN 11
#define I2C_SDA_PIN 10
#define CODEC_I2C_INST i2c1

//side note- consider maybe using timeout later if we encounter problems for i2c_read or i2c_write those verisons are 
//i2c_read_timeout_us and i2c_write_timeout_us

//initialize rp2350 hardware so it can communicate w/ the ADC and DAC

bool codec_init(void) {
    // Initialize I2C1 at 100 kHz
    //done bc correlates with standard mode, 100kHz. standard mode since works with everything better
    i2c_init(CODEC_I2C_INST, 100000);

    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

    if (!ads7128_init()) { return false; }
    if (!pcm5122_init()) { return false; }
    
    return true;
}

i2c_inst_t *codec_get_i2c_inst() {
    return CODEC_I2C_INST;
}