
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <syscalls.h>

void fade_to_black(uint8_t *restrict buffer, size_t size) { // Software emulated fade.
    for (uint8_t i = 0; i < UINT8_MAX; i++) {
        for (size_t j = 0; j < size; j++) {
            buffer[j] -= (buffer[j] > 0);
        }
    }
}

int main() {
    println("Waiting for framebuffer information from init process...");

    uint8_t *buffer = NULL;
    uint64_t size = 0, width = 0, height = 0, pitch = 0;

    while (1) {
        pid_t pid;
        uint64_t call, arg0, arg1, arg2, arg3;

        int64_t receive_result = sys_rpc_receive(&pid, &call, &arg0, &arg1, &arg2, &arg3);
        if (receive_result != SYS_SUCCESS) {
            println("sys_rpc_receive failed: %li", receive_result);
            return -1;
        }

        if (pid != 0) {
            sys_rpc_return(pid, (int64_t) -1);
            continue;
        }

        if (call == 0) {
            println("No UEFI framebuffer available.");
        }
        else {
            uint64_t physical_base = call;
            size = arg0;
            width = arg1;
            height = arg2;
            pitch = arg3;

            buffer = 0;

            if (sys_map_mmio(physical_base, size, (uintptr_t*) &buffer) != SYS_SUCCESS) {
                println("Mapping UEFI framebuffer failed.");
                return -1;
            }

            println("UEFI framebuffer at %lx, size %lu, pitch: %lu. %lux%lu@X", (uint64_t) buffer, size, pitch, width, height);
        }

        sys_rpc_return(pid, 0);
        goto init;
    }

    init:

    if (buffer != NULL) {
        println("Fading to black...");
        fade_to_black(buffer, size);
        println("Faded.");

        memset(buffer, 0x50, size);
    }

    println("Creating a shared memory mapping of size 4096 * 2.");
    void *address = NULL;
    size_t length = 4096 * 2;
    SharedMemoryID id = 0;
    println("sys_memory_share_create result: %li", sys_memory_share_create(&address, length, MMAP_ACCESS_READ | MMAP_ACCESS_WRITE, &id));
    println("shared memory id: %lu", id);

    println("Mapping it in!");
    void *new_address = NULL;
    println("sys_memory_share_map result: %li", sys_memory_share_map(id, &new_address, MMAP_ACCESS_READ));

    sys_exit(0);

    while (true);

    return 0;
}
