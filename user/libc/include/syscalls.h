
#pragma once

#include <stddef.h>
#include <stdint.h>

#define SYS_SUCCESS 0

static inline int64_t sys_print(const char* buf) {
    int64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (0), "D" (buf)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

static inline int64_t sys_create_thread(void *function, void *arg, uint64_t *out_thread_id) {
    uint64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (1), "D" (function), "S" (arg), "d" (out_thread_id)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

static inline void sys_exit_thread(void *result) {
    asm volatile (
        "syscall\n"
        :
        : "a" (3), "D" (result)
        :  "rcx", "r11", "memory"
    );
}

static inline int64_t sys_wait_thread(uint64_t id, void **out_result) {
    int64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (4), "D" (id), "S" (out_result)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

static inline void sys_yield() {
    asm volatile (
        "syscall\n"
        :
        : "a" (5)
        :  "rcx", "r11", "memory"
    );
}

static inline uint64_t sys_get_thread_id() {
    uint64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (6)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

static inline uint64_t sys_get_pid() {
    uint64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (7)
        :  "rcx", "r11", "memory"
    );

    return ret;
}

static inline int64_t sys_create_process(void *content, size_t content_len, const char* name, size_t name_len, uint64_t* out_pid) {
    int64_t ret;

    register long r_num asm("rax") = 8;
    register long r_a1  asm("rdi") = (uint64_t) content;
    register long r_a2  asm("rsi") = (uint64_t) content_len;
    register long r_a3  asm("rdx") = (uint64_t) name;
    register long r_a4  asm("r10") = (uint64_t) name_len; // Note: Kernel uses r10, NOT rcx
    register long r_a5  asm("r8")  = (uint64_t) out_pid;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "r" (r_num), "r" (r_a1), "r" (r_a2), "r" (r_a3), "r" (r_a4), "r" (r_a4), "r" (r_a5)
        : "rcx", "r11", "memory"
    );

    return ret;
}

#define SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED -2
#define SYS_ERR_IRQCTL_AWAIT_DUPLICATE -3
#define SYS_ERR_IRQCTL_AWAIT_CANCELLED -4
#define SYS_ERR_IRQCTL_CANCEL_NOT_AWAITED -5
#define SYS_ERR_IRQCTL_VECTOR_IN_USE -6
#define SYS_ERR_IRQCTL_VECTOR_OUT_OF_RANGE -7
#define SYS_ERR_IRQCTL_REQUEST_INVALID -8
#define SYS_ERR_IRQCTL_UNSET_VECTOR_AWAITING -9
typedef enum {
    IRQCTL_SET    = 0x0,
    IRQCTL_AWAIT  = 0x1,
    IRQCTL_CANCEL = 0x2,
    IRQCTL_UNSET  = 0x3,
} IRQCTLRequest;
static inline int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t vector) {
    int64_t ret;

    asm volatile (
        "syscall\n"
        : "=a" (ret)
        : "a" (9), "D" (request), "S" (vector)
        :  "rcx", "r11", "memory"
    );

    return ret;
}
