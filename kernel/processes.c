
#include "processes.h"
#include "allocator.h"
#include "context_switching.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "pmm.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

Process *processes = null;
uint64_t next_pid = 0;

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

Process *process_create_critical(void *entry, PML4 *pml4, char *path) {
    Process *process = process_create(entry, pml4, path);
    process->is_critical = true;

    return process;
}

Process *process_create(void *entry, PML4 *pml4, char *path) {
    Process *new_process = allocate_process();

    size_t path_size = strnlen(path, UINT8_MAX) + 1;
    char *new_path = kmalloc(path_size);
    memcpy(new_path, path, path_size);

    new_process->cr3 = pml4;
    new_process->name = new_path;
    new_process->pid = next_pid++;
    new_process->state = PROCESS_ALIVE;
    new_process->is_critical = false;

    new_process->threads = null;
    new_process->thread_count = 0;
    new_process->next_thread_id = 0;

    new_process->vas.next_stack_top = PROCESS_VAS_STACK_REGION_TOP;

    kprintln("PROC: Created process %ld from \"%s\".", new_process->pid, path);

    process_create_thread(entry, new_process);

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

Thread *process_create_thread(void *entry, Process *process) {
    Thread *thread = allocate_thread(process);
    
    thread->kernel_stack_base = p2v(pmm_alloc_page());
    thread->kernel_stack_size = PAGE_SIZE;

    thread->user_stack_phys_base = pmm_alloc_page();
    thread->user_stack_size = PAGE_SIZE;
    thread->user_stack_mapped_base = (void*) process->vas.next_stack_top;

    process->vas.next_stack_top -= thread->user_stack_size + PAGE_SIZE; // Stack overflow guards by adding unmapped page in between.

    uint64_t user_stack_top = ((uint64_t) thread->user_stack_mapped_base) + thread->user_stack_size;

    vmm_map(process->cr3, (uint64_t) thread->user_stack_mapped_base, thread->user_stack_phys_base, PT_USER | PT_RW | PT_NX);

    uint64_t *sp = (uint64_t *)(thread->kernel_stack_base + thread->kernel_stack_size);

    *--sp = 0x18 | 3; // SS (user data selector | RPL3)
    *--sp = user_stack_top; // RSP
    *--sp = 0x202; // RFLAGS: IF=1, bit1 reserved=1
    *--sp = 0x20 | 3; // CS (user code selector | RPL3)
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
    *--sp = 0; // rdi
    *--sp = 0; // rsi
    *--sp = 0; // rdx
    *--sp = 0; // rcx
    *--sp = 0; // rbx
    *--sp = 0; // rax

    thread->kernel_rsp = (uint64_t)sp;
    thread->local_id = process->next_thread_id++;

    thread->state = THREAD_READY;

    kprintln("PROC: Created thread %ld for process %ld, user stack location: %lx / %lx.", thread->local_id, process->pid, thread->user_stack_phys_base, (uint64_t) thread->user_stack_mapped_base);

    return thread;
}

Process *process_list() {
    return processes;
}

static void teardown_process_with_switch(Process *process, int64_t status);
static void teardown_thread(Thread* thread, uint64_t result) {
    vmm_switch_to_kernel_address_space();

    pmm_free_page(v2p(thread->kernel_stack_base));
    pmm_free_page(thread->user_stack_phys_base); // Isn't this kinda handled by process termination?

    thread->kernel_stack_base = 0;
    thread->kernel_rsp = 0;
    thread->kernel_stack_size = 0;
    thread->user_stack_phys_base = 0;
    thread->user_stack_mapped_base = 0;
    thread->user_stack_size = 0;

    thread->state = THREAD_TERMINATED;
    thread->result = result;

    thread->owner->thread_count--;

    kprintln("PROC: Thread %ld of process %ld exited with result %ld.",
        thread->local_id, thread->owner->pid, result);
    
    for (Thread *t = thread->owner->threads; t != null; t = t->next)
        if (t->state != THREAD_TERMINATED) return;
    
    // There are no threads remaining.

    kprintln("Process %ld no longer has any threads running.", thread->owner->pid);

    teardown_process_with_switch(thread->owner, result);
}

static void teardown_thread_with_switch(Thread* thread, uint64_t result) {
    teardown_thread(thread, result);

    ctx_switching_switch_next_immediate();
}

static void teardown_process_with_switch(Process *process, int64_t status) {
    if (process->state == PROCESS_TERMINATING) return;

    kprintln("PROC: Terminating process %ld with %ld alive threads...", process->pid, process->thread_count);

    vmm_switch_to_kernel_address_space();

    process->state = PROCESS_TERMINATING;

    for (Thread *thread = process->threads; thread != null; thread = thread->next) {
        if (thread->state != THREAD_TERMINATED)
            teardown_thread(thread, -1);
    }

    // TODO: At this stage, you'd also free all memory allocated to the process,
    // but that's not a thing right now so I'll let this memory leak pass.
    // INCORRECT

    kfree(process->name);
    process->name = null;

    vmm_destroy_user_address_space_and_free_memory(process->cr3); // Already frees.
    process->cr3 = null;

    if (process->prev != null) process->prev->next = process->next;
    if (process->next != null) process->next->prev = process->prev;

    uint64_t pid = process->pid;
    bool was_critical = process->is_critical;

    kfree(process);

    kprintln("PROC: Process %ld terminated with status %ld.", pid, status);

    if (was_critical) PANIC("Critical process %ld has been terminated!", pid);

    ctx_switching_switch_next_immediate();
}

extern void processes_exit_trampoline(void *function, uint64_t arg1, uint64_t arg2);

bool process_begin_thread_teardown(Thread* thread, uint64_t result) {
    if (thread->state == THREAD_TERMINATED)
    {
        kprintln("PROC: Process %ld tried to terminate thread %ld but it was already terminated.", thread->owner->pid, thread->local_id);
        return false;
    }

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
