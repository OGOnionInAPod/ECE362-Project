#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "lcd/lcd.h"
#include "uart/uart.h"
#include "dac/dac.h"
#include "display/display.h"

#define I2C_SDA_PIN 10
#define I2C_SCL_PIN 11

#define I2C_INST i2c1

static const uint8_t wave[30] = {
     60,  71,  82,  92, 101, 108, 112, 115, 115, 112,
    108, 101,  92,  82,  71,  60,  49,  38,  28,  19,
     12,   8,   5,   5,   8,  12,  19,  28,  38,  49
};

int main() {

    init_uart();
    init_uart_irq();
    setbuf(stdout, NULL);

    i2c_init(I2C_INST, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C); gpio_set_pulls(I2C_SDA_PIN, false, false);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C); gpio_set_pulls(I2C_SCL_PIN, false, false);

    dac_init();


    /* DISPLAY TEST
    start_display();

    int offset = 0;
    while(1) {
        display_set_amplitudes(wave, offset, 30-offset);
        display_set_amplitudes(wave+(30-offset), 0, offset);
        offset = (offset + 1) % 30;
        sleep_ms(50);
    }
    */

    sleep_ms(5000);
    printf("PLL Status Register: %#x\n", dac_reg_read(4));
    printf("FS Status Register 1: %#x\n", dac_reg_read(91));
    printf("FS Status Register 2: %#x\n", dac_reg_read(92));
    printf("FS Status Register 3: %#x\n", dac_reg_read(93));
    printf("FS Status Register 4: %#x\n", dac_reg_read(94));
    printf("Power State Register: %#x\n", dac_reg_read(118));

    while(1) {
        dac_send_sample(0x7fff7fff);
    }
    return 0;
}