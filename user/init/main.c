
#include "boot_module.h"
#include <stddef.h>
#include <stdint.h>

#define SYS_SUCCESS 0

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

int64_t sys_create_thread(void *function, uint64_t *out_thread_id) {
    uint64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (1), "D" (function), "S" (out_thread_id)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

void sys_exit_thread(void *result) {
    asm volatile (
        "syscall\n"
        :
        : "a" (3), "D" (result)
        :  "rcx", "r11", "memory"
    );
}

int64_t sys_wait_thread(uint64_t id, void **out_result) {
    int64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (4), "D" (id), "S" (out_result)
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

uint64_t sys_get_pid() {
    uint64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (7)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

int64_t sys_create_process(void *content, size_t content_len, const char* name, size_t name_len, uint64_t* out_pid, uint64_t flags) {
    int64_t ret;

    register long r_num asm("rax") = 8;
    register long r_a1  asm("rdi") = (uint64_t) content;
    register long r_a2  asm("rsi") = (uint64_t) content_len;
    register long r_a3  asm("rdx") = (uint64_t) name;
    register long r_a4  asm("r10") = (uint64_t) name_len; // Note: Kernel uses r10, NOT rcx
    register long r_a5  asm("r8")  = (uint64_t) out_pid;
    register long r_a6  asm("r9")  = (uint64_t) flags;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "r" (r_num), "r" (r_a1), "r" (r_a2), "r" (r_a3), "r" (r_a4), "r" (r_a4), "r" (r_a5), "r" (r_a6)
        : "rcx", "r11", "memory"
    );

    return ret;
}

void *thread_test() {
    sys_print("This is a printing log from another thread!");

    sys_exit_thread((void*) sys_get_thread_id());
}

int main(BootModule *modules) {
    sys_print("Init started.");

    uint64_t new_t;
    
    if (sys_create_thread(thread_test, &new_t) != SYS_SUCCESS) {
        sys_print("Could not create new thread. Terminating.");
        return -1;
    }

    sys_print("created new thread.\n");

    uint64_t result;
    
    if (sys_wait_thread(new_t, (void**) &result) != SYS_SUCCESS) {
        sys_print("Failure waiting for thread. Terminating.");
        return -1;
    }

    for (int i = 0; i < BOOT_MODULE_COUNT; i++) {
        if (!modules[i].is_read) continue;

        sys_print((char*) modules[i].path);
    }

    uint64_t pid;
    if (sys_create_process((void *) modules[1].address, modules[1].size,
        (const char*) &modules[1].path, 14, &pid, 0) != SYS_SUCCESS) {
        sys_print("Failure creating process.");
        return -1;
    }

    sys_yield();

    return result + sys_get_pid();
}
