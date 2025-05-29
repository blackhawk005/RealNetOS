#pragma once
#include <stdint.h>
#include <stddef.h>

#ifndef IP_H
#define IP_H

#define IP_VERSION 4
#define IP_HEADER_LEN 20
#define IP_PROTO_UDP 17

typedef struct{
    uint8_t version_ihl;        // Version (4 bits) + Internet header length (4 bits)
    uint8_t tos;            // Type of service
    uint16_t total_len;     // Total Length
    uint16_t id;            // Identification
    uint16_t flags_frag;    // Flags (3 bits) + Fragment offset (13 bits)
    uint16_t ttl;           // Time to live
    uint8_t protocol;       // Protocol
    uint16_t checksum;      // Header checksum
    uint32_t src_ip;      // Source Address
    uint32_t dest_ip;     // Destination address
} __attribute__((packed)) ipv4_header_t;

// Called by ethernet layer on receiving an IPv4 packet
void ip_recv(const char* packet, int len);

// Send an IPv4 Packet
void ip_send(uint32_t dst_ip, const char* payload, int len, uint8_t proto);

// Compute checksum for given data buffer
uint16_t ip_checksum(const void* vdata, size_t length);

#endif