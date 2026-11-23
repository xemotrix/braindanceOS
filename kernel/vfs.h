#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

#define SEEK_SET 1 // offset set to offset bytes.
#define SEEK_CUR 2 // offset set to its current location plus offset bytes.
#define SEEK_END 3 // offset set to the size of the file plus offset bytes.

#define OPEN_READ 0x1   // can read the file
#define OPEN_WRITE 0x2  // can write to the file
#define OPEN_RDWR 0x3   // both
#define OPEN_CREATE 0x4 // create it if it doesn't exist
#define OPEN_APPEND 0x8 // on every write, write at the end

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
int vfs_seek(size_t fd, size_t offset, uint8_t whence);
int vfs_write(size_t fd, const void *buf, size_t count);
vfs_dentry *vfs_readdir(size_t fd);

#endif
