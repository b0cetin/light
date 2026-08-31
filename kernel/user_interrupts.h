
#pragma once

#include "apic.h"
#include "processes.h"
#include "types.h"
#include <stdint.h>

typedef struct {
    Process *reserver;
    Thread *awaiter;
    uint8_t irq;
    uint8_t vector;
    uint16_t queue;
} ReservableIRQ;

#define RESERVABLE_IRQ_TABLE_MAX 256

#define USER_IRQ_AWAKE_INTERRUPT 0
#define USER_IRQ_AWAKE_CANCEL 1

static inline bool user_irq_is_irq_in_range(uint8_t irq) {
    return irq < ioapic_get_gsi_count();
}

static inline bool user_irq_is_reserved(ReservableIRQ *reservation) {
    return reservation->reserver != null;
}

ReservableIRQ *user_irq_get_reservation(uint8_t irq);
ReservableIRQ *user_irq_find_reservation_from_vector(uint8_t vector);

void user_irq_reserve(uint8_t irq, Process *process);
void user_irq_unreserve(uint8_t irq, Process *process);
void user_irq_force_unreserve_all(Process *process);

void user_irq_await(uint8_t irq, Thread *thread);
void user_irq_awaken_by_vector(uint8_t vector);
void user_irq_cancel(uint8_t irq, Process *process);
