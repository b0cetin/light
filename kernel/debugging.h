
#pragma once

void kernel_printf(const char *fmt, ...);
void kprintln(const char *fmt, ...);

void kernel_panic(const char *file, int line, const char *fmt, ...);

#define PANIC(msg, ...) kernel_panic(__FILE__, __LINE__, msg, ##__VA_ARGS__)

#define assert_msg(condition, message) if (!(condition)) PANIC(message)
#define assert(condition) assert_msg(condition, "assertion failed!");

#if defined(__GNUC__)
#define NOT_IMPLEMENTED PANIC("\"%s\" not implemented!", __FUNCTION__);
#else
#define NOT_IMPLEMENTED PANIC("Function not implemented!");
#endif
