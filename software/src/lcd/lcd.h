#ifndef LCD_H
#define LCD_H

#include <stdio.h>
#include "commands.h"

#define LCD_SPI_INSTANCE spi1

// LCD Screen Pins
#define LCD_RX_PIN 12
#define LCD_CSn_PIN 13
#define LCD_SCK_PIN 14
#define LCD_TX_PIN 15
#define LCD_DC_PIN 16
#define LCD_RST_PIN 17
#define LCD_LED_PIN 18

#define LCD_GRAM_SIZE 172800

void lcd_init();
void lcd_hw_rst();

void lcd_issue_command(lcd_command_t command);
void lcd_issue_command_with_args(lcd_command_t command, uint8_t *argv);
void lcd_issue_command_with_rets(lcd_command_t command, uint8_t *retv);
void lcd_issue_command_with_all(lcd_command_t command, uint8_t *argv, uint8_t *retv);


#endif