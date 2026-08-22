
#include "interrupts.h"
#include "cpu_exceptions.h"
#include "debugging.h"
#include "types.h"
#include "user_interrupts.h"
#include <stdint.h>

#define INTERRUPT_COUNT 224

void (*int_handlers[INTERRUPT_COUNT])(InterruptRegisters *);

void register_interrupt_handler(uint64_t interrupt_index, void (*int_handler)(InterruptRegisters *)) {
    if (interrupt_index == 0) {
        PANIC("Tried to register interrupt handler for 32. Already reserved for context switching!");
    }

    if (int_handlers[interrupt_index] != null) {
        PANIC("Interrupt index re-registered: %d", interrupt_index);
    }

    int_handlers[interrupt_index] = int_handler;
}

void isr_handler(InterruptRegisters *regs) {
    kprintln("Interrupt %d", regs->interrupt_number);

    if (regs->interrupt_number < 32) {
        handle_cpu_exception(regs);
        return;
    }

    uint64_t interrupt_index = regs->interrupt_number - 32;

    if (interrupt_index >= INTERRUPT_COUNT) {
        PANIC("Interrupt index reached over possible threshold (%d): %d", INTERRUPT_COUNT, interrupt_index);
    }

    if (int_handlers[interrupt_index] != null) {
        int_handlers[interrupt_index](regs);
        return;
    }

    if (user_irq_get_reservation(interrupt_index)->reserver != null) {
        user_irq_awaken(interrupt_index);
        return;
    }

    PANIC("No handler registered for interrupt index: %d", interrupt_index);
}
