#include "stdio.h"
#include "../drivers/vga_text.h"
#include <stdarg.h>
#include <stddef.h>

void kprintf(const char *format, ...) {
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
        put_char('%');
        break;
      case 'd':
        put_number(va_arg(args, int32_t));
        break;
      case 'u':
        put_number(va_arg(args, uint32_t));
        break;
      case 'x':
        put_hex(va_arg(args, uint32_t));
        break;
      case 'l':
        put_hex(va_arg(args, uint64_t));
        break;
      case 'c':
        put_char((char)va_arg(args, int));
        break;
      case 's':
        put_string(va_arg(args, const char *));
        break;
      case 'v':
        put_sv(va_arg(args, SV));
        break;
      }
      format++;
      continue;
    }
    put_char(c);
    format++;
  }

  va_end(args);
}
