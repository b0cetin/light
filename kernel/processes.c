
#include "processes.h"
#include "allocator.h"
#include "context_switching.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "pmm.h"
#include "types.h"
#include "user_interrupts.h"
#include "userspace.h"
#include "utils/hashtables/u64toaddr_hashtable.h"
#include "vmm.h"
#include <stdint.h>

Process *processes = null;
uint64_t next_pid = 0;

Hashtable *pid_to_process;

void process_init() {
    pid_to_process = ht_create();
}

static Process *allocate_process() {
    if (processes == null) {
        processes = kmalloc(sizeof(Process));
        return processes;
    }

    Process *latest = processes;
    while (latest->next != null) latest = latest->next;

    Process *new_process = kmalloc(sizeof(Process));
    latest->next = new_process;
    new_process->prev = latest;

    return new_process;
}

static Process *process_create_core(char *name, uint64_t pid) {
    Process *new_process = allocate_process();

    size_t path_size = strnlen(name, PROCESS_NAME_MAX) + 1;
    char *new_path = kmalloc(path_size);
    memcpy(new_path, name, path_size);

    new_process->name = new_path;
    new_process->pid = pid;
    new_process->state = PROCESS_STARTING;

    if (!ht_add(pid_to_process, new_process->pid, (uintptr_t) new_process))
        PANIC("Cannot create process %ld (named \"%s\"): PID cannot be added to the table!");

    new_process->threads = null;
    new_process->thread_count = 0;
    new_process->next_thread_id = 0;

    kprintln("PROC: Created process %ld named \"%s\".", new_process->pid, name);

    return new_process;
}

Process *process_create(void *entry, PML4 *pml4, char *name) {
    Process *new_process = process_create_core(name, next_pid++);

    new_process->user_cr3 = pml4;
    new_process->is_ring_0 = false;

    new_process->user_vas.next_stack_top = PROCESS_VAS_STACK_REGION_TOP;

    new_process->state = PROCESS_ALIVE;

    process_create_thread(entry, 0, new_process);

    return new_process;
}

// Kernel-space processes should absolutely not call syscalls.
// There's no reason to perform syscalls. All kernel functionality is available.
// If `pid` argument is NULL, then a PID will be chosen automatically.
// If not, the requested PID will be used. WARNING: PID CLASHES WILL PANIC
// THE SYSTEM.
Process *process_create_kernel(void *entry, char *name, uint64_t *requested_pid) {
    uint64_t pid;

    if (requested_pid != null) {
        pid = *requested_pid;

        if (ht_lookup(pid_to_process, pid, null))
            PANIC("process_create_kernel called with requested pid %ld, but it clashes! (entry: %lx, name: %s)", pid, (uint64_t) entry, name);
    }
    else pid = next_pid++;

    Process *new_process = process_create_core(name, pid);

    new_process->user_cr3 = null;
    new_process->is_ring_0 = true;

    new_process->state = PROCESS_ALIVE;

    kprintln("PROC: This process runs in kernel space.");

    process_create_thread(entry, 0, new_process);

    return new_process;
}

static Thread *allocate_thread(Process *process) {
    process->thread_count++;

    if (process->threads == null) {
        process->threads = kmalloc(sizeof(Thread));
        process->threads->owner = process;
        return process->threads;
    }

    Thread *latest = process->threads;
    while (latest->next != null) latest = latest->next;

    Thread *new_thread = kmalloc(sizeof(Thread));
    latest->next = new_thread;

    new_thread->owner = process;

    return new_thread;
}

