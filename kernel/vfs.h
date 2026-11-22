#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  SEEK_SET,
  SEEK_CUR,
  SEEK_END,
} seek_whence;

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
typedef int (*vfs_read_f)(void *fs_data, size_t *cursor, void *out,
                          size_t count);
typedef int (*vfs_write_f)(void *fs_data, size_t *cursor, const void *in,
                           size_t count);

typedef struct {
  vfs_readdir_f readdir;
  vfs_read_f read;
  vfs_write_f write;
} vfs_ops;

typedef struct {
  vfs_ops ops;
  void *root_fs_data;
} vfs_init_data;

void vfs_init(vfs_init_data init);

size_t vfs_file_size(size_t fd);
size_t vfs_file_cursor(size_t fd);

int vfs_open(const char *path);
int vfs_close(size_t fd);
int vfs_read(size_t fd, void *buf, size_t count);
int vfs_seek(size_t fd, size_t offset, seek_whence whence);
int vfs_write(size_t fd, const void *buf, size_t count);
vfs_dentry *vfs_readdir(size_t fd);

#endif
