#ifndef LCD_H
#define LCD_H

#include <stdio.h>
#include <stdbool.h>

#include "hardware/pio.h"

#include "commands/commands.h"

#define LCD_PIO_INST pio0
#define LCD_PIO_SM 0

// LCD Screen Pins
#define LCD_RX_PIN 12
#define LCD_CSn_PIN 13
#define LCD_SCK_PIN 14
#define LCD_TX_PIN 15
#define LCD_DC_PIN 16
#define LCD_RST_PIN 17
#define LCD_LED_PIN 18

#define LCD_GRAM_SIZE 172800

typedef enum {
    LCD_TRANSFER_DOUBLE = 1,
    LCD_TRANSFER_NOWRAP = 0,
    LCD_TRANSFER_SINGLE = -1,
} lcd_transfer_t;

void lcd_init_pio();

void lcd_init();
void lcd_hw_rst();

void lcd_set_display(bool on);
void lcd_set_backlight(bool on);

void lcd_blocking_command(lcd_command_t command);
void lcd_blocking_command_with_args(lcd_command_t command, const uint8_t *argv);
void lcd_blocking_command_with_rets(lcd_command_t command, uint8_t *retv);
void lcd_blocking_command_with_all(lcd_command_t command, const uint8_t *argv, uint8_t *retv);

void lcd_memset8(uint8_t data, size_t len);
void lcd_memset16(uint16_t data, size_t len);

void lcd_nonblocking_command(lcd_command_t command);
void lcd_nonblocking_command_with_args(lcd_command_t command, const uint8_t *argv);
void lcd_nonblocking_command_with_all(lcd_command_t command, const uint8_t *argv, uint8_t *retv);

void lcd_memset(const uint8_t *data, int transfer_count, int wrapping);

#endif