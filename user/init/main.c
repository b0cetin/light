
#include "boot_module.h"
#include <syscalls.h>
#include <stddef.h>
#include <stdint.h>

void *thread_test(void *arg) {
    sys_print("This is a printing log from another thread!");

    sys_exit_thread((void*) 0);
}

int main(BootModule *modules) {
    sys_print("Init started.");

    uint64_t new_t;
    
    if (sys_create_thread(thread_test, (void*) 15, &new_t) != SYS_SUCCESS) {
        sys_print("Could not create new thread. Terminating.");
        return -1;
    }

    uint64_t result;
    if (sys_wait_thread(new_t, (void**) &result) != SYS_SUCCESS) {
        sys_print("Failure waiting for thread. Terminating.");
        return -1;
    }

    for (int i = 0; i < BOOT_MODULE_COUNT; i++) {
        if (!modules[i].is_read) continue;

        sys_print((char*) modules[i].path);
    }

    uint64_t pid;
    if (sys_create_process((void *) modules[1].address, modules[1].size,
        (const char*) &modules[1].path, 14, &pid) != SYS_SUCCESS) {
        sys_print("Failure creating process.");
        return -1;
    }

    sys_interrupt_control(IRQCTL_SET, 1);

    while (1);

    return -1;
}
