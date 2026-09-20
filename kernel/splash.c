#include "splash.h"
#include "types.h"
#include "screen.h"
#include "irq.h"
#include "keyboard.h"

#define BOX_X      8
#define BOX_Y      5
#define BOX_WIDTH  64
#define BOX_HEIGHT 14
#define BAR_LENGTH 28

static void draw_box(int start_x, int start_y, int width, int height) {
    // Top border: ╔══════════════════════════════════════════════════════════════╗
    screen_set_cursor(start_x, start_y);
    print_char((char)0xC9); // ╔
    for (int i = 0; i < width - 2; i++) {
        print_char((char)0xCD); // ═
    }
    print_char((char)0xBB); // ╗

    // Middle rows: ║                                                              ║
    for (int y = 1; y < height - 1; y++) {
        screen_set_cursor(start_x, start_y + y);
        print_char((char)0xBA); // ║
        for (int i = 0; i < width - 2; i++) {
            print_char(' ');
        }
        print_char((char)0xBA); // ║
    }

    // Bottom border: ╚══════════════════════════════════════════════════════════════╝
    screen_set_cursor(start_x, start_y + height - 1);
    print_char((char)0xC8); // ╚
    for (int i = 0; i < width - 2; i++) {
        print_char((char)0xCD); // ═
    }
    print_char((char)0xBC); // ╝
}

static void draw_progress_bar(int step) {
    screen_set_cursor(18, 13);
    set_color(VGA_LIGHT_CYAN, VGA_BLUE);
    print_char('[');

    set_color(VGA_LIGHT_GREEN, VGA_BLUE);
    for (int i = 0; i < step; i++) {
        print_char('#');
    }

    set_color(VGA_WHITE, VGA_BLUE);
    for (int i = step; i < BAR_LENGTH; i++) {
        print_char(' ');
    }

    set_color(VGA_LIGHT_CYAN, VGA_BLUE);
    print_char(']');
    print_char(' ');

    int percent = (step * 100) / BAR_LENGTH;
    set_color(VGA_YELLOW, VGA_BLUE);
    if (percent < 10) {
        print("  ");
    } else if (percent < 100) {
        print(" ");
    }
    print_dec(percent);
    print_char('%');
}

void splash_screen(void) {
    // 1. Fill entire background with Royal Blue
    set_color(VGA_WHITE, VGA_BLUE);
    clear_screen();

    // 2. Draw outer double-line box in Bright White / Cyan on Blue
    set_color(VGA_LIGHT_CYAN, VGA_BLUE);
    draw_box(BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT);

    // 3. Title: "MiniOS v0.1" (Gold/Yellow)
    set_color(VGA_YELLOW, VGA_BLUE);
    screen_set_cursor(33, 7);
    print("MiniOS v0.1");

    // 4. Subtitle: "A 32-bit x86 Educational OS" (White)
    set_color(VGA_WHITE, VGA_BLUE);
    screen_set_cursor(26, 8);
    print("A 32-bit x86 Educational OS");

    // 5. Author: "Built by Sankalp" (Light Cyan)
    set_color(VGA_LIGHT_CYAN, VGA_BLUE);
    screen_set_cursor(32, 10);
    print("Built by Sankalp");

    // 6. Status: "Booting system components..." (Light Green)
    set_color(VGA_LIGHT_GREEN, VGA_BLUE);
    screen_set_cursor(18, 12);
    print("Booting system components...");

    // 7. Initial empty progress bar
    draw_progress_bar(0);

    // 8. Fake progress bar animation using timer ticks (~2-3 seconds)
    for (int step = 1; step <= BAR_LENGTH; step++) {
        // Sleep 1 or 2 ticks per step (PIT fires at ~18.2 Hz)
        if (step == 8 || step == 18 || step == BAR_LENGTH) {
            sleep_ticks(3); // slight pause at milestones
        } else {
            sleep_ticks(1);
        }
        draw_progress_bar(step);
    }

    // 9. Short delay before showing prompt
    sleep_ticks(4);

    // 10. Prompt: "Press any key to continue..."
    set_color(VGA_YELLOW, VGA_BLUE);
    screen_set_cursor(26, 15);
    print("Press any key to continue...");

    // 11. Wait for keypress from user
    keyboard_getchar();
}

