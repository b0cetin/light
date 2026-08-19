
#include "debugging.h"
#include "fb_graphics.h"
#include "serial.h"
#include "types.h"
#include <stdarg.h>
#include <stdint.h>

void print_int(int n) {
    if (n < 0) {
        write_serial('-');
        n = -n;
    }
    if (n / 10) {
        print_int(n / 10);
    }
    write_serial((n % 10) + '0');
}

void print_uint64(uint64_t n) {
    if (n / 10) {
        print_uint64(n / 10);
    }
    write_serial((n % 10) + '0');
}

void print_hex(unsigned int n) {
    char *chars = "0123456789ABCDEF";
    for (int i = 28; i >= 0; i -= 4) {
        write_serial(chars[(n >> i) & 0xF]);
    }
}

void print_hex64(uint64_t n) {
    char *chars = "0123456789ABCDEF";
    // Start at bit 60 and work down to 0
    for (int i = 60; i >= 0; i -= 4) {
        write_serial(chars[(n >> i) & 0xF]);
    }
}

void vkprintf(const char *fmt, va_list args) {
    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++;
            bool isLong = false;

            if (*fmt == 'l') {
                isLong = true;
                fmt++; // Move past 'l' to the type specifier (d, x, etc)
            }

            switch (*fmt) {
            case 'd': {
                if (isLong) {
                    uint64_t val = va_arg(args, uint64_t);
                    print_uint64(val);
                } else {
                    int val = va_arg(args, int);
                    print_int(val); // Keep your old signed print_int for small %d
                }
                break;
            }
            case 'x': {
                if (isLong) {
                    uint64_t val = va_arg(args, uint64_t);
                    print_hex64(val);
                } else {
                    unsigned int val = va_arg(args, unsigned int);
                    print_hex(val); // Your old 32-bit hex
                }
                break;
            }
            case 's': {
                char *s = va_arg(args, char *);
                puts_serial(s);
                break;
            }
            case '%': {
                write_serial('%');
                break;
            }
            }
        } else {
            write_serial(*fmt);
        }
        fmt++;
    }
}

void kernel_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vkprintf(fmt, args);
    va_end(args);
}

void kprintln(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vkprintf(fmt, args);
    va_end(args);

    write_serial('\n');
}

void kernel_panic(const char *file, int line, const char *fmt, ...) {
    __asm__ volatile("cli");

    kernel_printf("\n*** KERNEL PANIC ***\n");
    kernel_printf("Location: %s:%d\n", file, line);
    kernel_printf("Message: ");

    va_list args;
    va_start(args, fmt);
    vkprintf(fmt, args);
    va_end(args);

    write_serial('\n');

    fb_clear(COLOR_RED);
    fb_draw_text(fb_width() / 2 - 13 * 8, fb_height() / 2 - 8, "Kernel panic.", COLOR_WHITE);

    while (true)
        __asm__ volatile ("hlt");
}
