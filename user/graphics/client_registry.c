
#include "client_registry.h"
#include "interface.h"
#include "stdio.h"
#include "syscalls.h"

void client_registry_respond(pid_t client) {
    sys_rpc_return(client, 0);

    rpc_result_t rpc_result;

    for (uint16_t i = 0; i < UINT8_MAX; i++) {
        ServerInterface *interface = get_interface(i);
        if (interface == NULL) continue;

        rpc_result = sys_rpc_invoke(client, 0, interface->handler, interface->interface_id, 0, 0);
        if (rpc_result.error_code != SYS_SUCCESS || rpc_result.result != 0) {
            println("client_registry: failed invoking send RPC, stopping.");
            sys_exit_thread((void*) -1);
        }
    }

    rpc_result = sys_rpc_invoke(client, 1, 0, 0, 0, 0);
    if (rpc_result.error_code != SYS_SUCCESS || rpc_result.result != 0) {
        println("client_registry: failed invoking stop RPC, stopping.");
        sys_exit_thread((void*) -1);
    }

    sys_exit_thread(0); // FIXME: An stdlib implementation of something like pthreads is needed.
}
