
#pragma once

#include "gdt.h"
#include <stdint.h>

typedef struct {
    uint32_t reserved0;
    uint64_t rsp0;        // Ring 0 Stack Pointer
    uint64_t rsp1;        // Ring 1 Stack Pointer
    uint64_t rsp2;        // Ring 2 Stack Pointer
    uint64_t reserved1;
    uint64_t ist1;        // Interrupt Stack Table 1
    uint64_t ist2;        // Interrupt Stack Table 2
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;  // Offset to I/O Permission Bit Map
} __attribute__((packed)) TSS64;

void tss_init(GDTEntry *gdt);
void tss_set_rsp0(void *kernel_stack_top);
