#include "../include/genet.h"
#include "../include/uart.h"
#include "../include/lib.h"
#include "../include/net/ethernet.h"

dma_desc_t rx_ring[GENET_DMA_DESC_COUNT] __attribute__((aligned(16)));
char rx_buffers[GENET_DMA_DESC_COUNT][ETH_FRAME_SIZE] __attribute__((aligned(16)));

dma_desc_t tx_ring[GENET_DMA_DESC_COUNT] __attribute__((aligned(16)));
char tx_buffers[GENET_DMA_DESC_COUNT][ETH_FRAME_SIZE] __attribute__((aligned(16)));

static int tx_index = 0;
static int rx_index = 0;

void genet_init(){
    uart_puts("Initializint GENET Ethernet..\n");

    genet_setup_rx_ring();

    // Bring up MAC
    MMIO32(GENET_MAC_CTRL) |= 0x1;

    genet_setup_rx_ring();

    uart_puts("GENET Ethernet initialized.\n");
}

void genet_send(const char *data, int len) {
    // TODO: Setup DMA buffer and transmit
    if (len > ETH_FRAME_SIZE){
        uart_puts("TX: Frame too large\n");
        return;
    }

    int index = tx_index;
    tx_index = (tx_index + 1) % GENET_DMA_DESC_COUNT;

    for (int i = 0; i < len; i++){
        tx_buffers[index][i] = data[i];
    }

    tx_ring[index].addr_lo = (uint32_t)(uintptr_t) tx_buffers[index];
    tx_ring[index].addr_hi = 0;
    tx_ring[index].length = len;
    tx_ring[index].status = (1 << 31);

    MMIO32(GENET_TX_RING_BASE) = (uint32_t)(uintptr_t)&tx_ring[0];

    uart_puts("TX: Packet queued\n");
}

int genet_recv(char *buf) {
    // TODO: Poll RX descriptor for received packet
    dma_desc_t* desc = &rx_ring[rx_index];
    if (!(desc->status & 0x80000000)){
        int len = desc->length;

        if (len > 0 && len <= ETH_FRAME_SIZE){
            memcpy(buf, rx_buffers[rx_index], len);
        }

        // Re-arm the descriptor
        desc->status = 0x80000000;

        rx_index = (rx_index + 1) % GENET_DMA_DESC_COUNT;
        return len;
    }
    return 0;
}

void genet_setup_rx_ring(void){
    for (int i = 0; i < GENET_DMA_DESC_COUNT; i++){
        rx_ring[i].addr_lo = (uint32_t)((uint64_t)&rx_buffers[i] & 0xFFFFFFFF);
        rx_ring[i].addr_hi = (uint32_t)(((uint64_t)&rx_buffers[i] >> 32) & 0xFFFFFFFF);
        rx_ring[i].length = ETH_FRAME_SIZE;
        rx_ring[i].status = 0x80000000;
    }

    // Write base address of RXX ring to DMA engine
    MMIO32(GENET_RX_RING_BASE) = (uint64_t)&rx_ring[0] & 0xFFFFFFFF;

    uart_puts("RX ring configured.\n");
}

void genet_handle_rx(void){
    char buf[ETH_FRAME_SIZE];
    int len;

    while ((len = genet_recv(buf)) > 0) {
        ethernet_handle(buf, len);
    }
}


void genet_poll(void) {
    genet_handle_rx();
}