Thread *process_create_thread(void *entry, uint64_t arg0, Process *process) {
    bool is_ring_0 = process->is_ring_0;

    Thread *thread = allocate_thread(process);
    
    thread->kernel_stack_base = p2v(pmm_alloc_page());
    thread->kernel_stack_size = PAGE_SIZE;

    uint64_t kernel_stack_top = ((uint64_t) thread->kernel_stack_base) + thread->kernel_stack_size;

    uint64_t user_stack_top = 0;
    if (!is_ring_0) {
        thread->user_stack_phys_base = pmm_alloc_page();
        thread->user_stack_size = PAGE_SIZE;
        thread->user_stack_mapped_base = (void*) process->user_vas.next_stack_top;

        process->user_vas.next_stack_top -= thread->user_stack_size + PAGE_SIZE; // Stack overflow guards by adding unmapped page in between.

        user_stack_top = ((uint64_t) thread->user_stack_mapped_base) + thread->user_stack_size;

        vmm_map(process->user_cr3, (uint64_t) thread->user_stack_mapped_base, thread->user_stack_phys_base, PT_USER | PT_RW | PT_NX);
    }

    uint64_t *sp = (uint64_t *)(thread->kernel_stack_base + thread->kernel_stack_size);

    *--sp = is_ring_0 ? 0x10 : (0x18 | 3); // SS (user data selector | RPL3)
    *--sp = is_ring_0 ? kernel_stack_top : user_stack_top; // RSP
    *--sp = 0x202; // RFLAGS: IF=1, bit1 reserved=1
    *--sp = is_ring_0 ? 0x8 : (0x20 | 3); // CS (user code selector | RPL3)
    *--sp = (uint64_t)entry; // RIP

    *--sp = 0; // r15
    *--sp = 0; // r14
    *--sp = 0; // r13
    *--sp = 0; // r12
    *--sp = 0; // r11
    *--sp = 0; // r10
    *--sp = 0; // r9
    *--sp = 0; // r8
    *--sp = 0; // rbp
    *--sp = arg0; // rdi
    *--sp = 0; // rsi
    *--sp = 0; // rdx
    *--sp = 0; // rcx
    *--sp = 0; // rbx
    *--sp = 0; // rax

    thread->kernel_rsp = (uint64_t)sp;
    thread->local_id = process->next_thread_id++;

    thread->state = THREAD_READY;

    if (is_ring_0)
        kprintln("PROC: Created thread %ld for process %ld, kernel stack location: %lx.", thread->local_id, process->pid, thread->kernel_stack_base);
    else
        kprintln("PROC: Created thread %ld for process %ld, user stack location: %lx / %lx.", thread->local_id, process->pid, thread->user_stack_phys_base, (uint64_t) thread->user_stack_mapped_base);

    return thread;
}

Process *process_list() {
    return processes;
}

Process *process_find(uint64_t pid) {
    Process *process;
    if (!ht_lookup(pid_to_process, pid, (uintptr_t*) &process))
        return null;

    return process;
}

Thread *process_find_thread(Process *process, uint64_t thread_id) {
    // NOTE: I am not using Hashtables for threads because I didn't deem it necessary.
    // If we get to use a lotta threads for one process one day, then maybe I'll reconsider.

    Thread *thread = process->threads;
    while (thread != null) {
        if (thread->local_id == thread_id) return thread;
        thread = thread->next;
    }

    return null;
}

static void teardown_process_with_switch(Process *process, int64_t status);
static void teardown_thread(Thread* thread, uint64_t result) {
    vmm_switch_to_kernel_address_space();

    pmm_free_page(v2p(thread->kernel_stack_base));
    pmm_free_page(thread->user_stack_phys_base);

    thread->kernel_stack_base = 0;
    thread->kernel_rsp = 0;
    thread->kernel_stack_size = 0;
    thread->user_stack_phys_base = 0;
    thread->user_stack_mapped_base = 0;
    thread->user_stack_size = 0;

    process_rpc_receive_cancel(thread);
    // NOTE: This is here because there is a number in process keeping track of the amount of receivers.
    
    ThreadChainItem *item = thread->unblock_on_termination;
    while (item != null) {
        ThreadChainItem *next = item->next;

        process_unblock_thread(item->thread, result);

        kfree(item);
        item = next;
    }

    Process *owning_process = thread->owner;

    owning_process->thread_count--;

    kprintln("PROC: Thread %ld of process %ld exited with result %ld.",
        thread->local_id, owning_process->pid, result);

    Thread *prev = null;
    for (Thread *i = owning_process->threads; i != null; i = i->next)
    {
        if (i == thread) break;
        prev = i;
    }

    if (prev != null) prev->next = thread->next;
    else owning_process->threads = null;
    
    kfree(thread);
    
    if (owning_process->thread_count > 0) return;
    
    // There are no threads remaining.

    kprintln("PROC: Process %ld no longer has any threads running.", owning_process->pid);

    teardown_process_with_switch(owning_process, result);
}

