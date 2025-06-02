#include "../include/net/dma.h"
#include "../include/lib.h"

static uint8_t dma_pool[DMA_BUFFER_COUNT][DMA_BUFFER_SIZE];
static dma_buffer_t dma_descriptors[DMA_BUFFER_COUNT];

void dma_init(){
    for (int i = 0; i < DMA_BUFFER_COUNT; i++){
        dma_descriptors[i].buffer = dma_pool[i];
        dma_descriptors[i].in_use = 0;
    }
}

uint8_t* dma_alloc_buffer(){
    for (int i = 0; i < DMA_BUFFER_COUNT; i++){
        if (!dma_descriptors[i].in_use){
            dma_descriptors[i].in_use = 1;
            return dma_descriptors[i].buffer;
        }
    }
    return 0;
}

void dma_free_buffer(uint8_t* buf){
    for (int i = 0; i < DMA_BUFFER_COUNT; i++){
        if (dma_descriptors[i].buffer == buf){
            dma_descriptors[i].in_use = 0;
            return;
        }
    }
}