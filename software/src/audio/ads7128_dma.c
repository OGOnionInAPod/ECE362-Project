
#include "ads7128_dma.h"
#include "codec.h"

#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/regs/i2c.h" //for i2c hardware register definitions
#include "pico/time.h" //for timer functions
#include <stddef.h>

#define ADS7128_ADDR 0x10
#define ADC_DMA_BYTES (ADS7128_DMA_SAMPLES * 2)

//DMA status values (dma_poll)
#define ADC_DMA_IDLE  0
#define ADC_DMA_BUSY  1
#define ADC_DMA_READY 2
#define ADC_DMA_ERROR 3

//I2C read command and stop bit
#define I2C_READ_CMD (1u << 8)
#define I2C_STOP_CMD (1u << 9)

//DMA channels used for ADC communication
static int adc_dma_tx = -1;
static int adc_dma_rx = -1;

static int status = ADC_DMA_IDLE;
static uint64_t start_time = 0;

//Arrays used during DMA transfers
static uint32_t commands[ADC_DMA_BYTES];
static uint32_t received[ADC_DMA_BYTES];

bool ads7128_dma_init(void) {

    //Don't claim DMA channels more than once
    if (adc_dma_tx >= 0 || adc_dma_rx >= 0) {
        return false;
    }

    //Claim two unused DMA channels for ADC use
    adc_dma_tx = dma_claim_unused_channel(false);

    if (adc_dma_tx < 0) { //verify is valid channel
        return false;
    }

    adc_dma_rx = dma_claim_unused_channel(false);

    if (adc_dma_rx < 0) { //if could not find valid RX, unclaim TX
        dma_channel_unclaim(adc_dma_tx);
        adc_dma_tx = -1;
        return false;
    }

    status = ADC_DMA_IDLE;
    return true;
}

bool ads7128_dma_start(void) {

    //Check DMA channels and status
    if (adc_dma_tx < 0 || adc_dma_rx < 0 ||
        status != ADC_DMA_IDLE) {
        return false;
    }

    i2c_hw_t *hw = i2c_get_hw(codec_get_i2c_inst());

    //Make sure I2C is not currently busy
    if (hw->status & I2C_IC_STATUS_ACTIVITY_BITS) {
        return false;
    }

    //Create read commands for each incoming byte
    for (int i = 0; i < ADC_DMA_BYTES; i++) {
        commands[i] = I2C_READ_CMD;
    }

    //Stop I2C after the final byte
    commands[ADC_DMA_BYTES - 1] |= I2C_STOP_CMD;

    //Set ADC address
    hw->enable = 0;
    hw->tar = ADS7128_ADDR;
    hw->enable = 1;

    //Clear old I2C flags
    (void)hw->clr_stop_det;
    (void)hw->clr_tx_abrt;

    //Configure DMA TX
    dma_channel_config tx_config =
        dma_channel_get_default_config(adc_dma_tx);

    channel_config_set_transfer_data_size(&tx_config, DMA_SIZE_32);
    channel_config_set_read_increment(&tx_config, true);
    channel_config_set_write_increment(&tx_config, false);
    channel_config_set_dreq(&tx_config, DREQ_I2C1_TX);

    dma_channel_configure(adc_dma_tx, &tx_config, &hw->data_cmd, commands, ADC_DMA_BYTES, false);

    //Configure DMA RX
    dma_channel_config rx_config = dma_channel_get_default_config(adc_dma_rx);

    channel_config_set_transfer_data_size(&rx_config, DMA_SIZE_32);
    channel_config_set_read_increment(&rx_config, false);
    channel_config_set_write_increment(&rx_config, true);
    channel_config_set_dreq(&rx_config, DREQ_I2C1_RX);

    dma_channel_configure(adc_dma_rx, &rx_config, received, &hw->data_cmd, ADC_DMA_BYTES, false);

    //Enable I2C DMA transfers
    hw->dma_cr = 3u;

    status = ADC_DMA_BUSY;
    start_time = time_us_64(); //will use this to compare in dma_poll to compare for timing and get status

    //Start both DMA channels
    dma_start_channel_mask((1u << adc_dma_tx) | (1u << adc_dma_rx));

    return true;
}

int ads7128_dma_poll(void) {

    if (status != ADC_DMA_BUSY) {
        return status;
    }

    i2c_hw_t *hw = i2c_get_hw(codec_get_i2c_inst());

    //Check for I2C errors, if so stop everything
    if (hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS) {
        dma_channel_abort(adc_dma_tx);
        dma_channel_abort(adc_dma_rx);
        hw->dma_cr = 0;
        (void)hw->clr_tx_abrt;
        status = ADC_DMA_ERROR;
        return status;
    }

    //Check if both DMA channels finished to get READY status
    if (!dma_channel_is_busy(adc_dma_tx) && !dma_channel_is_busy(adc_dma_rx) && (hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS)) {
        (void)hw->clr_stop_det;
        hw->dma_cr = 0;
        status = ADC_DMA_READY;
    }

    //Check for timeout (100 ms) aka (100000 us) 
    if (time_us_64() - start_time > 100000) {
        dma_channel_abort(adc_dma_tx);
        dma_channel_abort(adc_dma_rx);
        hw->dma_cr = 0;
        status = ADC_DMA_ERROR;
        return status;
    }

    return status;
}

bool ads7128_dma_get_samples(uint16_t *samples, uint8_t *channels) {

    //Only get samples after DMA finishes
    if (status != ADC_DMA_READY ||
        samples == NULL || channels == NULL) {
        return false;
    }

    //Convert received bytes into ADC samples
    for (int i = 0; i < ADS7128_DMA_SAMPLES; i++) {

        uint8_t high = (uint8_t)received[2 * i];
        uint8_t low = (uint8_t)received[2 * i + 1];

        //Extract 12-bit ADC sample
        samples[i] = ((uint16_t)high << 4) | (low >> 4);

        //Extract channel ID
        channels[i] = low & 0x0F;
    }

    status = ADC_DMA_IDLE;
    return true;
}
