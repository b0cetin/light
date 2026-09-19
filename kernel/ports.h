
#pragma once

#include <stdint.h>
#include <stddef.h>
#include "references.h"
#include "types.h"

#define IPC_MESSAGE_LIMIT 256
#define IPC_HANDLE_LIMIT 8

typedef struct IPCMessage {
    size_t buffer_size;
    uint8_t *buffer;
    size_t handle_count;
    HandleID handles[IPC_HANDLE_LIMIT];
    struct IPCMessage *next;
} IPCMessage;

#define NULL_IPCMSG (IPCMessage *) null
typedef struct {
    // Used for authorizing termination.
    PID owner;
    bool terminated;

    // Reflects the active size of the pool.
    size_t pool_size;

    // Linked list of queued messages in the pool. May be null for no messages.
    IPCMessage *queue;
} IPCPort;

KernelObjectID ipc_create_port(PID pid);
bool ipc_terminate_port(PID requester, KernelObjectID kid);

// Reserved for destruction of port. Use `ipc_terminate_port` for termination.
void ipc_destroy_port(KernelObjectID kid);
