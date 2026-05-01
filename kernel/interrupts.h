
#pragma once

#include <stdint.h>

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t interrupt_number;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss; // Pushed automatically by CPU
} __attribute__((packed)) InterruptRegisters;

void register_interrupt_handler(uint64_t interrupt_index, void (*int_handler)(InterruptRegisters *));
