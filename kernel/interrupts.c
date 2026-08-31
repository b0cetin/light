
#include "interrupts.h"
#include "cpu_exceptions.h"
#include "debugging.h"
#include "types.h"
#include "user_interrupts.h"
#include <stdint.h>

#define INTERRUPT_COUNT 256

// First 32 are reserved.
void (*int_handlers[INTERRUPT_COUNT])(InterruptRegisters *);

void register_interrupt_handler(uint8_t interrupt_index, void (*int_handler)(InterruptRegisters *)) {
    if (interrupt_index < 32)
        PANIC("Interrupt index is smaller 32, reaching into CPU exceptions!");

    if (interrupt_index == 32 || interrupt_index == 0x81)
        PANIC("Tried to register interrupt handler for 32 or 0x81. Already reserved for context switching!");

    if (int_handlers[interrupt_index] != null)
        PANIC("Interrupt index re-registered: %d", interrupt_index);

    int_handlers[interrupt_index] = int_handler;
}

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

    if (user_irq_find_reservation_from_vector(regs->interrupt_number)->reserver != null) {
        user_irq_awaken_by_vector(regs->interrupt_number);
        return;
    }

    PANIC("No handler registered for interrupt index: %d", regs->interrupt_number);
}

bool interrupts_is_handler_registered(uint8_t interrupt_index) {
    return int_handlers[interrupt_index] != null;
}

// Returns an unused interrupt vector. Returns 0 if none found.
uint8_t interrupts_get_empty_vector() {
    for (uint8_t i = 33; i < UINT8_MAX; i++) {
        if (i == 0x81) continue;

        if (int_handlers[i] == null)
            return i;
    }

    return 0;
}
