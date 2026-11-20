#include "vmm.h"
#include "../kernel/panic.h"
#include "pmm.h"

// https://wiki.osdev.org/X86_Paging
// https://wiki.osdev.org/Setting_Up_Paging

#define FLG_DIR_READ_WRITE 0x2
#define FLG_DIR_PRESENT 0x1

#define N_DIRECTORY_ENTRIES 1024
#define N_PAGETABLE_ENTRIES 1024
#define N_ENTRIES N_DIRECTORY_ENTRIES *N_PAGETABLE_ENTRIES

#define FLG_TBL_READ_WRITE 0x2
#define FLG_TBL_PRESENT 0x1
#define FLG_TBL_RESERVED (1 << 9) // this is not used by the CPU

#define _2MB 0x200000

#define PAGE_ADDRESS_MASK 0xFFFFF000
// not present && not reserved
#define TABLE_ENTRY_FREE(entry)                                                \
  (!((entry) & FLG_TBL_PRESENT) && !((entry) & FLG_TBL_RESERVED))

extern void __load_page_directory(uint32_t *);
extern void __enable_paging();

size_t *page_dir;

struct entry_idx {
  uint16_t dir_idx;   // idx in directory
  uint16_t table_idx; // idx in table
};

struct addr_range {
  size_t start;
  size_t end;
};

struct addr_range vaddr_ranges[RANGE_MAX] = {
    [RANGE_IDENTITY] = (struct addr_range){.start = 0x0, .end = 0x400000},
    [RANGE_HEAP] = (struct addr_range){.start = 0xD0000000, .end = 0xE0000000},
};

static inline struct entry_idx entry_idx_from_vaddress(size_t vaddress) {
  // table_idx = (address / PAGE_SIZE) / N_DIRECTORY_ENTRIES
  // entry_idx = (address / PAGE_SIZE) % N_PAGETABLE_ENTRIES
  return (struct entry_idx){
      .dir_idx = (vaddress >> 22) & 0x3ff,
      .table_idx = (vaddress >> 12) & 0x3ff,
  };
}

static inline size_t vaddress_from_entry_idx(struct entry_idx eidx) {
  return (eidx.dir_idx << 22) | (eidx.table_idx << 12);
}

static bool is_entry_free(struct entry_idx eidx) {
  size_t dir_entry = page_dir[eidx.dir_idx];
  if (!(dir_entry & FLG_DIR_PRESENT))
    return false;

  size_t *table = (size_t *)(dir_entry & PAGE_ADDRESS_MASK);
  if (!TABLE_ENTRY_FREE(table[eidx.table_idx]))
    return false;

  return true;
}

static void init_table(size_t dir_idx) {
  size_t dir_entry = page_dir[dir_idx];
  ASSERT(!(dir_entry & FLG_DIR_PRESENT))
  size_t phys_table_addr = (size_t)pmm_get_page(0);
  for (size_t i = 0; i < N_PAGETABLE_ENTRIES; i++) {
    ((size_t *)phys_table_addr)[i] = 0;
  }
  page_dir[dir_idx] = phys_table_addr | (FLG_DIR_READ_WRITE | FLG_DIR_PRESENT);
}

static int8_t find_n_non_free_entries(enum vaddr_range range, size_t n_pages,
                                      struct entry_idx *res) {
  struct addr_range rng = vaddr_ranges[range];

  size_t found_consecutive_entries = 0;
  size_t found_start = 0;
  for (size_t vaddr = rng.start; vaddr < rng.end; vaddr += PAGE_SIZE) {
    struct entry_idx eidx = entry_idx_from_vaddress(vaddr);
    size_t dir_entry = page_dir[eidx.dir_idx];

    if (!(dir_entry & FLG_DIR_PRESENT)) {
      init_table(eidx.dir_idx);
      dir_entry = page_dir[eidx.dir_idx];
    }
    size_t *table = (size_t *)(dir_entry & PAGE_ADDRESS_MASK);
    size_t table_entry = table[eidx.table_idx];

    if (!TABLE_ENTRY_FREE(table_entry)) {
      found_consecutive_entries = 0;
      continue;
    }

    found_consecutive_entries++;
    if (found_consecutive_entries == 1)
      found_start = vaddr;

    if (found_consecutive_entries == n_pages) {
      *res = entry_idx_from_vaddress(found_start);
      return 1;
    }
  }

  return 0;
}

