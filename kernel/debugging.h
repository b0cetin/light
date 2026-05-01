
#pragma once

void kernel_printf(const char *fmt, ...);
void kernel_println(const char *fmt, ...);

void kernel_panic(const char *file, int line, const char *fmt, ...);

#define PANIC(msg, ...) kernel_panic(__FILE__, __LINE__, msg, ##__VA_ARGS__)
