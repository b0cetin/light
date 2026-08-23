
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
    
    active_thread = pick_next_thread();

    if (active_thread == null) PANIC("No thread available to switch to.");
    
    void *kernel_stack_top = active_thread->kernel_stack_base + active_thread->kernel_stack_size;
    tss_set_rsp0(kernel_stack_top);
    syscalls_set_kernel_stack(kernel_stack_top);

    if (active_thread->owner->is_ring_0)
        vmm_switch_to_kernel_address_space();
    else
        vmm_switch_to_user_address_space(active_thread->owner->user_cr3); // TODO: Optimize for setting to the same space back to back

    active_thread->state = THREAD_RUNNING;

    return active_thread->kernel_rsp;
}

extern void switch_to_context_immediately(uint64_t rsp);
// Switches to the next thread in queue without touching the previous.
void ctx_switching_switch_next_immediate()
{
    kprintln("CTX: Switching immediately!");

    active_thread = pick_next_thread();

    if (active_thread == null) PANIC("No thread available to switch to.");
    
    void *kernel_stack_top = active_thread->kernel_stack_base + active_thread->kernel_stack_size;
    tss_set_rsp0(kernel_stack_top);
    syscalls_set_kernel_stack(kernel_stack_top);

    vmm_switch_to_user_address_space(active_thread->owner->user_cr3);

    active_thread->state = THREAD_RUNNING;

    switch_to_context_immediately(active_thread->kernel_rsp);
}

void ctx_switching_init(Thread *_idle_thread) {
    if (has_init) PANIC("ctx_switching_init called when already initialized.");

    idle_thread = _idle_thread;
    has_init = true;
}

Thread *ctx_switching_get_active_thread() {
    return active_thread;
}
