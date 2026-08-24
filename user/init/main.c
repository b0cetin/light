
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

    launch_module(&modules[1]);

    while (1);

    return 0;
}
