
#include "init_info.h"
#include "string.h"
#include <stdio.h>
#include <syscalls.h>
#include <stddef.h>
#include <stdint.h>

uint64_t launch_module(BootModule *module) {
    size_t path_size = strnlen(module->path, BOOT_MODULE_PATH_SIZE);

    uint64_t pid;
    int64_t error_code = sys_create_process((void*) module->address, module->size, module->path, path_size, &pid);
    if (error_code != SYS_SUCCESS) {
        println("Failure launching boot module \"%s\": %li", module->path, error_code);
        sys_exit(-1);
    }

    return pid;
}

int main(InitInfo *init) {
    sys_print("Init started.");

    println("Listing loaded modules now.");
    for (int i = 0; i < BOOT_MODULE_COUNT; i++) {
        BootModule *module = &init->modules[i];
        if (!module->is_read) continue;
        println("%i: %s", i, module->path);
    }

    launch_module(&init->modules[1]); // keyboard
    uint64_t gfx_pid = launch_module(&init->modules[2]); // graphics
    launch_module(&init->modules[3]);

    sys_yield();

    println("Infinitely waiting...");

    while (1)
        ;

    // Initialize modules
    
    {
        rpc_result_t rpc;
        
        for (int i = 0; i < 3; i++) {
            if (init->framebuffer.available) {
                rpc = sys_rpc_invoke(gfx_pid, init->framebuffer.base,
                                     init->framebuffer.size, init->framebuffer.width, init->framebuffer.height,
                                     init->framebuffer.pitch);
            }
            else {
                rpc = sys_rpc_invoke(gfx_pid, 0, 0, 0, 0, 0);
            }

            if (rpc.error_code != SYS_SUCCESS) {
                println("Sending framebuffer data to the graphics server failed. Retrying...");
                sys_yield();
            }
            else {
                println("Sent framebuffer data to the graphics server.");
                break;
            }
        }

        if (rpc.error_code != SYS_SUCCESS) {
            println("Could not framebuffer data to the graphics server.");
            return -1;
        }
    }

    launch_module(&init->modules[3]); // test

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
        default:
            sys_rpc_return(pid, (int64_t) -1);
            break;
        }
    }

    return 0;
}
