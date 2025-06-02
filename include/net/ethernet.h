#pragma once
#include <stdint.h>


#define ETH_ALEN 6
#define ETH_HEADER_LEN 14

// Supported EtherTypes
#define ETH_P_IP 0x0800
#define ETH_P_ARP 0x0806

void ethernet_send(uint32_t dest_ip, uint16_t ethertype, const char* payload, int len);
void ethernet_handle(const char* frame, int len);