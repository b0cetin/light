
#pragma once

#include "vmm.h"
#include <stddef.h>
#include <stdint.h>

#define MAX_PROCESS_COUNT 256

typedef enum { THREAD_READY, THREAD_RUNNING, THREAD_BLOCKED, THREAD_TERMINATED } ThreadState;

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
    uint64_t tid;
    struct Process *owner;

    // List
    struct Thread *prev;
    struct Thread *next;
} Thread;

typedef struct Process {
    // Processor
    PLM4* cr3;

    // Metadata
    uint64_t pid;
    char *path;

    // Threads
    struct Thread *threads;
    size_t thread_count;

    // List
    struct Process *prev;
    struct Process *next;
} Process;

Process *process_create(void *entry, PLM4 *plm4, char *path);
void process_create_thread(void *entry, Process *process);

Process *process_list();
