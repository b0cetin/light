
#include "context_switching.h"
#include "defined_syscalls.h"
#include "user_interrupts.h"
#include "processes.h"

int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t irq) {
    if (irq > UINT8_MAX || !user_irq_is_irq_in_range(irq))
        return SYS_ERR_IRQCTL_IRQ_OUT_OF_RANGE;

    Thread *thread = ctx_switching_get_active_thread();
    Process *process = thread->owner;

    UserIRQReservation *reservation = user_irq_get_reservation(irq);

    switch (request) {
        case IRQCTL_SET: {
            if (reservation != null)
                return SYS_ERR_IRQCTL_IRQ_IN_USE;

            user_irq_reserve(irq, process);
            return SYS_SUCCESS;
        }
        case IRQCTL_UNSET: {
            if (reservation == null)
                return SYS_ERR_IRQCTL_IRQ_NOT_RESERVED;

            if (reservation->reserver != process)
                return SYS_ERR_IRQCTL_IRQ_IN_USE;

            if (reservation->awaiter != null)
                return SYS_ERR_IRQCTL_UNSET_IRQ_AWAITING;

            user_irq_unreserve(irq, process);
            return SYS_SUCCESS;
        }
        case IRQCTL_AWAIT: {
            if (reservation == null)
                return SYS_ERR_IRQCTL_IRQ_NOT_RESERVED;

            if (reservation->reserver != process)
                return SYS_ERR_IRQCTL_IRQ_IN_USE;

            if (reservation->awaiter != null)
                return SYS_ERR_IRQCTL_AWAIT_DUPLICATE;

            if (user_irq_await(irq, thread))
                sys_yield();

            if (thread->wake_result == USER_IRQ_AWAKE_CANCEL)
                return SYS_ERR_IRQCTL_AWAIT_CANCELLED;

            return SYS_SUCCESS;
        }
        case IRQCTL_CANCEL: {
            if (reservation == null)
                return SYS_ERR_IRQCTL_IRQ_NOT_RESERVED;

            if (reservation->reserver != process)
                return SYS_ERR_IRQCTL_IRQ_IN_USE;

            if (reservation->awaiter == null)
                return SYS_ERR_IRQCTL_CANCEL_NOT_AWAITED;

            user_irq_cancel(irq, process);
            return SYS_SUCCESS;
        }
        default:
            return SYS_ERR_IRQCTL_REQUEST_INVALID;
    }
}
