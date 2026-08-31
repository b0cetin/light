
#include "interrupts.h"
#include "cpu_exceptions.h"
#include "debugging.h"
#include "types.h"
#include "user_interrupts.h"
#include <stdint.h>

#define INTERRUPT_COUNT 256

// First 32 are reserved.
void (*int_handlers[INTERRUPT_COUNT])(InterruptRegisters *);

void isr_handler(InterruptRegisters *regs) {
    if (regs->interrupt_number < 32) {
        handle_cpu_exception(regs);
        return;
    }

    if (regs->interrupt_number >= INTERRUPT_COUNT) {
        PANIC("Interrupt index reached over possible threshold (%d): %d", INTERRUPT_COUNT, regs->interrupt_number);
    }

    if (int_handlers[regs->interrupt_number] != null) {
        int_handlers[regs->interrupt_number](regs);
        return;
    }

    PANIC("No handler registered for interrupt index: %d", regs->interrupt_number);
}

void register_interrupt_handler(uint8_t interrupt_index, void (*int_handler)(InterruptRegisters *)) {
    if (interrupt_index < 32)
        PANIC("Interrupt index is smaller 32, reaching into CPU exceptions!");

    if (interrupt_index == 32 || interrupt_index == 0x81)
        PANIC("Tried to register interrupt handler for 32 or 0x81. Already reserved for context switching!");

    if (int_handlers[interrupt_index] != null)
        PANIC("Interrupt index re-registered: %d", interrupt_index);

    int_handlers[interrupt_index] = int_handler;
}

void interrupts_remove_interrupt_handler(uint8_t interrupt_index, void (*int_handler)(InterruptRegisters *)) {
    if (int_handlers[interrupt_index] == null)
        PANIC("Clear interrupt handler re-cleared: %d", interrupt_index);

    if (int_handlers[interrupt_index] != int_handler)
        PANIC("Can't remove interrupt handler: Handler mismatch! %lx != %lx", int_handlers[interrupt_index], int_handler);

    int_handlers[interrupt_index] = null;
}

bool interrupts_is_handler_registered(uint8_t interrupt_index) {
    return int_handlers[interrupt_index] != null;
}

#define USER_IRQ_VECTOR_BASE 48

// Returns an unused interrupt vector. Returns 0 if none found.
uint8_t interrupts_get_empty_vector() {
    for (uint8_t i = USER_IRQ_VECTOR_BASE; i < UINT8_MAX; i++) {
        if (i == 0x81) continue;

        if (int_handlers[i] == null)
            return i;
    }

    return 0;
}
