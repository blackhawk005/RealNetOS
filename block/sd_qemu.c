#include "../include/block/sd.h"
#include "../include/uart.h"
#include "../include/lib.h"

#define SD_SECTOR_SIZE 512
#define TOTAL_SECTORS  1024  // Simulated total sectors (512 KiB)
#define RESERVED_SECTORS 1
#define NUM_FATS 1
#define SECTORS_PER_FAT 8
#define SECTORS_PER_CLUSTER 1
#define ROOT_CLUSTER 2

static uint8_t fake_sd_card[TOTAL_SECTORS][SD_SECTOR_SIZE];

static void write_u16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static void write_u32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void fat32_format_fake_sd(void) {
    memset(fake_sd_card, 0, sizeof(fake_sd_card));

    // Boot sector (LBA 0)
    uint8_t* bs = fake_sd_card[0];
    bs[0] = 0xEB; bs[1] = 0x58; bs[2] = 0x90; // JMP
    memcpy(&bs[3], "REALNET ", 8);
    write_u16(&bs[11], SD_SECTOR_SIZE);
    bs[13] = SECTORS_PER_CLUSTER;
    write_u16(&bs[14], RESERVED_SECTORS);
    bs[16] = NUM_FATS;
    write_u16(&bs[17], 0); // Root entries (FAT32)
    write_u16(&bs[19], 0); // Total sectors 16
    bs[21] = 0xF8;         // Media
    write_u16(&bs[22], 0); // FAT size 16
    write_u16(&bs[24], 0); // Sectors/track
    write_u16(&bs[26], 0); // Num heads
    write_u32(&bs[28], 0); // Hidden sectors
    write_u32(&bs[32], TOTAL_SECTORS);
    write_u32(&bs[36], SECTORS_PER_FAT);
    write_u16(&bs[40], 0); // Ext flags
    write_u16(&bs[42], 0); // FS version
    write_u32(&bs[44], ROOT_CLUSTER);
    write_u16(&bs[48], 1); // FSInfo sector (unused)
    write_u16(&bs[50], 6); // Backup boot sector (unused)
    bs[510] = 0x55;
    bs[511] = 0xAA;

    // FAT table (starts at LBA 1)
    uint32_t fat_lba = RESERVED_SECTORS;
    uint8_t* fat0 = fake_sd_card[fat_lba];
    write_u32(&fat0[0], 0x0FFFFFF8); // FAT[0]
    write_u32(&fat0[4], 0x0FFFFFFF); // FAT[1]
    write_u32(&fat0[8], 0x0FFFFFFF); // FAT[2] root dir
    write_u32(&fat0[12], 0x0FFFFFFF); // FAT[3] test file

    // Root directory cluster (cluster 2)
    uint32_t cluster_heap_start = RESERVED_SECTORS + (NUM_FATS * SECTORS_PER_FAT);
    uint32_t root_lba = cluster_heap_start + (ROOT_CLUSTER - 2) * SECTORS_PER_CLUSTER;
    uint8_t* root = fake_sd_card[root_lba];

    // Create file: /TEST (no extension)
    const char* content = "hello from fat32\n";
    uint32_t content_len = 18;

    memset(root, 0, SD_SECTOR_SIZE);
    memcpy(&root[0], "TEST    ", 8);
    memcpy(&root[8], "   ", 3);
    root[11] = 0x20; // archive
    write_u16(&root[20], 0); // high cluster
    write_u16(&root[26], 3); // low cluster
    write_u32(&root[28], content_len);

    // Write file data to cluster 3
    uint32_t file_lba = cluster_heap_start + (3 - 2) * SECTORS_PER_CLUSTER;
    memcpy(fake_sd_card[file_lba], content, content_len);
}

int sd_init(void) {
    uart_puts("SD: Initialized (simulated)\n");
    fat32_format_fake_sd();
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

int sd_read_blocks(uint32_t lba, uint8_t* buffer, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (sd_read_block(lba + i, buffer + i * SD_SECTOR_SIZE) != 0) return -1;
    }
    return 0;
}

int sd_write_blocks(uint32_t lba, const uint8_t* buffer, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        if (sd_write_block(lba + i, buffer + i * SD_SECTOR_SIZE) != 0) return -1;
    }
    return 0;
}
