#include "pcm5122.h"

#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"

#include "codec.h"

#include "../dac.pio.h"

// I2S/TDM (PIO)
#define PCM5122_WSEL_PIN 3
#define PCM5122_BCLK_PIN 4
#define PCM5122_DATA_PIN 5

#define PCM5122_I2C_ADR1 0
#define PCM5122_I2C_ADR2 1
#define PCM5122_I2C_ADDR (0b10011 << 2u | PCM5122_I2C_ADR2 << 1u | PCM5122_I2C_ADR1 << 0u)

#define PCM5122_PIO_INST pio1
#define PCM5122_PIO_SM 0

static int32_t output[2] = {0};

bool pcm5122_init() {
    for (uint i = 0; i < count_of(daci2s_program_instructions); i++) {
        PCM5122_PIO_INST->instr_mem[i] = daci2s_program_instructions[i];
    }
    pio_sm_config pio_config = daci2s_program_get_default_config(0);

    if (pio_sm_set_consecutive_pindirs(PCM5122_PIO_INST, PCM5122_PIO_SM,
        PCM5122_WSEL_PIN, 3, true // 3, 4, and 5 are all outputs
    ) != PICO_OK) {
        return false;
    }

    gpio_set_function_masked(
        1u << PCM5122_WSEL_PIN |
        1u << PCM5122_DATA_PIN |
        1u << PCM5122_BCLK_PIN ,
    pio_get_funcsel(PCM5122_PIO_INST));

    sm_config_set_out_pin_base(&pio_config, PCM5122_DATA_PIN);       // Just data
    sm_config_set_sideset_pin_base(&pio_config, PCM5122_WSEL_PIN);   // 2'{ BCLK, WSEL }

    // Appropriate clock ratio for 48k samples per second at 64 clocks per sample and 4 cycles per clock
    sm_config_set_clkdiv_int_frac(&pio_config, 12, 53);

    if (pio_sm_init(PCM5122_PIO_INST, PCM5122_PIO_SM, 0, &pio_config) != PICO_OK) {
        return false;
    }
    pio_sm_set_enabled(PCM5122_PIO_INST, PCM5122_PIO_SM, true);

    if (!pcm5122_write_register(PCM5122_REG_PLLREF, 0x10)) { return false; } // PLL reference is BCLK
    if (!pcm5122_write_register(PCM5122_REG_IGRFLG, 0x18)) { return false; } // Disable missing SCK and halt detection
    if (!pcm5122_write_register(PCM5122_REG_I2SFMT, 0x03)) { return false; } // Set I2S word length to 32 (64 per frame)

    sleep_ms(200); // Should be more than enough for device startup

    int channel = dma_claim_unused_channel(true);
    dma_channel_config_t dma_config = dma_channel_get_default_config(channel);

    /*
     * Total payload is 2 32-bit signed integers (left/right channels).
     * 64 total bits aka 8 bytes of data. 8 byte transfer requires the
     * lower 3 bits to change so we set a three bit ring, increment the
     * read address, and start an endless transfer. Behavior is that
     * the output is the last provided sample. DAC needs PIO clock to
     * maintain PLL lock so sending most recent sample repeatedly is
     * better than filling empty space with zero bytes.
     */
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    channel_config_set_ring(&dma_config, false, 3);
    channel_config_set_read_increment(&dma_config, DMA_ADDRESS_UPDATE_INCREMENT);
    channel_config_set_write_increment(&dma_config, DMA_ADDRESS_UPDATE_NONE);

    // Pace transfers with the PIO FIFO (TX)
    channel_config_set_dreq(&dma_config, pio_get_dreq(PCM5122_PIO_INST, PCM5122_PIO_SM, true));

    dma_channel_configure(channel, &dma_config,
        &PCM5122_PIO_INST->txf[PCM5122_PIO_SM],
        output,
        dma_encode_transfer_count_with_self_trigger(2),
        true
    );

    return true;
}

bool pcm5122_read_register(uint8_t reg, uint8_t *value) {
    if (i2c_write_blocking(codec_get_i2c_inst(), PCM5122_I2C_ADDR, &reg, 1, true) == PICO_ERROR_GENERIC) { return false; }
    if (i2c_read_blocking(codec_get_i2c_inst(), PCM5122_I2C_ADDR, value, 1, false) == PICO_ERROR_GENERIC) { return false; }
    return true;
}

bool pcm5122_write_register(uint8_t reg, uint8_t value) {
    uint8_t d[2] = { reg, value };
    if (i2c_write_blocking(codec_get_i2c_inst(), PCM5122_I2C_ADDR, d, 2, false) == PICO_ERROR_GENERIC) { return false; }
    return true;
}

void pcm5122_set_output(int32_t left, int32_t right) {
    output[0] = left;
    output[1] = right;
}