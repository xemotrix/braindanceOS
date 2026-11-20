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

int strcmp(const char *s1, const char *s2) {
  for (;;) {
    char c1 = *s1++;
    char c2 = *s2++;
    if (c1 == 0 && c2 == 0)
      return 0;
    if (c1 < c2)
      return -1;
    if (c1 > c2)
      return 1;
  }
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    char c1 = *s1++;
    char c2 = *s2++;
    if (c1 == 0 && c2 == 0)
      return 0;
    if (c1 < c2)
      return -1;
    if (c1 > c2)
      return 1;
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  for (size_t i = 0; i < n; i++) {
    *(uint8_t *)s = (uint8_t)c;
  }
  return s;
}
