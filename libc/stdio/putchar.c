#ifdef __is_libk
#include <drivers/vga_text.h>
#endif

#ifdef __is_libk
int putchar(int c) {
  vga_put_char((char)c);
  return (unsigned char)c;
}
#else
#warning "libc userspace not implemented"
#endif
