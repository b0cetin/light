
#include <stddef.h>
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

uint64_t sys_create_thread(void *function) {
    uint64_t id;

    asm volatile (
        "syscall\n"
        : "=a" (id)
        : "a" (1), "D" (function)
        :  "rcx", "r11", "memory"
    );

    return id;
}

void sys_exit_thread(void *result) {
    asm volatile (
        "syscall\n"
        :
        : "a" (3), "D" (result)
        :  "rcx", "r11", "memory"
    );
}

void *sys_wait_thread(uint64_t thread) {
    void *ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (4), "D" (thread)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

void sys_yield() {
    asm volatile (
        "syscall\n"
        :
        : "a" (5)
        :  "rcx", "r11", "memory"
    );
}

uint64_t sys_get_thread_id() {
    uint64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (6)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

void *thread_test() {
    sys_print("This is a printing log from another thread!\n");

    sys_exit_thread((void*) sys_get_thread_id());
}

int main() {
    sys_print("init process started.\n");

    uint64_t new_t = sys_create_thread(thread_test);

    sys_print("created new thread.\n");

    uint64_t result = (uint64_t) sys_wait_thread(new_t);

    return ++result;
}
