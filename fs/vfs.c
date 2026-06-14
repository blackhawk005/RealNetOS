#include "../include/fs/vfs.h"
#include "../include/lib.h"

static struct filesystem* fs_list[4];  // Support up to 4 FS types
static int fs_count = 0;

struct mount rootfs = { .fs = NULL, .root = NULL };  // GLOBAL MOUNT
static vnode_t root_node;

int register_filesystem(struct filesystem* fs) {
    if (fs_count >= 4) return -1;
    fs_list[fs_count++] = fs;
    return 0;
}

int vfs_mount(struct filesystem* fs) {
    if (fs == NULL || fs->setup_mount == NULL) return -1;
    rootfs.fs = fs;
    rootfs.root = &root_node;
    root_node.fs = fs;
    root_node.fops = NULL;
    root_node.vops = NULL;
    root_node.internal = NULL;
    return fs->setup_mount(fs, &root_node);
}

static int vfs_walk(const char* pathname, vnode_t** out_node, vnode_t** out_parent, char* out_leaf) {
    if (pathname == NULL || pathname[0] != '/') return -1;
    if (rootfs.root == NULL) return -1;

    vnode_t* current = rootfs.root;

    char path[128];
    strcpy(path, pathname);

    char* token = strtok(path, "/");
    if (token == NULL) {
        if (out_node) *out_node = current;
        if (out_parent) *out_parent = NULL;
        if (out_leaf) out_leaf[0] = '\0';
        return 0;
    }

    while (token != NULL) {
        char* next = strtok(NULL, "/");
        if (next == NULL) {
            if (out_parent) *out_parent = current;
            if (out_leaf) strcpy(out_leaf, token);
            if (out_node) {
                if (current->vops == NULL || current->vops->lookup == NULL) return -1;
                vnode_t* found = NULL;
                if (current->vops->lookup(current, token, &found) != 0) return -1;
                *out_node = found;
            }
            return 0;
        }

        if (current->vops == NULL || current->vops->lookup == NULL) return -1;
        vnode_t* next_node = NULL;
        if (current->vops->lookup(current, token, &next_node) != 0) return -1;
        current = next_node;
        token = next;
    }
    return -1;
}

int vfs_open(const char* pathname, file_t** target) {
    vnode_t* node = NULL;
    if (vfs_walk(pathname, &node, NULL, NULL) != 0) return -1;

    file_t* f = (file_t*)kmalloc(sizeof(file_t));
    f->vnode = node;
    f->fops = node->fops;
    f->f_pos = 0;
    f->internal = NULL;

    *target = f;
    return 0;
}

int vfs_create(const char* pathname, file_t** target) {
    vnode_t* parent = NULL;
    char leaf[64];
    if (vfs_walk(pathname, NULL, &parent, leaf) != 0) return -1;
    if (parent == NULL || parent->vops == NULL || parent->vops->create == NULL) return -1;

    vnode_t* created = NULL;
    if (parent->vops->create(parent, leaf, &created) != 0) return -1;

    file_t* f = (file_t*)kmalloc(sizeof(file_t));
    f->vnode = created;
    f->fops = created->fops;
    f->f_pos = 0;
    f->internal = NULL;
    *target = f;
    return 0;
}

int vfs_list_root(int (*cb)(const char* name, int is_dir, void* ctx), void* ctx) {
    if (rootfs.root == NULL) return -1;
    if (rootfs.root->vops == NULL || rootfs.root->vops->readdir == NULL) return -1;
    return rootfs.root->vops->readdir(rootfs.root, cb, ctx);
}

int vfs_list_dir(const char* pathname, int (*cb)(const char* name, int is_dir, void* ctx), void* ctx) {
    vnode_t* node = NULL;
    if (vfs_walk(pathname, &node, NULL, NULL) != 0) return -1;
    if (node == NULL || node->vops == NULL || node->vops->readdir == NULL) return -1;
    return node->vops->readdir(node, cb, ctx);
}

int vfs_mkdir(const char* pathname) {
    vnode_t* parent = NULL;
    char leaf[64];
    if (vfs_walk(pathname, NULL, &parent, leaf) != 0) return -1;
    if (parent == NULL || parent->vops == NULL || parent->vops->mkdir == NULL) return -1;
    vnode_t* created = NULL;
    return parent->vops->mkdir(parent, leaf, &created);
}

int vfs_unlink(const char* pathname) {
    vnode_t* parent = NULL;
    char leaf[64];
    if (vfs_walk(pathname, NULL, &parent, leaf) != 0) return -1;
    if (parent == NULL || parent->vops == NULL || parent->vops->unlink == NULL) return -1;
    return parent->vops->unlink(parent, leaf);
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
