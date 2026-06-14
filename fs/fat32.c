#include "../include/fs/fat32.h"
#include "../include/lib.h"
#include "../include/block/sd.h"
#include "../include/uart.h"

static struct fat32_fs_info fs_info;

struct fat32_node {
    uint32_t first_cluster;
    uint32_t size;
    uint32_t dirent_lba;
    uint16_t dirent_offset;
    uint8_t is_dir;
    uint32_t parent_cluster;
};

static struct vnode_ops fat_dir_ops;

static int name_to_83(const char* name, char out[11]) {
    for (int i = 0; i < 11; i++) out[i] = ' ';
    int base_len = 0;
    int ext_len = 0;
    int seen_dot = 0;

    for (size_t i = 0; name[i]; i++) {
        char c = name[i];
        if (c == '.') {
            if (seen_dot) return -1;
            seen_dot = 1;
            continue;
        }
        if (c == '/' || c == '\\') return -1;
        if (c >= 'a' && c <= 'z') c = (char)(c - 32);
        if (!seen_dot) {
            if (base_len >= 8) return -1;
            out[base_len++] = c;
        } else {
            if (ext_len >= 3) return -1;
            out[8 + ext_len++] = c;
        }
    }

    if (base_len == 0) return -1;
    return 0;
}

static void name83_to_string(const uint8_t* name83, char* out, size_t out_len) {
    int pos = 0;
    int i = 0;
    while (i < 8 && name83[i] != ' ') {
        if (pos + 1 < (int)out_len) out[pos++] = (char)name83[i];
        i++;
    }
    int has_ext = 0;
    for (int j = 8; j < 11; j++) {
        if (name83[j] != ' ') {
            has_ext = 1;
            break;
        }
    }
    if (has_ext && pos + 1 < (int)out_len) out[pos++] = '.';
    for (int j = 8; j < 11; j++) {
        if (name83[j] == ' ') break;
        if (pos + 1 < (int)out_len) out[pos++] = (char)name83[j];
    }
    if (out_len > 0) out[pos < (int)out_len ? pos : (int)out_len - 1] = '\0';
}

static uint32_t cluster_to_lba(uint32_t cluster) {
    return fs_info.cluster_heap_start + (cluster - 2) * fs_info.sectors_per_cluster;
}

static uint32_t fat_read_entry(uint32_t cluster) {
    uint8_t sector[512];
    uint32_t offset = cluster * 4;
    uint32_t lba = fs_info.fat_start_lba + (offset / 512);
    uint32_t off = offset % 512;

    if (sd_read_block(lba, sector) != 0) return 0;
    uint32_t v = sector[off] | (sector[off + 1] << 8) | (sector[off + 2] << 16) | (sector[off + 3] << 24);
    return v & 0x0FFFFFFF;
}

static int fat_write_entry(uint32_t cluster, uint32_t value) {
    uint8_t sector[512];
    uint32_t offset = cluster * 4;
    uint32_t lba = fs_info.fat_start_lba + (offset / 512);
    uint32_t off = offset % 512;

    if (sd_read_block(lba, sector) != 0) return -1;
    sector[off] = (uint8_t)(value & 0xFF);
    sector[off + 1] = (uint8_t)((value >> 8) & 0xFF);
    sector[off + 2] = (uint8_t)((value >> 16) & 0xFF);
    sector[off + 3] = (uint8_t)((value >> 24) & 0xFF);
    return sd_write_block(lba, sector);
}

static uint32_t fat_alloc_cluster(void) {
    uint32_t fat_entries = (fs_info.sectors_per_fat * fs_info.bytes_per_sector) / 4;
    uint8_t zero[512];
    memset(zero, 0, sizeof(zero));

    for (uint32_t c = 2; c < fat_entries; c++) {
        if (fat_read_entry(c) == 0) {
            if (fat_write_entry(c, 0x0FFFFFFF) != 0) return 0;
            uint32_t lba = cluster_to_lba(c);
            for (uint32_t s = 0; s < fs_info.sectors_per_cluster; s++) {
                sd_write_block(lba + s, zero);
            }
            return c;
        }
    }
    return 0;
}

