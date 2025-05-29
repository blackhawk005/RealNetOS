#include "../include/net/packet_queue.h"
#include "../include/lib.h"

void packet_queue_init(packet_queue_t* q){
    q->head = q->tail = q->count = 0;
}

int packet_queue_push(packet_queue_t* q, const char* data, int len, int src_ip){
    if (q->count == PACKET_QUEUE_SIZE){
        return -1;
    }

    int index = q->tail;
    memcpy(q->buffer[index], data, len);
    q->lengths[index] = len;
    q->src_ips[index] = src_ip;

    q->tail = (q->tail + 1) % PACKET_QUEUE_SIZE;
    q->count++;
    return 0;
}

int packet_queue_pop(packet_queue_t* q, char* out, int* len, uint32_t* src_ip){
    if (q->count == 0){
        return -1;
    }

    int index = q->head;
    memcpy(out, q->buffer[index], q->lengths[index]);
    *len = q->lengths[index];
    *src_ip = q->src_ips[index];

    q->head = (q->head + 1) % PACKET_QUEUE_SIZE;
    q->count--;
    return 0;
}