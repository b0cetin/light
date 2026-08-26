
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
    if (get_from_init(8000000)) {
        uint64_t physical_base = get_from_init(8000001);
        uint64_t size = get_from_init(8000002);
        uint64_t width = get_from_init(8000003);
        uint64_t height = get_from_init(8000004);

        void *virtual_base = 0;

        if (sys_map_mmio(physical_base, size, (uintptr_t*) &virtual_base) != SYS_SUCCESS) {
            println("Mapping UEFI framebuffer failed.");
            return -1;
        }

        println("UEFI framebuffer at %lx, size %lu. %lux%lu@X", (uint64_t) virtual_base, size, width, height);

        println("Fading to black...");
        fade_to_black(virtual_base, size);
        println("Faded.");
    }
    else {
        println("No UEFI framebuffer detected.");
    } 

    while (true);

    return 0;
}
