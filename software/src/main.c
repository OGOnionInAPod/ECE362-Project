#include <stdio.h>
#include "pico/stdlib.h"
#include "lcd/lcd.h"
#include "uart/uart.h"
#include "display/display.h"

int main() {

    init_uart();
    init_uart_irq();
    setbuf(stdout, NULL);

    start_display();

    while(1) { tight_loop_contents(); }
    return 0;
}