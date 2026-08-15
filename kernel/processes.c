
#include "processes.h"
#include "allocator.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "pmm.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

Process *processes = null;
uint64_t next_pid = 0;
uint64_t next_tid = 0;

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

Process *process_create(void *entry, PLM4 *plm4, char *path) {
    Process *new_process = allocate_process();

    size_t path_size = strnlen(path, UINT8_MAX) + 1;
    char *new_path = kmalloc(path_size);
    memcpy(new_path, path, path_size);

    new_process->cr3 = plm4;
    new_process->path = new_path;
    new_process->pid = next_pid++;

    new_process->threads = null;
    new_process->thread_count = 0;

    kprintln("PROC: Created process %ld from \"%s\".", new_process->pid, path);

    process_create_thread(entry, new_process);

    return new_process;
}

static Thread *allocate_thread(Process *process) {
    if (process->threads == null) {
        process->threads = kmalloc(sizeof(Thread));
        process->threads->owner = process;
        return process->threads;
    }

    Thread *latest = process->threads;
    while (latest->next != null) latest = latest->next;

    Thread *new_thread = kmalloc(sizeof(Thread));
    latest->next = new_thread;
    new_thread->prev = latest;

    process->threads->owner = process;

    return new_thread;
}

void process_create_thread(void *entry, Process *process) {
    Thread *thread = allocate_thread(process);
    
    thread->kernel_stack_base = p2v(pmm_alloc_page());
    thread->kernel_stack_size = PAGE_SIZE;

    thread->user_stack_phys_base = pmm_alloc_page();
    thread->user_stack_size = PAGE_SIZE;
    thread->user_stack_mapped_base = (void*) 0x10000000;

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
    thread->tid = next_tid++;

    thread->state = THREAD_READY;

    kprintln("PROC: Created thread %ld for process %ld, user stack location: %lx / %lx.", thread->tid, process->pid, thread->user_stack_phys_base, (uint64_t) thread->user_stack_mapped_base);
}

Process *process_list() {
    return processes;
}
