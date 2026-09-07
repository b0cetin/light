
#include "backends/efi_fb.h"
#include "client_registry.h"
#include "graphics_backend.h"
#include "interface.h"
#include "interfaces/interfaces.h"
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <syscalls.h>

void fade_to_black(uint8_t *restrict buffer, size_t size) { // Software emulated fade.
    for (uint8_t i = 0; i < UINT8_MAX; i++) {
        for (size_t j = 0; j < size; j++) {
            buffer[j] -= (buffer[j] > 0);
        }
    }
}

DisplayBackend active_backend;

// Used by efi_framebuffer.h
uint64_t efi_fb_phys_base;
size_t efi_fb_size;
uint64_t efi_fb_width, efi_fb_height, efi_fb_pitch;

void exit(int64_t status) {
    active_backend.destroy();
    sys_exit(status);
}

void receive_rpc_thread() {
    pid_t caller;
    uint64_t call_num, arg0, arg1, arg2, arg3;
    int64_t receive_result;

    while (true) {
        receive_result = sys_rpc_receive(&caller, &call_num, &arg0, &arg1, &arg2, &arg3);
        if (receive_result != SYS_SUCCESS) {
            println("sys_rpc_receive failed with %li.", receive_result);
            exit(-1);
        }

        if (caller == 0) { // init
            sys_rpc_return(caller, INT64_C(-1));
            println("unhandled rpc call from init: %lu", call_num);
            exit(-1);
        }

        if (call_num == CLIENT_REGISTRY_POLL_CALL_NUM) { // poll registry
            sys_create_thread(client_registry_respond, (void*) caller, NULL);
            continue;
        }

        uint64_t out = INT64_MIN;
        receive_call(caller, call_num, arg0, arg1, arg2, arg3, &out);
        sys_rpc_return(caller, out);
    }
}

int main() {
    println("Waiting for framebuffer information from init process...");

    uint8_t *buffer = NULL;

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
            efi_fb_phys_base = call;
            efi_fb_size = arg0;
            efi_fb_width = arg1;
            efi_fb_height = arg2;
            efi_fb_pitch = arg3;

            buffer = 0;

            if (sys_map_mmio(efi_fb_phys_base, efi_fb_size, (uintptr_t*) &buffer) != SYS_SUCCESS) {
                println("Mapping boot UEFI framebuffer failed.");
                return -1;
            }

            println("UEFI framebuffer at %lx, size %lu, pitch: %lu. %lux%lu@X", efi_fb_phys_base, efi_fb_size, efi_fb_pitch, efi_fb_width, efi_fb_height);
        }

        goto init;
    }

    init:

    if (buffer != NULL) {
        println("Fading to black...");
        fade_to_black(buffer, efi_fb_size);
        println("Faded.");

        if (sys_memory_unmap(buffer, efi_fb_size, 0) != SYS_SUCCESS) {
            println("Unmapping boot EFI framebuffer failed!");
            return -1;
        }

        println("Unmapped boot EFI buffer.");
    }

    active_backend = efi_fb_get();

    if (!active_backend.init()) {
        println("Initializing display backend failed!");
        return -1;
    }

    println("EFI framebuffer display backend initialized.");

    register_interface(interfaces_compositor());
    register_interface(interfaces_shared_memory());

    println("All interfaces registered.");

    // log properties
    {
        DisplayProperties start_properties;
        
        if (!active_backend.poll_properties(&start_properties))
            println("Failed polling display properties.");
        else {
            println("Display properties: %lux%lu@%lu (pixel format: %lu)",
                start_properties.width, start_properties.height,
                start_properties.refresh_rate, start_properties.pixel_format);
        }
    }

    sys_create_thread(receive_rpc_thread, NULL, NULL);
    sys_create_thread(receive_rpc_thread, NULL, NULL);
    sys_create_thread(receive_rpc_thread, NULL, NULL);
    sys_create_thread(receive_rpc_thread, NULL, NULL);
    sys_create_thread(receive_rpc_thread, NULL, NULL);

    println("Listening for RPCs now.");

    sys_rpc_return(0, 0); // Return control back to init process.

    while (1);

    exit(0);
}
