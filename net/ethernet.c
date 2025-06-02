#include "../include/net/ethernet.h"
#include "../include/genet.h"
#include "../include/lib.h"
#include "../include/net/ip.h"
#include "../include/uart.h"

// Dummy MACs
static const uint8_t local_mac[ETH_ALEN]  = { 0xB8, 0x27, 0xEB, 0x12, 0x34, 0x56 };
static const uint8_t dest_mac[ETH_ALEN]   = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };  // Broadcast

void ethernet_send(uint32_t dest_ip, uint16_t ethertype, const char* payload, int len) {
    char frame[ETH_HEADER_LEN + ETH_FRAME_SIZE];
    
    // Fill Ethernet Header
    memcpy(frame + 0, dest_mac, ETH_ALEN);
    memcpy(frame + 6, local_mac, ETH_ALEN);

    frame[12] = (ethertype >> 8) & 0xFF;
    frame[13] = ethertype & 0xFF;

    // Copy payload
    memcpy(frame + ETH_HEADER_LEN, payload, len);

    // Send via GENET
    genet_send(frame, ETH_HEADER_LEN + len);
}

void ethernet_handle(const char* frame, int len){
    if (len < ETH_HEADER_LEN){
        uart_puts("Ethernet: Frame too short\n");
        return;
    }

    const uint8_t* dest_mac = (const uint8_t*)(frame + 0);
    const uint8_t* src_mac = (const uint8_t*)(frame + 6);
    uint16_t ethertype = (frame[12] << 8) | frame[13]; 

    const char* payload = frame + ETH_HEADER_LEN;
    int payload_len = len - ETH_HEADER_LEN;

    switch (ethertype)
    {
    case ETH_P_IP:
        uart_puts("Ethernet: IP packet received\n");
        ip_recv(payload, payload_len);
        break;
    case ETH_P_ARP:
        uart_puts("Ethernet: ARP packet received (not handled)\n");
        break;
    default:
        uart_puts("Ethernet: Unknown Ethertype\n");
        break;
    }
}