
#include "bootinfo.h"
#include "debugging.h"
#include "fb_graphics.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "pmm.h"
#include "processor_info.h"
#include "ps2_keyboard_driver.h"
#include "serial.h"
#include "vmm.h"
#include <stdint.h>

void kernel_main(BootInfo *boot_info) {
    kernel_println("Switched to kernel stack.");

    gdt_init();
    idt_init();

    pmm_init(boot_info);
    vmm_init();

    pmm_print_stats();

    fb_init(boot_info);

    pic_remap(32); // After CPU exceptions
    ps2_keyboard_init();
    pit_init();
    enable_interrupts();

    // char vendor[13];
    // read_cpu_vendor_id(vendor);
    // kernel_println(vendor);

    fb_clear(COLOR_BLACK);
    fb_draw_pixel(50, 50, COLOR_RED);
    fb_draw_rect(100, 100, 50, 50, COLOR_GREEN);

    fb_draw_text(50, 500, "Hello, world!", COLOR_WHITE);

    pmm_print_stats();

    while (1) {
        __asm__ volatile("hlt");
    }
}

// 16KB stack: usually enough for whatever will happen, increase if necessary
uint8_t kernel_stack[16384];
// Aligned to 16 bytes for the System V ABI
void *kernel_stack_top = (void *)((uint64_t)kernel_stack + sizeof(kernel_stack));

extern void set_stack_and_jump(void *top, BootInfo *boot_info, void (*kernel_main)(BootInfo *boot_info));

void _start(BootInfo *boot_info) {
    init_serial();
    puts_serial("Kernel started.\n");

    set_stack_and_jump(kernel_stack_top, boot_info, kernel_main);
}
