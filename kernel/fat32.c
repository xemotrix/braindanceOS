#include "fat32.h"
#include "../drivers/ata.h"
#include "../utils/string.h"
#include "heap.h"
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

#define DELETED_MARKER 0xE5
#define END_MARKER 0x0

#define FAT32_ADDR_MASK 0x0FFFFFFF
#define FAT32_EOF 0x0FFFFFF8
#define FAT32_BAD_CLUSTER 0x0FFFFFF7

#define DIV_ROUND_UP(a, b) (((a) + (b) - 1) / (b))

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
  // to be able to update the on-disk dirent
  uint32_t dirent_sector;
  uint32_t dirent_idx;
} fat32_dentry_t;

typedef struct {
  uint32_t first_sector;
  size_t entry_idx;
} fat32_file_t;

//=====STATE=================
static uint32_t *fat_table;
static uint32_t fat_first_sector;
static uint32_t fat_n_sectors;
static uint32_t fat_n_entries; // total number of clusters
static uint32_t root_cluster_no;
static uint32_t root_sector;
static uint32_t first_data_sector;
static uint32_t sectors_per_cluster;
static uint32_t sector_size;
static uint32_t cluster_size;
static fat32_dentry_t root_dentry;
static uint32_t total_sectors;
static size_t n_fats;

#define FIRST_SECTOR_IN_CLUSTER(c)                                             \
  (first_data_sector + sectors_per_cluster * ((c) - 2))

#define CLUSTER_FROM_SECTOR(s)                                                 \
  ((((s) - first_data_sector) / sectors_per_cluster) + 2)

//===========================

static inline uint32_t fat_get_next_cluster(uint32_t cluster_no) {
  uint32_t table_value = fat_table[cluster_no] & FAT32_ADDR_MASK;
  if (table_value >= FAT32_EOF) {
    return 0; // 0 is not valid as it's always reserved
  }
  if (table_value == FAT32_BAD_CLUSTER) {
    PANIC("bad cluster!");
  }
  return table_value;
}

static int parse_dir_entry(union fat32_dir_entry de, uint32_t dirent_sector,
                           fat32_dentry_t *out) {
  out->dirent_idx++;
  if (de.std.attributes != 0xf) { // STD
    out->size = de.std.file_size;
    out->attributes = de.std.attributes;
    out->creation_time = de.std.creation_time;
    out->creation_date = de.std.creation_date;
    out->last_accessed_date = de.std.last_accessed_date;
    out->last_modification_time = de.std.last_modification_time;
    out->last_modification_date = de.std.last_modification_date;
    out->first_cluster = ((uint32_t)de.std.first_cluster_no_high) << 16 |
                         (uint32_t)de.std.first_cluster_no_low;
    out->dirent_sector = dirent_sector;
    return 0;
  } else { // LFN
    int8_t order = (de.lfn.order & 0xf) - 1;
    size_t filename_idx = 0;
    for (int j = 0; j < 10 / 2; j++) {
      char c = (char)(((uint16_t *)de.lfn.text1)[j] & 0xffff);
      out->file_name[(order * 13) + filename_idx++] = c;
    }
    for (int j = 0; j < 12 / 2; j++) {
      char c = (char)(((uint16_t *)de.lfn.text2)[j] & 0xffff);
      out->file_name[(order * 13) + filename_idx++] = c;
    }
    for (int j = 0; j < 4 / 2; j++) {
      char c = (char)(((uint16_t *)de.lfn.text3)[j] & 0xffff);
      out->file_name[(order * 13) + filename_idx++] = c;
    }
    return 1;
  }
}

static int fat32_get_next_dir_entry(fat32_file_t *dir,
                                    fat32_dentry_t *fat32_de) {
  union fat32_dir_entry *fat32_dentries = kmalloc(cluster_size);

  size_t dirent_sector = dir->first_sector;
  ata_read_disk(dirent_sector, sectors_per_cluster, fat32_dentries);
  if (dir->entry_idx >= cluster_size / sizeof(union fat32_dir_entry))
    PANIC("multi-cluster not supported");

  union fat32_dir_entry de = fat32_dentries[dir->entry_idx];

  while ((uint8_t)de.std.fname[0] == DELETED_MARKER)
    de = fat32_dentries[++dir->entry_idx]; // skip deleted entries

  if (de.std.fname[0] == END_MARKER) {
    kfree(fat32_dentries);
    return 0; // no more directory entries
  }

  for (;;) {
    de = fat32_dentries[dir->entry_idx++];
    if (!parse_dir_entry(de, dir->first_sector, fat32_de))
      break;
  }

  kfree(fat32_dentries);
  return 1;
}

