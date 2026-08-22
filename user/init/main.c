
#include "boot_module.h"
#include <syscalls.h>
#include <stddef.h>
#include <stdint.h>

void *keyboard_watcher() {
    sys_print("Keyboard watcher thread active.");

    sys_interrupt_control(IRQCTL_SET, 1);

    int64_t status;
    while (1) {
        status = sys_interrupt_control(IRQCTL_AWAIT, 1);

        if (status == SYS_ERR_IRQCTL_VECTOR_NOT_RESERVED) break;
        if (status == SYS_ERR_IRQCTL_AWAIT_CANCELLED) break;

        if (status == SYS_SUCCESS)
            sys_print("Keyboard interrupt received!");
        else
            break;
    }

    sys_print("Break from while loop!");

    sys_interrupt_control(IRQCTL_UNSET, 1);

    sys_exit_thread((void*) status);
}

int main(BootModule *modules) {
    sys_print("Init started.");

    uint64_t thread_id;
    if (sys_create_thread(keyboard_watcher, NULL, &thread_id) != SYS_SUCCESS) {
        sys_print("sys_create_thread failed.");
        return -1;
    }

    while (1);

    int64_t result;
    sys_wait_thread(thread_id, (void**) &result);

    return result;
}
