#ifndef PANIC_H
#define PANIC_H

#include "../kernel/irq.h"
#include "../utils/stdio.h"

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

#define FILE_LOC __FILE__ ":" STRINGIFY(__LINE__)

#define PANIC(fmt, ...)                                                        \
  do {                                                                         \
    CLI;                                                                       \
    kprintf("panic at " FILE_LOC ": ");                                        \
    kprintf(fmt __VA_OPT__(, ) __VA_ARGS__);                                   \
    for (;;) {                                                                 \
      HLT;                                                                     \
    }                                                                          \
  } while (0)

#define ASSERT(expr)                                                           \
  do {                                                                         \
    if (!(expr)) {                                                             \
      PANIC("assert failed: " #expr);                                          \
    }                                                                          \
  } while (0);

#define UNREACHABLE PANIC("UNREACHABLE");

#endif
