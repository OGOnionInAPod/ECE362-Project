#include <stdio.h>
#include <string.h>
#include "display.h"
#include "lcd/lcd.h"
#include "pico/stdlib.h"

#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240

#define BIN_GAP        1
#define BIN_COUNT      30
#define BIN_COL_WIDTH  (DISPLAY_WIDTH/BIN_COUNT)
#define BIN_BAR_WIDTH  (BIN_COL_WIDTH - 2*BIN_GAP)
#define BIN_MARGIN     ((DISPLAY_WIDTH - (BIN_COL_WIDTH * BIN_COUNT)) / 2)

#define DISPLAY_UPDATE_ALARM_NUM 0
#define DISPLAY_UPDATE_PERIOD 1000000
#define DISPLAY_UPDATE_TIMER_INST timer0_hw
#define DISPLAY_UPDATE_ALARM_IRQ timer_hardware_alarm_get_irq_num(DISPLAY_UPDATE_TIMER_INST, DISPLAY_UPDATE_ALARM_NUM)

static uint8_t MADCTL = 0x20; // Row/Col Exchange
static uint8_t ROWCONF[4] = {0x00, 0x00, 0x00, 0xEF}; // 0xEF = 239
static uint8_t COLCONF[4] = {0x00, 0x00, 0x01, 0x3F}; // 0x13F = 319

static uint16_t current_bin = 0;

static uint8_t amps_previous[BIN_COUNT] = { 0 };
static uint8_t amps_current[BIN_COUNT] = { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 };

void set_colconf(uint16_t start, uint16_t end) {
    COLCONF[0] = (start >> 8) & 0xff;
    COLCONF[1] = (start >> 0) & 0xff;
    COLCONF[2] = (end   >> 8) & 0xff;
    COLCONF[3] = (end   >> 0) & 0xff;
}

void set_rowconf(uint16_t start, uint16_t end) {
    ROWCONF[0] = (start >> 8) & 0xff;
    ROWCONF[1] = (start >> 0) & 0xff;
    ROWCONF[2] = (end   >> 8) & 0xff;
    ROWCONF[3] = (end   >> 0) & 0xff;
}

void update_bin_irq() {
    hw_clear_bits(&DISPLAY_UPDATE_TIMER_INST->intr, 1u << DISPLAY_UPDATE_ALARM_NUM);

    uint16_t col_start = (current_bin * BIN_COL_WIDTH) + BIN_GAP + BIN_MARGIN;
    uint16_t col_end   = col_start + BIN_BAR_WIDTH - 1;

    set_colconf(col_start, col_end);

    lcd_blocking_command_with_args(LCD_COM_CASET, COLCONF);

    uint8_t amp_previous = amps_previous[current_bin];
    uint8_t amp_current = amps_current[current_bin];

    uint16_t upper_block_start, upper_block_end;
    uint16_t lower_block_start, lower_block_end;

    uint8_t fill_color;
    size_t fill_size;
    if (amp_current > amp_previous) {
        fill_color = 0xff;
        fill_size = (amp_current-amp_previous)*BIN_BAR_WIDTH*2;
    } else {
        fill_color = 0x00;
        fill_size = (amp_previous-amp_current)*BIN_BAR_WIDTH*2;
    }

    upper_block_start = 120 + MIN(amp_current, amp_previous);
    upper_block_end   = 120 + MAX(amp_current, amp_previous);

    lower_block_start = 119 - MAX(amp_current, amp_previous);
    lower_block_end   = 119 - MIN(amp_current, amp_previous);

    set_rowconf(upper_block_start, upper_block_end);
    lcd_memset8(fill_color, fill_size);

    set_rowconf(lower_block_start, lower_block_end);
    lcd_memset8(fill_color, fill_size);

    amps_previous[current_bin] = amps_current[current_bin];

    current_bin = (current_bin + 1) % (BIN_COUNT);

    uint64_t target = timer_hw->timerawl + DISPLAY_UPDATE_PERIOD;
    DISPLAY_UPDATE_TIMER_INST->alarm[DISPLAY_UPDATE_ALARM_NUM] = (uint32_t)target;
}

void start_display() {

    lcd_init();

    lcd_nonblocking_command(LCD_COM_SWRESET);
    sleep_ms(120); // 120ms needed before exiting sleep state

    lcd_nonblocking_command(LCD_COM_SLPOUT);
    sleep_ms(5); // 5ms required to leave sleep

    lcd_nonblocking_command(LCD_COM_DISPON); // Display on

    lcd_set_display(true);
    lcd_set_backlight(true);

    // Set initial r/w pointer behavior
    lcd_blocking_command_with_args(LCD_COM_MADCTL, &MADCTL);
    lcd_blocking_command_with_args(LCD_COM_PASET, ROWCONF);
    lcd_blocking_command_with_args(LCD_COM_CASET, COLCONF);

    uint8_t color_arg = 0x55; // 16-bit pixels (as opposed to 18-bit packed)
    lcd_blocking_command_with_args(LCD_COM_PIXSET, &color_arg);

    // Fill the screen
    lcd_memset16(0x0000, 240*320);

    hw_set_bits(&DISPLAY_UPDATE_TIMER_INST->inte, 1u << DISPLAY_UPDATE_ALARM_NUM);
    irq_set_exclusive_handler(DISPLAY_UPDATE_ALARM_IRQ, update_bin_irq);
    irq_clear(DISPLAY_UPDATE_ALARM_IRQ);
    irq_set_enabled(DISPLAY_UPDATE_ALARM_IRQ, true);

    uint64_t target = DISPLAY_UPDATE_TIMER_INST->timerawl + DISPLAY_UPDATE_PERIOD;
    DISPLAY_UPDATE_TIMER_INST->alarm[DISPLAY_UPDATE_ALARM_NUM] = (uint32_t)target;
}