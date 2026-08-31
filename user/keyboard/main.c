
#include <stdio.h>
#include <syscalls.h>
#include <stddef.h>
#include <stdint.h>

int main() {
    println("Keyboard driver started.");

    while (sys_port_io_in(0x64, PIO_SIZE_BYTE) & 1) {
        sys_port_io_in(0x60, PIO_SIZE_BYTE); // Read and discard stale bytes
        println("Discarding stale byte...");
    }

    int64_t status = 0;

    status = sys_interrupt_control(IRQCTL_SET, 1);

    if (status != SYS_SUCCESS) {
        println("IRQCTL_SET returned %li!", status);
        return -1;
    }

    while (1) {
        status = sys_interrupt_control(IRQCTL_AWAIT, 1);
        if (status != SYS_SUCCESS) {
            println("IRQCTL_AWAIT returned %li!", status);
            break;
        }

        uint8_t scancode = sys_port_io_in(0x60, PIO_SIZE_BYTE);
        println("Scancode: %#hhx", scancode);
    }

    println("Shutting down keyboard driver.");

    sys_interrupt_control(IRQCTL_UNSET, 1);

    return 0;
}
