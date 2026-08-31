
#pragma once

#include "apic.h"
#include "processes.h"
#include "types.h"
#include <stdint.h>

typedef struct {
    uint8_t irq;
    uint8_t assigned_vector;
    Process *reserver;
    Thread *awaiter;
    uint16_t queue;
} UserIRQReservation;

#define USER_IRQ_AWAKE_INTERRUPT 0
#define USER_IRQ_AWAKE_CANCEL 1

void user_irq_init();

static inline bool user_irq_is_irq_in_range(uint8_t irq) {
    return irq < ioapic_get_gsi_count();
}

UserIRQReservation *user_irq_get_reservation(uint8_t irq);
UserIRQReservation *user_irq_find_reservation_from_vector(uint8_t vector);

void user_irq_reserve(uint8_t irq, Process *process);
void user_irq_unreserve(uint8_t irq, Process *process);
void user_irq_force_unreserve_all(Process *process);

bool user_irq_await(uint8_t irq, Thread *thread);
void user_irq_cancel(uint8_t irq, Process *process);
