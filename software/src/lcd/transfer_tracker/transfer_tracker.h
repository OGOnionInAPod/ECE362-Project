#ifndef TRANSFER_TRACKER_H
#define TRANSFER_TRACKER_H

#include "pico/stdlib.h"
#include "hardware/dma.h"

void init_transfer_queue();
void enqueue_transfer(int comm_chan, int data_chan, uint32_t *enc_comm, uint32_t *enc_data);

#endif