
#include "syscalls.h"
#include "debugging.h"
#include "msr.h"
#include "syscalls/defined_syscalls.h"
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

int64_t syscall_handler(uint64_t call_number, uint64_t arg1, uint64_t arg2, 
                          uint64_t arg3, uint64_t arg4, uint64_t arg5,
                          uint64_t arg6)
{
    switch (call_number) {
        case 0:
            return sys_print((char*) arg1);
        case 1:
            return sys_create_thread((void*) arg1, (void *)arg2, (uint64_t*) arg3);
        case 2:
            sys_exit(arg1);
            return 0;
        case 3:
            sys_exit_thread((void *) arg1);
            return 0;
        case 4:
            return sys_wait_thread(arg1, (void **) arg2);
        case 5:
            sys_yield();
            return 0;
        case 6:
            return sys_get_thread_id();
        case 7:
            return sys_get_pid();
        case 8:
            return sys_create_process((void*) arg1, arg2,
                    (const char*) arg3, arg4,
                    (uint64_t*) arg5);
        case 9:
            return sys_interrupt_control((IRQCTLRequest) arg1, arg2);
        default:
            kprintln("SYSCALLS: Unknown syscall %ld called.", call_number);
            return -1;
    }
    return 0;
}
