#pragma once
#include <stdint.h>


#define ETH_ALEN 6
#define ETH_HEADER_LEN 14

void ethernet_send(uint32_t dest_ip, uint16_t ethertype, const char* payload, int len);