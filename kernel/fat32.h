#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

#define FAT32_ATTR_READ_ONLY 0x1
#define FAT32_ATTR_HIDDEN 0x2
#define FAT32_ATTR_SYSTEM 0x4
#define FAT32_ATTR_VOLUME_ID 0x8
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE 0x20

typedef struct {
  char file_name[255];
  uint32_t size;
  uint8_t attributes;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_accessed_date;
  uint16_t last_modification_time;
  uint16_t last_modification_date;
} fat32_dentry_t;

typedef struct {
  uint32_t first_sector;
  size_t entry_idx;
} fat32_file_t;

void fat32_init();

fat32_file_t fat32_root();
int8_t fat32_get_next_dir_entry(fat32_file_t *vfs_dir,
                                fat32_dentry_t *fat32_de);

#endif
