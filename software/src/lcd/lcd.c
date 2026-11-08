#include "lcd.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lcd.pio.h"
#include "hardware/pio.h"

void lcd_init() {
    // Setup LCD and RESET as regular GPIO controllable pins. PIO doesn't need these
    gpio_set_function(LCD_LED_PIN, GPIO_FUNC_SIO); gpio_set_dir(LCD_LED_PIN, GPIO_OUT); gpio_put(LCD_LED_PIN, 0);
    gpio_set_function(LCD_RST_PIN, GPIO_FUNC_SIO); gpio_set_dir(LCD_RST_PIN, GPIO_OUT); gpio_put(LCD_RST_PIN, 1);

    for (uint i = 0; i < count_of(lcdspi_program_instructions); i++) {
        LCD_PIO_INST->instr_mem[i] = lcdspi_program_instructions[i];
    }
    pio_sm_config c = lcdspi_program_get_default_config(0);

    // Set directions for the pins
    pio_sm_set_consecutive_pindirs(LCD_PIO_INST, LCD_PIO_SM,
        LCD_CSn_PIN, 4, true // 13, 14, 15, 16 are all outputs
    );
    // Set initial levels for this pins on PIO
    pio_sm_set_pins_with_mask(LCD_PIO_INST, LCD_PIO_SM,
        // Pin values
        1u << LCD_CSn_PIN |
        0u << LCD_SCK_PIN |
        0u << LCD_DC_PIN  |
        0u << LCD_TX_PIN  ,
        // Pin mask
        1u << LCD_CSn_PIN |
        1u << LCD_SCK_PIN |
        1u << LCD_DC_PIN  |
        1u << LCD_TX_PIN
    );

    // Hand the pads off to PIO now that
    // levels and directions are defined
    gpio_set_function_masked(
        1u << LCD_CSn_PIN |
        1u << LCD_SCK_PIN |
        1u << LCD_DC_PIN  |
        1u << LCD_TX_PIN  ,
    GPIO_FUNC_PIO0);

    sm_config_set_out_pin_base(&c, LCD_TX_PIN);
    sm_config_set_set_pin_base(&c, LCD_DC_PIN);
    sm_config_set_sideset_pin_base(&c, LCD_CSn_PIN);

    pio_sm_init(LCD_PIO_INST, LCD_PIO_SM, 0, &c);    // Configure and reset SM
    pio_sm_set_enabled(LCD_PIO_INST, LCD_PIO_SM, true); // Start the SM

    lcd_hw_rst(); // Perform hardware reset (via RST pin)
}

void lcd_hw_rst() {
    sleep_ms(10);
    gpio_put(LCD_RST_PIN, 0);
    sleep_ms(20);
    gpio_put(LCD_RST_PIN, 1);
    sleep_ms(150); // Chip needs time to come out of reset
}

void lcd_set_display(bool on) {
    if (on) {
        lcd_issue_command(LCD_COM_DISPON); // Display on
    } else {
        lcd_issue_command(LCD_COM_DISPOFF); // Display off
    }
}

void lcd_set_backlight(bool on) {
    if (on) {
        gpio_put(LCD_LED_PIN, 1);
    } else {
        gpio_put(LCD_LED_PIN, 0);
    }
}

static inline uint32_t lcd_encode_packet(uint8_t data, bool is_data) {
    return ((uint32_t)is_data << 31 | (uint32_t)data << 23);
}

void lcd_issue_command(lcd_command_t command) {
    assert((command.argc == 0) & (command.retc == 0));
    lcd_issue_command_with_all(command, NULL, NULL);
}

void lcd_issue_command_with_args(lcd_command_t command, const uint8_t *argv) {
    assert(command.retc == 0);
    lcd_issue_command_with_all(command, argv, NULL);
}

void lcd_issue_command_with_rets(lcd_command_t command, uint8_t *retv) {
    assert(command.argc == 0);
    lcd_issue_command_with_all(command, NULL, retv);
}

void lcd_issue_command_with_all(lcd_command_t command, const uint8_t *argv, uint8_t *retv) {
    pio_sm_put_blocking(LCD_PIO_INST, LCD_PIO_SM, lcd_encode_packet(command.opcode, false)); 

    for (int arg = 0; arg < command.argc; arg++) {
        pio_sm_put_blocking(LCD_PIO_INST, LCD_PIO_SM, lcd_encode_packet(argv[arg], true)); 
    }
    for (int ret = 0; ret < command.retc; ret++) {
        tight_loop_contents(); // TODO: implement reading
    }
}