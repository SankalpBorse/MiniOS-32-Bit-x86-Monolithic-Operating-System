#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"

enum vga_color {
    VGA_BLACK         = 0,
    VGA_BLUE          = 1,
    VGA_GREEN         = 2,
    VGA_CYAN          = 3,
    VGA_RED           = 4,
    VGA_MAGENTA       = 5,
    VGA_BROWN         = 6,
    VGA_LIGHT_GREY    = 7,
    VGA_DARK_GREY     = 8,
    VGA_LIGHT_BLUE    = 9,
    VGA_LIGHT_GREEN   = 10,
    VGA_LIGHT_CYAN    = 11,
    VGA_LIGHT_RED     = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW        = 14,
    VGA_WHITE         = 15,
};

#define COLOR_BLACK         VGA_BLACK
#define COLOR_BLUE          VGA_BLUE
#define COLOR_GREEN         VGA_GREEN
#define COLOR_CYAN          VGA_CYAN
#define COLOR_RED           VGA_RED
#define COLOR_MAGENTA       VGA_MAGENTA
#define COLOR_BROWN         VGA_BROWN
#define COLOR_LIGHT_GREY    VGA_LIGHT_GREY
#define COLOR_DARK_GREY     VGA_DARK_GREY
#define COLOR_LIGHT_BLUE    VGA_LIGHT_BLUE
#define COLOR_LIGHT_GREEN   VGA_LIGHT_GREEN
#define COLOR_LIGHT_CYAN    VGA_LIGHT_CYAN
#define COLOR_LIGHT_RED     VGA_LIGHT_RED
#define COLOR_LIGHT_MAGENTA VGA_LIGHT_MAGENTA
#define COLOR_YELLOW        VGA_YELLOW
#define COLOR_WHITE         VGA_WHITE

#define MAKE_COLOR(bg, fg) (fg), (bg)

void screen_init(void);
void clear_screen(void);
void set_color(uint8_t fg, uint8_t bg);
void screen_set_cursor(int x, int y);
void print_char(char c);
void print(const char* str);
void println(const char* str);
void print_dec(uint32_t n);
void print_hex(uint32_t n);

#endif
