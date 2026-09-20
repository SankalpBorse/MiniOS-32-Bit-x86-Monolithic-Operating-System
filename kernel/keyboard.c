#include "keyboard.h"
#include "isr.h"
#include "irq.h"
#include "screen.h"

#define KEY_BUFFER_SIZE 256
static char key_buffer[KEY_BUFFER_SIZE];
static volatile int key_head = 0;
static volatile int key_tail = 0;

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Scancode table for standard US QWERTY keyboard (Make codes 0x00 - 0x39)
static const char scancode_ascii[128] = {
    0,   27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

// Shifted scancode table
static const char scancode_ascii_shift[128] = {
    0,   27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static int shift_active = 0;

static void keyboard_enqueue(char c) {
    int next = (key_head + 1) % KEY_BUFFER_SIZE;
    if (next != key_tail) {
        key_buffer[key_head] = c;
        key_head = next;
    }
}

char keyboard_getchar(void) {
    while (key_head == key_tail) {
        __asm__ volatile ("hlt");
    }
    char c = key_buffer[key_tail];
    key_tail = (key_tail + 1) % KEY_BUFFER_SIZE;
    return c;
}

static void keyboard_callback(registers_t* regs) {
    (void)regs;
    uint8_t scancode = inb(0x60);

    // Track shift key press and release
    if (scancode == 0x2A || scancode == 0x36) {        // Left or Right Shift pressed
        shift_active = 1;
        outb(0x20, 0x20);
        return;
    } else if (scancode == (0x2A | 0x80) || scancode == (0x36 | 0x80)) { // Shift released
        shift_active = 0;
        outb(0x20, 0x20);
        return;
    }

    // Filter break code (key release events have bit 7 set)
    if (scancode & 0x80) {
        outb(0x20, 0x20);
        return;
    }

    char c = 0;
    if (scancode < 128) {
        c = shift_active ? scancode_ascii_shift[scancode] : scancode_ascii[scancode];
    }

    if (c != 0) {
        keyboard_enqueue(c);
    }

    // Send EOI to Master PIC
    outb(0x20, 0x20);
}

void keyboard_init(void) {
    key_head = 0;
    key_tail = 0;
    shift_active = 0;
    irq_install_handler(1, keyboard_callback);
}
