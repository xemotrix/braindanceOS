char *strcpy(char *dest, const char *src) {
  char *dst_aux = dest;
  while ((*dst_aux++ = *src++)) {
  }
  return dest;
}
