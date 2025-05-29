#include "../include/net/ethernet.h"
#include "../include/genet.h"
#include "../include/lib.h"

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