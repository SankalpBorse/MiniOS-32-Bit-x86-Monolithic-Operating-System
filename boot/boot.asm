[BITS 16]
[ORG 0x7C00]

start:
    ; Critical Bug 4 fix: Save boot drive number passed by BIOS in DL
    mov [BOOT_DRIVE], dl
    mov [0x7006], dl

    ; Setup real-mode segments and stack safely
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; -------------------------------------------------------------------------
    ; Milestone 9: BIOS Memory Detection
    ; -------------------------------------------------------------------------
    ; 1. Conventional Memory: INT 0x12 -> returns AX in KB
    int 0x12
    mov [0x7000], ax

    ; 2. Extended Memory: INT 0x15, AX=0xE801
    xor cx, cx
    xor dx, dx
    mov ax, 0xE801
    int 0x15
    jc .mem_fallback
    test ax, ax
    jnz .mem_store
    mov ax, cx
    mov bx, dx
    jmp .mem_store

.mem_fallback:
    ; Fallback: INT 0x15, AH=0x88 (returns extended memory in KB in AX)
    mov ah, 0x88
    int 0x15
    xor bx, bx

.mem_store:
    mov [0x7002], ax        ; 1MB to 16MB in KB
    mov [0x7004], bx        ; Above 16MB in 64KB blocks

    ; Print "BOOT OK" via BIOS INT 0x10
    mov si, msg_boot
    call print_string

    ; Load kernel sectors to 0x1000:0x0000 (0x10000 physical)
    mov ax, 0x1000
    mov es, ax
    xor bx, bx          ; ES:BX = 0x1000:0x0000

    mov ah, 0x02        ; BIOS read sectors
    mov al, 30          ; Read 30 sectors
    mov ch, 0           ; Cylinder 0
    mov cl, 2           ; Start sector 2
    mov dh, 0           ; Head 0
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error

    ; Print "LOAD OK" via BIOS INT 0x10
    mov si, msg_load
    call print_string

    ; -------------------------------------------------------------------------
    ; Transition to 32-bit Protected Mode
    ; -------------------------------------------------------------------------
    cli                 ; 1. Disable real mode interrupts

    ; 2. Enable A20 gate via Fast A20 gate (port 0x92)
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 3. Load Global Descriptor Table (GDT)
    lgdt [gdt_descriptor]

    ; 4. Set Protected Mode bit (PE) in CR0
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 5. Far jump to flush 16-bit prefetch queue and enter 32-bit code
    jmp CODE_SEG:init_pm

disk_error:
    mov si, msg_disk_err
    call print_string
hang_rm:
    hlt
    jmp hang_rm

; -----------------------------------------------------------------------------
; 16-bit Real Mode String Print Routine
; -----------------------------------------------------------------------------
print_string:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

; -----------------------------------------------------------------------------
; Global Descriptor Table (GDT)
; -----------------------------------------------------------------------------
gdt_start:
gdt_null:               ; Mandatory null descriptor (8 bytes)
    dd 0x0
    dd 0x0

gdt_code:               ; 32-bit Kernel Code Segment Descriptor
    dw 0xFFFF           ; Limit (bits 0-15) = 4GB (with 4K granularity)
    dw 0x0000           ; Base (bits 0-15) = 0x0
    db 0x00             ; Base (bits 16-23) = 0x0
    db 10011010b        ; Access: Present=1, Priv=00, Descr=1, Exec=1, Conf=0, Read=1, Acc=0
    db 11001111b        ; Flags: Granularity=1 (4K), 32-bit=1, 64-bit=0, AVL=0 | Limit (16-19)=0xF
    db 0x00             ; Base (bits 24-31) = 0x0

gdt_data:               ; 32-bit Kernel Data Segment Descriptor
    dw 0xFFFF           ; Limit (bits 0-15) = 4GB
    dw 0x0000           ; Base (bits 0-15) = 0x0
    db 0x00             ; Base (bits 16-23) = 0x0
    db 10010010b        ; Access: Present=1, Priv=00, Descr=1, Exec=0, Dir=0, Write=1, Acc=0
    db 11001111b        ; Flags: Granularity=1 (4K), 32-bit=1, 64-bit=0, AVL=0 | Limit (16-19)=0xF
    db 0x00             ; Base (bits 24-31) = 0x0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1   ; GDT size minus 1
    dd gdt_start                 ; GDT linear address

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; -----------------------------------------------------------------------------
; Data Section (16-bit)
; -----------------------------------------------------------------------------
BOOT_DRIVE:   db 0
msg_boot:     db "BOOT OK", 0x0D, 0x0A, 0
msg_load:     db "LOAD OK", 0x0D, 0x0A, 0
msg_disk_err: db "DISK ERROR", 0x0D, 0x0A, 0

; -----------------------------------------------------------------------------
; 32-bit Protected Mode Entry Point
; -----------------------------------------------------------------------------
[BITS 32]
init_pm:
    ; Reload segment registers with DATA_SEG (0x10)
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Setup 32-bit protected mode stack
    mov ebp, 0x90000
    mov esp, ebp

    ; Jump directly to the kernel loaded at 0x10000
    jmp CODE_SEG:0x10000

; -----------------------------------------------------------------------------
; Boot Sector Signature
; -----------------------------------------------------------------------------
times 510 - ($ - $$) db 0
dw 0xAA55
