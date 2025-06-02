#include <stdint.h>

#ifndef DMA_H
#define DMA_H

#define DMA_BUFFER_SIZE 2048
#define DMA_BUFFER_COUNT 128

typedef struct  
{
    uint8_t* buffer;
    int in_use;
} dma_buffer_t;

void dma_init();
uint8_t* dma_alloc_buffer();
void dma_free_buffer(uint8_t* buf);

#endif