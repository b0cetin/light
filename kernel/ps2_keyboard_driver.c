
#include "ps2_keyboard_driver.h"
#include "debugging.h"
#include "interrupts.h"
#include "io.h"
#include "pic.h"

void handle_keyboard_interrupt(InterruptRegisters *_) {
    uint8_t scancode = inb(0x60);

    kernel_println("Keyboard IRQ! Scancode: %x", scancode);

    pic_send_eoi(PS2_KEYBOARD_IRQ);
}

void ps2_keyboard_init() {
    pic_clear_mask(PS2_KEYBOARD_IRQ);
    register_interrupt_handler(PS2_KEYBOARD_IRQ, handle_keyboard_interrupt);

    kernel_println("PS/2 keyboard driver initialized.");
}
