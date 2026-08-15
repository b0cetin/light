
#include "allocator.h"
#include "boot_modules.h"
#include "bootinfo.h"
#include "debugging.h"
#include "fb_graphics.h"
#include "gdt.h"
#include "idt.h"
#include "msr.h"
#include "pic.h"
#include "pit.h"
#include "pmm.h"
#include "processor_info.h"
#include "ps2_keyboard_driver.h"
#include "serial.h"
#include "syscalls.h"
#include "userspace.h"
#include "vmm.h"
#include <stdint.h>

// 16KB stack: usually enough for whatever will happen, increase if necessary
uint8_t __attribute__((aligned(16))) kernel_stack[16384];
// Aligned to 16 bytes for the System V ABI
void *kernel_stack_top = (void *)((uint64_t)kernel_stack + sizeof(kernel_stack));

extern void set_stack_and_jump(void *top, BootInfo *boot_info, void (*kernel_main)(BootInfo *boot_info));

void kernel_main(BootInfo *boot_info) {
    kprintln("Switched to kernel stack.");

    boot_modules_init(boot_info);

    gdt_init();
    idt_init();

    pmm_init(boot_info);
    vmm_init();

    pmm_print_stats();

    alloc_init();

    fb_init(boot_info);

    pic_remap(32); // After CPU exceptions
    ps2_keyboard_init();

    msr_ensure();
    syscalls_init();

    char vendor[13];
    cpuid_read_vendor(vendor);
    kprintln("CPU vendor: %s", vendor);

    cpuid_check_apic() ? kprintln("APIC is supported.") : kprintln("APIC is not supported.");

    fb_clear(COLOR_BLACK);

    fb_draw_text(fb_width() / 2 - 20, fb_height() / 2 - 4, "light", COLOR_WHITE);

    pmm_print_stats();

    kprintln("Kernel init ended. Switching to userspace.");

    for (int i = 0; i < 40; i++) kernel_printf("\n");

    start_first_user_process();

    // while (1) {
    //     __asm__ volatile("hlt");
    // }
}

void _start(BootInfo *boot_info) {
    init_serial();
    puts_serial("Kernel started.\n");

    set_stack_and_jump(kernel_stack_top, boot_info, kernel_main);
}
