
#pragma once

#include "processes.h"
#include "types.h"
#include <stdint.h>

typedef struct {
    Process *reserver;
    Thread *awaiter;
    uint16_t queue;
} ReservableIRQ;

#define RESERVABLE_IRQ_TABLE_MAX 15

#define USER_IRQ_AWAKE_INTERRUPT 0
#define USER_IRQ_AWAKE_CANCEL 1

static inline bool user_irq_is_vector_in_range(uint8_t vector) {
    return vector > 0 && vector < RESERVABLE_IRQ_TABLE_MAX && vector != 2;
}

static inline bool user_irq_is_reserved(ReservableIRQ *reservation) {
    return reservation->reserver != null;
}

ReservableIRQ *user_irq_get_reservation(uint8_t vector);

void user_irq_reserve(uint8_t vector, Process *process);
void user_irq_unreserve(uint8_t vector, Process *process);
void user_irq_force_unreserve_all(Process *process);

void user_irq_await(uint8_t vector, Thread *thread);
void user_irq_awaken(uint8_t vector);
void user_irq_cancel(uint8_t vector, Process *process);
