
#include "gdt.h"
#include "debugging.h"
#include "tss.h"
#include <stdint.h>

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) GDTDescriptor;

static GDTEntry gdt[7];
static GDTDescriptor gdt_desc;

extern void gdt_flush(GDTDescriptor *gdt_ptr); // Assembly code

extern void tss_flush(void); // Assembly code

GDTEntry setup_entry(uint8_t access, uint8_t flags)
{
    return (GDTEntry){0, 0, 0, access, flags, 0};
}

void gdt_init(void *kernel_stack_top) {
    // Null Descriptor
    gdt[0] = (GDTEntry){0, 0, 0, 0, 0, 0};

    // Magic access and flag values from https://wiki.osdev.org/GDT_Tutorial

    // Kernel Code
    gdt[1] = setup_entry(0x9A, 0xA0);

    // Kernel Data
    gdt[2] = setup_entry(0x92, 0xC0);

    // User Data
    gdt[3] = setup_entry(0xF2, 0xC0);

    // User Code
    gdt[4] = setup_entry(0xFA, 0xA0);

    // Task State Segment
    tss_init(gdt, kernel_stack_top);

    gdt_desc.limit = sizeof(gdt) - 1;
    gdt_desc.base = (uint64_t)&gdt;

    gdt_flush(&gdt_desc);

    tss_flush();

    kernel_println("GDT & TSS initialized.");
}
