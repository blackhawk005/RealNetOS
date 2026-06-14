#include "../include/mailbox.h"

#define MMIO32(addr) (*(volatile uint32_t*)(uintptr_t)(addr))

// Raspberry Pi 4 mailbox base
#define MBOX_BASE 0xFE00B880UL

#define MBOX_READ     (MBOX_BASE + 0x00)
#define MBOX_STATUS   (MBOX_BASE + 0x18)
#define MBOX_WRITE    (MBOX_BASE + 0x20)

#define MBOX_FULL     0x80000000
#define MBOX_EMPTY    0x40000000

int mbox_call(uint8_t channel, volatile uint32_t* mbox) {
    uint32_t addr = (uint32_t)((uintptr_t)mbox);
    if (addr & 0xF) return 0; // must be 16-byte aligned
    uint32_t msg = (addr & ~0xF) | (channel & 0xF);

    while (MMIO32(MBOX_STATUS) & MBOX_FULL) { }
    MMIO32(MBOX_WRITE) = msg;

    while (1) {
        while (MMIO32(MBOX_STATUS) & MBOX_EMPTY) { }
        uint32_t resp = MMIO32(MBOX_READ);
        if (resp == msg) {
            return mbox[1] == 0x80000000;
        }
    }
    return 0;
}
