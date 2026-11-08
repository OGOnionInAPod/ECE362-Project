#include <stdio.h>
#include <string.h>
#include "display.h"
#include "lcd/lcd.h"
#include "pico/stdlib.h"

static uint8_t gmem[LCD_GRAM_SIZE];

#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240

#define BIN_GAP        1
#define BIN_COUNT      30
#define BIN_COL_WIDTH  (DISPLAY_WIDTH/BIN_COUNT)
#define BIN_BAR_WIDTH  (BIN_COL_WIDTH - 2*BIN_GAP)
#define BIN_MARGIN     ((DISPLAY_WIDTH - (BIN_COL_WIDTH * BIN_COUNT)) / 2)

#define DISPLAY_UPDATE_ALARM_NUM 0
#define DISPLAY_UPDATE_PERIOD 1000
#define DISPLAY_UPDATE_TIMER_INST timer0_hw
#define DISPLAY_UPDATE_ALARM_IRQ timer_hardware_alarm_get_irq_num(DISPLAY_UPDATE_TIMER_INST, DISPLAY_UPDATE_ALARM_NUM)

static uint8_t MADCTL = 0x20; // Row/Col Exchange
static uint8_t ROWCONF[4] = {0x00, 0x00, 0x00, 0xEF}; // 0xEF = 239
static uint8_t COLCONF[4] = {0x00, 0x00, 0x01, 0x3F}; // 0x13F = 319

static uint16_t current_bin = 0;

static uint8_t amps_previous[BIN_COUNT] = { 0 };
static uint8_t amps_current[BIN_COUNT] = { 0 };

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

    uint16_t col_start = (current_bin  ) * BIN_COL_WIDTH + BIN_GAP + BIN_MARGIN;
    uint16_t col_end   = col_start + BIN_BAR_WIDTH - 1;

    set_colconf(col_start, col_end);

    //lcd_issue_command_with_args(LCD_COM_CASET, COLCONF);

    uint8_t amp_previous = amps_previous[current_bin];
    uint8_t amp_current = amps_current[current_bin];
    int16_t amp_delta = amp_current - amp_previous;

    if (amp_delta > 0) {
        //set_rowconf(amp_previous+120, amp_current+120);
        /* Write the first delta */
        //set_rowconf(119-amp_current, 119-amp_previous);
        /* Write the second delta */

        /*
            1. Compute the start and end row for each delta block
            2. Schedule a DMA transfer to send ROWCONF to the LCD
            3. Schedule a DMA transfer to send the write command
            4. Schedule a DMA transfer to transmit the data
            Notes: These could be one DMA transfer, but that would
            require a large buffer that accounts for the worst-case
            scenario (amplitude delta of 0->119) plus the size of
            the commands. This is wasteful, as the data written is
            a constant color, and thus doesn't need a buffer allocated.
            A single value with a non-incrementing read address would
            suffice. If possible, best approach is to configure all
            three, have 2 CHAIN_TO 3 CHAIN_TO 4. One set of these
            for each of the two blocks.
        */
    } else if (amp_delta < 0) {
        //set_rowconf(amp_current+120, amp_previous+120);
        /* Write the first delta */
        //set_rowconf(119-amp_previous, 119-amp_current);
        /* Write the second delta */

        /*
            Replicate the above with different bounds
        */
    }

    current_bin = (current_bin + 1) % (BIN_COUNT);

    uint64_t target = timer_hw->timerawl + DISPLAY_UPDATE_PERIOD;
    DISPLAY_UPDATE_TIMER_INST->alarm[DISPLAY_UPDATE_ALARM_NUM] = (uint32_t)target;
}

void start_display() {

    lcd_init();

    lcd_issue_command(LCD_COM_SWRESET);
    sleep_ms(120); // 120ms needed before exiting sleep state

    lcd_issue_command(LCD_COM_SLPOUT);
    sleep_ms(5); // 5ms required to leave sleep

    lcd_issue_command(LCD_COM_DISPON); // Display on

    // Set initial r/w pointer behavior
    lcd_issue_command_with_args(LCD_COM_MADCTL, &MADCTL);
    lcd_issue_command_with_args(LCD_COM_PASET, ROWCONF);
    lcd_issue_command_with_args(LCD_COM_CASET, COLCONF);

    uint8_t color_arg = 0x55; // 16-bit pixels (as opposed to 18-bit packed)
    lcd_issue_command_with_args(LCD_COM_PIXSET, &color_arg);

    // This block fills the screen with white pixels
    lcd_command_t dummy_write = LCD_COM_RAMWR;
    dummy_write.argc = LCD_GRAM_SIZE;
    memset(gmem, 0x0f, LCD_GRAM_SIZE);
    lcd_issue_command_with_args(dummy_write, gmem);

    lcd_set_display(true);
    lcd_set_backlight(true);

    hw_set_bits(&DISPLAY_UPDATE_TIMER_INST->inte, 1u << DISPLAY_UPDATE_ALARM_NUM);
    irq_set_exclusive_handler(DISPLAY_UPDATE_ALARM_IRQ, update_bin_irq);
    irq_clear(DISPLAY_UPDATE_ALARM_IRQ);
    irq_set_enabled(DISPLAY_UPDATE_ALARM_IRQ, true);

    uint64_t target = DISPLAY_UPDATE_TIMER_INST->timerawl + DISPLAY_UPDATE_PERIOD;
    DISPLAY_UPDATE_TIMER_INST->alarm[DISPLAY_UPDATE_ALARM_NUM] = (uint32_t)target;
}