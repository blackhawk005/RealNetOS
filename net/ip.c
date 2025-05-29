#include "../include/net/ip.h"
#include "../include/uart.h"
#include "../include/lib.h"
#include "../include/net/udp.h"
#include "../include/net/net_utils.h"
#include "../include/net/ethernet.h"
#include "../include/net/tcp.h"
#include <stddef.h>

static uint32_t local_ip = 0xC0A80002;      // 192.168.0.2

uint16_t ip_checksum(const void* vdata, size_t length){
    const uint8_t* data = vdata;
    uint32_t acc = 0;

    for (size_t i = 0; i + 1 < length; i += 2){
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += ntohs(word);
    }

    if (length & 1){
        uint16_t word = data[length - 1] << 8;
        acc += ntohs(word);
    }

    while (acc >> 16){
        acc = (acc & 0xFFF) + (acc >> 16);
    }
    return ~acc;
}

void ip_recv(const char* packet, int len){
    if (len < IP_HEADER_LEN){
        return;
    }

    const ipv4_header_t* hdr = (const ipv4_header_t*)packet;
    int ihl = (hdr->version_ihl & 0x0F) * 4;
    if (ihl < IP_HEADER_LEN || len < ntohs(hdr->total_len)){
        return;
    }

    if (hdr->version_ihl >> 4 != IP_VERSION){
        return;
    }

    if (ip_checksum(hdr, ihl) != 0){
        return;
    }

    const char* payload = packet + ihl;
    int payload_len = ntohs(hdr->total_len) - ihl;

    if (hdr->protocol == IP_PROTO_UDP){
        udp_recv(payload, payload_len, ntohl(hdr->src_ip));
    }
    else if (hdr->protocol == 6) { // TCP
        tcp_recv(payload, payload_len, ntohl(hdr->src_ip));
    }
}

void ip_send(uint32_t dest_ip, const char* data, int len, uint8_t protocol){
    char buffer[1500];
    ipv4_header_t* hdr = (ipv4_header_t*)buffer;

    hdr->version_ihl = (IP_VERSION << 4) | (IP_HEADER_LEN / 4);
    hdr->tos = 0;
    hdr->total_len = htons(IP_HEADER_LEN + len);
    hdr->id = htons(0x1234);         // static for now
    hdr->flags_frag = htons(0x4000);
    hdr->ttl = 64;
    hdr->protocol = protocol;
    hdr->src_ip = htonl(local_ip);
    hdr->dest_ip = htonl(dest_ip);
    hdr->checksum = 0;
    hdr->checksum = ip_checksum(hdr, IP_HEADER_LEN);

    memcpy(buffer + IP_HEADER_LEN, data, len);
    ethernet_send(dest_ip, 0x0800, buffer, IP_HEADER_LEN + len);
}
