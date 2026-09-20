#include "irq.h"
#include "idt.h"
#include "screen.h"

static isr_t irq_routines[16] = { 0 };

extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void io_wait(void) {
    __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

void irq_install_handler(int irq, isr_t handler) {
    irq_routines[irq] = handler;
}

void irq_uninstall_handler(int irq) {
    irq_routines[irq] = 0;
}

static void irq_remap(void) {
    // ICW1: Start initialization sequence
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    // ICW2: Master PIC vector offset 0x20 (32), Slave PIC offset 0x28 (40)
    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();

    // ICW3: Master has slave on IRQ2 (0x04), Slave cascade identity (0x02)
    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();

    // ICW4: 8086/88 mode
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    // Unmask IRQ0 (timer) and IRQ1 (keyboard); mask all slave IRQs
    outb(0x21, 0xFC);   /* 11111100 — unmask IRQ0 timer, IRQ1 keyboard */
    outb(0xA1, 0xFF);   /* mask all slave for now */
}

void irq_init(void) {
    irq_remap();

    // Install gates for IRQ 0-15 (INT 32-47)
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);
}

void irq_handler(registers_t* regs) {
    int irq = regs->int_no - 32;

    if (irq >= 0 && irq < 16 && irq_routines[irq] != 0) {
        irq_routines[irq](regs);
    }

    // Send EOI to Slave if applicable
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);
    }
    // Always send EOI to Master
    outb(0x20, 0x20);
}

static volatile uint32_t timer_ticks = 0;

static void timer_callback(registers_t* regs) {
    (void)regs;
    timer_ticks++;

    // Every ~18 ticks is 1 second with standard PIT frequency (~18.22 Hz)
    uint32_t seconds = timer_ticks / 18;

    // Write changing digit to top-right VGA cell (offset 158 = char, 159 = color)
    volatile uint8_t* vga = (volatile uint8_t*)0xB8000;
    vga[158] = '0' + (seconds % 10);
    vga[159] = 0x0A; // Bright green
}

void timer_init(void) {
    irq_install_handler(0, timer_callback);
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

void sleep_ticks(uint32_t ticks) {
    uint32_t start = timer_ticks;
    while ((timer_ticks - start) < ticks) {
        __asm__ volatile ("hlt");
    }
}

