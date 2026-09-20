#ifndef IDT_H
#define IDT_H

#include "types.h"

// IDT gate entry structure (8 bytes)
struct idt_entry {
    uint16_t base_low;   // Lower 16 bits of handler address
    uint16_t sel;        // Kernel segment selector (0x08)
    uint8_t  always0;    // Reserved, always 0
    uint8_t  flags;      // Type and attributes (0x8E = 32-bit Interrupt Gate)
    uint16_t base_high;  // Upper 16 bits of handler address
} __attribute__((packed));

// Pointer structure passed to LIDT instruction (6 bytes)
struct idt_ptr {
    uint16_t limit;      // Table limit (size - 1)
    uint32_t base;       // Base address of the IDT
} __attribute__((packed));

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void idt_init(void);

#endif