static uint32_t fat_get_cluster_by_index(uint32_t first, uint32_t index, int create) {
    uint32_t c = first;
    if (c == 0) return 0;

    for (uint32_t i = 0; i < index; i++) {
        uint32_t next = fat_read_entry(c);
        if (next >= 0x0FFFFFF8) {
            if (!create) return 0;
            uint32_t n = fat_alloc_cluster();
            if (n == 0) return 0;
            if (fat_write_entry(c, n) != 0) return 0;
            c = n;
        } else {
            c = next;
        }
    }
    return c;
}

static int fat_update_dirent_size(struct fat32_node* node, uint32_t size) {
    uint8_t sector[512];
    if (sd_read_block(node->dirent_lba, sector) != 0) return -1;
    uint32_t off = node->dirent_offset;
    sector[off + 28] = (uint8_t)(size & 0xFF);
    sector[off + 29] = (uint8_t)((size >> 8) & 0xFF);
    sector[off + 30] = (uint8_t)((size >> 16) & 0xFF);
    sector[off + 31] = (uint8_t)((size >> 24) & 0xFF);
    return sd_write_block(node->dirent_lba, sector);
}

static uint32_t fat_dir_start_cluster(struct vnode* dir_node) {
    if (dir_node == NULL || dir_node->internal == NULL) {
        return fs_info.root_cluster;
    }
    struct fat32_node* info = (struct fat32_node*)dir_node->internal;
    return info->first_cluster;
}

static int fat_dir_find(uint32_t start_cluster, const char name83[11], struct fat32_node* out) {
    uint8_t sector[512];
    uint32_t cluster = start_cluster;
    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        uint32_t base_lba = cluster_to_lba(cluster);
        for (uint32_t s = 0; s < fs_info.sectors_per_cluster; s++) {
            if (sd_read_block(base_lba + s, sector) != 0) return -1;
            for (uint32_t i = 0; i < 512; i += 32) {
                uint8_t first = sector[i];
                if (first == 0x00) return -1;
                if (first == 0xE5) continue;
                if (sector[i + 11] == 0x0F) continue;
                if (sector[i + 11] & 0x08) continue;
                int match = 1;
                for (int j = 0; j < 11; j++) {
                    if (sector[i + j] != (uint8_t)name83[j]) {
                        match = 0;
                        break;
                    }
                }
                if (match) {
                    uint32_t hi = sector[i + 20] | (sector[i + 21] << 8);
                    uint32_t lo = sector[i + 26] | (sector[i + 27] << 8);
                    out->first_cluster = (hi << 16) | lo;
                    out->size = sector[i + 28] | (sector[i + 29] << 8) | (sector[i + 30] << 16) | (sector[i + 31] << 24);
                    out->dirent_lba = base_lba + s;
                    out->dirent_offset = (uint16_t)i;
                    out->is_dir = (sector[i + 11] & 0x10) ? 1 : 0;
                    out->parent_cluster = start_cluster;
                    return 0;
                }
            }
        }
        cluster = fat_read_entry(cluster);
    }
    return -1;
}

static int fat_dir_find_free(uint32_t start_cluster, uint32_t* out_lba, uint16_t* out_off, uint32_t* out_dir_cluster) {
    uint8_t sector[512];
    uint32_t cluster = start_cluster;
    uint32_t last_cluster = 0;

    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        last_cluster = cluster;
        uint32_t base_lba = cluster_to_lba(cluster);
        for (uint32_t s = 0; s < fs_info.sectors_per_cluster; s++) {
            if (sd_read_block(base_lba + s, sector) != 0) return -1;
            for (uint32_t i = 0; i < 512; i += 32) {
                uint8_t first = sector[i];
                if (first == 0x00 || first == 0xE5) {
                    *out_lba = base_lba + s;
                    *out_off = (uint16_t)i;
                    *out_dir_cluster = cluster;
                    return 0;
                }
            }
        }
        cluster = fat_read_entry(cluster);
    }
    if (last_cluster == 0) return -1;

    uint32_t new_cluster = fat_alloc_cluster();
    if (new_cluster == 0) return -1;
    if (fat_write_entry(last_cluster, new_cluster) != 0) return -1;
    uint32_t base_lba = cluster_to_lba(new_cluster);
    if (sd_read_block(base_lba, sector) != 0) return -1;
    *out_lba = base_lba;
    *out_off = 0;
    *out_dir_cluster = new_cluster;
    return 0;
}

