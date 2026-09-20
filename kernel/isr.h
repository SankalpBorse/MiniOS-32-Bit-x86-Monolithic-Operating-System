#ifndef ISR_H
#define ISR_H

#include "types.h"

// Structure defining CPU state pushed onto stack by assembly stubs
typedef struct registers {
    uint32_t ds;                                     // Data segment selector
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha
    uint32_t int_no, err_code;                       // Interrupt number and error code
    uint32_t eip, cs, eflags, useresp, ss;           // Pushed by CPU automatically
} registers_t;

typedef void (*isr_t)(registers_t*);

void isr_init(void);
void register_interrupt_handler(uint8_t n, isr_t handler);

#endif

