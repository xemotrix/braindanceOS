#include "heap.h"
#include "../drivers/vga_text.h"
#include "panic.h"
#include "vmm.h"

#include <stdbool.h>

#define PAGE_SIZE 0x1000
#define USED_FLAG 0x1
#define SIZE_MASK 0xfffffffe

#define ALLIGN 32
#define ALLIGN_T uint32_t
#define ROUND_UP_ALLIGN(x)                                                     \
  (((x) + sizeof(ALLIGN_T) - 1) & ~(sizeof(ALLIGN_T) - 1))

struct heap_header {
  size_t witness; // for debug
  size_t size;
  struct heap_header *next_free;
  struct heap_header *prev_free;
};

struct heap_header *first_free;
struct heap_header *heap_start;

static void check_heap(const char *msg) {
  put_string(msg);
  put_nl();
  struct heap_header *h = first_free;
  size_t block_num = 0;
  while (h) {
    if (h->witness != 0XFACEC0DE) {
      PANIC("missing 0XFACEC0DE\n");
    }
    if (h->size & USED_FLAG) {
      PANIC("got to non free block\n");
    }
    if (!h->prev_free && block_num > 0) {
      PANIC("backlink broken\n");
    }
    if (h->next_free) {
      if (h->next_free == h) {
        PANIC("next points to self\n");
      }
      if (h->next_free < h) {
        PANIC("next precedes self\n");
      }
    }
    if (h->prev_free) {
      if (h->prev_free == h) {
        PANIC("prev points to self\n");
      }
      if (h->prev_free > h) {
        PANIC("prev after self\n");
      }
    }
    block_num++;
    h = h->next_free;
  }
}

static inline void print_header(struct heap_header *hh) {
  put_string("HEADER at: ");
  put_hex((uint32_t)hh);
  if (hh->size & USED_FLAG)
    put_string(" (used)");
  else
    put_string(" (free)");
  put_nl();
  put_string("  h->size: ");
  put_number((uint32_t)hh->size & SIZE_MASK);
  put_nl();
  put_string("  h->prev: ");
  put_hex((uint32_t)hh->prev_free);
  put_nl();
  put_string("  h->next_free: ");
  put_hex((uint32_t)hh->next_free);
  put_nl();
}

static void debug_heap() {
  struct heap_header *h = first_free;
  check_heap("debug_header");
  while (h) {
    put_string("HEADER ");
    put_hex((size_t)h);
    put_string(": size:");
    put_hex_full((size_t)h->size);
    put_string(" next:");
    put_hex_full((size_t)h->next_free);
    put_string(" prev:");
    put_hex_full((size_t)h->prev_free);
    put_nl();
    h = h->next_free;
  }
}

void init_heap() {
  struct heap_header *hh = vmm_get_page(RANGE_HEAP);
  heap_start = hh;
  *hh = (struct heap_header){
      .witness = 0XFACEC0DE,
      .size = PAGE_SIZE - sizeof(struct heap_header),
      .next_free = 0,
      .prev_free = 0,
  };
  heap_start = hh;
  first_free = hh;
}

static int8_t add_heap_page(struct heap_header *hh) {
  // assume get_page gets contiguous page in range
  if (!vmm_get_page(RANGE_HEAP))
    return 0;
  hh->size += PAGE_SIZE;
  return 1;
}

void memncpy(void *src, void *dst, size_t len) {
  char *dst_char = (char *)dst;
  char *src_char = (char *)src;
  for (size_t i = 0; i < len; i++) {
    dst_char[i] = src_char[i];
  }
}

void *krealloc(void *ptr, size_t req_size) {
  if (!first_free)
    init_heap();

  if (!ptr)
    return kmalloc(req_size);

  struct heap_header *to_realloc = ((struct heap_header *)ptr) - 1;
  size_t to_realloc_size = to_realloc->size & SIZE_MASK;
  if (req_size <= to_realloc_size)
    return ptr;

  ALLIGN_T *new_mem = kmalloc(req_size);
  if (!new_mem)
    return 0;

  ALLIGN_T *sized_ptr = ptr;
  for (size_t i = 0; i < to_realloc_size / sizeof(*sized_ptr); i++) {
    new_mem[i] = sized_ptr[i];
  }

  if (!kfree(ptr))
    return 0;
  return new_mem;
}

void *kmalloc(size_t req_size) {
  if (!first_free)
    init_heap();

  struct heap_header *h = first_free;
  for (;;) {
    size_t block_size = h->size & SIZE_MASK;
    if (req_size > block_size) {
      if (!h->next_free)
        add_heap_page(h);
      else
        h = h->next_free;
      continue;
    }

    if (req_size + ALLIGN + sizeof(struct heap_header) >= block_size) {
      if (h == first_free) {
        if (!h->next_free)
          PANIC("out of stuff!\n");

        first_free = h->next_free;
      }
      if (h->prev_free)
        h->prev_free->next_free = h->next_free;
      if (h->next_free)
        h->next_free->prev_free = h->prev_free;
      h->size |= USED_FLAG;
      h->next_free = (void *)0xDEAD;
      h->prev_free = (void *)0xDEAD;
      return (void *)(h + 1);
    }

    size_t req_alligned =
        ROUND_UP_ALLIGN(req_size + sizeof(struct heap_header));

    struct heap_header *new_free =
        (struct heap_header *)((size_t)h + req_alligned);
    *new_free = (struct heap_header){
        .witness = 0XFACEC0DE,
        .size = block_size - req_alligned - sizeof(struct heap_header),
        .next_free = h->next_free,
        .prev_free = h->prev_free,
    };

    if (h->prev_free)
      h->prev_free->next_free = new_free;
    else
      first_free = new_free;
    if (h->next_free)
      h->next_free->prev_free = new_free;

    h->size = (req_alligned - sizeof(struct heap_header)) | USED_FLAG;
    h->next_free = (void *)0xDEAD;
    h->prev_free = (void *)0xDEAD;

    return (void *)(h + 1);
  }

  return 0;
}

int8_t kfree(void *ptr) {
  struct heap_header *to_free_hh = ((struct heap_header *)ptr) - 1;
  if (!(to_free_hh->size & USED_FLAG)) {
    PANIC("trying to free already free block");
  }

  to_free_hh->size &= ~USED_FLAG;

  // TODO: defragmentation
  struct heap_header *hh = first_free;
  for (;;) {
    if ((size_t)hh < (size_t)to_free_hh) {
      if (!hh->next_free) {
        hh->next_free = to_free_hh;
        to_free_hh->prev_free = hh;
        to_free_hh->next_free = 0;
        break;
      }
      hh = hh->next_free;
      continue;
    }

    if (hh->prev_free) {
      hh->prev_free->next_free = to_free_hh;
      to_free_hh->prev_free = hh->prev_free;
    } else {
      first_free = to_free_hh;
      to_free_hh->prev_free = 0;
    }

    hh->prev_free = to_free_hh;
    to_free_hh->next_free = hh;

    break;
  }
  return 1;
}
