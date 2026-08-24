
#pragma once

#include <stdarg.h>
#include <stddef.h>

int println(const char * restrict format, ...);
int vprintln(const char * restrict format, va_list ap);

int snprintf(char *restrict str, size_t size, const char *restrict format, ...);
int vsnprintf(char * restrict str, size_t size, const char * restrict format, va_list ap);
