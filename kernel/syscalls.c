
#include "syscalls.h"
#include "allocator.h"
#include "context_switching.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "msr.h"
#include "processes.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint64_t kernel_stack;
    uint64_t user_stack;
} __attribute__((packed)) CPULocalStruct;

CPULocalStruct local_cpu_data;

extern void syscall_entry();

void syscalls_init() {
    uint64_t efer = msr_read(MSR_EFER);
    msr_write(MSR_EFER, efer | 0x1); // IA32_EFER.SCE = true
    kprintln("SYSCALLS: IA32_EFER.SCE set to true.");

    // STAR Layout
    // [ 63 : USER Selector Base : 48 ] [ 47 : KERNEL Selector Base : 32 ]
    // [ 31 : UNUSED IN 64-bit : 0 ]

    // NOTE: SYSCALL/SYSRET are very unsafe instructions. They are unsafe
    // to ensure speed. GDT has to be set up in an exact way for them
    // to be used.
    uint64_t star = ((uint64_t) 0x0008 << 32) | ((uint64_t) 0x0010 << 48);
    msr_write(MSR_STAR, star);

    msr_write(MSR_LSTAR, (uint64_t) (void *) syscall_entry);

    msr_write(MSR_SFMASK, 0x200);
    // Disable interrupts in RFLAGS while we are still in the user stack.

    local_cpu_data.user_stack = 0; // Will be set later by the user.

    msr_write(MSR_GS_BASE, 0); // This will flip when transitioning to user mode.
    msr_write(MSR_KERNEL_GS_BASE, (uint64_t)&local_cpu_data);

    kprintln("SYSCALLS initialized.");
}

void syscalls_set_kernel_stack(void *kernel_stack_top) {
    local_cpu_data.kernel_stack = (uint64_t) kernel_stack_top;
}

#define PRINT_RREFIX "USER: "
int64_t sys_print(const char* buf) {
    if (buf == null) {
        kprintln("SYSCALLS: sys_print called with null as buffer!");
        process_begin_process_teardown(ctx_switching_get_active_thread()->owner, -1);
        return -1;
    }

    size_t len = strnlen(buf, UINT8_MAX);

    char *str = kmalloc(len + sizeof(PRINT_RREFIX) + 1);

    memcpy(str, PRINT_RREFIX, sizeof(PRINT_RREFIX));
    memcpy(str + sizeof(PRINT_RREFIX) - 1, buf, len);

    kernel_printf(str);

    kfree(str);
    return 0;
}

uint64_t sys_create_thread(void *function) {
    kprintln("SYSCALLS: sys_create_thread called by %ld with entry point %lx.", ctx_switching_get_active_thread()->local_id, (uint64_t) function);
    return process_create_thread(function, ctx_switching_get_active_thread()->owner)->local_id;
}

void sys_exit(uint64_t status) {
    asm volatile ("swapgs");

    process_begin_process_teardown(ctx_switching_get_active_thread()->owner, status);
    // TODO: Maybe change specification to report errors?
}

void sys_exit_thread(uint64_t result) {
    asm volatile ("swapgs");

    process_begin_thread_teardown(ctx_switching_get_active_thread(), result);
    // TODO: Maybe change specification to report errors?
}

void sys_yield() {
    // NOTE: The interrupt handler doesn't touch swapgs.
    // GS register is only relevant for context switching
    // for the syscall. Therefore, if we don't swap to
    // the user's gs now, if the yielded thread performs
    // a syscall, then the GS register will point to
    // the wrong memory.

    // NOTE: The same can be said about every syscall
    // that context switches to another thread.
    // iretq doesn't touch the GS register so we
    // need to fix it ourselves.

    asm volatile (
        "swapgs\n"
        "int $0x81\n"
        "swapgs\n"
        ::: "memory"
    );
}

uint64_t sys_wait_thread(uint64_t id) {
    Thread *target = process_try_get_thread_from_id(ctx_switching_get_active_thread()->owner, id);

    if (target == null) {
        kprintln("Thread %ld of process %ld invoked sys_wait_thread with an invalid thread id.",
            ctx_switching_get_active_thread()->local_id, ctx_switching_get_active_thread()->owner->pid);
        process_crash_with_switch(ctx_switching_get_active_thread()->owner);
        return -1;
    }

    while (target->state != THREAD_TERMINATED) {
        sys_yield();
    }

    return target->result;
}

uint64_t sys_get_thread_id() {
    return ctx_switching_get_active_thread()->local_id;
}

uint64_t sys_get_pid() {
    return ctx_switching_get_active_thread()->owner->pid;
}

int64_t syscall_handler(uint64_t call_number, uint64_t arg1, uint64_t arg2, 
                          uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    switch (call_number) {
        case 0:
            return sys_print((char*) arg1);
        case 1:
            return sys_create_thread((void*) arg1);
        case 2:
            sys_exit(arg1);
            return 0;
        case 3:
            sys_exit_thread(arg1);
            return 0;
        case 4:
            return sys_wait_thread(arg1);
        case 5:
            sys_yield();
            return 0;
        case 6:
            return sys_get_thread_id();
        case 7:
            return sys_get_pid();
        default:
            kprintln("SYSCALLS: Unknown syscall %ld called.", call_number);
            return -1;
    }
    return 0;
}
