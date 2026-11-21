#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "dac/dac.h"

#include "../dac.pio.h"

// I2S/TDM (PIO)
#define DAC_WSEL_PIN 3
#define DAC_BCLK_PIN 4
#define DAC_DATA_PIN 5

#define DAC_I2C_ADR1 0
#define DAC_I2C_ADR2 1
#define DAC_I2C_ADDR (0b10011 << 2u | DAC_I2C_ADR2 << 1u | DAC_I2C_ADR1 << 0u)

#define DAC_PIO_INST pio1
#define DAC_PIO_SM 0

#define DAC_I2C_INST i2c1

void dac_init() {
    for (uint i = 0; i < count_of(daci2s_program_instructions); i++) {
        DAC_PIO_INST->instr_mem[i] = daci2s_program_instructions[i];
    }
    pio_sm_config c = daci2s_program_get_default_config(0);

    pio_sm_set_consecutive_pindirs(DAC_PIO_INST, DAC_PIO_SM,
        DAC_WSEL_PIN, 3, true // 3, 4, and 5 are all outputs
    );

    gpio_set_function_masked(
        1u << DAC_WSEL_PIN |
        1u << DAC_DATA_PIN |
        1u << DAC_BCLK_PIN ,
    pio_get_funcsel(DAC_PIO_INST));

    sm_config_set_out_pin_base(&c, DAC_DATA_PIN);       // Just data
    sm_config_set_sideset_pin_base(&c, DAC_WSEL_PIN);   // 2'{ BCLK, WSEL }

    sm_config_set_clkdiv_int_frac(&c, 24, 106);

    pio_sm_init(DAC_PIO_INST, DAC_PIO_SM, 0, &c);
    pio_sm_set_enabled(DAC_PIO_INST, DAC_PIO_SM, true);
    
    sleep_ms(200);

    dac_reg_write(37, 0x18); // Disable missing SCK and halt detection
    dac_reg_write(40, 0x00); // Set I2S word length to 16
}

int dac_reg_read(uint8_t reg) {
    uint8_t result;

    if (i2c_write_blocking(DAC_I2C_INST, DAC_I2C_ADDR, &reg, 1, true) == PICO_ERROR_GENERIC) {
        return -1;
    }
    if (i2c_read_blocking(DAC_I2C_INST, DAC_I2C_ADDR, &result, 1, false) == PICO_ERROR_GENERIC) {
        return -1;
    }
    return (int)result;
}

int dac_reg_write(uint8_t reg, uint8_t value) {
    uint8_t d[2] = { reg, value };
    if (i2c_write_blocking(DAC_I2C_INST, DAC_I2C_ADDR, d, 2, false) == PICO_ERROR_GENERIC) {
        return -1;
    }
    return 0;
}

void dac_send_sample(uint32_t sample) {
    pio_sm_put_blocking(DAC_PIO_INST, DAC_PIO_SM, sample);
}