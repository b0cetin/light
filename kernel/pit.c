
#include "pit.h"
#include "debugging.h"
#include "interrupts.h"
#include "pic.h"

// TODO: Unfinished.

uint8_t count = 0;

void handle_pit_irq0_interrupt(InterruptRegisters *_) {
    count++;

    if (count >= 64) {
        count = 0;
        kernel_println("pit irq x64");
    }
    pic_send_eoi(PIT_IRQ0);
}

void pit_init() {
    pic_clear_mask(PIT_IRQ0);
    register_interrupt_handler(PIT_IRQ0, handle_pit_irq0_interrupt);

    kernel_println("PIT driver initialized.");
}
