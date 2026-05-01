
#include "pit.h"
#include "debugging.h"
#include "interrupts.h"
#include "pic.h"

// TODO: Unfinished.

void handle_pit_irq0_interrupt(InterruptRegisters *_) {
    kernel_println("pit irq");
    pic_send_eoi(PIT_IRQ0);
}

void pit_init() {
    pic_clear_mask(PIT_IRQ0);
    register_interrupt_handler(PIT_IRQ0, handle_pit_irq0_interrupt);

    kernel_println("PIT driver initialized.");
}
