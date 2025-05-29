#pragma once
#include <stdint.h>

#define UDP_HEADER_SIZE 8
#define UDP_MAX_DATA_SIZE 1472  // Ethernet MTU - IP/UDP header size
#define LOCAL_PORT 1234

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

void udp_send(const char* data, int len, uint32_t dest_ip, uint16_t dest_port);
void udp_recv(const char* packet, int len, uint32_t src_ip);
int udp_receive(char* buf, int* len, uint32_t* src_ip, uint16_t* src_port);