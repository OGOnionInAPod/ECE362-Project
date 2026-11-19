#include "codec_i2c.h"
#include <stddef.h> //to ensure NULL is correctly in here
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "connections.h"

//side note- consider maybe using timeout later if we encounter problems for i2c_read or i2c_write those verisons are 
//i2c_read_timeout_us and i2c_write_timeout_us

//initialize rp2350 hardware so it can communicate w/ the ADC and DAC

void codec_i2c_init(void) {

// Initialize I2C1 at 100 kHz
//done bc correlates with standard mode, 100kHz. standard mode since works with everything better
i2c_init(i2c1, 100000);

gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);

//enable pull-ups
gpio_pull_up(I2C_SDA_PIN);
gpio_pull_up(I2C_SCL_PIN);

}

bool codec_i2c_write(uint8_t device_addr, //address of device we write to, register in that device, config value to write
                     uint8_t reg,
                     uint8_t value) {

    uint8_t data[2] = {reg, value}; //array of 2-bytes, reg addr val and value to write

    int result = i2c_write_blocking( //we do i2c write to controller i2c1, controller will do "Stop" releasing the bus
        i2c1,
        device_addr,
        data,
        2,
        false
    ); 

    return result == 2; //return true if successfully wrote those two bytes
}

bool codec_i2c_read(uint8_t device_addr, uint8_t reg, uint8_t *value) {

    if (value == NULL) {
        return false;
    }

    int result = i2c_write_blocking(i2c1, device_addr, &reg, 1, true); //first we do a write of address byte to read next

    if (result != 1) {
        return false; //check if write addr was successful
    }

    result = i2c_read_blocking(i2c1, device_addr, value, 1, false); //then we finally read the byte and do "Stop" releasing bus

    return result == 1; //return true if byte was successfully read
}


