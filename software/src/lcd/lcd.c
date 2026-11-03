#include "lcd.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"

void lcd_init() {

    gpio_set_function(LCD_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(LCD_CSn_PIN, GPIO_FUNC_SPI);

    gpio_set_function(LCD_LED_PIN, GPIO_FUNC_SIO); gpio_set_dir(LCD_LED_PIN, GPIO_OUT); gpio_put(LCD_LED_PIN, 1);
    gpio_set_function(LCD_RST_PIN, GPIO_FUNC_SIO); gpio_set_dir(LCD_RST_PIN, GPIO_OUT); gpio_put(LCD_RST_PIN, 1);
    gpio_set_function(LCD_DC_PIN, GPIO_FUNC_SIO); gpio_set_dir(LCD_DC_PIN, GPIO_OUT); gpio_put(LCD_DC_PIN, 1);

    printf("LCD SPI configured @ %d Hz\n", spi_init(LCD_SPI_INSTANCE, 10000000));
    spi_set_format(LCD_SPI_INSTANCE, 8, 0, 0, SPI_MSB_FIRST);
    
    lcd_hw_rst();
    lcd_issue_command(LCD_COM_SWRESET);
    sleep_ms(120); // 120ms needed before exiting sleep state

    lcd_issue_command(LCD_COM_SLPOUT);
    sleep_ms(5); // 5ms required to leave sleep

    lcd_issue_command(LCD_COM_DISPON); // Display on
}

void lcd_hw_rst() {
    sleep_ms(10);
    gpio_put(LCD_RST_PIN, 0);
    sleep_ms(20);
    gpio_put(LCD_RST_PIN, 1);
    sleep_ms(150); // Chip needs time to come out of reset
}

void lcd_issue_command(lcd_command_t command) {
    assert((command.argc == 0) & (command.retc == 0));
    lcd_issue_command_with_all(command, NULL, NULL);
}

void lcd_issue_command_with_args(lcd_command_t command, uint8_t *argv) {
    assert(command.retc == 0);
    lcd_issue_command_with_all(command, argv, NULL);
}

void lcd_issue_command_with_rets(lcd_command_t command, uint8_t *retv) {
    assert(command.argc == 0);
    lcd_issue_command_with_all(command, NULL, retv);
}

void lcd_issue_command_with_all(lcd_command_t command, uint8_t *argv, uint8_t *retv) {
    gpio_put(LCD_DC_PIN, 0); // Next byte is command
    spi_write_blocking(LCD_SPI_INSTANCE, &command.opcode, 1); // Send command

    gpio_put(LCD_DC_PIN, 1); // Next bytes are data
    if (command.argc > 0) {
        spi_write_blocking(LCD_SPI_INSTANCE, argv, command.argc); // Send parameters
    }

    if (command.retc > 0) {
        spi_read_blocking(LCD_SPI_INSTANCE, 0, retv, command.retc); // Read return data
    }
}