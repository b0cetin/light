
#include "userspace.h"
#include "allocator.h"
#include "boot_modules.h"
#include "bootinfo.h"
#include "context_switching.h"
#include "debugging.h"
#include "elf_loader.h"
#include "kernel_lib.h"
#include "pit.h"
#include "processes.h"
#include "syscalls.h"
#include "tss.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

extern void enter_userspace(uint64_t kernel_rsp);

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

    if (entry_point == null) PANIC("Cannot load first module into userspace!");

    char* ascii_path = kmalloc(READ_MODULE_PATH_SIZE * 2);
    convert_utf16_to_ascii(boot_modules_get_all()->path, ascii_path);
    Process *process = process_create(entry_point, user_address_space, ascii_path);
    kfree(ascii_path);

    Thread *thread = process->threads;

    void *kernel_stack_top = thread->kernel_stack_base + thread->kernel_stack_size;
    tss_set_rsp0(kernel_stack_top);
    syscalls_set_kernel_stack(kernel_stack_top);

    kprintln("kernel_stack_top: %lx", (uint64_t) kernel_stack_top);

    vmm_switch_to_user_address_space(process->cr3);

    ctx_switching_set_initial_thread(thread);

    pit_init();

    enter_userspace(thread->kernel_rsp);
}