static int fat_dir_is_empty(uint32_t start_cluster) {
    uint8_t sector[512];
    uint32_t cluster = start_cluster;
    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        uint32_t base_lba = cluster_to_lba(cluster);
        for (uint32_t s = 0; s < fs_info.sectors_per_cluster; s++) {
            if (sd_read_block(base_lba + s, sector) != 0) return 0;
            for (uint32_t i = 0; i < 512; i += 32) {
                uint8_t first = sector[i];
                if (first == 0x00) return 1;
                if (first == 0xE5) continue;
                if (sector[i + 11] == 0x0F) continue;
                if (sector[i + 11] & 0x08) continue;
                if (sector[i] == '.' && (sector[i + 1] == ' ' || sector[i + 1] == '.')) continue;
                return 0;
            }
        }
        cluster = fat_read_entry(cluster);
    }
    return 1;
}

static void fat_free_chain(uint32_t start_cluster) {
    uint32_t c = start_cluster;
    while (c >= 2 && c < 0x0FFFFFF8) {
        uint32_t next = fat_read_entry(c);
        fat_write_entry(c, 0);
        c = next;
    }
}

static int fat32_read(file_t* file, void* buf, size_t len) {
    struct fat32_node* node = (struct fat32_node*)file->vnode->internal;
    if (node == NULL || node->is_dir) return -1;
    if (file->f_pos >= node->size) return 0;

    size_t remaining = len;
    size_t available = node->size - file->f_pos;
    if (remaining > available) remaining = available;

    uint32_t cluster_size = fs_info.bytes_per_sector * fs_info.sectors_per_cluster;
    uint8_t sector[512];
    size_t copied = 0;

    while (remaining > 0) {
        uint32_t cluster_index = (uint32_t)(file->f_pos / cluster_size);
        uint32_t cluster_offset = (uint32_t)(file->f_pos % cluster_size);
        uint32_t cluster = fat_get_cluster_by_index(node->first_cluster, cluster_index, 0);
        if (cluster == 0) break;

        uint32_t lba = cluster_to_lba(cluster) + (cluster_offset / fs_info.bytes_per_sector);
        uint32_t off = cluster_offset % fs_info.bytes_per_sector;
        if (sd_read_block(lba, sector) != 0) break;

        size_t take = fs_info.bytes_per_sector - off;
        if (take > remaining) take = remaining;
        memcpy((uint8_t*)buf + copied, sector + off, take);

        file->f_pos += take;
        copied += take;
        remaining -= take;
    }

    return (int)copied;
}

static int fat32_write(file_t* file, const void* buf, size_t len) {
    struct fat32_node* node = (struct fat32_node*)file->vnode->internal;
    if (node == NULL || node->is_dir) return -1;

    uint32_t cluster_size = fs_info.bytes_per_sector * fs_info.sectors_per_cluster;
    size_t remaining = len;
    size_t written = 0;
    uint8_t sector[512];

    while (remaining > 0) {
        uint32_t cluster_index = (uint32_t)(file->f_pos / cluster_size);
        uint32_t cluster_offset = (uint32_t)(file->f_pos % cluster_size);
        uint32_t cluster = fat_get_cluster_by_index(node->first_cluster, cluster_index, 1);
        if (cluster == 0) break;

        uint32_t lba = cluster_to_lba(cluster) + (cluster_offset / fs_info.bytes_per_sector);
        uint32_t off = cluster_offset % fs_info.bytes_per_sector;
        if (sd_read_block(lba, sector) != 0) break;

        size_t take = fs_info.bytes_per_sector - off;
        if (take > remaining) take = remaining;
        memcpy(sector + off, (const uint8_t*)buf + written, take);
        if (sd_write_block(lba, sector) != 0) break;

        file->f_pos += take;
        written += take;
        remaining -= take;
    }

    if (file->f_pos > node->size) {
        node->size = (uint32_t)file->f_pos;
        fat_update_dirent_size(node, node->size);
    }

    return (int)written;
}

static int fat32_close(file_t* file) {
    (void)file;
    return 0;
}

