#ifndef VMM_H
#define VMM_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 0x1000

enum vaddr_range {
  RANGE_IDENTITY,
  RANGE_HEAP,
  RANGE_MAX,
};

void vmm_init();
void *vmm_get_page(enum vaddr_range range);
void *vmm_get_n_pages(enum vaddr_range range, size_t n_pages);
void *vmm_get_n_pages_protected(enum vaddr_range range, size_t n_pages);

int8_t vmm_free_page(size_t addr);

#endif
