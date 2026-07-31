
#include <stdint.h>

int64_t sys_print(const char* buf) {
    int64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (0), "D" (buf)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

int main() {
    sys_print("This is a message from userspace.\n");

    while (1) {}
}