static void reserve_page(struct entry_idx eidx) {
  size_t dir_entry = page_dir[eidx.dir_idx];
  ASSERT(dir_entry & FLG_DIR_PRESENT);

  size_t *table_entry = (size_t *)(page_dir[eidx.dir_idx] & PAGE_ADDRESS_MASK);
  table_entry[eidx.table_idx] |= FLG_TBL_RESERVED;
}
static int8_t map_page(size_t virtual, size_t physical) {
  struct entry_idx eidx = entry_idx_from_vaddress(virtual);
  size_t dir_entry = page_dir[eidx.dir_idx];

  size_t *page_table;
  if (dir_entry & FLG_DIR_PRESENT) {
    page_table = (size_t *)(page_dir[eidx.dir_idx] & PAGE_ADDRESS_MASK);
  } else {
    if (virtual >= vaddr_ranges[RANGE_IDENTITY].start &&
        virtual < vaddr_ranges[RANGE_IDENTITY].end) {
      page_table = pmm_get_page(0);
    } else {
      page_table = pmm_get_page(vaddr_ranges[RANGE_IDENTITY].end);
    }
    page_dir[eidx.dir_idx] =
        (size_t)page_table | (FLG_DIR_READ_WRITE | FLG_DIR_PRESENT);
  }

  if (!TABLE_ENTRY_FREE(page_table[eidx.table_idx])) {
    return 0;
  }

  page_table[eidx.table_idx] =
      (PAGE_ADDRESS_MASK & physical) |
      (FLG_TBL_RESERVED | FLG_TBL_READ_WRITE | FLG_TBL_PRESENT);

  return 1;
}

int8_t vmm_free_page(size_t virtual) {
  struct entry_idx eidx = entry_idx_from_vaddress(virtual);
  size_t dir_entry = page_dir[eidx.dir_idx];
  if (!(dir_entry & FLG_DIR_PRESENT))
    return 0;
  size_t *table = (size_t *)(dir_entry & PAGE_ADDRESS_MASK);

  size_t tbl_entry = table[eidx.table_idx];
  if (!(tbl_entry & FLG_DIR_PRESENT)) {
    table[eidx.table_idx] = 0; // to also unset reserved
    return 0;
  }

  size_t physical_addr = tbl_entry & PAGE_ADDRESS_MASK;
  if (!pmm_free_page(physical_addr))
    PANIC("vmm pointed to invalid physical page");

  *(size_t *)tbl_entry = 0x0;
  return 1;
}

void *vmm_get_n_pages(enum vaddr_range range, size_t n_pages) {
  struct entry_idx first_non_present_entry_idx;
  if (!find_n_non_free_entries(range, n_pages, &first_non_present_entry_idx))
    return 0;

  size_t first_vaddr = vaddress_from_entry_idx(first_non_present_entry_idx);
  for (size_t i = 0; i < n_pages; i++) {
    size_t virt_addr = first_vaddr + i * PAGE_SIZE;
    size_t phys_addr = (size_t)pmm_get_page(vaddr_ranges[RANGE_IDENTITY].end);
    if (!phys_addr)
      PANIC("out of physical memory");
    if (!map_page(virt_addr, phys_addr))
      PANIC("unable to map page");
  }

  return (void *)first_vaddr;
}

void *vmm_get_n_pages_protected(enum vaddr_range range, size_t n_pages) {

  struct entry_idx first_non_present_entry_idx;
  if (!find_n_non_free_entries(range, n_pages + 2,
                               &first_non_present_entry_idx))
    return 0;
  size_t first_vaddr = vaddress_from_entry_idx(first_non_present_entry_idx);
  for (size_t i = 0; i < n_pages + 2; i++) {
    size_t virt_addr = first_vaddr + i * PAGE_SIZE;
    if (i == 0 || i == n_pages + 1) {
      reserve_page(entry_idx_from_vaddress(virt_addr));
      continue;
    }
    size_t phys_addr = (size_t)pmm_get_page(vaddr_ranges[RANGE_IDENTITY].end);
    if (!phys_addr)
      PANIC("out of physical memory");
    if (!map_page(virt_addr, phys_addr))
      PANIC("unable to map page");
  }

  return (void *)(first_vaddr + PAGE_SIZE);
}

void *vmm_get_page(enum vaddr_range range) { return vmm_get_n_pages(range, 1); }

void vmm_init() {
  page_dir = pmm_get_page(0);

  // all directory entries RW
  for (int i = 0; i < N_DIRECTORY_ENTRIES; i++) {
    page_dir[i] = FLG_DIR_READ_WRITE;
  }

  // identity mapping the first 4MB
  for (size_t i = 0; i < N_PAGETABLE_ENTRIES; i++) {
    size_t addr = i * PAGE_SIZE;
    map_page(addr, addr);
  }

  __load_page_directory(page_dir);
  __enable_paging();
}
