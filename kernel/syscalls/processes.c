
#include "defined_syscalls.h"
#include "../processes.h"
#include "../context_switching.h"
#include "../debugging.h"
#include "../userspace.h"
#include "../elf_loader.h"
#include "../allocator.h"
#include "../kernel_lib.h"
#include "utils.h"

void sys_exit(int64_t status) {
    process_begin_process_teardown(ctx_switching_get_active_thread()->owner, status);
    // TODO: Maybe change specification to report errors?
}

uint64_t sys_get_pid() {
    return ctx_switching_get_active_thread()->owner->pid;
}

int64_t sys_create_process(void *content, size_t content_len, const char* name, size_t name_len, uint64_t *out_pid) {
    if (ctx_switching_get_active_thread()->owner->pid != PROCESS_INIT_PID)
    {
        kprintln("SYSCALLS: sys_create_process: Process %ld tried to invoke sys_create_process, but it's not the init process! (expected %ld)",
            ctx_switching_get_active_thread()->owner->pid, PROCESS_INIT_PID);
        return -1;
    }

    if (!is_valid_mapped_user_range((uintptr_t) content, content_len)) {
        kprintln("SYSCALLS: sys_create_process: User-supplied content range is not valid.");
        return -1;
    }
    if (!is_valid_mapped_user_range((uintptr_t) name, name_len)) {
        kprintln("SYSCALLS: sys_create_process: User-supplied name range is not valid. Size: %ld, address: %lx", name_len, (uint64_t) name);
        return -1;
    }
    if (!is_valid_mapped_user_range((uintptr_t) out_pid, sizeof(uint64_t*))) {
        kprintln("SYSCALLS: sys_create_process: User-supplied out_pid parameter is not valid.");
        return -1;
    }

    if (name_len >= PROCESS_NAME_MAX) {
        kprintln("SYSCALLS: sys_create_process: Requested name for process is too big: %ld bytes", name_len);
        return -1;
    }

    PML4* address_space = vmm_create_user_address_space();

    void *entry_point = load_elf(address_space, content, content_len);
    if (entry_point == null) {
        kprintln("SYSCALLS: sys_create_process: Failure loading ELF file.");
        vmm_destroy_user_address_space(address_space); // FIXME: Memory leak: Clean up ELF file.
        return -1;
    }

    char *c_name = kmalloc(name_len + 1);
    memcpy(c_name, name, name_len);
    c_name[name_len] = '\0';

    Process *process;

    process = process_create(entry_point, address_space, c_name);

    *out_pid = process->pid;
    return 0;
}
