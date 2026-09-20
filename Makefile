TARGET     = build/os.img
BOOT_SRC   = boot/boot.asm
BOOT_BIN   = build/boot.bin

K_ENTRY    = kernel/kernel_entry.asm
K_STUBS    = kernel/stubs.asm
K_SRCS     = kernel/kernel.c kernel/screen.c kernel/idt.c \
             kernel/isr.c kernel/irq.c kernel/keyboard.c kernel/shell.c \
             kernel/splash.c kernel/mem.c

LINKER     = kernel/linker.ld
KERNEL_PE  = build/kernel.pe
KERNEL_BIN = build/kernel.bin
IMAGE      = build/os.img

CC         = gcc
CFLAGS     = -m32 -ffreestanding -fno-builtin -nostdlib \
             -fno-pie -fno-stack-protector -fno-leading-underscore \
             -fno-asynchronous-unwind-tables \
             -mgeneral-regs-only -mno-sse -mno-sse2 -mno-mmx \
             -Os -Ikernel

OBJS       = build/kernel_entry.o build/stubs.o \
             $(patsubst kernel/%.c, build/%.o, $(K_SRCS))

MAX_SECTORS = 30

all: $(IMAGE) verify

build:
	mkdir -p build

$(BOOT_BIN): $(BOOT_SRC) | build
	nasm -f bin $< -o $@

build/kernel_entry.o: $(K_ENTRY) | build
	nasm -f elf32 $< -o $@

build/stubs.o: $(K_STUBS) | build
	nasm -f elf32 $< -o $@

build/%.o: kernel/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_BIN): $(OBJS) $(LINKER) | build
	ld --image-base 0x0 -T $(LINKER) -o $(KERNEL_PE) $(OBJS)
	objcopy -O binary -j .text $(KERNEL_PE) $@

$(IMAGE): $(BOOT_BIN) $(KERNEL_BIN) | build
	dd if=/dev/zero     of=$(IMAGE) bs=512 count=2880 2>/dev/null
	dd if=$(BOOT_BIN)   of=$(IMAGE) conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$(IMAGE) seek=1 conv=notrunc 2>/dev/null

verify: $(KERNEL_BIN)
	$(eval KERNEL_SIZE    := $(shell wc -c < $(KERNEL_BIN)))
	$(eval KERNEL_SECTORS := $(shell echo $$(( ($(KERNEL_SIZE) + 511) / 512 ))))
	@echo "Kernel: $(KERNEL_SIZE) bytes = $(KERNEL_SECTORS) sectors"
	@if [ $(KERNEL_SECTORS) -gt $(MAX_SECTORS) ]; then \
		echo "ERROR: kernel too big!"; exit 1; fi
	@echo "OK: fits within $(MAX_SECTORS) sectors"

run: $(IMAGE)
	qemu-system-i386 -drive file=$(IMAGE),format=raw

clean:
	rm -rf build/
