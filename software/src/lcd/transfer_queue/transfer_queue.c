#include "transfer_queue.h"

#include <stdlib.h>
#include <string.h>

#include "hardware/dma.h"

#include "../lcd.h"

#define COMPLETED_IRQ_NUM 0
#define QUEUE_MAX_LEN_BITS 6

/*
 * Note: We need to update the read address, transfer count, and
 * control register for each transfer. The write address always
 * targets the same peripheral, but we will always write them
 * all anyways, since there's no config that gives us trigger
 * without the write address for a transfer length of 3
 *
 *            +0x0        +0x4          +0x8          +0xC (Trigger)
 * Alias 0: | READ_ADDR | WRITE_ADDR  | TRANS_COUNT | CTRL
 * Alias 1: | CTRL      | READ_ADDR   | WRITE_ADDR  | TRANS_COUNT
 * Alias 2: | CTRL      | TRANS_COUNT | READ_ADDR   | WRITE_ADDR
 * Alias 3: | CTRL      | WRITE_ADDR  | TRANS_COUNT | READ_ADDR
 * 
 * So we match that order in the structure.
 */

typedef struct {
    uint32_t read_address;
    uint32_t write_address;
    uint32_t transfer_count;
    uint32_t control_status;
} transfer_control_block_t;

static transfer_control_block_t TRANSFER_QUEUE[1u << QUEUE_MAX_LEN_BITS];

static int transfer_channel;

static size_t qhead;
static volatile size_t qservicing;
static volatile bool qidle;

static void* free_list[1u << QUEUE_MAX_LEN_BITS];
static volatile size_t freeable;

static void start_transfer() {
    dma_channel_hw_addr(transfer_channel)->read_addr = TRANSFER_QUEUE[qservicing].read_address;
    dma_channel_hw_addr(transfer_channel)->write_addr = TRANSFER_QUEUE[qservicing].write_address;
    dma_channel_hw_addr(transfer_channel)->transfer_count = TRANSFER_QUEUE[qservicing].transfer_count;
    dma_channel_hw_addr(transfer_channel)->ctrl_trig = TRANSFER_QUEUE[qservicing].control_status;
}

static void transfer_completed_callback() {
    if (dma_irqn_get_channel_status(COMPLETED_IRQ_NUM, transfer_channel)) {
        dma_irqn_acknowledge_channel(COMPLETED_IRQ_NUM, transfer_channel);

        free_list[qservicing] = (void*)TRANSFER_QUEUE[qservicing].read_address;
        freeable++;

        TRANSFER_QUEUE[qservicing] = (transfer_control_block_t){0};
        
        qservicing = (qservicing + 1) % (1u << QUEUE_MAX_LEN_BITS);
        if (qservicing != qhead) {
            start_transfer();
        } else {
            qidle = true;
        }
    }
}

void init_transfer_queue() {

    memset(TRANSFER_QUEUE, 0, sizeof(TRANSFER_QUEUE));

    qidle = true;

    qhead = 0;
    qservicing = 0;

    memset(free_list, 0, sizeof(free_list));
    freeable = 0;

    transfer_channel = dma_claim_unused_channel(true);

    irq_set_exclusive_handler(dma_get_irq_num(COMPLETED_IRQ_NUM), transfer_completed_callback);
    irq_set_enabled(dma_get_irq_num(COMPLETED_IRQ_NUM), true);
}

void enqueue_transfer(uint32_t *transfer_data, size_t transfer_count, int wrapping) {

    if (transfer_count == 0) { return; }

    while ((qhead + 1) % (1u << QUEUE_MAX_LEN_BITS) == qservicing) {
        tight_loop_contents();
    }

    dma_irqn_set_channel_enabled(COMPLETED_IRQ_NUM, transfer_channel, false);
    if (freeable > (sizeof(free_list) / sizeof(free_list[0])) / 2) {
        for (int i = 0; i < (sizeof(free_list) / sizeof(free_list[0])); i++)  {
            free(free_list[i]);
            free_list[i] = NULL;
        }
        freeable = 0;
    }
    dma_irqn_set_channel_enabled(COMPLETED_IRQ_NUM, transfer_channel, true);

    dma_channel_config_t transfer_conf = dma_channel_get_default_config(transfer_channel);
    channel_config_set_dreq(&transfer_conf, pio_get_dreq(LCD_PIO_INST, LCD_PIO_SM, true));

    channel_config_set_transfer_data_size(&transfer_conf, DMA_SIZE_32);
    channel_config_set_write_increment(&transfer_conf, false);

    channel_config_set_irq_quiet(&transfer_conf, false);

    if (wrapping > 0) {
        channel_config_set_ring(&transfer_conf, false, wrapping);
        channel_config_set_read_address_update_type(&transfer_conf, DMA_ADDRESS_UPDATE_INCREMENT);
    } else if (wrapping == 0) {
        channel_config_set_ring(&transfer_conf, false, 0);
        channel_config_set_read_address_update_type(&transfer_conf, DMA_ADDRESS_UPDATE_NONE);
    } else if (wrapping < 0) {
        channel_config_set_ring(&transfer_conf, false, 0);
        channel_config_set_read_address_update_type(&transfer_conf, DMA_ADDRESS_UPDATE_INCREMENT);
    }

    TRANSFER_QUEUE[qhead] = (transfer_control_block_t){
        .read_address =  (uint32_t)transfer_data,
        .write_address = (uint32_t)&LCD_PIO_INST->txf[LCD_PIO_SM],
        .transfer_count = dma_encode_transfer_count(transfer_count),
        .control_status = channel_config_get_ctrl_value(&transfer_conf)
    };

    dma_irqn_set_channel_enabled(COMPLETED_IRQ_NUM, transfer_channel, false);

    qhead = (qhead + 1) % (1u << QUEUE_MAX_LEN_BITS);
    
    if (qidle) {
        qidle = false;
        start_transfer();
    }

    dma_irqn_set_channel_enabled(COMPLETED_IRQ_NUM, transfer_channel, true);
}

void flush_transfer_queue() {
    while (!qidle) {
        tight_loop_contents();
    }
}