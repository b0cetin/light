
#pragma once

#include "types.h"
#include "vmm.h"
#include <stddef.h>
#include <stdint.h>

typedef enum { THREAD_READY, THREAD_RUNNING, THREAD_TERMINATED } ThreadState;
typedef enum { PROCESS_ALIVE, PROCESS_TERMINATING } ProcessState;

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

    // Termination
    uint64_t result;

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
Thread *process_create_thread(void *entry, Process *process);

bool process_begin_thread_teardown(Thread *thread, uint64_t result);
bool process_begin_process_teardown(Process* process, int64_t status);

Process *process_list();

Thread *process_try_get_thread_from_id(Process *process, uint64_t id);
void process_crash_with_switch(Process *process);
