
#pragma once
#include "serial.h"

void idt_init();

static inline void enable_interrupts() {
    __asm__ volatile("sti");
    puts_serial("Interrupts enabled.\n");
}
