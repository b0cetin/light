
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

typedef struct __attribute((packed)) {
    uint8_t available;

    uint64_t base;
    uint64_t size;

    uint32_t width;
    uint32_t height;
} UEFIFramebuffer;

typedef struct __attribute__((packed)) {
    ReadModule modules[READ_MODULE_COUNT];

    UEFIFramebuffer framebuffer;
} InitInfo;

extern void enter_userspace(uint64_t kernel_rsp);

static uint64_t prepare_init_info(BootInfo *boot_info, PML4* address_space) {
    #define INIT_INFO_BASE 0x0000000600000000ULL

    InitInfo *init_info = p2v(pmm_alloc_page());
    memzero(init_info, sizeof(InitInfo));
    vmm_map(address_space, INIT_INFO_BASE, v2p(init_info), PT_USER | PT_NX);

    // Boot modules
    {
        ReadModule *boot_modules = init_info->modules;

        memcpy(boot_modules, boot_modules_get_all(), sizeof(ReadModule) * READ_MODULE_COUNT);
        
        uint64_t cursor = INIT_INFO_BASE + PAGE_SIZE;
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
    }

    // Framebuffer
    {
        if (boot_info->framebuffer.PhysicalFramebufferBase != 0) {
            init_info->framebuffer.available = 1;

            init_info->framebuffer.base = boot_info->framebuffer.PhysicalFramebufferBase;
            init_info->framebuffer.size = boot_info->framebuffer.FramebufferSize;

            init_info->framebuffer.width = boot_info->framebuffer.HorizontalResolution;
            init_info->framebuffer.height = boot_info->framebuffer.VerticalResolution;
        }
    }
    
    return INIT_INFO_BASE;
}

static void patch_in_arg(uint64_t arg, Thread* initialized_thread) {
    uint64_t *sp = (uint64_t*) initialized_thread->kernel_rsp;

    sp = sp + 5; // %rdi register

    *sp = arg;
}

// NOTE: I guess this shouldn't be here anymore as this is a kernel-space process?
void idle_thread() {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

static Thread *create_idle_thread() {
    Process *process = process_create_kernel(idle_thread, "system_idle");

    process->pid = PROCESS_IDLE_PID;
    process->prev->next = null;
    process->prev = null;

    return process->threads;
}

// Drops into first loaded module.
// This call never returns.
void start_first_user_process(BootInfo *boot_info) {
    PML4 *user_address_space = vmm_create_user_address_space();

    void *entry_point = load_elf(user_address_space, p2v(boot_modules_get_all()->physical_location), boot_modules_get_all()->size);

    if (entry_point == null) PANIC("Cannot load first module into userspace!");

    uint64_t init_info = prepare_init_info(boot_info, user_address_space);

    char* ascii_path = kmalloc(READ_MODULE_PATH_SIZE * 2);
    convert_utf16_to_ascii(boot_modules_get_all()->path, ascii_path);
    Process *process = process_create(entry_point, user_address_space, ascii_path);
    kfree(ascii_path);

    Thread *thread = process->threads;

    patch_in_arg(init_info, thread);

    void *kernel_stack_top = thread->kernel_stack_base + thread->kernel_stack_size;
    tss_set_rsp0(kernel_stack_top);
    syscalls_set_kernel_stack(kernel_stack_top);

    vmm_switch_to_user_address_space(process->user_cr3);

    ctx_switching_init(create_idle_thread());

    pit_init();

    enter_userspace(thread->kernel_rsp);
}
