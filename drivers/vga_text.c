#include "vga_text.h"
#include "ports.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define DEFAULT_COLOR (COLOR_RED | (COLOR_BLACK << 4))

uint16_t *video_mem = (uint16_t *)0xb8000;
uint16_t cursor = 0;
uint8_t color = DEFAULT_COLOR;

static void update_hardware_cursor() {
  outb(0x3D4, 0x0E);
  outb(0x3D5, (cursor >> 8) & 0xFF);
  outb(0x3D4, 0x0F);
  outb(0x3D5, cursor & 0xFF);
}

void clear_screen() {
  for (int i = 0; i < VGA_HEIGHT * VGA_WIDTH; i++)
    put_char(' ');
}

void scroll() {
  for (int i = 0; i < VGA_HEIGHT - 1; i++) {
    for (int j = 0; j < VGA_WIDTH; j++) {
      video_mem[i * VGA_WIDTH + j] = video_mem[(i + 1) * VGA_WIDTH + j];
    }
  }

  for (int i = 0; i < VGA_WIDTH; i++) {
    video_mem[(VGA_HEIGHT - 1) * VGA_WIDTH + i] = ' ' | (color << 8);
  }

  cursor -= VGA_WIDTH;
}

void reset_color() { color = DEFAULT_COLOR; }
void set_color(uint8_t fg, uint8_t bg) { color = fg | (bg << 4); }
void reset_cursor() {
  cursor = 0;
  update_hardware_cursor();
}
void set_cursor(uint16_t row, uint16_t col) {
  cursor = row * VGA_WIDTH + col;
  update_hardware_cursor();
}

void cursor_advance() {
  if (cursor >= VGA_WIDTH * VGA_HEIGHT)
    return;
  cursor++;
  update_hardware_cursor();
}
void cursor_back() {
  if (cursor <= 0)
    return;
  cursor--;
  update_hardware_cursor();
}

void put_char(char c) {
  if (c == '\n') {
    cursor += 80 - (cursor % 80);
    update_hardware_cursor();
    return;
  }
  if (cursor / VGA_WIDTH == VGA_HEIGHT) {
    scroll();
  }
  video_mem[cursor++] = c | (color << 8);
  update_hardware_cursor();
}

void put_nl() { put_char('\n'); }

void put_number(int32_t num) {
  // INT32_MAX IS 2147483647
  // so max size is len(2147483647) + 1 (sign) + 1(\0)
  // ==12
  char buf[12] = {0};

  if (num == 0) {
    buf[0] = '0';
    put_string(buf);
    return;
  }

  int is_negative = num < 0;

  if (num < 0)
    num = -num;

  int idx = 0;
  while (num != 0) {
    int digit = num % 10;
    buf[idx++] = '0' + ((char)digit);
    num /= 10;
  }

  if (is_negative)
    buf[idx++] = '-';

  for (int i = 0; i < idx / 2; i++) {
    char aux = buf[i];
    buf[i] = buf[idx - i - 1];
    buf[idx - i - 1] = aux;
  }

  put_string(buf);
}

char hex_chars[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

void put_hex(uint64_t num) {
  // UINT64_MAX IS 0xffffffffffffffff
  // so max size 18+\0
  char buf[19] = {0};
  int idx = 0;
  if (num == 0) {
    buf[idx++] = '0';
    buf[idx++] = 'x';
    buf[idx++] = '0';
    put_string(buf);
    return;
  }
  while (num != 0) {
    buf[idx++] = hex_chars[num & 0xf];
    num >>= 4;
  }
  buf[idx++] = 'x';
  buf[idx++] = '0';

  for (int i = 0; i < idx / 2; i++) {
    char aux = buf[i];
    buf[i] = buf[idx - i - 1];
    buf[idx - i - 1] = aux;
  }
  put_string(buf);
}

void put_hex_full(uint32_t num) {
  // UINT32_MAX IS 0xffffffff
  // so max size 10+\0
  char buf[11] = {0};
  int idx = 0;

  for (size_t i = 0; i < 8; i++) {
    // while (num != 0) {
    buf[idx++] = hex_chars[num & 0xf];
    num >>= 4;
  }
  buf[idx++] = 'x';
  buf[idx++] = '0';

  for (int i = 0; i < idx / 2; i++) {
    char aux = buf[i];
    buf[i] = buf[idx - i - 1];
    buf[idx - i - 1] = aux;
  }
  put_string(buf);
}

void put_string(const char *msg) {
  int i = 0;
  char c;
  while ((c = msg[i++])) {
    put_char(c);
  }
}

void put_sv(SV sv) {
  for (size_t i = 0; i < sv.len; i++) {
    put_char(sv.str[i]);
  }
}
