#ifndef VFS_H
#define VFS_H

#include "../utils/string_view.h"
#include <stddef.h>
#include <stdint.h>

// info to fill by file system drivers
typedef struct {
  char name[256];
  size_t size;
  bool is_dir;
  void *fs_data; // any data the filesystem driver needs to find the thing
} vfs_dentry;

// info about the underlying file, not dependent on who opens it
typedef struct vfs_node {
  struct vfs_node *parent;
  vfs_dentry dentry;
} vfs_node;

typedef int (*vfs_readdir_f)(void *fs_data, size_t *cursor, vfs_dentry *dentry);

typedef struct {
  vfs_readdir_f readdir;
} vfs_ops;

typedef struct {
  vfs_ops ops;
  void *root_fs_data;
} vfs_init_data;

void vfs_init(vfs_init_data init);
int vfs_open(const char *path);
int vfs_close(size_t fd);

vfs_dentry *vfs_readdir(size_t fd);

#endif
