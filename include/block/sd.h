#pragma once
#include <stdint.h>

int sd_init(void);
int sd_read_block(uint32_t lba, uint8_t* buffer);
int sd_write_block(uint32_t lba, const uint8_t* buffer);