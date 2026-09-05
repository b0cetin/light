
#pragma once

#include "types.h"
#include "vas.h"
#include <stddef.h>
#include <stdint.h>

typedef uint64_t SharedMemoryID;
#define SMEM_ID_NULL 0

void smem_init();
SharedMemoryID smem_add(VirtualMemoryObject *object);
VirtualMemoryObject *smem_get(SharedMemoryID id);
void smem_remove(VirtualMemoryObject *object);

static inline bool smem_is_shared(VirtualMemoryObject *object) {
    return object->smem_id != SMEM_ID_NULL;
}
