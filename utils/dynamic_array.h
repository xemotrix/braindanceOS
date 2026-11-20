#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "../kernel/heap.h"
#include <stddef.h>

#define DEFARR(type)                                                           \
  typedef struct {                                                             \
    size_t len;                                                                \
    size_t cap;                                                                \
    type *elts;                                                                \
  } type##_array

#define DEFARR_STRUCT(type)                                                    \
  typedef struct {                                                             \
    size_t len;                                                                \
    size_t cap;                                                                \
    struct type *elts;                                                         \
  } type##_array

#define DA_FREE(xs)                                                            \
  do {                                                                         \
    kfree((xs).elts);                                                          \
    (xs).elts = 0;                                                             \
    (xs).len = 0;                                                              \
    (xs).cap = 0;                                                              \
  } while (0)

#define DA_APPEND(xs, x)                                                       \
  do {                                                                         \
    if ((xs).len >= (xs).cap) {                                                \
      if (!xs.cap)                                                             \
        (xs).cap = 16;                                                         \
      else                                                                     \
        (xs).cap *= 2;                                                         \
      (xs).elts = krealloc((xs).elts, (xs).cap * sizeof(*(xs).elts));          \
    }                                                                          \
    (xs).elts[(xs).len++] = (x);                                               \
  } while (0)

#endif
