#include <stdio.h>
#include "pico/stdlib.h"
#include "lcd/lcd.h"
#include "uart/uart.h"
#include "display/display.h"

static const uint8_t wave[30] = {
     60,  71,  82,  92, 101, 108, 112, 115, 115, 112,
    108, 101,  92,  82,  71,  60,  49,  38,  28,  19,
     12,   8,   5,   5,   8,  12,  19,  28,  38,  49
};

int main() {

    init_uart();
    init_uart_irq();
    setbuf(stdout, NULL);

    start_display();

    int offset = 0;
    while(1) {
        display_set_amplitudes(wave, offset, 30-offset);
        display_set_amplitudes(wave+(30-offset), 0, offset);
        offset = (offset + 1) % 30;
        sleep_ms(50);
    }
    return 0;
}