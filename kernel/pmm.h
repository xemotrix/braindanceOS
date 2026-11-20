#ifndef PMM_H
#define PMM_H

#include <stddef.h>
#include <stdint.h>

#define E820_ENTRY_SIZE 24

enum memory_region_type {
  REGION_TYPE_USABLE_RAM = 1,
  REGION_TYPE_RESERVED,
  REGION_TYPE_ACPI_RECLAIMABLE,
  REGION_TYPE_ACPI_NVS_MEMORY,
  REGION_TYPE_UNUSABLE,
};

struct e820_entry {
  uint64_t base;
  uint64_t length;
  enum memory_region_type type;
  uint32_t attributes;
} __attribute__((packed));

struct mem_stats {
  uint32_t n_entries;
  const struct e820_entry *entries;
};

void pmm_init();
struct mem_stats pmm_get_mem_stats();
void *pmm_get_page(size_t min_addr);
int8_t pmm_free_page(uint32_t address);

#endif
