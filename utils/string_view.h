#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include "dynamic_array.h"
#include <stdbool.h>
#include <stddef.h>

DEFARR(char);

typedef struct {
  size_t len;
  const char *str;
} SV;

DEFARR(SV);

struct SV_builder {
  char_array char_array;
};

void svbuilder_write_char(struct SV_builder *b, char c);
void svbuilder_write_cstring(struct SV_builder *b, char *str);

bool sveq(SV a, const char *cstring);

SV svcat(SV a, SV b);
SV svbuild(struct SV_builder b);
SV svlitlen(char *str, size_t len);
SV svbuilder_view(struct SV_builder *b);
SV svsubstring(SV a, size_t start, size_t len);

#endif
