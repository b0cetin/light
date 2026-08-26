
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <syscalls.h>

uint64_t get_from_init(uint64_t request_code) {
    rpc_result_t rpc_result = sys_rpc_invoke(0, request_code, 0, 0, 0, 0);;

    if (rpc_result.error_code != SYS_SUCCESS) {
        println("get_from_init: RPC (%lu) failed with error code: %li", request_code, rpc_result.error_code);
        sys_exit(-1);
    }

    return rpc_result.result;
}

void fade_to_black(uint8_t *restrict buffer, size_t size) { // Software emulated fade.
    for (uint8_t i = 0; i < UINT8_MAX; i++) {
        for (size_t j = 0; j < size; j++) {
            buffer[j] -= (buffer[j] > 0);
        }
    }
}

int main() {
    println("GFX: Waiting for framebuffer information from init process...");

    uint8_t *buffer = NULL;
    uint64_t size, width, height;

    while (1) {
        pid_t pid;
        uint64_t call, arg0, arg1, arg2, arg3;

        if (sys_rpc_receive(&pid, &call, &arg0, &arg1, &arg2, &arg3) != SYS_SUCCESS)
            return -1;

        if (pid != 0) {
            sys_rpc_return(pid, (int64_t) -1);
            continue;
        }

        switch (call) {
        case 0: {
            uint64_t physical_base = arg0;
            size = arg1;
            width = arg2;
            height = arg3;

            buffer = 0;

            if (sys_map_mmio(physical_base, size, (uintptr_t*) &buffer) != SYS_SUCCESS) {
                println("Mapping UEFI framebuffer failed.");
                return -1;
            }

            println("UEFI framebuffer at %lx, size %lu. %lux%lu@X", (uint64_t) buffer, size, width, height);
            goto init;
        }
        case 1:
            println("No UEFI framebuffer available.");
            goto init;
        default:
            sys_rpc_return(pid, (int64_t) -1);
            break;
        }
    }

    init:

    if (buffer != NULL) {
        println("Fading to black...");
        fade_to_black(buffer, size);
        println("Faded.");
    }

    while (true);

    return 0;
}
