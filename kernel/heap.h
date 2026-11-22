#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

void *kmalloc(size_t req_size);
int8_t kfree(void *ptr);
void *krealloc(void *ptr, size_t req_size);
void memncpy(const void *src, void *dst, size_t len);

#endif
