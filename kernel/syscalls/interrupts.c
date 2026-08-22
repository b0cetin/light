
#include "context_switching.h"
#include "defined_syscalls.h"
#include "../debugging.h"
#include "../user_interrupts.h"
#include "processes.h"

int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t vector) {
    if (vector > UINT8_MAX || !user_irq_is_vector_in_range(vector))
        return SYS_ERR_IRQCTL_VECTOR_OUT_OF_RANGE;

    Thread *thread = ctx_switching_get_active_thread();
    Process *process = thread->owner;

    ReservableIRQ *reservation = user_irq_get_reservation(vector);

    switch (request) {
        case IRQCTL_SET: {
            if (user_irq_is_reserved(reservation))
                return SYS_ERR_IRQCTL_VECTOR_IN_USE;

            user_irq_reserve(vector, process);
            return SYS_SUCCESS;
        }
        case IRQCTL_UNSET: {
            if (!user_irq_is_reserved(reservation))
                return SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED;

            if (reservation->reserver != process)
                return SYS_ERR_IRQCTL_VECTOR_IN_USE;

            if (reservation->awaiter != null)
                return SYS_ERR_IRQCTL_UNSET_VECTOR_AWAITING;

            user_irq_unreserve(vector, process);
            return SYS_SUCCESS;
        }
        case IRQCTL_AWAIT: {
            if (!user_irq_is_reserved(reservation))
                return SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED;

            if (reservation->reserver != process)
                return SYS_ERR_IRQCTL_VECTOR_IN_USE;

            if (reservation->awaiter != null)
                return SYS_ERR_IRQCTL_AWAIT_DUPLICATE;

            user_irq_await(vector, thread);

            if (thread->state == THREAD_BLOCKED)
                sys_yield();

            if (thread->wake_result == USER_IRQ_AWAKE_CANCEL)
                return SYS_ERR_IRQCTL_AWAIT_CANCELLED;

            return SYS_SUCCESS;
        }
        case IRQCTL_CANCEL: {
            if (!user_irq_is_reserved(reservation))
                return SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED;

            if (reservation->reserver != process)
                return SYS_ERR_IRQCTL_VECTOR_IN_USE;

            if (reservation->awaiter == null)
                return SYS_ERR_IRQCTL_CANCEL_NOT_AWAITED;

            user_irq_cancel(vector, process);

            return SYS_SUCCESS;
        }
        default:
            return SYS_ERR_IRQCTL_REQUEST_INVALID;
    }
}
