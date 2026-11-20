#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>

#define RBDEF(type, size)                                                      \
  typedef struct {                                                             \
    uint16_t write_idx;                                                        \
    uint16_t read_idx;                                                         \
    type elts[size];                                                           \
  } type##_RB

#define rb_write(bfr, elt)                                                     \
  do {                                                                         \
    (bfr).elts[(bfr).write_idx++] = (elt);                                     \
    if ((bfr).write_idx >= sizeof((bfr).elts) / sizeof(*(bfr).elts))           \
      (bfr).write_idx %= sizeof((bfr).elts) / sizeof(*(bfr).elts);             \
  } while (0);

// requires gcc
#define rb_read(bfr)                                                           \
  ({                                                                           \
    if (bfr.read_idx >= sizeof((bfr).elts) / sizeof(*(bfr).elts))              \
      bfr.read_idx %= sizeof((bfr).elts) / sizeof(*(bfr).elts);                \
    bfr.elts[bfr.read_idx++];                                                  \
  })

#endif
