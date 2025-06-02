#include "../include/block/sd.h"
#include "../include/uart.h"
#include "../include/lib.h"

#define SD_SECTOR_SIZE 512
#define TOTAL_SECTORS  1024  // Simulated total sectors (512 KiB)
static uint8_t fake_sd_card[TOTAL_SECTORS][SD_SECTOR_SIZE];

int sd_init(void) {
    uart_puts("SD: Initialized (simulated)\n");
    memset(fake_sd_card, 0, sizeof(fake_sd_card));
    return 0;
}

int sd_read_block(uint32_t lba, uint8_t* buffer) {
    if (lba >= TOTAL_SECTORS) {
        uart_puts("SD: Read failed, LBA out of range\n");
        return -1;
    }

    memcpy(buffer, fake_sd_card[lba], SD_SECTOR_SIZE);
    return 0;
}

int sd_write_block(uint32_t lba, const uint8_t* buffer) {
    if (lba >= TOTAL_SECTORS) {
        uart_puts("SD: Write failed, LBA out of range\n");
        return -1;
    }

    memcpy(fake_sd_card[lba], buffer, SD_SECTOR_SIZE);
    return 0;
}