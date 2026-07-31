
#include "userspace.h"
#include "boot_modules.h"
#include "debugging.h"
#include "elf_loader.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

extern void enter_userspace(uint64_t entry_point, uint64_t user_rsp);

uint8_t __attribute__((aligned(16))) user_stack[4096];
void *user_stack_top = (void *)((uint64_t)user_stack + sizeof(user_stack));

void map_range_identically(PLM4 *pml4, uint64_t start, uint64_t size, uint64_t flags) {
    uint64_t first_page = start & ~(0xFFFULL);
    uint64_t last_page = (start + size + 4095) & ~(0xFFFULL);

    for (uint64_t addr = first_page; addr < last_page; addr += PAGE_SIZE) {
        vmm_map(pml4, addr, addr, flags);
    }
}

// Drops into first loaded module.
// This call never returns.
void start_first_user_process(void) {
    PLM4 *user_address_space = vmm_create_user_address_space();

    void *entry_point = load_elf(user_address_space, p2v(boot_modules_get_all()->physical_location));

    if (entry_point == null) {
        vmm_destroy_user_address_space(user_address_space);
        PANIC("Cannot load first module into userspace!");
    }

    vmm_map(user_address_space, v2p(user_stack_top), v2p(user_stack_top), PT_USER | PT_RW | PT_NX);

    vmm_switch_to_user_address_space(user_address_space);

    enter_userspace((uint64_t) entry_point, v2p(user_stack_top));
}
