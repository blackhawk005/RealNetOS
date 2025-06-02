#include "../include/fs/vfs.h"
#include "../include/lib.h"

static struct filesystem* fs_list[4];  // Support up to 4 FS types
static int fs_count = 0;

int register_filesystem(struct filesystem* fs) {
    if (fs_count >= 4) return -1;
    fs_list[fs_count++] = fs;
    return 0;
}

int vfs_open(const char* pathname, file_t** target) {
    // TODO: Path parsing & FS lookup
    return -1;
}

int vfs_close(file_t* file) {
    if (file && file->fops && file->fops->close)
        return file->fops->close(file);
    return -1;
}

int vfs_read(file_t* file, void* buf, size_t len) {
    if (file && file->fops && file->fops->read)
        return file->fops->read(file, buf, len);
    return -1;
}

int vfs_write(file_t* file, const void* buf, size_t len) {
    if (file && file->fops && file->fops->write)
        return file->fops->write(file, buf, len);
    return -1;
}