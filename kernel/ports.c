
#include "ports.h"
#include "allocator.h"
#include "debugging.h"
#include "kobjects.h"
#include "references.h"
#include "types.h"

// Returns `NULL_KOBJECT` if `kobject_create_without_init` fails.
KernelObjectID ipc_create_port(PID owner) {
    KernelObjectEntry *entry = null;
    KernelObjectID id = NULL_KOBJECT;
    if (!kobject_create_without_init(KOBJECT_PORT, &id, &entry)) return NULL_KOBJECT;

    entry->object.ipc_port.owner = owner;
    entry->object.ipc_port.pool_size = 0;
    entry->object.ipc_port.queue = NULL_IPCMSG;
    entry->object.ipc_port.terminated = false;

    return id;
}

void terminate_port(IPCPort *port) {
    port->terminated = true;

    size_t destroyed_messages = 0;
    
    while (port->queue != null) {
        IPCMessage *next = port->queue->next;
        kfree(port->queue);
        port->queue = next;

        destroyed_messages++;
    }

    if (destroyed_messages) kprintln("IPC: Terminated port, %ld messages destroyed.", destroyed_messages);

    port->pool_size = 0;
}

// Returns false if the requester isn't the owner. Assertions for errors.
bool ipc_terminate_port(PID requester, KernelObjectID kid) {
    KernelObjectEntry *entry = null;
    assert_msg(kobject_resolve(kid, &entry), "kobject_resolve failed for kernel object ID %ld", kid);
    assert_msg(entry->type == KOBJECT_PORT, "kernel object %ld is of type %ld, %ld expected.", kid, entry->type, KOBJECT_PORT);
    assert_msg(!entry->object.ipc_port.terminated, "ipc port (kid %ld) is already terminated", kid);

    if (entry->object.ipc_port.owner != requester) return false;

    terminate_port(&entry->object.ipc_port);
    return true;
}

void ipc_destroy_port(KernelObjectID kid) {
    KernelObjectEntry *entry = null;
    assert_msg(kobject_resolve(kid, &entry), "kobject_resolve failed for kernel object ID %ld", kid);
    assert_msg(entry->type != KOBJECT_PORT, "kernel object %ld is of type %ld, %ld expected.", entry->type, KOBJECT_PORT);
    assert_msg(!entry->object.ipc_port.terminated, "ipc port (kid %ld) is already terminated", kid);

    terminate_port(&entry->object.ipc_port);
}
