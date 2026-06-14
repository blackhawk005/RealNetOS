#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct vnode vnode_t;
typedef struct file file_t;

struct  vnode
{
    struct filesystem* fs;
    struct file_ops* fops;
    struct vnode_ops* vops;
    void* internal;
};

struct file {
    vnode_t* vnode;
    size_t f_pos;
    struct file_ops* fops;
    void* internal;
};

struct file_ops {
    int (*read)(file_t*, void* buf, size_t len);
    int (*write)(file_t*, const void* buf, size_t len);
    int (*close)(file_t*);
};

struct vnode_ops {
    int (*lookup)(struct vnode* dir_node, const char* name, struct vnode** target);
    int (*create)(struct vnode* dir_node, const char* name, struct vnode** target);
    int (*readdir)(struct vnode* dir_node, int (*cb)(const char* name, int is_dir, void* ctx), void* ctx);
    int (*mkdir)(struct vnode* dir_node, const char* name, struct vnode** target);
    int (*unlink)(struct vnode* dir_node, const char* name);
};

struct filesystem {
    const char* name;
    int (*setup_mount)(struct filesystem* fs, vnode_t* root);
};

struct mount {
    struct filesystem* fs;
    vnode_t* root;
};

struct dentry {
    const char* name;
    vnode_t* vnode;
    struct dentry* next; // linked list of siblings
};

int register_filesystem(struct filesystem* fs);
int vfs_mount(struct filesystem* fs);
int vfs_open(const char* pathname, file_t** target);
int vfs_create(const char* pathname, file_t** target);
int vfs_list_root(int (*cb)(const char* name, int is_dir, void* ctx), void* ctx);
int vfs_list_dir(const char* pathname, int (*cb)(const char* name, int is_dir, void* ctx), void* ctx);
int vfs_mkdir(const char* pathname);
int vfs_unlink(const char* pathname);
int vfs_close(file_t* file);
int vfs_read(file_t* file, void* buf, size_t len);
int vfs_write(file_t* file, const void* buf, size_t len);
