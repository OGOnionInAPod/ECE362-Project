#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "lcd/lcd.h"
#include "uart/uart.h"

#include "audio/codec.h"
#include "audio/pcm5122.h"

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

    if (codec_init()) {
        printf("Audio hardware successfully initialized!\n");
    } else {
        printf("Error initializing audio hardware!\n");
    }


    /* DISPLAY TEST
    start_display();

    int offset = 0;


    //initialize I2C communication
    codec_i2c_init();

    //configure the ADS7128 ADC
    bool adc_ready = ads7128_init();

    if (adc_ready) {
        printf("ADC initialized successfully!\n");
    } else {
        printf("ADC initialization failed!\n");
    }


    while(1) {
        // Existing display functionality
        display_set_amplitudes(wave, offset, 30 - offset);
        display_set_amplitudes(wave + (30 - offset), 0, offset);
        offset = (offset + 1) % 30;

        //adc sample acquisition
        if (adc_ready) {
            uint16_t sample;
            uint8_t channel;

            if (ads7128_read_sample(&sample, &channel)) {
                printf("Channel %u: %u\n",
                    (unsigned)channel,
                    (unsigned)sample);
            } else {
                printf("ADC sample read failed\n");
            }
        }

        sleep_ms(50);
    }
    */

    uint8_t v; pcm5122_read_register(PCM5122_REG_PWRSTT, &v);

    printf("Power State: %#x\n", v);

    while(1) {
        pcm5122_send_sample(0xdead, 0xbeef);
    }
    return 0;


}