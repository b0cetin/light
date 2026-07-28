
#include "tss.h"
#include "gdt.h"
#include "kernel_lib.h"
#include <stdint.h>

TSS64 tss;

void write_tss_descriptor(GDTEntry *gdt, uint32_t index, uint64_t base, uint32_t limit) {
    // Slot 1 (Low 8 bytes): Standard descriptor fields + Access = 0x89
    gdt[index] = (GDTEntry){
        .limit_low   = limit & 0xFFFF,
        .base_low    = base & 0xFFFF,
        .base_middle = (base >> 16) & 0xFF,
        .access      = 0x89,                  // Present, Ring 0, Type 0x9 (Available 64-bit TSS)
        .flags       = (limit >> 16) & 0x0F,  // Byte granularity (no flags shift needed here)
        .base_high   = (base >> 24) & 0xFF
    };

    // Slot 2 (High 8 bytes): Holds upper 32 bits of base address
    uint64_t base_high_32 = base >> 32;
    gdt[index + 1] = (GDTEntry){
        .limit_low   = base_high_32 & 0xFFFF,         // Base bits 32–47
        .base_low    = (base_high_32 >> 16) & 0xFFFF, // Base bits 48–63
        .base_middle = 0,
        .access      = 0,
        .flags       = 0,
        .base_high   = 0
    };
}

// Should only be invoked by gdt.c.
// kernel_stack_top is the virtual address.
void tss_init(GDTEntry *gdt, void *kernel_stack_top)
{
    memzero(&tss, sizeof(tss));

    tss.rsp0 = (uint64_t) kernel_stack_top;

    write_tss_descriptor(gdt, 5, (uint64_t)&tss, sizeof(tss) - 1);
}
