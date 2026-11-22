#include "ads7128.h"

#include <stddef.h>

#include "hardware/i2c.h"

#include "codec.h"

//deafult address value from datasheet 8.3.5, but could be different. Check and "scan" for address to find actual value when testing
#define ADS7128_ADDR 0x10

//ADS7128 command opcodes, got from datasheet 8.5/8.6
#define ADS7128_WRITE_OPCODE 0x08
#define ADS7128_READ_OPCODE  0x10


#define ADS7128_REG_SEQUENCE_CFG 0x10 //sequence configuration register (SEQUENCE_CFG)
#define ADS7128_REG_PIN_CFG 0x05 //pin configuration reg address (PIN_CFG)
#define ADS7128_REG_AUTO_SEQ_CH_SEL 0x12 //channel selection reg address 
#define ADS7128_REG_DATA_CFG 0x02 //data configuration reg address
#define ADS7128_APPEND_CHANNEL_ID 0x10


bool ads7128_write_register(uint8_t reg, uint8_t value) {

    uint8_t data[3] = {ADS7128_WRITE_OPCODE, reg, value};

    int result = i2c_write_blocking(codec_get_i2c_inst(), ADS7128_ADDR, data, 3, false);

    return result == 3; 
}

bool ads7128_read_register(uint8_t reg, uint8_t *value) {
    if (value == NULL) {
        return false;
    }

    uint8_t command[2] = {ADS7128_READ_OPCODE, reg};

    int result = i2c_write_blocking(codec_get_i2c_inst(), ADS7128_ADDR, command, 2, true); //write reg address first

    if (result != 2) {
        return false;
    }

    result = i2c_read_blocking(codec_get_i2c_inst(), ADS7128_ADDR, value, 1, false); //now read then STOP

    return result == 1;
}

bool ads7128_init(void) {
    uint8_t value = 0;

    //configure all 8 channels as analog inputs (last 5 don't matter bc selecting first 3 only but all can be analog input)
    //from pcb I assume we're only using 3 channels A0, A1, A2
    if (!ads7128_write_register(ADS7128_REG_PIN_CFG, 0x00)) { //write 0x00 (all 0's) to the PIN_CFG register so all are analog inputs
        return false;
    }

    //read back the register to verify configuration
    if (!ads7128_read_register(ADS7128_REG_PIN_CFG, &value)) { //we do &value bc we want to store value there too
        return false;
    }

    if (value != 0x00) {
        return false;
    }

    //select AIN0, AIN1, and AIN2 for sequencing (sequencing is when i2c loops through selected channels for our analog data)
    if (!ads7128_write_register(ADS7128_REG_AUTO_SEQ_CH_SEL, 0x07)) { //switched back to 0x07 so we're using all 3 channels again (up for change if better option)
        return false;
    }

    //read back the channel selection
    if (!ads7128_read_register(ADS7128_REG_AUTO_SEQ_CH_SEL, &value)) {
        return false;
    }

    //verify the correct channels are enabled
    if (value != 0x07) {
        return false;
    }

    //select automatic sequencing mode
    if (!ads7128_write_register(ADS7128_REG_SEQUENCE_CFG, 0x01)) {
        return false;
    }

    //append channel ID to ADC conversion data (lower 4-bits)
    if (!ads7128_write_register(ADS7128_REG_DATA_CFG, ADS7128_APPEND_CHANNEL_ID)) {
        return false;
    }

    //verify channel ID configuration
    if (!ads7128_read_register(ADS7128_REG_DATA_CFG, &value)) {
        return false;
    }

    if (value != ADS7128_APPEND_CHANNEL_ID) {
        return false;
    }

    //enable automatic sequencing
    if (!ads7128_write_register(ADS7128_REG_SEQUENCE_CFG, 0x11)) { 
        return false;
    }

    //read back the sequencing configuration
    if (!ads7128_read_register(ADS7128_REG_SEQUENCE_CFG, &value)) {
        return false;
    }

    //verify automatic sequencing is enabled
    if (value != 0x11) {
        return false;
    }

    return true;
}


bool ads7128_read_sample(uint16_t *sample, uint8_t *channel) { //Retrieve one 12-bit conversion result
    if (sample == NULL || channel == NULL) {
        return false;
    }

    uint8_t data[2] = {0}; //create empty 2-byte array to store adc conversion data

    //read two bytes of ADC conversion data
    int result = i2c_read_blocking(codec_get_i2c_inst(), ADS7128_ADDR, data, 2, false);

    if (result != 2) {
        return false;
    }

    //extract the 12-bit adc result
    *sample = ((uint16_t)data[0] << 4) | (data[1] >> 4); //is this way bc i2c is MSB-first so data[0] is MSB

    //extract 4-bit channel identifier, will let us know what channel by number result
    //A0 - 0 A1 - 1 A2 - 2
    *channel = data[1] & 0x0F; 

    return true;
}



#define ADS7128_BURST_MAX_SAMPLES 64

bool ads7128_read_samples(uint16_t *samples, uint8_t *channels, size_t count) { //read multiple ADC samples

    if (samples == NULL || channels == NULL || count == 0 || count > ADS7128_BURST_MAX_SAMPLES) {
        return false;
    }

    //each ADC conversion produces 2 bytes
    uint8_t raw[ADS7128_BURST_MAX_SAMPLES * 2]; //so make an array that handles max count for our max samples of 64

    //read multiple conversions in one I2C transaction
    int result = i2c_read_blocking(codec_get_i2c_inst(), ADS7128_ADDR, raw, count * 2, false);

    if (result != (int)(count * 2)) {
        return false;
    }

    //decode each 16-bit conversion frame
    for (size_t i = 0; i < count; i++) { //extract each sample and its channel

        uint8_t high = raw[i * 2];
        uint8_t low  = raw[i * 2 + 1];

        samples[i] = ((uint16_t)high << 4) | (low >> 4);

        channels[i] = low & 0x0F;
    }

    return true;
}

