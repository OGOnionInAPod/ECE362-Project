#include "lcd.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "lcd.pio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

#include <stdlib.h>

#include "transfer_tracker/transfer_tracker.h"

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

    init_transfer_queue();

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
        lcd_blocking_command(LCD_COM_DISPON); // Display on
    } else {
        lcd_blocking_command(LCD_COM_DISPOFF); // Display off
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

void lcd_blocking_command(lcd_command_t command) {
    assert((command.argc == 0) & (command.retc == 0));
    lcd_blocking_command_with_all(command, NULL, NULL);
}

void lcd_blocking_command_with_args(lcd_command_t command, const uint8_t *argv) {
    assert(command.retc == 0);
    lcd_blocking_command_with_all(command, argv, NULL);
}

void lcd_blocking_command_with_all(lcd_command_t command, const uint8_t *argv, uint8_t *retv) {
    pio_sm_put_blocking(LCD_PIO_INST, LCD_PIO_SM, lcd_encode_packet(command.opcode, false)); 

    for (int arg = 0; arg < command.argc; arg++) {
        pio_sm_put_blocking(LCD_PIO_INST, LCD_PIO_SM, lcd_encode_packet(argv[arg], true)); 
    }
    for (int ret = 0; ret < command.retc; ret++) {
        tight_loop_contents(); // TODO: implement reading
    }
}

void lcd_memset8(uint8_t data, size_t len) {
    lcd_command_t c = LCD_COM_RAMWR; c.argc = len;
    lcd_nonblocking_command_with_all_wrapping(c, &data, NULL, 0);
}

void lcd_memset16(uint16_t data, size_t len) {
    lcd_command_t c = LCD_COM_RAMWR; c.argc = 2*len;
    uint16_t data_be = (data << 8) | (data >> 8);
    lcd_nonblocking_command_with_all_wrapping(c, (uint8_t*)&data_be, NULL, 1);
}

void lcd_nonblocking_command(lcd_command_t command) {
    assert((command.argc == 0) & (command.retc == 0));
    lcd_nonblocking_command_with_all_wrapping(command, NULL, NULL, -1);
}

void lcd_nonblocking_command_with_args(lcd_command_t command, const uint8_t *argv) {
    assert(command.retc == 0);
    lcd_nonblocking_command_with_all_wrapping(command, argv, NULL, -1);
}

void lcd_nonblocking_command_with_all(lcd_command_t command, const uint8_t *argv, uint8_t * retv) {
    lcd_nonblocking_command_with_all_wrapping(command, argv, retv, -1);
}

void lcd_nonblocking_command_with_all_wrapping(lcd_command_t command, const uint8_t *argv, uint8_t *retv, int wrapping) {

    size_t effective_argc;
    if (0 <= wrapping && wrapping < 31) {
        effective_argc = 1u << wrapping;
    }
    else if (wrapping == -1) {
        effective_argc = command.argc;
    } else { return; }

    // Allocate for command + data elements in the transfer
    uint32_t * comm_enc = malloc(sizeof(uint32_t));
    if (comm_enc == NULL) { return; }

    uint32_t * data_enc = malloc(sizeof(uint32_t) * effective_argc);
    if (data_enc == NULL) { return; }

    *comm_enc = lcd_encode_packet(command.opcode, false);
    for (int arg = 0; arg < effective_argc; arg++) {
        data_enc[arg] = lcd_encode_packet(argv[arg], true);
    }

    // Get an idle DMA channel
    int comm_chan = dma_claim_unused_channel(true);
    int data_chan = dma_claim_unused_channel(true);

    dma_channel_config_t comm_conf = dma_channel_get_default_config(comm_chan);
    channel_config_set_dreq(&comm_conf, pio_get_dreq(LCD_PIO_INST, LCD_PIO_SM, true));
    channel_config_set_transfer_data_size(&comm_conf, DMA_SIZE_32);
    channel_config_set_chain_to(&comm_conf, data_chan);

    dma_channel_configure(comm_chan, &comm_conf,
        &LCD_PIO_INST->txf[LCD_PIO_SM],
        comm_enc,
        dma_encode_transfer_count(1),
        false
    );

    dma_channel_config_t data_conf = dma_channel_get_default_config(data_chan);
    channel_config_set_dreq(&data_conf, pio_get_dreq(LCD_PIO_INST, LCD_PIO_SM, true));
    channel_config_set_transfer_data_size(&data_conf, DMA_SIZE_32);
    channel_config_set_write_address_update_type(&data_conf, DMA_ADDRESS_UPDATE_NONE);
    channel_config_set_irq_quiet(&data_conf, false);

    if (wrapping > 0) {
        channel_config_set_ring(&data_conf, false, wrapping);
        channel_config_set_read_address_update_type(&data_conf, DMA_ADDRESS_UPDATE_INCREMENT);
    } else if (wrapping == 0) {
        channel_config_set_ring(&data_conf, false, 0);
        channel_config_set_read_address_update_type(&data_conf, DMA_ADDRESS_UPDATE_NONE);
    } else if (wrapping < 0) {
        channel_config_set_ring(&data_conf, false, 0);
        channel_config_set_read_address_update_type(&data_conf, DMA_ADDRESS_UPDATE_INCREMENT);
    }

    dma_channel_configure(data_chan, &data_conf,
        &LCD_PIO_INST->txf[LCD_PIO_SM],
        data_enc,
        dma_encode_transfer_count(command.argc),
        false
    );

    enqueue_transfer(comm_chan, data_chan, comm_enc, data_enc);
}