static void teardown_thread_with_switch(Thread* thread, uint64_t result) {
    teardown_thread(thread, result);

    ctx_switching_switch_next_destructive();
}

static void teardown_process_with_switch(Process *process, int64_t status) {
    if (process->state == PROCESS_TERMINATING) return;

    kprintln("PROC: Terminating process %ld with %ld alive threads...", process->pid, process->thread_count);

    vmm_switch_to_kernel_address_space();

    process->state = PROCESS_TERMINATING;

    for (Thread *thread = process->threads; thread != null; thread = thread->next) {
        teardown_thread(thread, -1);
    }

    user_irq_force_unreserve_all(process);

    // FIXME: At this stage, you'd also free all memory allocated to the process,
    // but that's not a thing right now so I'll let this memory leak pass.

    kfree(process->name);
    process->name = null;

    vmm_destroy_user_address_space(process->user_cr3);
    process->user_cr3 = null;

    if (process->prev != null) process->prev->next = process->next;
    if (process->next != null) process->next->prev = process->prev;

    uint64_t pid = process->pid;

    kfree(process);

    kprintln("PROC: Process %ld terminated with status %ld.", pid, status);

    if (pid == PROCESS_INIT_PID || pid == PROCESS_IDLE_PID) PANIC("Protected process %ld has been terminated!", pid);

    ctx_switching_switch_next_destructive();
}

extern void processes_exit_trampoline(void *function, uint64_t arg1, uint64_t arg2);

bool process_begin_thread_teardown(Thread* thread, uint64_t result) {
    processes_exit_trampoline(teardown_thread_with_switch, (uint64_t) thread, result);
    return true;
}

bool process_begin_process_teardown(Process* process, int64_t status) {
    if (process->state == PROCESS_TERMINATING)
    {
        kprintln("PROC: Tried to terminate process %ld but it is already midway of termination.", process->pid);
        return false;
    }

    processes_exit_trampoline(teardown_process_with_switch, (uint64_t) process, status);
    return true;
}

Thread *process_try_get_thread_from_id(Process *process, uint64_t id) {
    for (Thread *thread = process->threads; thread != null; thread = thread->next)
        if (thread->local_id == id) return thread;
    
    return null;
}

void process_crash_with_switch(Process *process) {
    process_begin_process_teardown(process, -1);
}

void process_block_thread(Thread* thread, ThreadBlockReason reason, ThreadBlockTarget target) {
    thread->block_reason = reason;
    thread->block_target = target;
    thread->wake_result = 0;
    thread->state = THREAD_BLOCKED;

    // kprintln("PROC: Thread %ld of process %ld has been blocked with reason %ld.",
    //     thread->local_id, thread->owner->pid, reason);
    // Silence frequent log
}

void process_unblock_thread(Thread *thread, uint64_t result) {
    if (thread->state != THREAD_BLOCKED) {
        kprintln("PROC: process_unblock_thread called on unblocked thread %ld of process %ld.",
            thread->local_id, thread->owner->pid);
        return;
    }

    thread->block_reason = THREADBLOCK_NULL;
    thread->wake_result = result;
    thread->state = THREAD_READY;

    // kprintln("PROC: Thread %ld of process %ld has been unblocked.",
    //     thread->local_id, thread->owner->pid);
    // Silence frequent log
}

bool process_block_thread_for_another(Thread *thread, Thread *other) {
    if (thread->owner != other->owner)
        return false;

    ThreadChainItem *new_item = kmalloc(sizeof(ThreadChainItem));
    if (new_item == null) {
        kprintln("PROC: process_block_thread_for_another: kmalloc(ThreadChainItem) returned null!");
        return false;
    }

    process_block_thread(thread, THREADBLOCK_ANOTHER_THREAD, (ThreadBlockTarget) { .target_thread_id = other->local_id });

    new_item->thread = thread;

    if (other->unblock_on_termination != null) {
        ThreadChainItem *chain_item = other->unblock_on_termination;
        while (chain_item->next != null) chain_item = chain_item->next;
        chain_item->next = new_item;
    }
    else {
        other->unblock_on_termination = new_item;
    }

    return true;
}

