#include "../include/fs/vfs.h"
#include "../include/lib.h"

static const char dummy_test_data[] = "dummyfs: /test ok\n";

static int dummyfs_read(file_t* file, void* buf, size_t len) {
    const char* data = dummy_test_data;
    size_t data_len = 0;
    while (data[data_len]) data_len++;

    if (file->f_pos >= data_len) {
        return 0;
    }

    size_t remaining = data_len - file->f_pos;
    size_t to_copy = (len < remaining) ? len : remaining;
    memcpy(buf, data + file->f_pos, to_copy);
    file->f_pos += to_copy;
    return (int)to_copy;
}

static int dummyfs_lookup(struct vnode* dir_node, const char* name, struct vnode** target);

static struct file_ops dummy_file_ops = {
    .read = dummyfs_read,
    .write = NULL,
    .close = NULL,
};

static struct vnode_ops dummy_dir_ops = {
    .lookup = dummyfs_lookup,
    .create = NULL,
    .readdir = NULL,
    .mkdir = NULL,
    .unlink = NULL,
};

static struct vnode dummy_test_vnode = {
    .fs = NULL,
    .fops = &dummy_file_ops,
    .vops = NULL,
    .internal = NULL,
};

static int dummyfs_lookup(struct vnode* dir_node, const char* name, struct vnode** target) {
    if (strcmp(name, "test") == 0) {
        dummy_test_vnode.fs = dir_node->fs;
        *target = &dummy_test_vnode;
        return 0;
    }
    return -1;
}

static int dummyfs_setup_mount(struct filesystem* fs, vnode_t* root) {
    root->fs = fs;
    root->fops = NULL;
    root->vops = &dummy_dir_ops;
    root->internal = NULL;
    return 0;
}

struct filesystem dummy_fs = {
    .name = "dummyfs",
    .setup_mount = dummyfs_setup_mount
};
