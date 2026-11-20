#include "vfs.h"
#include "../utils/string.h"
#include "fat32.h"
#include "heap.h"

typedef struct FILE {
  fat32_file_t file;
} FILE;

FILE *vfs_open_root() {
  FILE *f = kmalloc(sizeof(FILE));
  *f = (FILE){.file = fat32_root()};
  return f;
}

int8_t vfs_close(FILE *f) { return kfree(f); }

int8_t vfs_readdir(FILE *dir, vfs_dirent_t *out) {
  fat32_dentry_t fat32de;
  if (!fat32_get_next_dir_entry(&dir->file, &fat32de))
    return 0;

  memcpy(out->file_name, fat32de.file_name, sizeof(out->file_name));
  out->size = fat32de.size;
  out->is_dir = fat32de.attributes & FAT32_ATTR_DIRECTORY;

  return 1;
}
