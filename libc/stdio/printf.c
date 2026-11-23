#ifdef __is_libk
#include <drivers/vga_text.h>
#include <stdarg.h>
#include <stdio.h>

void printf(const char *restrict format, ...) {
  va_list args;
  va_start(args, format);

  while (*format) {
    char c = *format;
    if (c == '%') {
      format++;
      switch (*format) {
      case '\0':
        break;
      case '%':
        putchar('%');
        break;
      case 'd':
        vga_put_number(va_arg(args, int32_t));
        break;
      case 'u':
        vga_put_number(va_arg(args, uint32_t));
        break;
      case 'x':
        vga_put_hex(va_arg(args, uint32_t));
        break;
      case 'l':
        vga_put_hex(va_arg(args, uint64_t));
        break;
      case 'c':
        putchar((char)va_arg(args, int));
        break;
      case 's':
        puts(va_arg(args, const char *));
        break;
      case 'v':
        vga_put_sv(va_arg(args, SV));
        break;
      }
      format++;
      continue;
    }
    putchar(c);
    format++;
  }

  va_end(args);
}
#else
#warning "libc userspace not implemented"
#endif
