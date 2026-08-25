
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <syscalls.h>

bool is_invoking;

void terminate_while_invoking() {
    while (true) {
        if (is_invoking) {
            println("Terminating while invoking!");
            sys_exit(0);
        }

        sys_yield();
    }
}

int main() {
    sys_yield();
    sys_yield();
    sys_yield();

    sys_create_thread(terminate_while_invoking, NULL, NULL);

    is_invoking = true;
    rpc_result_t result = sys_rpc_invoke(0, 67, 0, 0, 0, 0);
    is_invoking = false;
    
    if (result.error_code != SYS_SUCCESS) {
        println("sys_rpc_invoke returned: %ld", result.error_code);
        return -1;
    }

    println("RPC result: %ld", result.result);

    while (1);

    return 0;
}
