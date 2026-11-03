#include <stdio.h>
#include <string.h>
#include "display.h"
#include "lcd/lcd.h"

static uint8_t gmem[LCD_GRAM_SIZE];

#define BIN_COUNT 30
#define BIN_WIDTH (320/(BIN_COUNT+2))
#define BIN_HEIGHT 240

static uint16_t bin_buffer[BIN_WIDTH * BIN_HEIGHT];

static uint8_t amplitudes[BIN_COUNT] =
{0, 8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232};
static uint8_t MADCTL = 0x20; // Row/Col Exchange
static uint8_t ROWCONF[4] = {0x00, 0x00, 0x00, 0xF0}; // 0xF0 = 240
static uint8_t COLCONF[4] = {0x00, 0x00, 0x01, 0x40}; // 0x140 = 320

static uint16_t row_start = 0;
static uint16_t row_end = 0;

void display_loop() {

    // Set initial r/w pointer behavior
    lcd_issue_command_with_args(LCD_COM_MADCTL, &MADCTL);
    lcd_issue_command_with_args(LCD_COM_PASET, ROWCONF);
    lcd_issue_command_with_args(LCD_COM_CASET, COLCONF);

    uint8_t color_arg = 0x55; // 16-bit pixels (as opposed to 18-bit packed)
    lcd_issue_command_with_args(LCD_COM_PIXSET, &color_arg);

    // This block fills the screen with 0xffe0
    lcd_command_t dummy_write = LCD_COM_RAMWR;
    dummy_write.argc = LCD_GRAM_SIZE;
    for (int i = 0; i < LCD_GRAM_SIZE; i++) {
        gmem[i] = (i % 2 == 0) ? 0xff : 0xe0;
    }
    lcd_issue_command_with_args(dummy_write, gmem);

    while(1)
    {
        for (int bin = 1; bin < BIN_COUNT+1; bin++){

            row_start = (bin)*BIN_WIDTH;
            row_end = (bin+1)*BIN_WIDTH - 1;

            COLCONF[0] = (row_start >> 8) & 0xff;
            COLCONF[1] = (row_start >> 0) & 0xff;
            COLCONF[2] = (row_end   >> 8) & 0xff;
            COLCONF[3] = (row_end   >> 0) & 0xff;

            lcd_issue_command_with_args(LCD_COM_CASET, COLCONF);

            uint8_t amplitude = amplitudes[bin-1];

            uint bar_bytes = (amplitude             ) * BIN_WIDTH * sizeof(uint16_t);
            uint bak_bytes = (BIN_HEIGHT - amplitude) * BIN_WIDTH * sizeof(uint16_t);

            memset(bin_buffer          , 0xff, bar_bytes);
            memset(bin_buffer+bar_bytes, 0x00, bak_bytes);

            lcd_command_t dummy_write = LCD_COM_RAMWR;
            dummy_write.argc = sizeof(bin_buffer);
            lcd_issue_command_with_args(dummy_write, (uint8_t*)bin_buffer);

            amplitudes[bin-1] = ((bin-1 < 0) ? (amplitudes[BIN_COUNT-1]) : (amplitudes[bin-1] + 8)) % BIN_HEIGHT;
        }
    }
}