#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

typedef struct FILE FILE;

typedef struct {
  char file_name[255];
  uint32_t size;
  bool is_dir;
} vfs_dirent_t;

FILE *vfs_open_root();
int8_t vfs_close(FILE *f);
int8_t vfs_readdir(FILE *dir, vfs_dirent_t *out);

#endif
