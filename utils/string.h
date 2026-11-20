#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t strlen(const char *str);
void *memcpy(void *dest, const void *src, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);

#endif
