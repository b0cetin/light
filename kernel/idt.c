
#include "idt.h"
#include "debugging.h"
#include <stdint.h>

typedef struct {
    uint16_t isr_low;   // The lower 16 bits of the ISR's address
    uint16_t kernel_cs; // The GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t ist;        // The IST in the TSS that the CPU will load into RSP; set to zero for now
    uint8_t attributes; // Type and attributes; see the IDT page
    uint16_t isr_mid;   // The higher 16 bits of the lower 32 bits of the ISR's address
    uint32_t isr_high;  // The higher 32 bits of the ISR's address
    uint32_t reserved;  // Set to zero
} __attribute__((packed)) IDTEntry;

__attribute__((aligned(0x10))) IDTEntry idt[256];

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) IDTR;

IDTR idtr;

void set_idt_gate(int num, uint64_t handler) {
    idt[num].isr_low = (uint16_t)(handler & 0xFFFF);
    idt[num].kernel_cs = 0x08; // Kernel Code Segment
    idt[num].ist = 0;
    idt[num].attributes = 0x8E; // 0b10001110 -> Present, Ring 0, Interrupt Gate
    idt[num].isr_mid = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[num].isr_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[num].reserved = 0;
}

extern uint64_t isr_stub_table[256]; // Grabbed from `idt_asm.s`

void idt_init() {
    for (int i = 0; i < 256; i++) {
        set_idt_gate(i, isr_stub_table[i]);
    }

    // Load IDT
    idtr.limit = (sizeof(IDTEntry) * 256) - 1;
    idtr.base = (uint64_t)&idt;
    __asm__ volatile("lidt %0" : : "m"(idtr));

    kernel_println("IDT initialized.");
}
