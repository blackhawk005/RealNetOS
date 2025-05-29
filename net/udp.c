#include "../include/net/udp.h"
#include "../include/net/ip.h"
#include "../include/net/net_utils.h"
#include "../include/lib.h"

static char udp_rx_buf[UDP_MAX_DATA_SIZE];
static int udp_rx_len = 0;
static uint32_t udp_rx_src_ip = 0;
static uint16_t udp_rx_src_port = 0;

void udp_send(const char* data, int len, uint32_t dest_ip, uint16_t dest_port){
    uint8_t packet[UDP_HEADER_SIZE + UDP_MAX_DATA_SIZE];
    udp_header_t* hdr = (udp_header_t*)packet;

    hdr->src_port = htons(LOCAL_PORT);
    hdr->dest_port = htons(dest_port);
    hdr->length = htons(UDP_HEADER_SIZE + len);
    hdr->checksum = 0;      // Optional for IPv4, can skip

    memcpy(packet + UDP_HEADER_SIZE, data, len);

    ip_send(dest_ip, (const char*)packet, UDP_HEADER_SIZE + len, IP_PROTO_UDP);
}

void udp_recv(const char* packet, int len, uint32_t src_ip){
    if (len < UDP_HEADER_SIZE){
        return;
    }

    udp_header_t* hdr = (udp_header_t*)packet;
    int data_len = ntohs(hdr->length) - UDP_HEADER_SIZE;
    if (data_len <= 0 || data_len > UDP_MAX_DATA_SIZE){
        return;
    }

    memcpy(udp_rx_buf, packet + UDP_HEADER_SIZE, data_len);
    udp_rx_len = data_len;
    udp_rx_src_ip = src_ip;
    udp_rx_src_port = ntohl(hdr->src_port);
}

// This function copies the received packet to user buffer
int udp_receive(char* buf, int* len, uint32_t* src_ip, uint16_t* src_port) {
    if (udp_rx_len == 0) return 0;

    memcpy(buf, udp_rx_buf, udp_rx_len);
    *len = udp_rx_len;
    *src_ip = udp_rx_src_ip;
    *src_port = udp_rx_src_port;

    udp_rx_len = 0; // Clear buffer after delivery
    return 1;
}

// udp_recv() is called by the IP layer (ip_recv() in ip.c) when a UDP packet arrives.

// udp_receive() is a polling interface that allows higher layers (or threads) to fetch the latest received packet.

// You’ve correctly used LOCAL_PORT, htons, and buffer protections.

// Your GENET RX loop (in genet_handle_rx) should eventually decode Ethernet/IP/UDP payloads, which you're set up to do via ip_recv().