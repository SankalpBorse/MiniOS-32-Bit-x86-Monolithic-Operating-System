#include "shell.h"
#include "types.h"
#include "screen.h"
#include "keyboard.h"
#include "irq.h"
#include "mem.h"

#define CMD_BUFFER_SIZE 128

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static void print_prompt(void) {
    set_color(VGA_YELLOW, VGA_BLACK);
    print("> ");
    set_color(VGA_WHITE, VGA_BLACK);
}

static void get_cpu_vendor(char* buf) {
    uint32_t ebx, ecx, edx;
    __asm__ volatile(
        "cpuid"
        : "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );
    // EBX (bytes 0-3)
    buf[0] = (char)(ebx & 0xFF);
    buf[1] = (char)((ebx >> 8) & 0xFF);
    buf[2] = (char)((ebx >> 16) & 0xFF);
    buf[3] = (char)((ebx >> 24) & 0xFF);
    // EDX (bytes 4-7) - Note: EDX comes before ECX in CPUID vendor string
    buf[4] = (char)(edx & 0xFF);
    buf[5] = (char)((edx >> 8) & 0xFF);
    buf[6] = (char)((edx >> 16) & 0xFF);
    buf[7] = (char)((edx >> 24) & 0xFF);
    // ECX (bytes 8-11)
    buf[8] = (char)(ecx & 0xFF);
    buf[9] = (char)((ecx >> 8) & 0xFF);
    buf[10] = (char)((ecx >> 16) & 0xFF);
    buf[11] = (char)((ecx >> 24) & 0xFF);
    buf[12] = '\0';
}

static void cmd_sysinfo(void) {
    char cpu_vendor[13];
    get_cpu_vendor(cpu_vendor);

    // Read hardware memory info saved by bootloader at 0x7000
    uint16_t conv_mem   = *(volatile uint16_t*)0x7000;
    uint16_t ext_low    = *(volatile uint16_t*)0x7002;
    uint16_t ext_high   = *(volatile uint16_t*)0x7004;
    uint32_t ext_mem    = (uint32_t)ext_low + ((uint32_t)ext_high * 64);
    uint8_t  boot_drive = *(volatile uint8_t*)0x7006;
    uint32_t uptime_sec = timer_get_ticks() / 18;

    println("");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  CPU       : ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    print(cpu_vendor);
    set_color(VGA_WHITE, VGA_BLACK);
    println(" (via CPUID)");

    print("  Memory    : ");
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    print_dec(conv_mem);
    print(" KB conventional, ");
    print_dec(ext_mem);
    println(" KB extended");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  Timer     : ");
    set_color(VGA_YELLOW, VGA_BLACK);
    println("8253 PIT running at 18.2 Hz");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  Keyboard  : ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    println("PS/2 Controller active on IRQ1");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  VGA       : ");
    set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
    println("Text Mode 80x25 at 0xB8000");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  Boot Drive: ");
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    print_hex(boot_drive);
    if (boot_drive >= 0x80) {
        println(" (Hard Disk)");
    } else {
        println(" (Floppy Disk)");
    }

    set_color(VGA_WHITE, VGA_BLACK);
    print("  Uptime    : ");
    set_color(VGA_YELLOW, VGA_BLACK);
    print_dec(uptime_sec);
    println(" seconds");

    println("");
}

static void cmd_mem(void) {
    uint32_t used_kb, free_kb, total_kb;
    kmem_get_stats(&used_kb, &free_kb, &total_kb);

    println("");
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    println("  Memory Map:");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  0x00000000 - 0x000003FF   ");
    set_color(VGA_YELLOW, VGA_BLACK);
    print("IVT (Real Mode)         ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    println("1 KB");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  0x00000400 - 0x000004FF   ");
    set_color(VGA_YELLOW, VGA_BLACK);
    print("BIOS Data Area           ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    println("256 B");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  0x00007C00 - 0x00007DFF   ");
    set_color(VGA_YELLOW, VGA_BLACK);
    print("Bootloader              ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    println("512 B");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  0x00010000 - 0x0002A000   ");
    set_color(VGA_YELLOW, VGA_BLACK);
    print("Kernel                  ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    println("104 KB");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  0x00090000 - 0x0009FFFF   ");
    set_color(VGA_YELLOW, VGA_BLACK);
    print("Kernel Stack             ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    println("64 KB");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  0x000A0000 - 0x000BFFFF   ");
    set_color(VGA_YELLOW, VGA_BLACK);
    print("VGA Region              ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    println("128 KB");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  0x000C0000 - 0x000FFFFF   ");
    set_color(VGA_YELLOW, VGA_BLACK);
    print("BIOS ROM                ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    println("256 KB");

    set_color(VGA_WHITE, VGA_BLACK);
    print("  0x00100000 ->             ");
    set_color(VGA_YELLOW, VGA_BLACK);
    println("Heap (Free)");

    println("");
    set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
    print("  Heap:  ");
    set_color(VGA_WHITE, VGA_BLACK);
    print("Used: ");
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    print_dec(used_kb);
    print(" KB   ");

    set_color(VGA_WHITE, VGA_BLACK);
    print("Free: ");
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    print_dec(free_kb);
    print(" KB   ");

    set_color(VGA_WHITE, VGA_BLACK);
    print("Total: ");
    set_color(VGA_YELLOW, VGA_BLACK);
    print_dec(total_kb);
    println(" KB");

    println("");
}

static void cmd_help(void) {
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    println("Available commands:");
    set_color(VGA_WHITE, VGA_BLACK);
    println("  help     - Lists all available commands");
    println("  sysinfo  - Hardware detection and system diagnostics");
    println("  mem      - Memory layout map and kernel heap status");
    println("  clear    - Clears screen and redraws prompt");
    println("  about    - Author and project description");
    println("  version  - Displays operating system version");
    println("  reboot   - Hardware system reset via keyboard controller");
    println("  exit     - Powers down / shuts down the system");
    println("  shutdown - Powers down / shuts down the system");
}

static void cmd_clear(void) {
    clear_screen();
}

static void cmd_about(void) {
    set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
    println("About MiniOS:");
    set_color(VGA_WHITE, VGA_BLACK);
    println("  Project:     MiniOS");
    println("  Author:      Sankalp");
    println("  Description: Lightweight x86 32-bit Protected Mode OS built from scratch.");
    println("  Features:    Custom bootloader, GDT, A20, VGA driver, IDT/PIC, PS/2 shell, Heap.");
}

static void cmd_version(void) {
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    println("MiniOS v0.1");
}

static void cmd_reboot(void) {
    set_color(VGA_LIGHT_RED, VGA_BLACK);
    println("Rebooting system...");
    // Pulse CPU reset line by sending 0xFE to PS/2 controller port 0x64
    outb(0x64, 0xFE);
    while (1) {
        __asm__ volatile ("hlt");
    }
}

static void cmd_shutdown(void) {
    set_color(VGA_LIGHT_RED, VGA_BLACK);
    println("Powering down system...");

    // 1. QEMU ACPI poweroff
    outw(0x604, 0x2000);
    // 2. QEMU older / Bochs ACPI poweroff
    outw(0xB004, 0x2000);
    // 3. VirtualBox poweroff
    outw(0x4004, 0x3400);

    // Fallback: If ACPI poweroff didn't turn off machine, halt CPU safely
    set_color(VGA_YELLOW, VGA_BLACK);
    println("System halted. You may now safely turn off your computer.");
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

static void shell_dispatch(const char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "sysinfo") == 0) {
        cmd_sysinfo();
    } else if (strcmp(cmd, "mem") == 0) {
        cmd_mem();
    } else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(cmd, "about") == 0) {
        cmd_about();
    } else if (strcmp(cmd, "version") == 0) {
        cmd_version();
    } else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "shutdown") == 0) {
        cmd_shutdown();
    } else {
        set_color(VGA_LIGHT_RED, VGA_BLACK);
        print("Unknown command: '");
        print(cmd);
        println("'. Type 'help' for a list of commands.");
    }
}

void shell_init(void) {
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    println("MiniOS Shell v0.1 ready. Type 'help' for commands.");
    println("");
}

void shell_run(void) {
    char buffer[CMD_BUFFER_SIZE];
    int buffer_idx = 0;

    print_prompt();

    while (1) {
        char c = keyboard_getchar();

        if (c == '\n') {
            print_char('\n');
            buffer[buffer_idx] = '\0';

            // Skip leading spaces
            char* cmd = buffer;
            while (*cmd == ' ') cmd++;

            if (*cmd != '\0') {
                shell_dispatch(cmd);
            }

            buffer_idx = 0;
            print_prompt();
        } else if (c == '\b') {
            if (buffer_idx > 0) {
                buffer_idx--;
                print_char('\b');
            }
        } else if (c >= ' ' && c <= '~') {
            if (buffer_idx < CMD_BUFFER_SIZE - 1) {
                buffer[buffer_idx++] = c;
                print_char(c);
            }
        }
    }
}
