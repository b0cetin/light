
#include "registry.h"
#include <stdio.h>
#include <syscalls.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define CLIENT_REGISTRY_POLL_CALL_NUM UINT64_MAX
#define GRAPHICS_PID 2

bool poll_registry(GRegistry *out_registry) {
    GRegistry registry;
    registry.item_count = 0;
    registry.items = calloc(32, sizeof(GRegistryItem));

    rpc_result_t initial_rpc = sys_rpc_invoke(GRAPHICS_PID, CLIENT_REGISTRY_POLL_CALL_NUM, 0, 0, 0, 0);
    if (initial_rpc.error_code != SYS_SUCCESS)
    {
        println("libg: poll_registry: initial sys_rpc_invoke failed: %li", initial_rpc.error_code);
        return false;
    }
    if (initial_rpc.result != 0)
    {
        println("libg: poll_registry: initial sys_rpc_invoke returned non-zero value.");
        return false;
    }

    while (true) {
        pid_t caller;
        uint64_t call_num, arg0, arg1, arg2, arg3;

        if (sys_rpc_receive(&caller, &call_num, &arg0, &arg1, &arg2, &arg3) != SYS_SUCCESS) {
            println("libg: poll_registry: sys_rpc_receive failed.");
            return false;
        }

        if (caller != GRAPHICS_PID) {
            println("libg: poll_registry: received rpc from somewhere else.");
            sys_rpc_return(caller, -1);
            continue;
        }

        if (call_num > 0) {
            // end of registry
            sys_rpc_return(caller, 0);
            *out_registry = registry;
            return true;
        }

        if (registry.item_count >= 32) {
            println("libg: poll_registry: received too many items, cutting short.");
            sys_rpc_return(caller, -1);
            *out_registry = registry;
            return true;
        }

        size_t i = registry.item_count++;
        registry.items[i].target_pid = arg0;
        registry.items[i].interface_id = arg1;
    }
}
