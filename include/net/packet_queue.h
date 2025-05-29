#pragma once
#include <stdint.h>
#include <stddef.h>

#define PACKET_QUEUE_SIZE 64    
#define MAX_PACKET_SIZE 1535

typedef struct {
    char buffer[PACKET_QUEUE_SIZE][MAX_PACKET_SIZE];
    int lengths[PACKET_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    int src_ips[PACKET_QUEUE_SIZE];
} packet_queue_t;

void packet_queue_init(packet_queue_t* q);
int packet_queue_push(packet_queue_t* q, const char* data, int len, int src_ip);
int packet_queue_pop(packet_queue_t* q, char* out, int* len, uint32_t* src_ip);