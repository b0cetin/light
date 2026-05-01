
#include "pic.h"
#include "debugging.h"
#include "io.h"
#include <stdint.h>

#define ICW1_ICW4 0x01      /* Indicates that ICW4 will be present */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

#define CASCADE_IRQ 2
#define CASCADE_IDENTITY 0x02

// Writing to port 0x80 is a standard way to wait ~1-2 microseconds.
static inline void io_wait() {
    outb(0x80, 0);
}

static inline void io_out(uint16_t port, uint8_t val) {
    outb(port, val);
    io_wait();
}

void pic_remap(int32_t offset) {
    io_out(PIC_MASTER_COMMAND, ICW1_INIT | ICW1_ICW4); // starts the initialization sequence (in cascade mode)
    io_out(PIC_SLAVE_COMMAND, ICW1_INIT | ICW1_ICW4);

    io_out(PIC_MASTER_DATA, offset);           // ICW2: Master PIC vector offset
    io_out(PIC_SLAVE_DATA, offset + 8);        // ICW2: Slave PIC vector offset
    io_out(PIC_MASTER_DATA, 1 << CASCADE_IRQ); // ICW3: tell Master PIC that there is a slave PIC at IRQ2
    io_out(PIC_SLAVE_DATA, CASCADE_IDENTITY);  // ICW3: tell Slave PIC its cascade identity

    io_out(PIC_MASTER_DATA, ICW4_8086); // ICW4: have the PICs use 8086 mode (and not 8080 mode)
    io_out(PIC_SLAVE_DATA, ICW4_8086);

    pic_mask_all();

    kernel_println("PIC initialized and fully masked.");
}

void pic_set_mask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq_line -= 8;
    }
    value = inb(port) | (1 << irq_line);
    outb(port, value);

    kernel_println("PIC: Masking IRQ line %d.", irq_line);
}

void pic_clear_mask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = PIC_MASTER_DATA;
    } else {
        port = PIC_SLAVE_DATA;
        irq_line -= 8;
    }
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);

    kernel_println("PIC: Unmasking IRQ line %d.", irq_line);
}

void pic_mask_all() {
    outb(PIC_MASTER_DATA, 0xff);
    outb(PIC_SLAVE_DATA, 0xff);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC_SLAVE_COMMAND, 0x20);

    outb(PIC_MASTER_COMMAND, 0x20);
}
