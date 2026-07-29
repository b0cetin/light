
#include "userspace.h"
#include "pmm.h"
#include "vmm.h"
#include <stdint.h>

extern void enter_userspace(uint64_t entry_point, uint64_t user_rsp);

uint8_t __attribute__((aligned(16))) user_stack[16384];
void *user_stack_top = (void *)((uint64_t)user_stack + sizeof(user_stack));

// Sample Ring 3 code
void user_main(void) {
    // We are officially in Ring 3 now!
    
    // Trigger a system call
    asm volatile (
        "movq $67, %%rax\n"     // SYS_WRITE
        "syscall\n"
        :
        :
        : "rax", "rdi", "rsi", "rcx", "r11"
    );

    while (1) {}
}

void map_range_identically(PLM4 *pml4, uint64_t start, uint64_t size, uint64_t flags) {
    uint64_t first_page = start & ~(0xFFFULL);
    uint64_t last_page = (start + size + 4095) & ~(0xFFFULL);

    for (uint64_t addr = first_page; addr < last_page; addr += PAGE_SIZE) {
        vmm_map(pml4, addr, addr, flags);
    }
}

// Drops into test userspace function still in kernel code.
// This call never returns.
void start_first_user_process(void) {
    PLM4 *user_address_space = vmm_create_user_address_space();
    map_range_identically(user_address_space, 0, pmm_get_highest_phys_addr(), PT_USER | PT_RW);

    vmm_switch_to_user_address_space(user_address_space);

    enter_userspace(v2p((void*) user_main), v2p(user_stack_top));
}
