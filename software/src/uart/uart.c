#include <stdio.h>
#include "pico/stdlib.h"

#define BUFSIZE 32
char serbuf[BUFSIZE];
int seridx = 0;
int newline_seen = 0;

void init_uart() {
    // fill in
    gpio_set_function(0, UART_FUNCSEL_NUM(uart0, 0));
    gpio_set_function(1, UART_FUNCSEL_NUM(uart0, 1));

    uart_set_format(uart0, 8, 1, UART_PARITY_NONE);
    uart_init(uart0, 115200);
}

void uart_rx_handler() {
    // fill in.
    hw_set_bits(&uart0_hw->icr, UART_UARTICR_RXIC_BITS | UART_UARTICR_RTIC_BITS);

    if (seridx == BUFSIZE) return;

    char c = (char)uart0_hw->dr;

    newline_seen = (c == '\n');

    if (c == '\b') {
        uart_puts(uart0, "\b \b");
        seridx = MAX(seridx-1, 0);
        serbuf[seridx] = '\0';
    } else {
        uart_putc(uart0, c);
        serbuf[seridx] = c;
        seridx++;
    }
}

void init_uart_irq() {
    // fill in.
    irq_set_enabled(UART0_IRQ, true);
    irq_set_exclusive_handler(UART_IRQ_NUM(uart0), uart_rx_handler);

    uart_set_fifo_enabled(uart0, false);
    uart_set_irqs_enabled(uart0, true, false);

}

int _read(__unused int handle, char *buffer, int length) {
    // fill in.
    while (!newline_seen) { sleep_ms(5); }
    newline_seen = 0;

    memcpy(buffer, serbuf, MIN(seridx, length) * sizeof(char));
    seridx = 0;
    return length;
}

int _write(__unused int handle, char *buffer, int length) {
    for (int i = 0; i < length; i++) {
        uart_putc(uart0, buffer[i]);
    }
    return length;
}

void cmd_gpio(int argc, char **argv) {
    // This is the main command handler for the "gpio" command.
    // It will call either cmd_gpio_out or cmd_gpio_set based on the arguments.
    
    // Ensure that argc is at least 2, otherwise print an example use case and return.
    if (argc < 2) {
        printf("Usage: [peripheral] [command] [value]\n");
        printf("                                     \n");
        printf("Peripherals:                         \n");
        printf("- gpio                               \n");
        printf("                                     \n");
        printf("Commands:                            \n");
        printf("  gpio:                              \n");
        printf("  - out [0-47]                       \n");
        printf("  - set [1|0]                        \n");
        printf("                                     \n");
        return;
    }

    if (strcmp(argv[0], "gpio") == 0) {

        if (argc < 3) {
            printf("Error: 'gpio' peripheral requires command argument\n");
            return;
        }

        // If the second argument is "out":
        //      Ensure that argc is exactly 3, otherwise print an example use case and return.
        //      Convert the third argument to an integer pin number using atoi.
        //      Check if the pin number is valid (0-47), otherwise print an error and return.
        //      Set the pin to output using gpio_init and gpio_set_dir.
        //      Print a success message.
        if (strcmp(argv[1], "out") == 0) {
            if (argc != 3) {
                printf("Error: 'gpio out' requires exactly one parameter\n");
                return;
            }

            int pin = atoi(argv[2]);
            if ( (pin < 0) | (pin > 47) ) {
                printf("Invalid pin number: %d. Must be between 0 and 47.\n", pin);
                return;
            }
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_OUT);
            printf("Initialized pin %d as output.\n", pin);
        }

        else if (strcmp(argv[1], "in") == 0) {
            if (argc != 3) {
                printf("Error: 'gpio in' requires exactly one parameter\n");
                return;
            }

            int pin = atoi(argv[2]);
            if ( (pin < 0) | (pin > 47) ) {
                printf("Invalid pin number: %d. Must be between 0 and 47.\n", pin);
                return;
            }
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_IN);
            printf("Initialized pin %d as input.\n", pin);
        }
        
        // If the second argument is "set":
        //      Ensure that argc is exactly 4, otherwise print an example use case and return.
        //      Convert the third argument to an integer pin number using atoi.
        //      Check if the pin number is valid (0-47), otherwise print an error and return.
        //      Check if the pin has been initialized as a GPIO output, if not, return.
        //      Convert the fourth argument to an integer value (0 or 1) using atoi.
        //      Check if the value is valid (0 or 1), otherwise print an error and return.
        //      Set the pin to the specified value using gpio_put.
        //      Print a success message.
        else if (strcmp(argv[1], "set") == 0) {
            if (argc != 4) {
                printf("Error: 'gpio set' requires exactly two parameters\n");
                return;
            }

            int pin = atoi(argv[2]);
            if ( (pin < 0) | (pin > 47) ) {
                printf("Error: '%d' is not a valid pin number\n", pin);
                return;
            }
            if (!gpio_is_dir_out(pin)) {
                printf("Pin %d is not initialized", pin);
                return;
            }
            int value = atoi(argv[3]);
            if ( (value != 0) & (value != 1) ) {
                printf("Error: '%d' is not a valid output level\n", value);
                return;
            }
            gpio_put(pin, value);
            printf("Set pin %d to %d.\n", pin, value);

        }
        
        // Else, print an unknown command error.
        else {
            printf("Error: unknown gpio command '%s'\n", argv[1]);
            return;
        }

    } else {
        printf("Error: Unknown peripheral '%s'\n", argv[0]);
        return;
    }
}