#include "../include/fs/fat32.h"
#include "../include/lib.h"
#include "../include/block/sd.h"
#include "../include/uart.h"

static struct fat32_fs_info fs_info;

struct filesystem fat32_fs = {
    .name = "fat32",
    .setup_mount = fat32_setup_mount
};

int fat32_setup_mount(struct filesystem* fs, struct vnode* root) {
    uint8_t sector[512];

    if (sd_read_block(0, sector) != 0) {
        uart_puts("FAT32: Failed to read boot sector\n");
        return -1;
    }

    fs_info.bytes_per_sector = sector[11] | (sector[12] << 8);
    fs_info.sectors_per_cluster = sector[13];
    fs_info.reserved_sector_count = sector[14] | (sector[15] << 8);
    fs_info.num_fats = sector[16];
    fs_info.sectors_per_fat = sector[36] | (sector[37] << 8) | (sector[38] << 16) | (sector[39] << 24);
    fs_info.root_cluster = sector[44] | (sector[45] << 8) | (sector[46] << 16) | (sector[47] << 24);

    fs_info.fat_start_lba = fs_info.reserved_sector_count;
    fs_info.cluster_heap_start = fs_info.fat_start_lba + (fs_info.num_fats * fs_info.sectors_per_fat);

    uart_puts("FAT32: Mount successful\n");
    uart_puts(" - Bytes/sector: "); uart_puti(fs_info.bytes_per_sector); uart_puts("\n");
    uart_puts(" - Sectors/FAT: "); uart_puti(fs_info.sectors_per_fat); uart_puts("\n");

    root->fs = fs;
    root->fops = NULL;
    root->internal = NULL;

    return 0;
}