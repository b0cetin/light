
#include "user_interrupts.h"
#include "apic.h"
#include "debugging.h"
#include "interrupts.h"
#include "processes.h"
#include "types.h"
#include "utils/hashtables/u64toaddr_hashtable.h"
#include <stdint.h>

// FIXME: This system will probably need a refactor/redesign if APIC being adopted.

static ReservableIRQ table[RESERVABLE_IRQ_TABLE_MAX];
static Hashtable *vector_to_entry;

ReservableIRQ *user_irq_get_reservation(uint8_t irq) {
    if (!user_irq_is_irq_in_range(irq))
        PANIC("user_irq_get_reservation called with IRQ %d: Invalid range!", irq);

    return &table[irq];
}

ReservableIRQ *user_irq_find_reservation_from_vector(uint8_t vector) {
    ReservableIRQ *reservation = null;
    ht_lookup(vector_to_entry, vector, (uintptr_t*) &reservation);
    return reservation; // Will already return null if none found
}

void user_irq_reserve(uint8_t irq, Process *process) {
    ReservableIRQ *entry = user_irq_get_reservation(irq);

    if (user_irq_is_reserved(entry))
        PANIC("user_irq_reserve called on reserved vector.");

    entry->reserver = process;
    entry->awaiter = null;
    entry->vector = interrupts_get_empty_vector(); // FIXME: No.
    entry->irq = irq;

    if (!ht_add(vector_to_entry, irq, (uintptr_t) entry))
        PANIC("Cannot add reservation (IRQ %d) to vector hashtable!", irq);

    ioapic_route(irq, entry->vector);

    kprintln("USER_IRQ: IRQ %d (vector %d) is now reserved for process %ld.",
        irq, entry->vector, process->pid);
}

void user_irq_unreserve(uint8_t irq, Process *process) {
    ReservableIRQ *entry = user_irq_get_reservation(irq);

    if (entry->awaiter != null)
        PANIC("user_irq_unreserve called on awaited vector.");

    if (entry->reserver != process)
        PANIC("user_irq_unreserve called for process that didn't reserve it.");

    entry->reserver = null;
    entry->awaiter = null;
    entry->irq = 0;
    entry->vector = 0;
    entry->queue = 0;

    ht_remove(vector_to_entry, irq);

    ioapic_mask(irq);

    kprintln("USER_IRQ: Vector %d is now unreserved.", irq);
}

// If the thread is currently active, do not forget to context switch away from it.
// Only do this after a check to see if the thread was blocked. If there are interrupts
// enqueued, then no blocking will be done.
void user_irq_await(uint8_t irq, Thread *thread) {
    ReservableIRQ *entry = user_irq_get_reservation(irq);

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

    process_block_thread(thread, THREADBLOCK_IRQ, (ThreadBlockTarget) { .irq_vector = irq });
}

void user_irq_awaken_by_vector(uint8_t vector)
{
    ReservableIRQ *entry = null;

    if (!ht_lookup(vector_to_entry, vector, (uintptr_t*) &entry))
        PANIC("Cannot find reservation from vector %d!", vector);

    lapic_end_of_interrupt();

    if (entry->awaiter == null) {
        entry->queue++;
        return;
    }

    process_unblock_thread(entry->awaiter, USER_IRQ_AWAKE_INTERRUPT);
    entry->awaiter = null;
}

void user_irq_cancel(uint8_t irq, Process *process) {
    ReservableIRQ *entry = user_irq_get_reservation(irq);

    if (entry->reserver != process)
        PANIC("user_irq_cancel called by process that didn't reserve it.");

    if (entry->awaiter == null)
        PANIC("user_irq_cancel called on a reservation that is not being awaited.");

    process_unblock_thread(entry->awaiter, USER_IRQ_AWAKE_CANCEL);
    entry->awaiter = null;
}

void user_irq_force_unreserve_all(Process *process) {
    uint8_t unreserved_count = 0;

    for (uint64_t reservation_index = 0; reservation_index < RESERVABLE_IRQ_TABLE_MAX; reservation_index++) {
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
