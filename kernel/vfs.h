#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  char name[256];
  struct vfs_info_node *inode;
  size_t size;
  bool is_dir;
} vfs_dentry;

typedef int (*vfs_readdir_f)(void *fs_data, size_t *cursor, vfs_dentry *dentry);

typedef struct {
  vfs_readdir_f readdir;
} vfs_ops;

typedef struct {
  vfs_ops ops;
  void *root_fs_data;
} vfs_init_data;

void vfs_init(vfs_init_data init);
size_t vfs_open_root();
int vfs_close(size_t fd);

int vfs_readdir(size_t fd_dir, vfs_dentry *out);

#endif
