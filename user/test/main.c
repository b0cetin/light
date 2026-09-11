
#include "syscalls.h"
#include <registry.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    println("Test.");

    void *address = NULL;
    handle_t handle = 0;
    println("%ld", sys_memory_share_create(&address, 4096 * 200, MMAP_ACCESS_READ | MMAP_ACCESS_WRITE, &handle));
    println("handle: %lu", handle);
    println("remove: %ld", sys_memory_share_remove(handle));
    println("unmap: %ld", sys_memory_unmap(NULL, UINT64_MAX, 0));

    malloc(0);

    println("a");

    while (1);

    GRegistry registry;
    if (!poll_registry(&registry)) {
        println("Something went wrong while polling registry.");
        return -1;
    }

    println("Interface count: %ld", registry.item_count);

    while (1);

    return 0;
}
