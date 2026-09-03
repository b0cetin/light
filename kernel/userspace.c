
#include "userspace.h"
#include "allocator.h"
#include "apic.h"
#include "boot_modules.h"
#include "bootinfo.h"
#include "context_switching.h"
#include "debugging.h"
#include "elf_loader.h"
#include "kernel_lib.h"
#include "processes.h"
#include "syscalls.h"
#include "tss.h"
#include "types.h"
#include "vas.h"
#include "vmm.h"
#include <stdint.h>

typedef struct __attribute((packed)) {
    uint8_t available;

    uint64_t base;
    uint64_t size;

    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} UEFIFramebuffer;

typedef struct __attribute__((packed)) {
    ReadModule modules[READ_MODULE_COUNT];

    UEFIFramebuffer framebuffer;
} InitInfo;

extern void enter_userspace(uint64_t kernel_rsp);

static uint64_t prepare_init_info(BootInfo *boot_info, VAS* vas) {
    #define INIT_INFO_BASE 0x0000000600000000ULL

    VirtualMemoryObject *init_object = vmem_create_allocation(1, true);
    InitInfo *init_info = p2v(vmem_get_phys_page(init_object, 0));

    // Boot modules
    {
        ReadModule *boot_modules = init_info->modules;
        memcpy(boot_modules, boot_modules_get_all(), sizeof(ReadModule) * READ_MODULE_COUNT);
        
        for (int i = 0; i < READ_MODULE_COUNT; i++) {
            if (!boot_modules[i].is_read) continue;

            if (boot_modules[i].physical_location & ~PT_ADDRESS_MASK)
                PANIC("Boot module %d is not page aligned!", i);

            uint64_t page_count = (boot_modules[i].size + PAGE_SIZE - 1) / PAGE_SIZE;
            uintptr_t target_base = vas_get_unused_space(vas, page_count);

            if (target_base == 0) PANIC("No place found to map boot module %d to init process!", i);

            VirtualMemoryObject *module_object = vmem_create_slice_continuous(boot_modules[i].physical_location, page_count);
            vas_add_region(vas, target_base, VMEM_PERM_READ, VREGION_REASON_SYSTEM_INIT, module_object);

            boot_modules[i].physical_location = target_base;
            convert_utf16_to_ascii(boot_info->modules[i].path, (char*) &boot_modules[i].path);
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
            init_info->framebuffer.pitch = boot_info->framebuffer.PixelsPerScanLine;
        }
    }
    
    vas_add_region(vas, INIT_INFO_BASE, VMEM_PERM_READ, VREGION_REASON_SYSTEM_INIT, init_object);
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

static Thread *create_and_start_idle_thread() {
    uint64_t pid = PROCESS_IDLE_PID;
    Process *process = process_create_kernel("system_idle", &pid);

    process->prev->next = null;
    process->prev = null;

    process_start(process, idle_thread);

    return process->threads;
}

// Drops into first loaded module.
// This call never returns.
void start_first_user_process(BootInfo *boot_info) {
    char* ascii_path = kmalloc(READ_MODULE_PATH_SIZE * 2);
    convert_utf16_to_ascii(boot_modules_get_all()->path, ascii_path);
    Process *process = process_create(ascii_path);
    kfree(ascii_path);

    void *entry_point = load_elf(&process->user_vas, p2v(boot_modules_get_all()->physical_location), boot_modules_get_all()->size);
    if (entry_point == null) PANIC("Cannot load first module into userspace!");
    process_start(process, entry_point);

    uint64_t init_info = prepare_init_info(boot_info, &process->user_vas);

    Thread *thread = process->threads;

    patch_in_arg(init_info, thread);

    void *kernel_stack_top = thread->kernel_stack_base + thread->kernel_stack_size;
    tss_set_rsp0(kernel_stack_top);
    syscalls_set_kernel_stack(kernel_stack_top);

    vmm_switch_to_user_address_space(process->user_vas.cpu_address_table);

    ctx_switching_init(thread, create_and_start_idle_thread());

    lapic_timer_set(32, 100);

    kprintln("Switching to userspace.");

    enter_userspace(thread->kernel_rsp);
}
