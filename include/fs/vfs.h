#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct vnode vnode_t;
typedef struct file file_t;

struct  vnode
{
    struct filesystem* fs;
    struct file_ops* fops;
    void* internal;
};

struct file {
    vnode_t* vnode;
    size_t f_pos;
    struct file_ops* fops;
    void* interna;
};

struct file_ops {
    int (*read)(file_t*, void* buf, size_t len);
    int (*write)(file_t*, const void* buf, size_t len);
    int (*close)(file_t*);
};

struct filesystem {
    const char* name;
    int (*setup_mount)(struct filesystem* fs, vnode_t* root);
};

int register_filesystem(struct filesystem* fs);
int vfs_open(const char* pathname, file_t** target);
int vfs_close(file_t* file);
int vfs_read(file_t* file, void* buf, size_t len);
int vfs_write(file_t* file, const void* buf, size_t len);
