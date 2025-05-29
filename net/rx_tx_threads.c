#include "../include/net/packet_queue.h"
#include "../include/genet.h"
#include "../include/threads.h"
#include "../include/uart.h"

static packet_queue_t rx_queue, tx_queue;

void rx_thread(void* arg){
    char buf[MAX_PACKET_SIZE];
    int len;

    uart_puts("RX Thread started\n");

    while (1){
        len = genet_recv(buf);
        if (len > 0){
            packet_queue_push(&rx_queue, buf, len, 0);         // src_ip = 0 for now
        }
        yield();
    }
}

void tx_thread(void* arg) {
    char buf[ETH_FRAME_SIZE];
    int len;
    uint32_t src_ip;

    while (1) {
        if (packet_queue_pop(&tx_queue, buf, &len, &src_ip) == 0) {
            genet_send(buf, len);
        }
    }
}

void start_rx_tx_threads() {
    packet_queue_init(&rx_queue);
    packet_queue_init(&tx_queue);
}

// External access
packet_queue_t* get_rx_queue() { return &rx_queue; }
packet_queue_t* get_tx_queue() { return &tx_queue; }