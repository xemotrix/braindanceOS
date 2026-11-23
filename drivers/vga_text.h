#ifndef VGA_TEXT_H
#define VGA_TEXT_H

#include <stdint.h>
#include <utils/string_view.h>

#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHTGREY 7
#define VGA_COLOR_DARKGREY 8
#define VGA_COLOR_LIGHTBLUE 9
#define VGA_COLOR_LIGHTGREEN 10
#define VGA_COLOR_LIGHTCYAN 11
#define VGA_COLOR_LIGHTRED 12
#define VGA_COLOR_LIGHTMAGENTA 13
#define VGA_COLOR_LIGHTBROWN 14
#define VGA_COLOR_WHITE 15

void vga_reset_cursor();
void vga_set_cursor(uint16_t row, uint16_t col);

void vga_cursor_advance();
void vga_cursor_back();

void vga_put_char(char c);
void vga_put_number(int32_t num);
void vga_put_hex(uint64_t num);
void vga_put_hex_full(uint32_t num);
void vga_put_sv(SV sv);

void vga_scroll();
void vga_clear_screen();

#endif
