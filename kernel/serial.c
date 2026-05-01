
#include "serial.h"
#include "io.h"
#include "types.h"
#include <stdint.h>

#define COM1 0x3F8

bool serial_initialized;

int init_serial() {
    if (serial_initialized)
        return 0;

    outb(COM1 + 1, 0x00); // Disable all interrupts
    outb(COM1 + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(COM1 + 0, 0x03); // Set divisor to 3 (38400 baud)
    outb(COM1 + 1, 0x00); //                  (hi byte)
    outb(COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B); // IRQs enabled, RTS/DSR set

    // Check if serial is faulty (Loopback test)
    outb(COM1 + 4, 0x1E); // Set in loopback mode
    outb(COM1 + 0, 0xAE); // Test serial chip
    if (inb(COM1 + 0) != 0xAE)
        return 1;

    outb(COM1 + 4, 0x0F); // Set in normal operation mode

    serial_initialized = true;
    return 0;
}

int is_transmit_empty() {
    if (!serial_initialized)
        return 0;

    return inb(COM1 + 5) & 0x20;
}

void write_serial(char a) {
    if (!serial_initialized)
        return;

    while (is_transmit_empty() == 0)
        ;
    outb(COM1, a);
}

void puts_serial(char *str) {
    if (!serial_initialized)
        return;

    for (int i = 0; str[i] != '\0'; i++) {
        write_serial(str[i]);
    }
}
