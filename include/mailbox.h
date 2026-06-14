#pragma once
#include <stdint.h>

// Mailbox property interface (channel 8)
int mbox_call(uint8_t channel, volatile uint32_t* mbox);
