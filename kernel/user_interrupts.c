
#include "user_interrupts.h"
#include "debugging.h"
#include "pic.h"
#include "processes.h"
#include "types.h"
#include <stdint.h>

static ReservableIRQ table[RESERVABLE_IRQ_TABLE_MAX];

ReservableIRQ *user_irq_get_reservation(uint8_t vector) {
    if (!user_irq_is_vector_in_range(vector))
        PANIC("user_irq_get_reservation called with vector %d: Invalid range!", vector);

    return &table[vector];
}

void user_irq_reserve(uint8_t vector, Process *process) {
    ReservableIRQ *entry = user_irq_get_reservation(vector);

    if (user_irq_is_reserved(entry))
        PANIC("user_irq_reserve called on reserved vector.");

    entry->reserver = process;
    entry->awaiter = null;

    pic_clear_mask(vector);

    kprintln("USER_IRQ: Vector %d is now reserved for process %ld.",
        vector, process->pid);
}

void user_irq_unreserve(uint8_t vector, Process *process) {
    ReservableIRQ *entry = user_irq_get_reservation(vector);

    if (entry->awaiter != null)
        PANIC("user_irq_unreserve called on awaited vector.");

    if (entry->reserver != process)
        PANIC("user_irq_unreserve called for process that didn't reserve it.");

    entry->reserver = null;

    pic_set_mask(vector);

    kprintln("USER_IRQ: Vector %d is now unreserved.", vector);
}

// If the thread is currently active, do not forget to context switch away from it.
// Only do this after a check to see if the thread was blocked. If there are interrupts
// enqueued, then no blocking will be done.
void user_irq_await(uint8_t vector, Thread *thread) {
    ReservableIRQ *entry = user_irq_get_reservation(vector);

    if (entry->reserver != thread->owner)
        PANIC("user_irq_await called for process that didn't reserve it.");

    if (entry->awaiter != null)
        PANIC("user_irq_await called on awaited vector.");

    if (entry->queue > 0) {
        thread->wake_result = USER_IRQ_AWAKE_INTERRUPT;
        entry->queue--;
        return;
    }

    entry->awaiter = thread;

    process_block_thread(thread, THREADBLOCK_IRQ, (ThreadBlockTarget) { .irq_vector = vector });
}

void user_irq_awaken(uint8_t vector)
{
    ReservableIRQ *entry = user_irq_get_reservation(vector);

    pic_send_eoi(vector);

    if (entry->awaiter == null) {
        entry->queue++;
        return;
    }

    process_unblock_thread(entry->awaiter, USER_IRQ_AWAKE_INTERRUPT);
    entry->awaiter = null;
}

void user_irq_cancel(uint8_t vector, Process *process) {
    ReservableIRQ *entry = user_irq_get_reservation(vector);

    if (entry->reserver != process)
        PANIC("user_irq_cancel called by process that didn't reserve it.");

    if (entry->awaiter == null)
        PANIC("user_irq_cancel called on a vector that is not being awaited.");

    process_unblock_thread(entry->awaiter, USER_IRQ_AWAKE_CANCEL);
    entry->awaiter = null;
}

void user_irq_force_unreserve_all(Process *process) {
    uint8_t unreserved_count = 0;

    for (uint8_t reservation_index = 0; reservation_index < RESERVABLE_IRQ_TABLE_MAX; reservation_index++) {
        if (table[reservation_index].reserver == process) {
            table[reservation_index].reserver = null;
            table[reservation_index].awaiter = null;
            
            unreserved_count++;
        }
    }

    if (unreserved_count > 0) {
        kprintln("USER_IRQ: Unreserved all reservations of process %ld. There were %d.",
            process->pid, unreserved_count);
    }
}
