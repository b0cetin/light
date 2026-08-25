
#include <stdint.h>
#include "../processes.h"
#include "../context_switching.h"
#include "../debugging.h"
#include "defined_syscalls.h"
#include "utils.h"

int64_t sys_create_thread(void *function, void *arg, uint64_t *out_thread_id) {
    // kprintln("SYSCALLS: sys_create_thread called by %ld with entry point %lx.", ctx_switching_get_active_thread()->local_id, (uint64_t) function);

    if (!is_valid_user_range((uintptr_t) out_thread_id, 8) && out_thread_id != 0) return -1;

    Thread *thread = process_create_thread(function, (uint64_t) arg, ctx_switching_get_active_thread()->owner);
    if (out_thread_id != 0) *out_thread_id = thread->local_id;
    
    return 0;
}

void sys_exit_thread(void *result) {
    asm volatile ("swapgs");

    process_begin_thread_teardown(ctx_switching_get_active_thread(), (uint64_t) result);
    // TODO: Maybe change specification to report errors?
}

void sys_yield() {
    // NOTE: The interrupt handler doesn't touch swapgs.
    // GS register is only relevant for context switching
    // for the syscall. Therefore, if we don't swap to
    // the user's gs now, if the yielded thread performs
    // a syscall, then the GS register will point to
    // the wrong memory.

    // NOTE: The same can be said about every syscall
    // that context switches to another thread.
    // iretq doesn't touch the GS register so we
    // need to fix it ourselves.

    asm volatile (
        "swapgs\n"
        "int $0x81\n"
        "swapgs\n"
        ::: "memory"
    );
}

int64_t sys_wait_thread(uint64_t id, void **out_result) {
    if (!is_valid_user_range((uintptr_t) out_result, 8)) return -1;

    Thread *thread = ctx_switching_get_active_thread();

    Thread *target = process_try_get_thread_from_id(thread->owner, id);

    if (target == null) {
        kprintln("SYSCALLS: sys_wait_thread: Thread %ld of process %ld invoked sys_wait_thread with an invalid thread id.",
            thread->local_id, thread->owner->pid);
        return -1;
    }

    bool is_blocked = process_block_thread_for_another(thread, target);
    if (!is_blocked) return -1;

    sys_yield(); // Begin blocking.

    *out_result = (void*) thread->wake_result;
    return 0;
}

uint64_t sys_get_thread_id() {
    return ctx_switching_get_active_thread()->local_id;
}