static size_t fat32_find_empty_cluster() {
  size_t end = (total_sectors - first_data_sector) / sectors_per_cluster + 2;
  for (size_t i = 2; i < end; i++) {
    if ((fat_table[i] & FAT32_ADDR_MASK) == 0) {
      return i;
    }
  }
  PANIC("out of disk space");
}

static void fat32_patch_fat(size_t cluster_no) {
  uint32_t sector_in_fat =
      fat_first_sector + (cluster_no * sizeof(uint32_t) / sector_size);

  size_t entries_per_sector = sector_size / sizeof(uint32_t);
  size_t start = cluster_no & ~(entries_per_sector - 1); // round down

  ata_write_disk(sector_in_fat, 1, &fat_table[start]);
  if (n_fats == 2)
    ata_write_disk(fat_n_sectors + sector_in_fat, 1, &fat_table[start]);
}

static void fat32_append_empty_clusters(fat32_dentry_t *dirent, size_t count) {
  uint32_t cluster_no = 0;
  uint32_t next_cluster_no;
  if (dirent->first_cluster) {
    cluster_no = dirent->first_cluster;
    while ((next_cluster_no = fat_get_next_cluster(cluster_no)))
      cluster_no = next_cluster_no;
  }

  uint8_t *buf_zeros = kmalloc(cluster_size);
  memset(buf_zeros, 0, cluster_size);

  for (size_t i = 0; i < count; i++) {
    next_cluster_no = fat32_find_empty_cluster();
    uint64_t lba = FIRST_SECTOR_IN_CLUSTER(next_cluster_no);
    ata_write_disk(lba, sectors_per_cluster, buf_zeros);
    fat_table[next_cluster_no] = FAT32_EOF;
    fat32_patch_fat(next_cluster_no);

    if (cluster_no) { // if file was not empty to begin with
      fat_table[cluster_no] = (fat_table[cluster_no] & ~FAT32_ADDR_MASK) |
                              (next_cluster_no & FAT32_ADDR_MASK);
      fat32_patch_fat(cluster_no);
    } else { // if it was
      dirent->first_cluster = next_cluster_no;
    }
    cluster_no = next_cluster_no;
  }
  kfree(buf_zeros);
}

static void fat32_patch_dirent(fat32_dentry_t *dirent) {
  union fat32_dir_entry *entries = kmalloc(cluster_size);
  ata_read_disk(dirent->dirent_sector, sectors_per_cluster, entries);
  struct fat32_STD_dir_entry *dent = &entries[dirent->dirent_idx].std;
  dent->file_size = dirent->size;
  // for files that start empty:
  dent->first_cluster_no_high = dirent->first_cluster >> 16;
  dent->first_cluster_no_low = dirent->first_cluster & 0xffff;
  // todo, when we have RTC
  // dent->last_modification_time
  // dent->last_modification_date
  ata_write_disk(dirent->dirent_sector, sectors_per_cluster, entries);
  kfree(entries);
}

static void fill_fat_table() {
  size_t fat_table_size = fat_n_sectors * sector_size;
  fat_table = kmalloc(fat_table_size);
  ata_read_disk(fat_first_sector, fat_n_sectors, fat_table);
}

