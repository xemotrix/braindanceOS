#include <stddef.h>

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
