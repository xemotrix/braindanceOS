#include <stddef.h>

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
