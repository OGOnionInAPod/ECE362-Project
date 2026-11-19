#ifndef LCD_H
#define LCD_H

#include <stdio.h>
#include <stdbool.h>

#include "hardware/pio.h"

#include "commands/commands.h"

#define LCD_PIO_INST pio0
#define LCD_PIO_SM 0

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

#endif