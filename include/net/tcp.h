#include <stdint.h>
#include <stddef.h>

#ifndef TCP_H
#define TCP_H

#define TCP_HEADER_SIZE 20
#define TCP_MAX_DATA_SIZE 1460 // MTU - IP header (20) - TCP header (20)

#define TCP_RX_BUFFER_SIZE 2048

extern char tcp_rxx_buffer[TCP_RX_BUFFER_SIZE];
extern int tcp_rx_len;

// TCP Flags
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

typedef enum {
    TCP_CLOSED,
    TCP_SYN_SENT,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_TIME_WAIT,
} tcp_state_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;        // upper 4 bits
    uint8_t flags;              // TCP control flags
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_header_t;

// Public API
int tcp_connect(uint32_t dest_ip, uint16_t dest_port);
int tcp_send(const char* data, int len);
int tcp_receive(char* buf, int* len);
void tcp_close(void);

// Internal (Called by IP layer)
void tcp_recv(const char* packet, int len, uint32_t src_ip);
void tcp_listen(uint16_t port);

#endif