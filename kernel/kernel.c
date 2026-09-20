#include "types.h"
#include "screen.h"
#include "idt.h"
#include "isr.h"
#include "irq.h"
#include "keyboard.h"
#include "shell.h"
#include "splash.h"
#include "mem.h"

void kernel_main(void) {
    screen_init();

    // Initialize IDT and Exception handlers
    idt_init();
    isr_init();

    // Initialize PIC and Hardware IRQs
    irq_init();
    timer_init();

    // Initialize PS/2 Keyboard
    keyboard_init();

    // Initialize Kernel Dynamic Memory Heap at 1MB (0x100000)
    kmem_init();

    // Enable hardware interrupts so timer and keyboard work
    __asm__ volatile ("sti");

    // Display the Welcome Splash Screen with animated progress bar
    splash_screen();

    // Reset screen to standard theme and launch shell
    screen_init();
    shell_init();
    shell_run();

    while (1) {
        __asm__ volatile ("hlt");
    }
}
