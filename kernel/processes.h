
#pragma once

#include "types.h"
#include "vas.h"
#include "vmm.h"
#include <stddef.h>
#include <stdint.h>

#define PROCESS_NAME_MAX UINT8_MAX

typedef enum { THREAD_NULL, THREAD_READY, THREAD_RUNNING, THREAD_BLOCKED } ThreadState;
typedef enum { PROCESS_ALIVE, PROCESS_TERMINATING, PROCESS_STARTING } ProcessState;

typedef enum { THREADBLOCK_NULL, THREADBLOCK_ANOTHER_THREAD, THREADBLOCK_IRQ, THREADBLOCK_RPC_RECEIVE, THREADBLOCK_RPC_WAIT_REPLY } ThreadBlockReason;
typedef union { uint64_t target_thread_id; uint8_t irq_awaiting; uint64_t rpc_callee_pid; } ThreadBlockTarget;

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
    VASRegion *user_stack;

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

typedef struct Process {
    // Processor
    bool is_ring_0;

    // Memory
    VAS user_vas;

    // Metadata
    ProcessState state;
    uint64_t pid;
    char *name;

    // Threads
    struct Thread *threads;
    size_t thread_count;
    uint64_t next_thread_id;

    // RPC
    uint64_t threads_receiving_rpcs_count;

    // List
    struct Process *prev;
    struct Process *next;
} Process;

void process_init();

Process *process_create(char *name);
Process *process_create_kernel(char *name, uint64_t *requested_pid);
void process_start(Process* process, void *entry);

Thread *process_create_thread(void *entry, uint64_t arg0, Process *process);

void process_begin_thread_teardown(Thread *thread, uint64_t result);
bool process_begin_process_teardown(Process* process, int64_t status);

Process *process_list();
Process *process_find(uint64_t pid);
Thread *process_find_thread(Process *process, uint64_t thread_id);

Thread *process_try_get_thread_from_id(Process *process, uint64_t id);
void process_crash_with_switch(Process *process);

// CAUTION: Doesn't account whether or not the thread to be blocked is running.
void process_block_thread(Thread *thread, ThreadBlockReason reason, ThreadBlockTarget target);
void process_unblock_thread(Thread *thread, uint64_t result);

// Returns false if the given threads don't belong to the same process, or an allocation failed.
// Heed warning for process_block_thread.
bool process_block_thread_for_another(Thread *thread, Thread *other_thread_in_same_process);

// RPCs

typedef struct {
    uint64_t caller_pid;
    uint64_t caller_thread_id;
    uint64_t callee_pid;
    uint64_t call_number, arg0, arg1, arg2, arg3;
} RPC; // Purposefully don't use direct pointers in case either side of the connection goes down.

#define RPC_RECEIVE_CANCELLED UINT64_MAX

bool process_rpc_begin_receive(Thread *receiver);
typedef enum { RPC_INVOKE_SUCCESS, RPC_INVOKE_CALLEE_NOT_RECEIVING, RPC_INVOKE_DUPLICATE } ProcessRPCInvokeStatus;
ProcessRPCInvokeStatus process_rpc_invoke(RPC *rpc, Thread **out_receiver);
bool process_rpc_reply(Process *callee, Process *caller, uint64_t result, Thread **out_caller);
bool process_rpc_receive_cancel(Thread *receiver);
