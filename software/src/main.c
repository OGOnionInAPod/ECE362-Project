#include <stdio.h>
#include "pico/stdlib.h"
#include "lcd/lcd.h"
#include "uart/uart.h"
#include "display/display.h"

int main() {

    init_uart();
    init_uart_irq();
    setbuf(stdout, NULL);

    lcd_init();

    display_loop();

    return 0;
}