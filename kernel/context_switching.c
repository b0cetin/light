
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
    if (active_thread == null && process_list() != null) return process_list()->threads;

    if (active_thread->next != null) return active_thread->next;

    if (active_thread->owner->next != null) return active_thread->owner->next->threads;

    if (process_list() != null) return process_list()->threads;

    return null;
}

uint64_t isr_context_switch(uint64_t rsp) {
    kprintln("switching!");

    active_thread->kernel_rsp = rsp;
    Process *old_process = active_thread->owner;
    
    active_thread = pick_next_thread();

    if (active_thread == null) PANIC("No thread alive to switch to.");
    
    void *kernel_stack_top = active_thread->kernel_stack_base + active_thread->kernel_stack_size;
    tss_set_rsp0(kernel_stack_top);
    syscalls_set_kernel_stack(kernel_stack_top);

    if (old_process != active_thread->owner) {
        vmm_switch_to_user_address_space(active_thread->owner->cr3);
    }

    return active_thread->kernel_rsp;
}

void ctx_switching_set_initial_thread(Thread *thread) {
    if (active_thread != null) PANIC("set_initial_thread(Thread) called when a thread is already running.");

    active_thread = thread;
}
