#ifndef ADS7128_DMA_H
#define ADS7128_DMA_H

#include <stdbool.h>
#include <stdint.h>

#define ADS7128_DMA_SAMPLES 32

bool ads7128_dma_init(void);
bool ads7128_dma_start(void);
int ads7128_dma_poll(void); 
bool ads7128_dma_get_samples(uint16_t *samples, uint8_t *channels);

#endif

