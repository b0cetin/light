
#include "include/stdio.h"
#include "syscalls.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

int println(const char * restrict format, ...) {
    va_list args;
    va_start(args, format);
    int n = vprintln(format, args);
    va_end(args);
    return n;
}

int vprintln(const char * restrict format, va_list ap) {
    char buffer[200];
    int n = vsnprintf(buffer, sizeof(buffer), format, ap);
    sys_print(buffer);
    return n;
}

int snprintf(char *restrict str, size_t size, const char *restrict format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsnprintf(str, size, format, args);
    va_end(args);
    return n;
}

int vsnprintf(char * restrict str, size_t size, const char * restrict format, va_list ap) {
    int out_cursor = 0;

    enum {
        SIZE_CHAR,
        SIZE_SHORT,
        SIZE_INT,
        SIZE_LONG,
    } integer_size = SIZE_INT;
    bool is_alternate_form = false;

    bool is_converting = false;

    while (*format != '\0') {
        if (!is_converting) {
            integer_size = SIZE_INT;
            is_alternate_form = false;

            if (*format != '%') {
                if (out_cursor < size - 1)
                    str[out_cursor] = *format;

                out_cursor++;
                format++;
                
                continue;
            }

            format++; // Consume %
            is_converting = true;
        }

        if (*format == '%') {
            if (out_cursor < size - 1)
                str[out_cursor] = '%';

            out_cursor++;
            format++;
            is_converting = false;
            continue;
        }

        if (*format == '#') {
            is_alternate_form = true;
            format++;
            continue;
        }

        if (*format == 'l') {
            integer_size = SIZE_LONG;
            format++;
            continue;
        }

        if (*format == 'h') {
            integer_size = integer_size == SIZE_SHORT ? SIZE_CHAR : SIZE_SHORT;
            format++;
            continue;
        }

        if (*format == 'd' || *format == 'i') {
            int64_t integer;

            switch (integer_size) {
                case SIZE_CHAR:
                    integer = (int64_t) (int8_t) va_arg(ap, unsigned int);
                    break;
                case SIZE_SHORT:
                    integer = (int64_t) (int16_t) va_arg(ap, unsigned int);
                    break;
                case SIZE_INT:
                    integer = (int64_t) va_arg(ap, int);
                    break;
                case SIZE_LONG:
                    integer = va_arg(ap, long long);
                    break;
                default:
                    return -1;
            }

            if (integer < 0) {
                if (out_cursor < size - 1)
                    str[out_cursor] = '-';

                out_cursor++;

                integer = -integer;
            }

            char buffer[19]; // Big enough to fit 64-bit positive integer limit (without terminator)

            int i = 0;

            do {
                int remainder = integer % 10;
                buffer[i++] = remainder + '0';
                integer /= 10;
            } while (integer != 0);

            while (i > 0) {
                if (out_cursor < size - 1)
                    str[out_cursor] = buffer[--i];

                out_cursor++;
            }

            format++;
            is_converting = false;
            continue;
        }

        if (*format == 'u') {
            uint64_t integer;

            switch (integer_size) {
                case SIZE_CHAR:
                    integer = (uint64_t) (uint8_t) va_arg(ap, unsigned int);
                    break;
                case SIZE_SHORT:
                    integer = (uint64_t) (uint16_t) va_arg(ap, unsigned int);
                    break;
                case SIZE_INT:
                    integer = (uint64_t) va_arg(ap, unsigned int);
                    break;
                case SIZE_LONG:
                    integer = va_arg(ap, unsigned long long);
                    break;
                default:
                    return -1;
            }

            char buffer[20]; // Big enough to fit 64-bit unsigned integer limit (without terminator)

            int i = 0;

            do {
                int remainder = integer % 10;
                buffer[i++] = remainder + '0';
                integer /= 10;
            } while (integer != 0);

            while (i > 0) {
                if (out_cursor < size - 1)
                    str[out_cursor] = buffer[--i];

                out_cursor++;
            }

            format++;
            is_converting = false;
            continue;
        }

        if (*format == 'x' || *format == 'X') {
            uint64_t integer;

            switch (integer_size) {
                case SIZE_CHAR:
                    integer = (uint64_t) (uint8_t) va_arg(ap, unsigned int);
                    break;
                case SIZE_SHORT:
                    integer = (uint64_t) (uint16_t) va_arg(ap, unsigned int);
                    break;
                case SIZE_INT:
                    integer = (uint64_t) va_arg(ap, unsigned int);
                    break;
                case SIZE_LONG:
                    integer = va_arg(ap, unsigned long long);
                    break;
                default:
                    return -1;
            }

            if (integer != 0 && is_alternate_form) {
                if (out_cursor < size - 1)
                    str[out_cursor] = '0';

                out_cursor++;
                
                if (out_cursor < size - 1)
                    str[out_cursor] = *format;

                out_cursor++;
            }

            static const char *upper_chars = "0123456789ABCDEF";
            static const char *lower_chars = "0123456789abcdef";

            const char *chars = *format == 'X' ? upper_chars : lower_chars;

            int i;

            switch (integer_size) {
                case SIZE_CHAR:
                    i = 4;
                    break;
                case SIZE_SHORT:
                    i = 14;
                    break;
                case SIZE_INT:
                    i = 28;
                    break;
                case SIZE_LONG:
                    i = 60;
                    break;
                default:
                    return -1;
            }

            for (; i >= 0; i -= 4) {
                if (out_cursor < size - 1)
                    str[out_cursor] = chars[(integer >> i) & 0xF];

                out_cursor++;
            }

            format++;
            is_converting = false;
            continue;
        }

        if (*format == 'c') {
            uint8_t c = (uint8_t) va_arg(ap, unsigned int);

            if (out_cursor < size - 1)
                str[out_cursor] = c;

            out_cursor++;

            format++;
            is_converting = false;
            continue;
        }

        if (*format == 's') {
            const char *s = va_arg(ap, char*);

            while (*s != '\0') {
                if (out_cursor < size - 1)
                    str[out_cursor] = *s;

                out_cursor++;
                s++;
            }

            format++;
            is_converting = false;
            continue;
        }
    }

    str[out_cursor] = '\0';

    return out_cursor;
}
