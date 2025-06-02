#pragma once
#include "vfs.h"
#include <stdint.h>

extern struct filesystem fat32_fs;

struct fat32_fs_info {
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t reserved_sector_count;
    uint32_t num_fats;
    uint32_t fat_size_sectors;
    uint32_t root_cluster;
    uint32_t fat_start_lba;
    uint32_t cluster_heap_start;
    uint32_t total_sectors;
    uint32_t first_data_sector;
    uint32_t sectors_per_fat;
};

int fat32_setup_mount(struct filesystem* fs, struct vnode* root);