
#pragma once

#include "types.h"
#include "vas.h"
#include <stddef.h>
#include <stdint.h>

typedef enum { KOBJECT_PORT, KOBJECT_SHAREDMEMORY } KernelObjectType;

#define NULL_KOBJECT 0
typedef uint64_t KernelObjectID;

typedef struct {
    KernelObjectType type;
    bool exists;
    size_t ref_count;
    uint32_t generation;

    union {
        VirtualMemoryObject shared_memory;
    } object;
} KernelObjectEntry;

void kobjects_init();

bool kobject_resolve(KernelObjectID id, KernelObjectEntry **out);
bool kobject_create_without_init(KernelObjectType type, KernelObjectID *out_id, KernelObjectEntry **out);

void kobject_increase_ref(KernelObjectID id);
void kobject_decrease_ref(KernelObjectID id);

void kobject_reset(KernelObjectID id);

KernelObjectID kobject_find(KernelObjectEntry *entry);
