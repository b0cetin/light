
#include "userspace.h"
#include "allocator.h"
#include "boot_modules.h"
#include "bootinfo.h"
#include "context_switching.h"
#include "debugging.h"
#include "elf_loader.h"
#include "kernel_lib.h"
#include "pit.h"
#include "pmm.h"
#include "processes.h"
#include "syscalls.h"
#include "tss.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

extern void enter_userspace(uint64_t kernel_rsp);

static uint64_t prepare_boot_modules(PML4* address_space) {
    #define BOOT_MODULES_BASE 0x0000000600000000ULL

    ReadModule *boot_modules = p2v(pmm_alloc_page());
    memcpy(boot_modules, boot_modules_get_all(), sizeof(ReadModule) * READ_MODULE_COUNT);

    vmm_map(address_space, BOOT_MODULES_BASE, v2p(boot_modules), PT_USER | PT_NX);
    
    uint64_t cursor = BOOT_MODULES_BASE + PAGE_SIZE;
    for (int i = 0; i < READ_MODULE_COUNT; i++) {
        if (!boot_modules[i].is_read) continue;

        if (boot_modules[i].physical_location & 0xFFF)
            PANIC("Boot module %d is not 4 KB page aligned!", i);

        uint64_t page_count = ((boot_modules[i].physical_location + 0xFFF) & ~0xFFF) / PAGE_SIZE;
        uint64_t start = cursor;

        for (uint64_t j = 0; j < page_count; j++) {
            vmm_map(address_space, cursor, boot_modules[i].physical_location + j * PAGE_SIZE, PT_USER | PT_NX);
            cursor += PAGE_SIZE;
        }

        boot_modules[i].physical_location = start;
        
        convert_utf16_to_ascii(boot_modules[i].path, (char*) &boot_modules[i].path);
    }
    
    return BOOT_MODULES_BASE;
}

static void patch_in_boot_modules(uint64_t boot_modules_base, Thread* initialized_thread) {
    uint64_t *sp = (uint64_t*) initialized_thread->kernel_rsp;

    sp = sp + 5; // %rdi register

    *sp = boot_modules_base;
}

// Drops into first loaded module.
// This call never returns.
void start_first_user_process(void) {
    PML4 *user_address_space = vmm_create_user_address_space();

    void *entry_point = load_elf(user_address_space, p2v(boot_modules_get_all()->physical_location), boot_modules_get_all()->size);

    if (entry_point == null) PANIC("Cannot load first module into userspace!");

    uint64_t boot_modules = prepare_boot_modules(user_address_space);

    char* ascii_path = kmalloc(READ_MODULE_PATH_SIZE * 2);
    convert_utf16_to_ascii(boot_modules_get_all()->path, ascii_path);
    Process *process = process_create_critical(entry_point, user_address_space, ascii_path);
    kfree(ascii_path);

    Thread *thread = process->threads;

    patch_in_boot_modules(boot_modules, thread);

    void *kernel_stack_top = thread->kernel_stack_base + thread->kernel_stack_size;
    tss_set_rsp0(kernel_stack_top);
    syscalls_set_kernel_stack(kernel_stack_top);

    kprintln("kernel_stack_top: %lx", (uint64_t) kernel_stack_top);

    vmm_switch_to_user_address_space(process->cr3);

    ctx_switching_set_initial_thread(thread);

    pit_init();

    enter_userspace(thread->kernel_rsp);
}
