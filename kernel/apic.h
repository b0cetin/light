
#pragma once

#include <stdint.h>

void apic_init();

void lapic_end_of_interrupt();

// Frequency is in Hertz.
void lapic_timer_init(uint8_t vector, uint32_t frequency);
