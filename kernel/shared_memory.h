
#pragma once

#include "vmm.h"
#include <stddef.h>
#include <stdint.h>

typedef uint64_t SharedMemoryID;
#define SMEM_NULL_ID 0

typedef struct {
    SharedMemoryID id;

    size_t page_count;
    uint64_t *physical_pages;

    uint64_t map_flags;

    uint64_t reference_count;
} SharedMemoryEntry;

SharedMemoryID smem_create(PML4 *address_space, uintptr_t base, uint64_t page_count);
uint64_t smem_map(PML4 *address_space, uintptr_t base, SharedMemoryID id);

// TODO: smem_unmap: I didn't implement it because user VAS tracking is not a thing yet.