// fat32_write can assume:
// - count > 0, that is handled by vfs
int fat32_write(fat32_dentry_t *dirent, size_t *cursor, const void *in,
                size_t count) {

  // handle if cursor > size, zero the hole in the last pre-existing cluster
  if (*cursor > dirent->size && dirent->size % cluster_size != 0) {
    size_t old_last_cluster = dirent->first_cluster;
    for (size_t i = 0; i < dirent->size / cluster_size; i++)
      old_last_cluster = fat_get_next_cluster(old_last_cluster);

    size_t hole_start = dirent->size % cluster_size;
    size_t hole_end = cluster_size;
    // if cursor is in the same cluster, dont zero until the end
    if (*cursor / cluster_size == dirent->size / cluster_size)
      hole_end = *cursor % cluster_size;

    uint8_t *buf = kmalloc(cluster_size);
    size_t lba = FIRST_SECTOR_IN_CLUSTER(old_last_cluster);
    ata_read_disk(lba, sectors_per_cluster, buf);
    memset(buf + hole_start, 0, hole_end - hole_start);
    ata_write_disk(lba, sectors_per_cluster, buf);
    kfree(buf);
  }

  // append any extra clusters we will need, zeroed
  size_t start_n_clusters = DIV_ROUND_UP(dirent->size, cluster_size);
  size_t end_n_clusters = DIV_ROUND_UP(*cursor + count, cluster_size);
  if (end_n_clusters > start_n_clusters)
    fat32_append_empty_clusters(dirent, end_n_clusters - start_n_clusters);

  // find write start cluster
  size_t cluster_no = dirent->first_cluster;
  for (size_t i = 0; i < *cursor / cluster_size; i++)
    cluster_no = fat_get_next_cluster(cluster_no);

  size_t bytes_written = 0;
  size_t bytes_left = count;
  uint8_t *buf = kmalloc(cluster_size);

  for (;;) {
    size_t sector_no = FIRST_SECTOR_IN_CLUSTER(cluster_no);
    ata_read_disk(sector_no, sectors_per_cluster, buf);

    size_t offset_in_cluster = *cursor % cluster_size;
    size_t left_in_cluster = cluster_size - offset_in_cluster;
    size_t copy_size =
        bytes_left < left_in_cluster ? bytes_left : left_in_cluster;

    const void *src = (const uint8_t *)in + bytes_written;
    void *dst = (uint8_t *)buf + offset_in_cluster;
    memcpy(dst, src, copy_size);

    ata_write_disk(sector_no, sectors_per_cluster, buf);

    bytes_written += copy_size;
    *cursor += copy_size;
    bytes_left -= copy_size;
    if (bytes_left == 0)
      break;
    cluster_no = fat_get_next_cluster(cluster_no);
  }

  kfree(buf);
  if (*cursor > dirent->size)
    dirent->size = *cursor;
  fat32_patch_dirent(dirent);
  return bytes_written;
}

// fat32_read can assume:
// - count > 0, that is handled by vfs
// - *cursor points to somewhere withing the file (not past it)
int fat32_read(fat32_dentry_t *dirent, size_t *cursor, void *out,
               size_t count) {
  size_t bytes_read = 0;
  size_t bytes_left = count;
  if (count + *cursor > dirent->size)
    bytes_left = dirent->size - *cursor;

  // find start cluster
  size_t cluster_no = dirent->first_cluster;
  for (size_t i = 0; i < *cursor / cluster_size; i++) {
    cluster_no = fat_get_next_cluster(cluster_no);
  }

  uint8_t *buf = kmalloc(cluster_size);
  for (;;) {
    ata_read_disk(FIRST_SECTOR_IN_CLUSTER(cluster_no), sectors_per_cluster,
                  buf);

    size_t offset_in_cluster = *cursor % cluster_size;
    size_t left_in_cluster = cluster_size - offset_in_cluster;
    size_t copy_size =
        bytes_left < left_in_cluster ? bytes_left : left_in_cluster;

    void *dst = (uint8_t *)out + bytes_read;
    void *src = (uint8_t *)buf + *cursor % cluster_size;
    memcpy(dst, src, copy_size);

    bytes_read += copy_size;
    *cursor += copy_size;
    bytes_left -= copy_size;
    if (bytes_left == 0)
      break;
    cluster_no = fat_get_next_cluster(cluster_no);
  }
  kfree(buf);

  return bytes_read;
}

int fat32_readdir(fat32_dentry_t *dirent, size_t *cursor, vfs_dentry *out) {
  fat32_dentry_t fat32_de = {0};

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

  // todo: this leaks
  fat32_dentry_t *fs_data = kmalloc(sizeof(fat32_dentry_t));

  *fs_data = fat32_de;
  out->fs_data = fs_data;
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
  fat_first_sector = br.bpb.n_reserved_sectors;
  fat_n_sectors = br.sectors_per_fat;
  fat_n_entries = fat_n_sectors * sector_size / sizeof(uint32_t);
  total_sectors = br.bpb.large_sector_count;
  n_fats = br.bpb.n_fats;

  fill_fat_table();

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
      .ops =
          (vfs_ops){
              .readdir = (vfs_readdir_f)&fat32_readdir,
              .read = (vfs_read_f)&fat32_read,
              .write = (vfs_write_f)&fat32_write,
          },
      .root_fs_data = &root_dentry,
  };
}
