#ifndef TRANSFER_QUEUE_H
#define TRANSFER_QUEUE_H

#include "pico/stdlib.h"
#include "hardware/dma.h"

void init_transfer_queue();
void enqueue_transfer(uint32_t *transfer_data, size_t transfer_count, int wrapping);
void flush_transfer_queue();

#endif