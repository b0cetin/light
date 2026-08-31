
#pragma once

#include "types.h"
#include <stdint.h>

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t interrupt_number;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss; // Pushed automatically by CPU
} __attribute__((packed)) InterruptRegisters;

void register_interrupt_handler(uint8_t interrupt_index, void (*int_handler)(InterruptRegisters *));

bool interrupts_is_handler_registered(uint8_t interrupt_index);
uint8_t interrupts_get_empty_vector();
