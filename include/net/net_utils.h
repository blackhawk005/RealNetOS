#pragma once
#include <stdint.h>

// Convert 16=bit value from host to network byte order
static inline uint16_t htons(uint16_t x){
    return (x << 8) | (x >> 8);
}

// Convert 16=bit value from host to network byte order
static inline uint16_t ntohs(uint16_t x){
    return htons(x);                        // Same Operation, since it's symmetric
}

// Convert 32-bit number from network byte order to host byte order (big-endian to little-endian)
static inline uint32_t ntohl(uint32_t netlong){
    return ((netlong & 0xFF) << 24) |
           ((netlong & 0xFF00) << 8) |
           ((netlong & 0xFF0000) >> 8) |
           ((netlong & 0xFF000000) >> 24);
}

// Host to network long — same operation in embedded little-endian environments
#define htonl ntohl
