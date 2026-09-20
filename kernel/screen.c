#include "screen.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

static int cursor_x = 0;
static int cursor_y = 0;
static uint8_t current_color = 0x07; // Light grey on black

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void scroll(void) {
    volatile uint16_t* vga = VGA_MEMORY;
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga[y * VGA_WIDTH + x] = vga[(y + 1) * VGA_WIDTH + x];
        }
    }

    // Blank the last row
    uint16_t blank = (uint16_t)' ' | ((uint16_t)current_color << 8);
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }

    cursor_y = VGA_HEIGHT - 1;
    cursor_x = 0;
}

void set_color(uint8_t fg, uint8_t bg) {
    // Upper nibble = background, lower nibble = foreground
    current_color = (uint8_t)((bg << 4) | (fg & 0x0F));
}

void clear_screen(void) {
    volatile uint16_t* vga = VGA_MEMORY;
    uint16_t blank = (uint16_t)' ' | ((uint16_t)current_color << 8);

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = blank;
    }

    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void screen_init(void) {
    set_color(VGA_LIGHT_GREY, VGA_BLACK);
    clear_screen();
}

void screen_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH) cursor_x = x;
    if (y >= 0 && y < VGA_HEIGHT) cursor_y = y;
    update_cursor();
}

void print_char(char c) {
    volatile uint16_t* vga = VGA_MEMORY;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)' ' | ((uint16_t)current_color << 8);
        }
    } else if (c == '\t') {
        cursor_x = (cursor_x + 4) & ~3;
    } else if ((uint8_t)c >= ' ') {
        vga[cursor_y * VGA_WIDTH + cursor_x] = (uint16_t)(uint8_t)c | ((uint16_t)current_color << 8);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        scroll();
    }

    update_cursor();
}

void print(const char* str) {
    if (!str) return;
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

void println(const char* str) {
    print(str);
    print_char('\n');
}

void print_hex(uint32_t n) {
    print("0x");
    static const char hex_chars[] = "0123456789ABCDEF";
    char buf[9];
    buf[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        buf[i] = hex_chars[n & 0xF];
        n >>= 4;
    }
    print(buf);
}

void print_dec(uint32_t n) {
    if (n == 0) {
        print_char('0');
        return;
    }
    char buf[11];
    int i = 10;
    buf[i] = '\0';
    while (n > 0) {
        buf[--i] = '0' + (n % 10);
        n /= 10;
    }
    print(&buf[i]);
}

