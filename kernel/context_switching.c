
#include "context_switching.h"
#include "debugging.h"
#include "processes.h"
#include "syscalls.h"
#include "tss.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

Thread *active_thread = null;

static Thread *pick_next_thread() {
    if (active_thread == null) {
        for (Process *process = process_list(); process != null; process = process->next) {
            for (Thread *thread = process->threads; thread != null; thread = thread->next) {
                if (thread->state == THREAD_READY) return thread;
            }
        }
        return null;
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

    return null;
}

uint64_t isr_context_switch(uint64_t rsp) {
    kprintln("switching!");

    active_thread->kernel_rsp = rsp;
    Process *old_process = active_thread->owner;

    if (active_thread->state == THREAD_RUNNING)
        active_thread->state = THREAD_READY;
    
    active_thread = pick_next_thread();

    if (active_thread == null) PANIC("No thread available to switch to.");
    
    void *kernel_stack_top = active_thread->kernel_stack_base + active_thread->kernel_stack_size;
    tss_set_rsp0(kernel_stack_top);
    syscalls_set_kernel_stack(kernel_stack_top);

    if (old_process != active_thread->owner) {
        vmm_switch_to_user_address_space(active_thread->owner->cr3);
    }

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

    vmm_switch_to_user_address_space(active_thread->owner->cr3);

    active_thread->state = THREAD_RUNNING;

    switch_to_context_immediately(active_thread->kernel_rsp);
}

void ctx_switching_set_initial_thread(Thread *thread) {
    if (active_thread != null) PANIC("set_initial_thread(Thread) called when a thread is already running.");

    active_thread = thread;
}

Thread *ctx_switching_get_active_thread() {
    return active_thread;
}