static int fat32_lookup(struct vnode* dir_node, const char* name, struct vnode** target) {
    if (strcmp(name, ".") == 0) {
        *target = dir_node;
        return 0;
    }
    if (strcmp(name, "..") == 0) {
        uint32_t parent_cluster = fs_info.root_cluster;
        if (dir_node && dir_node->internal) {
            struct fat32_node* dn = (struct fat32_node*)dir_node->internal;
            parent_cluster = dn->parent_cluster ? dn->parent_cluster : fs_info.root_cluster;
        }
        vnode_t* node = (vnode_t*)kmalloc(sizeof(vnode_t));
        struct fat32_node* fnode = (struct fat32_node*)kmalloc(sizeof(struct fat32_node));
        fnode->first_cluster = parent_cluster;
        fnode->size = 0;
        fnode->dirent_lba = 0;
        fnode->dirent_offset = 0;
        fnode->is_dir = 1;
        fnode->parent_cluster = fs_info.root_cluster;
        node->fs = dir_node->fs;
        node->fops = NULL;
        node->vops = &fat_dir_ops;
        node->internal = fnode;
        *target = node;
        return 0;
    }

    char name83[11];
    if (name_to_83(name, name83) != 0) return -1;

    struct fat32_node info;
    uint32_t start_cluster = fat_dir_start_cluster(dir_node);
    if (fat_dir_find(start_cluster, name83, &info) != 0) return -1;

    vnode_t* node = (vnode_t*)kmalloc(sizeof(vnode_t));
    struct fat32_node* fnode = (struct fat32_node*)kmalloc(sizeof(struct fat32_node));
    memcpy(fnode, &info, sizeof(struct fat32_node));

    static struct file_ops fat_file_ops = {
        .read = fat32_read,
        .write = fat32_write,
        .close = fat32_close,
    };

    node->fs = dir_node->fs;
    node->internal = fnode;
    if (fnode->is_dir) {
        node->fops = NULL;
        node->vops = &fat_dir_ops;
    } else {
        node->fops = &fat_file_ops;
        node->vops = NULL;
    }
    *target = node;
    return 0;
}

static int fat32_create(struct vnode* dir_node, const char* name, struct vnode** target) {
    if (name == NULL || name[0] == '\0') return -1;
    char name83[11];
    if (name_to_83(name, name83) != 0) return -1;

    uint32_t start_cluster = fat_dir_start_cluster(dir_node);
    struct fat32_node exists;
    if (fat_dir_find(start_cluster, name83, &exists) == 0) return -1;

    uint32_t entry_lba = 0;
    uint16_t entry_off = 0;
    uint32_t dir_cluster = 0;
    if (fat_dir_find_free(start_cluster, &entry_lba, &entry_off, &dir_cluster) != 0) return -1;
    (void)dir_cluster;

    uint32_t new_cluster = fat_alloc_cluster();
    if (new_cluster == 0) return -1;

    uint8_t sector[512];
    if (sd_read_block(entry_lba, sector) != 0) return -1;
    memcpy(&sector[entry_off], name83, 11);
    sector[entry_off + 11] = 0x20; // archive
    memset(&sector[entry_off + 12], 0, 20);
    sector[entry_off + 20] = (uint8_t)((new_cluster >> 16) & 0xFF);
    sector[entry_off + 21] = (uint8_t)((new_cluster >> 24) & 0xFF);
    sector[entry_off + 26] = (uint8_t)(new_cluster & 0xFF);
    sector[entry_off + 27] = (uint8_t)((new_cluster >> 8) & 0xFF);
    sector[entry_off + 28] = 0;
    sector[entry_off + 29] = 0;
    sector[entry_off + 30] = 0;
    sector[entry_off + 31] = 0;
    if (sd_write_block(entry_lba, sector) != 0) return -1;

    vnode_t* node = (vnode_t*)kmalloc(sizeof(vnode_t));
    struct fat32_node* fnode = (struct fat32_node*)kmalloc(sizeof(struct fat32_node));
    fnode->first_cluster = new_cluster;
    fnode->size = 0;
    fnode->dirent_lba = entry_lba;
    fnode->dirent_offset = entry_off;
    fnode->is_dir = 0;
    fnode->parent_cluster = start_cluster;

    static struct file_ops fat_file_ops = {
        .read = fat32_read,
        .write = fat32_write,
        .close = fat32_close,
    };

    node->fs = dir_node->fs;
    node->fops = &fat_file_ops;
    node->vops = NULL;
    node->internal = fnode;
    *target = node;
    return 0;
}

