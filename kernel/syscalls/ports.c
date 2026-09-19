
#include "ports.h"
#include "context_switching.h"
#include "defined_syscalls.h"
#include "debugging.h"
#include "handles.h"
#include "kobjects.h"
#include "processes.h"
#include "references.h"
#include "syscalls/utils.h"
#include <stdint.h>

int64_t sys_port_create(sys_handle_t *out_handle) {
    if (out_handle != null && !is_valid_mapped_user_range((uintptr_t) out_handle, sizeof(sys_handle_t*))) return SYS_ERR_ARGUMENT_POINTER_INVALID;

    Process *process = ctx_switching_get_active_thread()->owner;
    assert(process);

    PID pid = process->pid;
    HandleTable *table = &process->handle_table;

    KernelObjectID new_port = ipc_create_port(pid);
    if (new_port == NULL_KOBJECT) return SYS_ERR_OUT_OF_MEMORY;

    HandleID handle = NULL_HANDLE;
    if (!handle_add(table, new_port, &handle)) return SYS_ERR_OUT_OF_MEMORY;

    if (out_handle != null) *out_handle = handle;
    return SYS_SUCCESS;
}

int64_t sys_port_terminate(sys_handle_t port) {
    Process *process = ctx_switching_get_active_thread()->owner;
    assert(process);

    PID pid = process->pid;
    HandleTable *table = &process->handle_table;

    KernelObjectID kid = NULL_KOBJECT;
    if (port == NULL_HANDLE || !handle_resolve(table, port, &kid)) return SYS_ERR_HANDLE_INVALID;

    KernelObjectEntry *entry = null;
    if (!kobject_resolve(kid, &entry) || entry->type != KOBJECT_PORT) return SYS_ERR_HANDLE_INVALID;
    if (entry->object.ipc_port.owner != pid) return SYS_ERR_INSUFFICIENT_PERMISSIONS;
    if (entry->object.ipc_port.terminated) return SYS_ERR_HANDLE_DESTROYED;

    assert(ipc_terminate_port(pid, kid));

    return SYS_SUCCESS;
}


int64_t sys_port_send(sys_handle_t port, sys_ipc_message_t *message) {
    NOT_IMPLEMENTED
}

int64_t sys_port_receive(sys_handle_t port, sys_ipc_message_t *out_message, uint64_t wait) {
    NOT_IMPLEMENTED
}

