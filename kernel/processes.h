
#pragma once

#include "types.h"
#include "vmm.h"
#include <stddef.h>
#include <stdint.h>

#define PROCESS_NAME_MAX UINT8_MAX

typedef enum { THREAD_READY, THREAD_RUNNING, THREAD_TERMINATED, THREAD_BLOCKED } ThreadState;
typedef enum { PROCESS_ALIVE, PROCESS_TERMINATING } ProcessState;

typedef enum { THREADBLOCK_NULL, THREADBLOCK_ANOTHER_THREAD, THREADBLOCK_IRQ } ThreadBlockReason;
typedef union { uint64_t target_thread_id; uint8_t irq_vector; } ThreadBlockTarget;

typedef struct ThreadChainItem {
    struct Thread *thread;
    struct ThreadChainItem *next;
} ThreadChainItem;

typedef struct Thread {
    // Kernel registers 
    uint64_t kernel_rsp;
    void *kernel_stack_base;
    uint64_t kernel_stack_size;

    // User registers
    uint64_t user_stack_phys_base;
    void *user_stack_mapped_base;
    uint64_t user_stack_size;

    // Metadata
    ThreadState state;
    uint64_t local_id;
    struct Process *owner;

    // Blocking
    ThreadBlockReason block_reason;
    ThreadBlockTarget block_target;
    uint64_t wake_result;

    // Termination
    ThreadChainItem *unblock_on_termination;

    // List
    struct Thread *next;
} Thread;

#define PROCESS_VAS_STACK_REGION_TOP 0x0000700000000000ULL
typedef struct {
    uint64_t next_stack_top;
} ProcessVASState;

typedef struct Process {
    // Processor
    PML4* cr3;

    // Metadata
    ProcessState state;
    uint64_t pid;
    char *name;
    bool is_critical;

    // Threads
    struct Thread *threads;
    size_t thread_count;
    uint64_t next_thread_id;
    ProcessVASState vas;

    // List
    struct Process *prev;
    struct Process *next;
} Process;

Process *process_create(void *entry, PML4 *plm4, char *path);
Process *process_create_critical(void *entry, PML4 *plm4, char *path);
Thread *process_create_thread(void *entry, uint64_t arg0, Process *process);

bool process_begin_thread_teardown(Thread *thread, uint64_t result);
bool process_begin_process_teardown(Process* process, int64_t status);

Process *process_list();

Thread *process_try_get_thread_from_id(Process *process, uint64_t id);
void process_crash_with_switch(Process *process);

// CAUTION: Doesn't account whether or not the thread to be blocked is running.
void process_block_thread(Thread *thread, ThreadBlockReason reason, ThreadBlockTarget target);
void process_unblock_thread(Thread *thread, uint64_t result);

// Returns false if the given threads don't belong to the same process, or an allocation failed.
// Heed warning for process_block_thread.
bool process_block_thread_for_another(Thread *thread, Thread *other_thread_in_same_process);
