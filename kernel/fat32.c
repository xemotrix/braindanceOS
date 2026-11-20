#include "fat32.h"
#include "../drivers/ata.h"
#include "../utils/string.h"
#include "panic.h"
#include <stddef.h>
#include <stdint.h>

// https://wiki.osdev.org/FAT#FAT_32

#define FAT32_ATTR_READ_ONLY 0x1
#define FAT32_ATTR_HIDDEN 0x2
#define FAT32_ATTR_SYSTEM 0x4
#define FAT32_ATTR_VOLUME_ID 0x8
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE 0x20

struct fat32_bios_parameter_block {
  uint32_t : 3 * 8; // JUMP
  uint64_t oem_identifier;
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t n_reserved_sectors;
  uint8_t n_fats;
  uint16_t n_root_dir_entries;
  uint16_t n_sectors_logical_volume;
  uint8_t media_descriptor_type;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t n_heads;
  uint32_t n_hiddlen_sectors;
  uint32_t large_sector_count;
} __attribute__((packed));

struct fat32_boot_record {
  struct fat32_bios_parameter_block bpb;
  uint32_t sectors_per_fat;
  uint16_t flags;
  uint16_t fat_version_number;
  uint32_t root_cluster_no;
  uint16_t fsiinfo_sector_no;
  uint16_t backup_root_sector_no;
  uint8_t reserved[12];
  uint8_t drive_no;
  uint8_t reserved2;
  uint8_t signature;
  uint32_t volume_id_serial;
  char volume_lbl[11];
  char sys_identifier[8];
  uint8_t boot_code[420];
  uint16_t boot_signature;
} __attribute__((packed));

struct fat32_fsinfo {
  uint32_t signature1;
  uint8_t reserved[480];
  uint32_t signature2;
  uint32_t last_known_free_cluster;
  uint32_t avail_search_start_cluster_no;
  uint8_t reserved2[12];
  uint32_t signature3;
} __attribute__((packed));

// long file name
struct fat32_LFN_dir_entry {
  uint8_t order;
  char text1[10];
  uint8_t attributes;
  uint8_t long_entry_type;
  uint8_t checksum;
  char text2[12];
  uint16_t zero;
  char text3[4];
} __attribute__((packed));

struct fat32_STD_dir_entry {
  char fname[11];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creation_duration; // duration in hundreds of a second
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_accessed_date;
  uint16_t first_cluster_no_high;
  uint16_t last_modification_time;
  uint16_t last_modification_date;
  uint16_t first_cluster_no_low;
  uint32_t file_size;
} __attribute__((packed));

union fat32_dir_entry {
  struct fat32_LFN_dir_entry lfn;
  struct fat32_STD_dir_entry std;
};

typedef struct {
  char file_name[256];
  uint32_t size;
  uint8_t attributes;
  uint16_t creation_time;
  uint16_t creation_date;
  uint16_t last_accessed_date;
  uint16_t last_modification_time;
  uint16_t last_modification_date;
  uint32_t first_cluster;
} fat32_dentry_t;

typedef struct {
  uint32_t first_sector;
  size_t entry_idx;
} fat32_file_t;

#define FAT32_ATTR_READ_ONLY 0x1
#define FAT32_ATTR_HIDDEN 0x2
#define FAT32_ATTR_SYSTEM 0x4
#define FAT32_ATTR_VOLUME_ID 0x8
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE 0x20

#define DELETED_MARKER 0xE5
#define END_MARKER 0x0

#define FIRST_SECTOR_IN_CLUSTER(c)                                             \
  (first_data_sector + sectors_per_cluster * ((c) - 2))
//=====STATE=================
uint32_t root_cluster_no;
uint32_t root_sector;
uint32_t first_data_sector;
uint32_t sectors_per_cluster;
uint32_t sector_size;
uint32_t cluster_size;
fat32_dentry_t root_dentry;

//===========================

void fat32_read_dir(size_t first_sector) {}

#define INIT_DIR_ENTRY(var_name)                                               \
  do {                                                                         \
    for (size_t i = 0; i < sizeof(var_name->file_name); i++) {                 \
      var_name->file_name[i] = 0;                                              \
    }                                                                          \
    var_name->size = 0;                                                        \
    var_name->attributes = 0;                                                  \
    var_name->creation_time = 0;                                               \
    var_name->creation_date = 0;                                               \
    var_name->last_accessed_date = 0;                                          \
    var_name->last_modification_time = 0;                                      \
    var_name->last_modification_date = 0;                                      \
    var_name->first_cluster = 0;                                               \
  } while (0);

