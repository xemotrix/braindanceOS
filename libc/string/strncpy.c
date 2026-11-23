#include <stddef.h>

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
