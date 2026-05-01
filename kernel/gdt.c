
#include "gdt.h"
#include "debugging.h"
#include <stdint.h>

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) GDTEntry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) GDTDescriptor;

static GDTEntry gdt[3];
static GDTDescriptor gdt_desc;

extern void gdt_flush(GDTDescriptor *gdt_ptr);

void gdt_init() {
    // Null Descriptor
    gdt[0] = (GDTEntry){0, 0, 0, 0, 0, 0};

    // Kernel Code (Access: 0x9A, Granularity: 0xA0)
    gdt[1] = (GDTEntry){0, 0, 0, 0x9A, 0xA0, 0}; // 10011010: Present (1), Ring 0 (00), Code (1), Executable (1), Readable (1)

    // Kernel Data (Access: 0x92, Granularity: 0x00)
    gdt[2] = (GDTEntry){0, 0, 0, 0x92, 0x00, 0}; // 10010010: Present (1), Ring 0 (00), Data (1), Writable (1)

    gdt_desc.limit = sizeof(gdt) - 1;
    gdt_desc.base = (uint64_t)&gdt;

    gdt_flush(&gdt_desc);

    kernel_println("GDT initialized.");
}
