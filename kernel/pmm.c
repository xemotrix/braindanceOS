// PHYSICAL MEMORY MANAGER
#include "pmm.h"

#define E820_BUFFER_ADDR 0x7500
#define MMAP_ENTRY_COUNT_ADDR 0x74fc

#define _1MB 0x100000
#define _2MB 0x200000
#define PAGE_SIZE 0x1000 // 4KB

// size reserved for the kernel code (starts at 1MB)
// at the end lives the physical page bitmap.
#define KERNEL_ZONE_OFFSET _1MB

const struct e820_entry *entries;
uint32_t n_entries;
uint8_t *bitmap;
uint32_t bitmap_n_pages;

static int8_t free_page_idx(uint32_t page_idx) {
  if (page_idx * PAGE_SIZE < _2MB) {
    return 0;
  }
  uint32_t mapbyte_idx = page_idx / 8;
  uint8_t mask = 0x80 >> page_idx % 8;
  bitmap[mapbyte_idx] &= ~mask;
  return 1;
}

int8_t pmm_free_page(uint32_t address) {
  if (address % PAGE_SIZE != 0) {
    return 0;
  }
  uint32_t idx = address / PAGE_SIZE;
  return free_page_idx(idx);
}

static int8_t reserve_page_idx(uint32_t page_idx) {
  if (page_idx * PAGE_SIZE < _2MB) {
    return 0;
  }
  uint32_t mapbyte_idx = page_idx / 8;
  uint8_t mask = 0x80 >> page_idx % 8;
  bitmap[mapbyte_idx] |= mask;
  return 1;
}

static int8_t get_page_state(uint32_t page_idx) {
  if (page_idx * PAGE_SIZE < _2MB) {
    return 1;
  }
  uint32_t bitmap_idx = page_idx / 8;
  uint8_t mask = 0x80 >> page_idx % 8;
  return (bitmap[bitmap_idx] & mask) != 0;
}

void *pmm_get_page(size_t min_addr) {
  min_addr = min_addr < _2MB ? _2MB : min_addr;
  for (uint32_t i = min_addr / PAGE_SIZE / 8; i < bitmap_n_pages / 8; i++) {
    if (bitmap[i] == 0xff)
      continue;
    for (uint32_t j = 0; j < 8; j++) {
      uint32_t page_idx = i * 8 + j;
      if (get_page_state(page_idx))
        continue;
      reserve_page_idx(page_idx);
      return (void *)(page_idx * PAGE_SIZE);
    }
    break;
  }
  return 0;
}

static void init_page_alloc() {
  // find end of bitmap mapped ram
  uint32_t end_address;
  uint8_t *base_of_main_section;
  for (int i = 0; i < n_entries; i++) {
    struct e820_entry e = entries[i];
    if (e.type != REGION_TYPE_USABLE_RAM || e.base + e.length <= _1MB)
      continue;
    end_address = e.base + e.length;
    base_of_main_section = (uint8_t *)e.base;
    break;
  }
  // TODO: panic if not found

  bitmap_n_pages = end_address / PAGE_SIZE;
  uint32_t n_bytes_in_bitmap = bitmap_n_pages / 8;

  // save the bitmap at the end of the first MB (where the kernel code is)
  bitmap = base_of_main_section + KERNEL_ZONE_OFFSET - n_bytes_in_bitmap;

  for (int i = 0; i < n_bytes_in_bitmap; i++) {
    bitmap[i] = 0xff;
  }

  for (int i = _2MB / PAGE_SIZE; i < bitmap_n_pages; i++) {
    free_page_idx(i);
  }
}

void pmm_init() {
  if (entries != 0)
    return;
  entries = (const struct e820_entry *)E820_BUFFER_ADDR;
  n_entries = *(volatile uint32_t *)MMAP_ENTRY_COUNT_ADDR;
  init_page_alloc();
}

struct mem_stats pmm_get_mem_stats() {
  return (struct mem_stats){
      .n_entries = n_entries,
      .entries = entries,
  };
}
