#pragma once
#include <stdint.h>

#ifndef GENET_H
#define GENET_H

#define GENET_BASE 0xFD580000UL

#define GENET_MAC_CTRL (GENET_BASE + 0x008)
#define GENET_MAC_STATUS (GENET_BASE + 0x00C)

#define GENET_MII_CTRL (GENET_BASE + 0x400)
#define GENET_MII_STATUS (GENET_BASE + 0x404)

#define GENET_DMA_BASE (GENET_BASE + 0x800)

#define GENET_RX_RING_BASE (GENET_DMA_BASE + 0x00)
#define GENET_TX_RING_BASE (GENET_DMA_BASE + 0x100)

#define GENET_DMA_DESC_COUNT 32
#define ETH_FRAME_SIZE 1536

typedef struct {
    volatile uint32_t addr_lo;  // 64 bit buffer address
    volatile uint32_t addr_hi;  // 64 bit buffer address
    volatile uint32_t length;   // length of frame
    volatile uint32_t status;   // status bits set by hardware
} dma_desc_t;

extern dma_desc_t rx_ring[GENET_DMA_DESC_COUNT];
extern char rx_buffers[GENET_DMA_DESC_COUNT][ETH_FRAME_SIZE];

extern dma_desc_t tx_ring[GENET_DMA_DESC_COUNT];
extern char tx_buffers[GENET_DMA_DESC_COUNT][ETH_FRAME_SIZE];

// Read/Write MMIO
#define MMIO32(addr) (*(volatile unsigned int*)addr)

void genet_init(void);
void genet_send(const char* data, int len);
int genet_recv(char* buf);

void genet_setup_rx_ring(void);
void genet_handle_rx(void);

void genet_poll(void);

#endif