static int fat32_readdir(struct vnode* dir_node, int (*cb)(const char* name, int is_dir, void* ctx), void* ctx) {
    uint8_t sector[512];
    uint32_t cluster = fat_dir_start_cluster(dir_node);

    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        uint32_t base_lba = cluster_to_lba(cluster);
        for (uint32_t s = 0; s < fs_info.sectors_per_cluster; s++) {
            if (sd_read_block(base_lba + s, sector) != 0) return -1;
            for (uint32_t i = 0; i < 512; i += 32) {
                uint8_t first = sector[i];
                if (first == 0x00) return 0;
                if (first == 0xE5) continue;
                uint8_t attr = sector[i + 11];
                if (attr == 0x0F) continue;
                if (attr & 0x08) continue;
                char name[16];
                name83_to_string(&sector[i], name, sizeof(name));
                if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
                int is_dir = (attr & 0x10) ? 1 : 0;
                if (cb(name, is_dir, ctx) != 0) return 0;
            }
        }
        cluster = fat_read_entry(cluster);
    }
    return 0;
}

static int fat32_mkdir(struct vnode* dir_node, const char* name, struct vnode** target) {
    if (name == NULL || name[0] == '\0') return -1;
    char name83[11];
    if (name_to_83(name, name83) != 0) return -1;

    uint32_t start_cluster = fat_dir_start_cluster(dir_node);
    struct fat32_node exists;
    if (fat_dir_find(start_cluster, name83, &exists) == 0) return -1;

    uint32_t entry_lba = 0;
    uint16_t entry_off = 0;
    uint32_t dir_cluster = 0;
    if (fat_dir_find_free(start_cluster, &entry_lba, &entry_off, &dir_cluster) != 0) return -1;
    (void)dir_cluster;

    uint32_t new_cluster = fat_alloc_cluster();
    if (new_cluster == 0) return -1;

    uint8_t sector[512];
    if (sd_read_block(entry_lba, sector) != 0) return -1;
    memcpy(&sector[entry_off], name83, 11);
    sector[entry_off + 11] = 0x10; // directory
    memset(&sector[entry_off + 12], 0, 20);
    sector[entry_off + 20] = (uint8_t)((new_cluster >> 16) & 0xFF);
    sector[entry_off + 21] = (uint8_t)((new_cluster >> 24) & 0xFF);
    sector[entry_off + 26] = (uint8_t)(new_cluster & 0xFF);
    sector[entry_off + 27] = (uint8_t)((new_cluster >> 8) & 0xFF);
    sector[entry_off + 28] = 0;
    sector[entry_off + 29] = 0;
    sector[entry_off + 30] = 0;
    sector[entry_off + 31] = 0;
    if (sd_write_block(entry_lba, sector) != 0) return -1;

    // Initialize directory with "." and ".."
    uint32_t dir_lba = cluster_to_lba(new_cluster);
    uint8_t dirsec[512];
    memset(dirsec, 0, sizeof(dirsec));
    memcpy(&dirsec[0], ".          ", 11);
    dirsec[11] = 0x10;
    dirsec[20] = (uint8_t)((new_cluster >> 16) & 0xFF);
    dirsec[21] = (uint8_t)((new_cluster >> 24) & 0xFF);
    dirsec[26] = (uint8_t)(new_cluster & 0xFF);
    dirsec[27] = (uint8_t)((new_cluster >> 8) & 0xFF);

    memcpy(&dirsec[32], "..         ", 11);
    dirsec[32 + 11] = 0x10;
    uint32_t parent_cluster = start_cluster;
    dirsec[32 + 20] = (uint8_t)((parent_cluster >> 16) & 0xFF);
    dirsec[32 + 21] = (uint8_t)((parent_cluster >> 24) & 0xFF);
    dirsec[32 + 26] = (uint8_t)(parent_cluster & 0xFF);
    dirsec[32 + 27] = (uint8_t)((parent_cluster >> 8) & 0xFF);

    if (sd_write_block(dir_lba, dirsec) != 0) return -1;

    vnode_t* node = (vnode_t*)kmalloc(sizeof(vnode_t));
    struct fat32_node* fnode = (struct fat32_node*)kmalloc(sizeof(struct fat32_node));
    fnode->first_cluster = new_cluster;
    fnode->size = 0;
    fnode->dirent_lba = entry_lba;
    fnode->dirent_offset = entry_off;
    fnode->is_dir = 1;
    fnode->parent_cluster = start_cluster;

    node->fs = dir_node->fs;
    node->fops = NULL;
    node->vops = &fat_dir_ops;
    node->internal = fnode;
    if (target) *target = node;
    return 0;
}