fat32_file_t fat32_root() {
  return (fat32_file_t){
      .first_sector = root_sector,
      .entry_idx = 0,
  };
}

int fat32_get_next_dir_entry(fat32_file_t *dir, fat32_dentry_t *fat32_de) {
  INIT_DIR_ENTRY(fat32_de);
  union fat32_dir_entry fat32_dentries[cluster_size];
  int direntry_size = sizeof(union fat32_dir_entry);

  ata_read_disk(dir->first_sector, sectors_per_cluster, &fat32_dentries);
  if (dir->entry_idx >= cluster_size / sizeof(union fat32_dir_entry))
    PANIC("multi-cluster not supported");

  union fat32_dir_entry de = fat32_dentries[dir->entry_idx];

  while ((uint8_t)de.std.fname[0] == DELETED_MARKER)
    de = fat32_dentries[++dir->entry_idx]; // skip deleted entries

  if (de.std.fname[0] == END_MARKER)
    return 0; // no more directory entries

  for (;;) {
    de = fat32_dentries[dir->entry_idx++];
    if (de.std.attributes != 0xf) {
      fat32_de->size = de.std.file_size;
      fat32_de->attributes = de.std.attributes;
      fat32_de->creation_time = de.std.creation_time;
      fat32_de->creation_date = de.std.creation_date;
      fat32_de->last_accessed_date = de.std.last_accessed_date;
      fat32_de->last_modification_time = de.std.last_modification_time;
      fat32_de->last_modification_date = de.std.last_modification_date;
      fat32_de->first_cluster = ((uint32_t)de.std.first_cluster_no_high) << 16 |
                                (uint32_t)de.std.first_cluster_no_low;
      break;
    }
    int8_t order = (de.lfn.order & 0xf) - 1;
    size_t filename_idx = 0;
    for (int j = 0; j < 10 / 2; j++) {
      char c = (char)(((uint16_t *)de.lfn.text1)[j] & 0xffff);
      fat32_de->file_name[(order * 13) + filename_idx++] = c;
    }
    for (int j = 0; j < 12 / 2; j++) {
      char c = (char)(((uint16_t *)de.lfn.text2)[j] & 0xffff);
      fat32_de->file_name[(order * 13) + filename_idx++] = c;
    }
    for (int j = 0; j < 4 / 2; j++) {
      char c = (char)(((uint16_t *)de.lfn.text3)[j] & 0xffff);
      fat32_de->file_name[(order * 13) + filename_idx++] = c;
    }
  }
  return 1;
}

int readdir(fat32_dentry_t *dirent, size_t *cursor, vfs_dentry *out) {
  fat32_dentry_t fat32_de;
  fat32_file_t dir = {
      .first_sector = FIRST_SECTOR_IN_CLUSTER(dirent->first_cluster),
      .entry_idx = *cursor / sizeof(union fat32_dir_entry),
  };

  int ok = fat32_get_next_dir_entry(&dir, &fat32_de);
  *cursor = dir.entry_idx * sizeof(union fat32_dir_entry);
  if (!ok)
    return 0; // no more dirs

  memcpy(out->name, fat32_de.file_name, sizeof(out->name));
  out->size = fat32_de.size;
  out->is_dir = fat32_de.attributes & FAT32_ATTR_DIRECTORY;
  return 1;
}

vfs_init_data fat32_init() {
  struct fat32_boot_record br;
  ata_read_disk(0, 1, &br);

  root_cluster_no = br.root_cluster_no;
  sectors_per_cluster = br.bpb.sectors_per_cluster;
  sector_size = br.bpb.bytes_per_sector;
  cluster_size = sector_size * sectors_per_cluster;
  first_data_sector =
      br.bpb.n_reserved_sectors + (br.bpb.n_fats * br.sectors_per_fat);
  root_sector = FIRST_SECTOR_IN_CLUSTER(br.root_cluster_no);

  root_dentry = (fat32_dentry_t){
      .file_name = "/",
      .size = 0,
      .attributes = FAT32_ATTR_DIRECTORY,
      .creation_time = 0,
      .creation_date = 0,
      .last_accessed_date = 0,
      .last_modification_time = 0,
      .last_modification_date = 0,
      .first_cluster = br.root_cluster_no,
  };

  return (vfs_init_data){
      .ops = (vfs_ops){.readdir = (vfs_readdir_f)&readdir},
      .root_fs_data = &root_dentry,
  };
}
