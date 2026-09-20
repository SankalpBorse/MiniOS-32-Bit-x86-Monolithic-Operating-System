ENTRY(_start)
SECTIONS {
    . = 0x10000;
    .text : {
        *(.entry)
        *(.text*)
        *(.rdata*)
        *(.rodata*)
        *(.data*)
    }
    .bss : {
        *(.bss*)
        *(COMMON)
    }
}
