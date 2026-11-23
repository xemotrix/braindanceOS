#include "string_view.h"
#include <kernel/heap.h>

void svbuilder_write_char(struct SV_builder *b, char c) {
  //
  DA_APPEND(b->char_array, c);
}

void svbuilder_write_cstring(struct SV_builder *b, char *str) {
  for (size_t i = 0; str[i]; i++) {
    svbuilder_write_char(b, str[i]);
  }
}

SV svlitlen(char *str, size_t len) {
  return (SV){
      .str = str,
      .len = len,
  };
}

bool sveq(SV a, const char *cstring) {
  for (size_t i = 0; i < a.len; i++) {
    char ac = a.str[i];
    char sc = cstring[i];
    if (!sc)
      return false;
    if (ac != sc)
      return false;
  }
  return true;
}

SV svsubstring(SV a, size_t start, size_t len) {
  if (start + len > a.len) {
    return (SV){.str = 0};
  }

  return (SV){
      .str = a.str + start,
      .len = len,
  };
}

SV svcat(SV a, SV b) {
  size_t tot_len = a.len + b.len;
  char *bfr = kmalloc(tot_len);
  memncpy(a.str, bfr, a.len);
  memncpy(b.str, (char *)bfr + a.len, b.len);
  return (SV){
      .str = bfr,
      .len = tot_len,
  };
}

SV svbuilder_view(struct SV_builder *b) {
  if (b->char_array.len >= b->char_array.cap) {
    DA_APPEND(b->char_array, '\0');
    b->char_array.len--;
  } else {
    b->char_array.elts[b->char_array.len] = '\0';
  }

  return (SV){
      .len = b->char_array.len + 1,
      .str = b->char_array.elts,
  };
}

char *svcstring(SV sv) {
  char *buf = kmalloc(sv.len + 1);
  memncpy(sv.str, buf, sv.len);
  buf[sv.len] = 0;
  return buf;
}

SV svbuild(struct SV_builder b) {
  size_t len = b.char_array.len;
  char *new_str = kmalloc(len);

  if (!new_str)
    return (SV){.len = 42};

  memncpy(b.char_array.elts, new_str, b.char_array.len);

  return (SV){
      .len = len,
      .str = new_str,
  };
}
