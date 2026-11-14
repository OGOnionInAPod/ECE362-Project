#include "transfer_tracker.h"

#include <stdlib.h>

#include "hardware/dma.h"

#define TT_DMA_IRQ_NUM 0

typedef struct transfer_profile_t {
    int comm_chan;
    int data_chan;
    uint32_t *enc_comm;
    uint32_t *enc_data;
    struct transfer_profile_t *next;
    struct transfer_profile_t *prev;
} transfer_profile_t;

static transfer_profile_t * in_flight;

void transfer_complete_callback() {
    if (in_flight == NULL) { return; }

    transfer_profile_t * culprit = NULL;

    int i = 0; do {
        if (TT_DMA_IRQ_NUM == 0) {
            if (dma_channel_get_irq0_status((in_flight+i)->data_chan)) {
                dma_channel_acknowledge_irq0((in_flight+i)->data_chan);
                culprit = (in_flight+i);
                break;
            }
        } else if (TT_DMA_IRQ_NUM == 1) {
            if (dma_channel_get_irq1_status((in_flight+i)->data_chan)) {
                dma_channel_acknowledge_irq1((in_flight+i)->data_chan);
                culprit = (in_flight+i);
                break;
            }
        } else { return; }
    } while ((in_flight+(i++))->next != NULL);

    if (culprit != NULL) {

        dma_channel_cleanup(culprit->comm_chan);
        dma_channel_cleanup(culprit->data_chan);

        if (TT_DMA_IRQ_NUM == 0) {
            dma_channel_set_irq0_enabled(culprit->data_chan, false);
        } else if (TT_DMA_IRQ_NUM == 1) {
            dma_channel_set_irq1_enabled(culprit->data_chan, false);
        } else { return; }

        dma_channel_unclaim(culprit->comm_chan);
        dma_channel_unclaim(culprit->data_chan);

        free(culprit->enc_comm);
        free(culprit->enc_data);

        if ((culprit->next != NULL) && (culprit->prev != NULL)) {
            culprit->next->prev = culprit->prev;
            culprit->prev->next = culprit->next;
        } else if (culprit->next != NULL) {
            culprit->next->prev = NULL;
            in_flight = culprit->next;
        } else if (culprit->prev != NULL) {
            culprit->prev->next = NULL;
        } else {
            in_flight = NULL;
        }
        free(culprit);
    }
}

void init_transfer_queue() {
    irq_set_exclusive_handler(dma_get_irq_num(TT_DMA_IRQ_NUM), transfer_complete_callback);
    irq_set_enabled(dma_get_irq_num(TT_DMA_IRQ_NUM), true);
}

void register_transfer(int comm_chan, int data_chan, uint32_t *enc_comm, uint32_t *enc_data) {

    transfer_profile_t *transfer = malloc(sizeof(transfer_profile_t));

    if (transfer == NULL) { return; }
    if (in_flight == NULL) { in_flight = transfer; }
    else {
        int i; for (i = 0; (in_flight+i)->next != NULL; i++);
        (in_flight+i)->next = transfer;
        transfer->prev = (in_flight+i);
    }

    transfer->comm_chan = comm_chan;
    transfer->data_chan = data_chan;
    transfer->enc_comm  = enc_comm;
    transfer->enc_data  = enc_data;
    transfer->next      = NULL;
    transfer->prev      = NULL;

    if (TT_DMA_IRQ_NUM == 0) {
        dma_channel_set_irq0_enabled(data_chan, true);
    } else if (TT_DMA_IRQ_NUM == 1) {
        dma_channel_set_irq1_enabled(data_chan, true);
    } else { return; }
}