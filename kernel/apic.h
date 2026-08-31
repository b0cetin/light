
#pragma once

#include <stdint.h>

void apic_timer_init();
void apic_init();

void lapic_end_of_interrupt();

// Frequency is in Hertz.
void lapic_timer_set(uint8_t vector, uint32_t frequency);

void ioapic_route(uint8_t irq, uint8_t vector);
void ioapic_mask(uint8_t irq);
uint8_t ioapic_get_gsi_count();
