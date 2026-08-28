
#include "context_switching.h"
#include "debugging.h"
#include "defined_syscalls.h"
#include "processes.h"
#include "syscalls/utils.h"
#include "types.h"
#include <stdint.h>

// FIXME: Increadibly unoptimized.
rpc_result_t sys_rpc_invoke(pid_t target, uint64_t call_number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    Thread *caller_t = ctx_switching_get_active_thread();
    Process *target_p = process_find(target);

    if (target_p == null)
        return (rpc_result_t) { .error_code = SYS_ERR_RPC_PID_NOT_FOUND, .result = 0 };

    RPC rpc = {
        .caller_pid = caller_t->owner->pid,
        .caller_thread_id = caller_t->local_id,
        .callee_pid = target,

        .call_number = call_number,
        .arg0 = arg0,
        .arg1 = arg1,
        .arg2 = arg2,
        .arg3 = arg3,
    };

    Thread *target_t = null;
    ProcessRPCInvokeStatus status = process_rpc_invoke(&rpc, &target_t);

    switch (status) {
        case RPC_INVOKE_CALLEE_NOT_RECEIVING:
            return (rpc_result_t) { .error_code = -1, .result = 0 };
        case RPC_INVOKE_DUPLICATE:
            return (rpc_result_t) { .error_code = SYS_ERR_RPC_TOO_MANY_CALLS, .result = 0 };
        default:
            break;
    }

    ctx_switching_switch_to_now(target_t);

    return (rpc_result_t){ .error_code = 0, .result = caller_t->wake_result };
}

int64_t sys_rpc_receive(pid_t *out_caller, uint64_t *out_call_number, uint64_t *out_arg0, uint64_t *out_arg1, uint64_t *out_arg2, uint64_t *out_arg3) {
    if (!is_valid_mapped_user_range((uintptr_t) out_caller, sizeof(uintptr_t))) return -1;
    if (!is_valid_mapped_user_range((uintptr_t) out_call_number, sizeof(uintptr_t))) return -1;
    if (!is_valid_mapped_user_range((uintptr_t) out_arg0, sizeof(uintptr_t))) return -1;
    if (!is_valid_mapped_user_range((uintptr_t) out_arg1, sizeof(uintptr_t))) return -1;
    if (!is_valid_mapped_user_range((uintptr_t) out_arg2, sizeof(uintptr_t))) return -1;
    if (!is_valid_mapped_user_range((uintptr_t) out_arg3, sizeof(uintptr_t))) return -1;

    Thread *thread = ctx_switching_get_active_thread();
    Process *process = thread->owner;

    if (!process_rpc_begin_receive(thread))
        PANIC("sys_rpc_receive called by blocked thread %ld (pid %ld)", thread->local_id, process->pid);

    sys_yield();

    if (thread->wake_result == RPC_RECEIVE_CANCELLED) { // sys_rpc_awaken was called
        *out_caller = process->pid;
        *out_call_number = UINT64_MAX;
        *out_arg0 = 0;
        *out_arg1 = 0;
        *out_arg2 = 0;
        *out_arg3 = 0;
        return SYS_SUCCESS;
    }

    RPC *rpc = (RPC*) thread->wake_result;

    *out_caller = rpc->caller_pid;
    *out_call_number = rpc->call_number;
    *out_arg0 = rpc->arg0;
    *out_arg1 = rpc->arg1;
    *out_arg2 = rpc->arg2;
    *out_arg3 = rpc->arg3;

    return SYS_SUCCESS;
}

int64_t sys_rpc_return(pid_t caller, uint64_t result) {
    Thread *responder_t = ctx_switching_get_active_thread();
    Process *responder_p = responder_t->owner;

    Process *caller_p = process_find(caller);
    if (caller_p == null) return SYS_ERR_RPC_CALLER_DEAD; // FIXME: Shouldn't we check for SYS_ERR_RPC_PID_NOT_CALLER first?

    Thread *caller_t = null;
    if (!process_rpc_reply(responder_p, caller_p, result, &caller_t))
        return SYS_ERR_RPC_PID_NOT_CALLER;

    ctx_switching_switch_to_now(caller_t);

    return SYS_SUCCESS;
}

uint64_t sys_rpc_awaken(uint64_t count) {
    if (count == 0) return 0;

    Process *process = ctx_switching_get_active_thread()->owner;

    uint64_t awake_count = 0;

    for (Thread *thread = process->threads; thread != null && awake_count < count; thread = thread->next) {
        if (process_rpc_receive_cancel(thread)) {
            awake_count++;
        }
    }

    return awake_count;
}
