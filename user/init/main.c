
#include "boot_module.h"
#include "string.h"
#include <stdio.h>
#include <syscalls.h>
#include <stddef.h>
#include <stdint.h>

uint64_t launch_module(BootModule *module) {
    size_t path_size = strnlen(module->path, BOOT_MODULE_PATH_SIZE);

    uint64_t pid;
    if (sys_create_process((void*) module->address, module->size, module->path, path_size, &pid)
        != SYS_SUCCESS)
        pid = UINT64_MAX;

    return pid;
}

int main(BootModule *modules) {
    sys_print("Init started.");

    println("Listing loaded modules now.");
    for (int i = 0; i < BOOT_MODULE_COUNT; i++) {
        BootModule *module = &modules[i];
        if (!module->is_read) continue;
        println("%i: %s", i, module->path);
    }

    launch_module(&modules[1]); // keyboard
    launch_module(&modules[2]); // graphics

    // Listen to incoming calls

    pid_t my_pid = sys_get_pid();

    while (1) {
        pid_t pid;
        uint64_t call, arg0, arg1, arg2, arg3;

        if (sys_rpc_receive(&pid, &call, &arg0, &arg1, &arg2, &arg3) != SYS_SUCCESS)
            break;

        if (pid == my_pid) {
            sys_rpc_return(pid, 0);
            println("Stopping init server...");
            break;
        }

        switch (call) {
        case 67:
            sys_yield();
            sys_yield();
            sys_yield();
            sys_yield();
            sys_yield();
            println("Returning result!");
            println("sys_rpc_return: %ld", sys_rpc_return(pid, 69 + arg0 + arg1 + arg2 + arg3));
            break;
        default:
            sys_rpc_return(pid, (int64_t) -1);
            break;
        }
    }

    return 0;
}