// RPCs

// Returns false if the thread is already blocked.
// Heed warning for process_block_thread.
bool process_rpc_begin_receive(Thread *receiver) {
    if (receiver->state == THREAD_BLOCKED) return false;

    receiver->owner->threads_receiving_rpcs_count++;
    process_block_thread(receiver, THREADBLOCK_RPC_RECEIVE, (ThreadBlockTarget) {0});

    kprintln("Thread %ld of process %ld is now receiving RPCs. That's %ld so far.",
        receiver->local_id, receiver->owner->pid, receiver->owner->threads_receiving_rpcs_count);

    return true;
}

// Heed warning for process_block_thread.
ProcessRPCInvokeStatus process_rpc_invoke(RPC *rpc, Thread **out_receiver) {
    Process *caller_p = process_find(rpc->caller_pid);
    if (caller_p == null) PANIC("process_rpc_invoke called with invalid caller PID.");
    Thread *caller_t = process_find_thread(caller_p, rpc->caller_thread_id);
    if (caller_t == null) PANIC("process_rpc_invoke called with invalid caller thread local id.");

    Process *callee_p = process_find(rpc->callee_pid);
    if (callee_p == null) PANIC("process_rpc_invoke called with invalid callee PID.");

    if (callee_p->threads_receiving_rpcs_count <= 0) return RPC_INVOKE_CALLEE_NOT_RECEIVING;

    for (Thread *thread = caller_p->threads; thread != null; thread = thread->next) {
        if (thread->state == THREAD_BLOCKED &&
            thread->block_reason == THREADBLOCK_RPC_WAIT_REPLY &&
            thread->block_target.rpc_callee_pid == rpc->callee_pid)
            return RPC_INVOKE_DUPLICATE;
    }

    for (Thread *i = callee_p->threads; i != null; i = i->next) {
        if (i->state == THREAD_BLOCKED && i->block_reason == THREADBLOCK_RPC_RECEIVE) {
            Thread *receiver = i;

            process_unblock_thread(receiver, (uint64_t) rpc);
            callee_p->threads_receiving_rpcs_count--;

            kprintln("Thread %ld of process %ld has received an RPC. %ld receiving threads left.",
                receiver->local_id, receiver->owner->pid, receiver->owner->threads_receiving_rpcs_count);

            process_block_thread(caller_t, THREADBLOCK_RPC_WAIT_REPLY, (ThreadBlockTarget){ .rpc_callee_pid = rpc->callee_pid });

            if (out_receiver != null) *out_receiver = receiver;
            return RPC_INVOKE_SUCCESS;
        }
    }

    return RPC_INVOKE_CALLEE_NOT_RECEIVING;
}

// Returns false when no thread was found from caller that was waiting for reply from the callee process.
bool process_rpc_reply(Process *callee, Process *caller, uint64_t result, Thread **out_caller) {
    for (Thread *i = caller->threads; i != null; i = i->next) {
        if (i->state == THREAD_BLOCKED &&
            i->block_reason == THREADBLOCK_RPC_WAIT_REPLY &&
            i->block_target.rpc_callee_pid == callee->pid) {
            Thread *caller = i;

            process_unblock_thread(caller, result);

            if (out_caller != null) *out_caller = caller;
            return true;
        }
    }

    return false;
}

// Returns false if the thread is not receiving RPCs.
bool process_rpc_receive_cancel(Thread *receiver) {
    if (receiver->state == THREAD_BLOCKED &&
        receiver->block_reason == THREADBLOCK_RPC_RECEIVE) {
        process_unblock_thread(receiver, RPC_RECEIVE_CANCELLED);

        kprintln("Thread %ld of process %ld has stopped receiving RPCs. %ld receiving threads left.",
            receiver->local_id, receiver->owner->pid, receiver->owner->threads_receiving_rpcs_count);
        return true;
    }

    return false;
}
