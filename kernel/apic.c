
#include "apic.h"
#include "acpi/madt.h"
#include "debugging.h"
#include "interrupts.h"
#include "io.h"
#include "msr.h"
#include "processor_info.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

MADT *madt;

void *lapic_base;
static uint32_t lapic_timer_ticks_per_ms = 0;

#define MSR_IA32_APIC_BASE_ENABLE (1 << 11)
#define APIC_VECTOR_DISABLED (1 << 16)

#define REG_SPURIOUS_INTERRUPT_VECTOR 0xF0
#define REG_END_OF_INTERRUPT 0xB0
#define REG_TASK_PRIORITY 0x80
#define REG_LVT_ERROR_VECTOR 0x0370

#define REG_LAPIC_TIMER_LVT 0x320
#define REG_LAPIC_TIMER_INIT 0x380
#define REG_LAPIC_TIMER_CURRENT 0x390
#define REG_LAPIC_TIMER_DIV 0x3E0

#define APIC_TIMER_MODE_PERIODIC (1 << 17)
#define APIC_TIMER_MASKED (1 << 16)
#define APIC_TIMER_DIV_16 0x0B

static inline void lapic_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t*) ((uint8_t*) lapic_base + offset) = value;
}

static inline uint32_t lapic_read(uint32_t offset) {
    return *(volatile uint32_t*) ((uint8_t*) lapic_base + offset);
}

static void pic_mask_all() {
    static const uint8_t PIC_MASTER_DATA = 0x21;
    static const uint8_t PIC_SLAVE_DATA = 0xA1;

    outb(PIC_MASTER_DATA, 0xff);
    outb(PIC_SLAVE_DATA, 0xff);

    kprintln("PIC fully masked.");
}

static void pit_sleep(uint32_t ms) {
    uint32_t count = ms * 1193; // 1193 ticks per ms (1.193182 MHz base rate)
    
    // Configure PIT Channel 2 in one-shot mode
    outb(0x61, (inb(0x61) & 0xDD) | 0x01); // Enable speaker gate, disable speaker output
    outb(0x43, 0xB0); // Channel 2, LSB/MSB, Mode 0, Binary
    outb(0x42, (uint8_t)(count & 0xFF)); // Initial count split LSB
    outb(0x42, (uint8_t)((count >> 8) & 0xFF)); // Initial count split MSB

    // Reset speaker flip-flop gate bit
    uint8_t val = inb(0x61) & 0xFE;
    outb(0x61, val); // Pull gate LOW to reset
    outb(0x61, val | 1); // Pull gate HIGH to start

    // Wait until PIT counter reaches 0 (Bit 5 goes high)
    while (!(inb(0x61) & 0x20));
}

void lapic_spurious_interrupt(InterruptRegisters *_) {
    kprintln("APIC: LAPIC: Spurious interrupt received!");
    // Spurious interrupts don't need EOI.
}

// Requires the ACPI manager to be initialized.
void apic_init() {
    // NOTE: This is written in a time where the kernel is single-core.
    // In the future, the LAPIC initialization code (not detection) will need to be run in every core.

    if (!cpuid_check_apic())
        PANIC("This system doesn't support APIC!");

    madt = acpi_find_madt();

    if (madt == null)
        PANIC("MADT could not be resolved for APIC!");

    pic_mask_all();

    uint64_t phys_base = madt->physical_local_interrupt_controller_address;

    uint8_t *cursor = ((uint8_t*) madt) + sizeof(MADT);
    while ((cursor - (uint8_t*) madt) < madt->header.length) {
        MADTRecordHeader *header = (MADTRecordHeader*) cursor;

        if (header->length == 0) break;

        if (header->type == MADT_TYPE_LAPIC_ADDRESS_OVERRIDE) {
            MADTLAPICAddressOverrideRecord *record = (MADTLAPICAddressOverrideRecord*) cursor;
            phys_base = record->physical_address_override;
        }

        cursor += header->length;
    }

    vmm_kmap_mmio(phys_base, PAGE_SIZE);
    lapic_base = p2v(phys_base);

    kprintln("APIC: LAPIC defined at: %lx", (uint64_t) lapic_base);

    // Hardware-enable the APIC
    uint64_t apic_base = msr_read(MSR_IA32_APIC_BASE);
    if (!(apic_base & MSR_IA32_APIC_BASE_ENABLE)) {
        apic_base |= MSR_IA32_APIC_BASE_ENABLE;
        msr_write(MSR_IA32_APIC_BASE, apic_base);
    }

    // Software-enable the APIC
    lapic_write(REG_SPURIOUS_INTERRUPT_VECTOR, 0x01FF);

    // Set the priority register to allow all
    lapic_write(REG_TASK_PRIORITY, 0);

    // Disable the error vector (or implement a handler in the future idk)
    lapic_write(REG_LVT_ERROR_VECTOR, APIC_VECTOR_DISABLED);

    // Implement our custom interrupt handlers
    register_interrupt_handler(0xFF, lapic_spurious_interrupt);

    kprintln("APIC enabled.");
}

void lapic_end_of_interrupt() {
    lapic_write(REG_END_OF_INTERRUPT, 0);
}

void lapic_timer_init(uint8_t vector, uint32_t frequency) {
    static const uint32_t start = 0xFFFFFFFF;

    lapic_write(REG_LAPIC_TIMER_DIV, APIC_TIMER_DIV_16);

    // Make the timer masked (so it doesn't issue interrupts) and set it to a vector.
    lapic_write(REG_LAPIC_TIMER_LVT, APIC_TIMER_MASKED | vector);

    // Set the initial count of an ungodly high number so it doesn't try to issue interrupts.
    lapic_write(REG_LAPIC_TIMER_INIT, start);

    pit_sleep(10); // Sleep for 10 milliseconds using the PIT.

    lapic_write(REG_LAPIC_TIMER_LVT, APIC_TIMER_MASKED); // Completely disabled.

    uint32_t elapsed_ticks = start - lapic_read(REG_LAPIC_TIMER_CURRENT);

    lapic_timer_ticks_per_ms = elapsed_ticks / 10; // We waited for 10 ms.

    kprintln("APIC: LAPIC: Timer calibrated at %d ticks/ms", lapic_timer_ticks_per_ms);

    uint32_t target_period_ms = 1000 / frequency;
    uint32_t initial_count = lapic_timer_ticks_per_ms * target_period_ms;

    lapic_write(REG_LAPIC_TIMER_DIV, APIC_TIMER_DIV_16);
    lapic_write(REG_LAPIC_TIMER_LVT, APIC_TIMER_MODE_PERIODIC | vector);
    lapic_write(REG_LAPIC_TIMER_INIT, initial_count);

    kprintln("APIC: LAPIC: Timer enabled at %d Hz (Vector %d)", frequency, vector);
} 
