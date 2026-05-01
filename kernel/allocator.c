
#include "allocator.h"
#include "debugging.h"
#include "pmm.h"
#include "vmm.h"
#include <stdint.h>

#define PAGE_SIZE 4096

void *malloc(uint64_t size) {
    if (size > PAGE_SIZE)
        PANIC("Tried to allocate than %d bytes (%d requested)!", PAGE_SIZE, size);

    uint64_t phys_pos = pmm_alloc_page();

    if (phys_pos == 0)
        PANIC("Out of memory!");

    return p2v(phys_pos);
}

void free(void *ptr) {
    pmm_mark_free(v2p(ptr));
}
