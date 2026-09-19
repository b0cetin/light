
#include "defined_syscalls.h"
#include "io.h"

uint32_t sys_port_io_in(uint16_t port, PORTIOSize size) {
    switch (size) {
        case PIO_SIZE_BYTE:
            return (uint32_t) inb(port);
        case PIO_SIZE_SHORT:
            return (uint32_t) inw(port);
        case PIO_SIZE_INT:
            return (uint32_t) inl(port);
        default:
            return 0;
    }
} 

void sys_port_io_out(uint16_t port, PORTIOSize size, uint32_t out) {
    switch (size) {
        case PIO_SIZE_BYTE:
            outb(port, out);
            break;
        case PIO_SIZE_SHORT:
            outw(port, out);
            break;
        case PIO_SIZE_INT:
            outl(port, out);
            break;
    }
}
