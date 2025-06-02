#include "../include/fs/vfs.h"
#include "../include/lib.h"

static int dummyfs_setup_mount(struct filesystem* fs, vnode_t* root) {
    root->fs = fs;
    root->fops = NULL;
    root->internal = NULL;
    return 0;
}

struct filesystem dummy_fs = {
    .name = "dummyfs",
    .setup_mount = dummyfs_setup_mount
};