
#include <stdio.h>
#include <syscalls.h>
#include <stddef.h>
#include <stdint.h>

int main() {
    println("Keyboard driver started.");

    sys_interrupt_control(IRQCTL_SET, 1);

    while (1) {
        if (sys_interrupt_control(IRQCTL_AWAIT, 1) != SYS_SUCCESS)
            break;

        uint8_t scancode = sys_port_io_in(0x60, PIO_SIZE_BYTE);
        println("Scancode: %#hhx", scancode);
    }

    println("Shutting down keyboard driver.");

    sys_interrupt_control(IRQCTL_UNSET, 1);

    return 0;
}
