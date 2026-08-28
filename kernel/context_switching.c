
#include "context_switching.h"
#include "debugging.h"
#include "processes.h"
#include "syscalls.h"
#include "tss.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

static Thread *idle_thread = null;
static bool has_init = false;

static Thread *active_thread = null;
static Thread *requested_next = null; // NOTE: Maybe consider a FIFO in the future?

static Thread *pick_next_thread() {
    if (active_thread == null) {
        for (Process *process = process_list(); process != null; process = process->next) {
            for (Thread *thread = process->threads; thread != null; thread = thread->next) {
                if (thread->state == THREAD_READY) return thread;
            }
        }

        return idle_thread;
    }

    for (Thread *thread = active_thread->next; thread != null; thread = thread->next) {
        if (thread->state == THREAD_READY) return thread;
    }

    if (active_thread->owner != null && active_thread->owner->next != null) {
        for (Process *process = active_thread->owner->next; process != null; process = process->next) {
            for (Thread *thread = process->threads; thread != null; thread = thread->next) {
                if (thread->state == THREAD_READY) return thread;
            }
        }
    }

    for (Process *process = process_list(); process != null; process = process->next) {
        for (Thread *thread = process->threads; thread != null; thread = thread->next) {
            if (thread == active_thread) continue;
            if (thread->state == THREAD_READY) return thread;
        }
    }

    if (active_thread->state == THREAD_READY) return active_thread;

    return idle_thread;
}

static Thread *get_next_thread() {
    if (requested_next == null) return pick_next_thread();
    else {
        Thread *next = requested_next;
        requested_next = null;
        return next;
    }
}

static uint64_t switch_core(Thread *next) {
    if (next == null) PANIC("No thread available to switch to.");
    
    active_thread = next;

    void *kernel_stack_top = active_thread->kernel_stack_base + active_thread->kernel_stack_size;
    tss_set_rsp0(kernel_stack_top);
    syscalls_set_kernel_stack(kernel_stack_top);

    if (active_thread->owner->is_ring_0) vmm_switch_to_kernel_address_space();
    else vmm_switch_to_user_address_space(active_thread->owner->user_cr3);

    active_thread->state = THREAD_RUNNING;

    return active_thread->kernel_rsp;
}

uint64_t isr_context_switch(uint64_t rsp) {
    static uint64_t count = 0;

    if (count++ >= 64) {
        kprintln("CTX: Switched 64 times.");
        count = 0;
    }

    if (active_thread != null) {
        active_thread->kernel_rsp = rsp;

        if (active_thread->state == THREAD_RUNNING)
            active_thread->state = THREAD_READY;
    }

    return switch_core(get_next_thread());
}

extern void switch_to_context_immediately(uint64_t rsp);
// Switches to the next thread in queue without touching the previous.
void ctx_switching_switch_next_destructive()
{
    kprintln("CTX: Switching destructively!");
    switch_to_context_immediately(switch_core(get_next_thread()));
}

// Switches to a specific thread immediately.
void ctx_switching_switch_to_now(Thread *target) {
    requested_next = target;
    __asm__ volatile ("int $0x81");
}

void ctx_switching_init(Thread *_idle_thread) {
    if (has_init) PANIC("ctx_switching_init called when already initialized.");

    idle_thread = _idle_thread;
    has_init = true;
}

Thread *ctx_switching_get_active_thread() {
    return active_thread;
}
