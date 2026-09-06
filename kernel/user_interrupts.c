
#include "user_interrupts.h"
#include "allocator.h"
#include "apic.h"
#include "context_switching.h"
#include "debugging.h"
#include "interrupts.h"
#include "processes.h"
#include "types.h"
#include "utils/hashtables/u64toaddr_hashtable.h"
#include <stdint.h>

Hashtable *reservations;
Hashtable *vector_to_reservation;

void user_irq_init() {
    reservations = ht_create();
    vector_to_reservation = ht_create();
}

UserIRQReservation *user_irq_get_reservation(uint8_t irq) {
    if (!user_irq_is_irq_in_range(irq))
        PANIC("user_irq_get_reservation called with IRQ %d: Invalid range!", irq);

    UserIRQReservation *reservation = null;
    ht_lookup(reservations, irq, (uintptr_t*) &reservation);
    return reservation;
}

UserIRQReservation *user_irq_find_reservation_from_vector(uint8_t vector) {
    UserIRQReservation *reservation = null;
    ht_lookup(vector_to_reservation, vector, (uintptr_t*) &reservation);
    return reservation;
}

void user_irq_interrupt_handler(InterruptRegisters *reg) {
    UserIRQReservation *reservation = null;

    if (!ht_lookup(vector_to_reservation, reg->interrupt_number, (uintptr_t*) &reservation))
        PANIC("user_irq_interrupt_handler: cannot find reservation from interrupt vector %d!", reg->interrupt_number);

    if (reservation->awaiter == null) {
        reservation->queue++;
        return;
    }

    Thread *awaiter = reservation->awaiter;

    process_unblock_thread(awaiter, USER_IRQ_AWAKE_INTERRUPT);
    reservation->awaiter = null;

    lapic_end_of_interrupt();

    ctx_switching_switch_to_now(awaiter);
}

void user_irq_reserve(uint8_t irq, Process *process) {
    if (user_irq_get_reservation(irq) != null)
        PANIC("user_irq_reserve called on reserved vector.");

    UserIRQReservation *reservation = kmalloc(sizeof(UserIRQReservation));
    if (reservation == null)
        PANIC("user_irq_reserve failed: system out of memory.");

    reservation->reserver = process;
    reservation->awaiter = null;
    reservation->irq = irq;
    reservation->queue = 0;

    reservation->assigned_vector = interrupts_get_empty_vector();
    register_interrupt_handler(reservation->assigned_vector, user_irq_interrupt_handler);

    if (!ht_add(reservations, irq, (uintptr_t) reservation))
        PANIC("Cannot add reservation (IRQ %d) to reservation hashtable!", irq);

    if (!ht_add(vector_to_reservation, reservation->assigned_vector, (uintptr_t) reservation))
        PANIC("Cannot add reservation (IRQ %d) to vector lookup hashtable!", irq);

    ioapic_route(irq, reservation->assigned_vector);

    kprintln("USER_IRQ: IRQ %d (vector %d) is now reserved for process %ld.",
        irq, reservation->assigned_vector, process->pid);
}

void irq_unreserve_internal(uint8_t irq) {
    UserIRQReservation *reservation = user_irq_get_reservation(irq);

    if (reservation == null)
        PANIC("irq_unreserve_internal called on unreserved IRQ.");

    if (reservation->awaiter != null)
        PANIC("irq_unreserve_internal called on awaited user IRQ.");

    ioapic_mask(reservation->assigned_vector);
    interrupts_remove_interrupt_handler(reservation->assigned_vector, user_irq_interrupt_handler);

    if (!ht_remove(reservations, irq)) PANIC("Cannot remove user IRQ %d from table!", irq);
    if (!ht_remove(vector_to_reservation, reservation->assigned_vector)) PANIC("Cannot remove user IRQ %d from vector lookup table!", irq);

    kfree(reservation);

    kprintln("USER_IRQ: Vector %d is now unreserved.", irq);
}

void user_irq_unreserve(uint8_t irq, Process *process) {
    UserIRQReservation *reservation = user_irq_get_reservation(irq);

    if (reservation == null)
        PANIC("user_irq_unreserve called on unreserved IRQ.");

    if (reservation->reserver != process)
        PANIC("user_irq_unreserve called for process that didn't reserve it.");

    irq_unreserve_internal(irq);
}

// If the thread is currently active, do not forget to context switch away from it.
// Only do this after a check to see if the thread was blocked. If there are interrupts
// enqueued, then no blocking will be done.
// The function returns true if a blocking was done (and therefore requires context
// switching.)
bool user_irq_await(uint8_t irq, Thread *thread) {
    UserIRQReservation *entry = user_irq_get_reservation(irq);

    if (entry == null)
        PANIC("user_irq_await called on unreserved IRQ.");

    if (entry->reserver != thread->owner)
        PANIC("user_irq_await called for process that didn't reserve it.");

    if (entry->awaiter != null)
        PANIC("user_irq_await called on awaited vector.");

    if (entry->queue > 0) {
        thread->wake_result = USER_IRQ_AWAKE_INTERRUPT;
        entry->queue--;
        return false;
    }

    entry->awaiter = thread;

    process_block_thread(thread, THREADBLOCK_IRQ, (ThreadBlockTarget) { .irq_awaiting = irq });
    return true;
}

void user_irq_cancel(uint8_t irq, Process *process) {
    UserIRQReservation *entry = user_irq_get_reservation(irq);

    if (entry == null)
        PANIC("user_irq_cancel called on unreserved IRQ.");

    if (entry->reserver != process)
        PANIC("user_irq_cancel called by process that didn't reserve it.");

    if (entry->awaiter == null)
        PANIC("user_irq_cancel called on a reservation that is not being awaited.");

    process_unblock_thread(entry->awaiter, USER_IRQ_AWAKE_CANCEL);
    entry->awaiter = null;
}

void user_irq_force_unreserve_all(Process *process) {
    uint8_t matching_irqs[UINT8_MAX];
    uint8_t unreserve_count = 0;

    for(size_t i = 0; i < reservations->capacity; i++) {
        Node *current = reservations->bucket[i];

        while (current != null) {
            UserIRQReservation *reservation = (UserIRQReservation*) current->address;

            if (reservation->reserver == process) {
                matching_irqs[unreserve_count] = reservation->irq;
                unreserve_count++;
            }

            current = current->next;
        }
    }

    for (uint8_t i = 0; i < unreserve_count; i++)
        irq_unreserve_internal(matching_irqs[i]);

    if (unreserve_count > 0) {
        kprintln("USER_IRQ: Unreserved all reservations of process %ld. There were %d.",
            process->pid, unreserve_count);
    }
}
