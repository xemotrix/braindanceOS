#ifndef VGA_TEXT_H
#define VGA_TEXT_H

#include "../utils/string_view.h"
#include <stdint.h>

#define COLOR_BLACK 0
#define COLOR_BLUE 1
#define COLOR_GREEN 2
#define COLOR_CYAN 3
#define COLOR_RED 4
#define COLOR_MAGENTA 5
#define COLOR_BROWN 6
#define COLOR_LIGHTGREY 7
#define COLOR_DARKGREY 8
#define COLOR_LIGHTBLUE 9
#define COLOR_LIGHTGREEN 10
#define COLOR_LIGHTCYAN 11
#define COLOR_LIGHTRED 12
#define COLOR_LIGHTMAGENTA 13
#define COLOR_LIGHTBROWN 14
#define COLOR_WHITE 15

void reset_color();
void set_color(uint8_t fg, uint8_t bg);

void reset_cursor();
void set_cursor(uint16_t row, uint16_t col);

void cursor_advance();
void cursor_back();

void put_char(char c);
void put_number(int32_t num);
void put_hex(uint64_t num);
void put_hex_full(uint32_t num);
void put_nl();
void put_string(const char *msg);
void put_sv(SV sv);

void scroll();
void clear_screen();

#endif
