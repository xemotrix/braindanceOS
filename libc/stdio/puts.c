#ifdef __is_libk
#include <drivers/vga_text.h>
#include <stdio.h>

int puts(const char *s) {
  char c;
  while ((c = *s++))
    putchar(c);
  return 1;
}

#else
#warning "libc userspace not implemented"
#endif
