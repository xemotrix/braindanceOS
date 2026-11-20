#include "string.h"
#include <stdint.h>

size_t strlen(const char *str) {
  size_t len = 0;
  while (str[len])
    len++;
  return len;
}

void *memcpy(void *dest, const void *src, size_t n) {
  for (size_t i = 0; i < n; i++) {
    ((uint8_t *)dest)[i] = ((uint8_t *)src)[i];
  }
  return dest;
}

char *strcpy(char *dest, const char *src) {
  char *dst_aux = dest;
  while ((*dst_aux++ = *src++)) {
  }
  return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
  bool found_term = false;
  for (size_t i = 0; i < n; i++) {
    if (found_term) {
      dest[i] = 0;
      continue;
    }
    dest[i] = src[i];
    if (!dest[i])
      found_term = true;
  }
  return dest;
}

void *memset(void *s, int c, size_t n) {
  for (size_t i = 0; i < n; i++) {
    *(uint8_t *)s = (uint8_t)c;
  }
  return s;
}