static int fat32_unlink(struct vnode* dir_node, const char* name) {
    if (name == NULL || name[0] == '\0') return -1;
    char name83[11];
    if (name_to_83(name, name83) != 0) return -1;

    uint32_t start_cluster = fat_dir_start_cluster(dir_node);
    struct fat32_node info;
    if (fat_dir_find(start_cluster, name83, &info) != 0) return -1;

    if (info.is_dir) {
        if (!fat_dir_is_empty(info.first_cluster)) return -1;
    }

    uint8_t sector[512];
    if (sd_read_block(info.dirent_lba, sector) != 0) return -1;
    sector[info.dirent_offset] = 0xE5;
    if (sd_write_block(info.dirent_lba, sector) != 0) return -1;

    if (info.first_cluster >= 2) {
        fat_free_chain(info.first_cluster);
    }
    return 0;
}

static struct vnode_ops fat_dir_ops = {
    .lookup = fat32_lookup,
    .create = fat32_create,
    .readdir = fat32_readdir,
    .mkdir = fat32_mkdir,
    .unlink = fat32_unlink,
};

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

    // Detect whether LBA0 is a FAT32 boot sector or an MBR.
    int is_boot = 0;
    if (sector[510] == 0x55 && sector[511] == 0xAA) {
        uint32_t bps = sector[11] | (sector[12] << 8);
        uint32_t spc = sector[13];
        uint32_t fatsz = sector[36] | (sector[37] << 8) | (sector[38] << 16) | (sector[39] << 24);
        uint32_t rootc = sector[44] | (sector[45] << 8) | (sector[46] << 16) | (sector[47] << 24);
        if ((bps == 512 || bps == 1024 || bps == 2048 || bps == 4096) &&
            spc != 0 && (spc & (spc - 1)) == 0 &&
            fatsz != 0 && rootc >= 2) {
            is_boot = 1;
        }
    }

    uint32_t boot_lba = 0;
    if (!is_boot) {
        // Parse MBR partition table for the first FAT32 partition.
        if (sector[510] != 0x55 || sector[511] != 0xAA) {
            uart_puts("FAT32: Invalid MBR signature\n");
            return -1;
        }
        uint32_t fat_lba = 0;
        for (int i = 0; i < 4; i++) {
            uint32_t off = 446 + (i * 16);
            uint8_t ptype = sector[off + 4];
            if (ptype == 0x0B || ptype == 0x0C) {
                fat_lba = sector[off + 8] |
                          (sector[off + 9] << 8) |
                          (sector[off + 10] << 16) |
                          (sector[off + 11] << 24);
                break;
            }
        }
        if (fat_lba == 0) {
            uart_puts("FAT32: No FAT32 partition found in MBR\n");
            return -1;
        }
        boot_lba = fat_lba;
        if (sd_read_block(boot_lba, sector) != 0) {
            uart_puts("FAT32: Failed to read partition boot sector\n");
            return -1;
        }
    }

    fs_info.partition_lba = boot_lba;
    fs_info.bytes_per_sector = sector[11] | (sector[12] << 8);
    fs_info.sectors_per_cluster = sector[13];
    fs_info.reserved_sector_count = sector[14] | (sector[15] << 8);
    fs_info.num_fats = sector[16];
    fs_info.sectors_per_fat = sector[36] | (sector[37] << 8) | (sector[38] << 16) | (sector[39] << 24);
    fs_info.root_cluster = sector[44] | (sector[45] << 8) | (sector[46] << 16) | (sector[47] << 24);

    fs_info.fat_start_lba = fs_info.partition_lba + fs_info.reserved_sector_count;
    fs_info.cluster_heap_start = fs_info.fat_start_lba + (fs_info.num_fats * fs_info.sectors_per_fat);

    uart_puts("FAT32: Mount successful\n");
    uart_puts(" - Partition LBA: "); uart_puti(fs_info.partition_lba); uart_puts("\n");
    uart_puts(" - Bytes/sector: "); uart_puti(fs_info.bytes_per_sector); uart_puts("\n");
    uart_puts(" - Sectors/FAT: "); uart_puti(fs_info.sectors_per_fat); uart_puts("\n");

    root->fs = fs;
    root->fops = NULL;
    root->vops = &fat_dir_ops;
    root->internal = NULL;

    return 0;
}
