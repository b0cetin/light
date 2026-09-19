
#pragma once

#include "ports.h"
#include "types.h"
#include "vas.h"
#include <stddef.h>
#include <stdint.h>
#include "references.h"

typedef enum { KOBJECT_PORT, KOBJECT_SHAREDMEMORY } KernelObjectType;

typedef struct {
    KernelObjectType type;
    bool exists;
    size_t ref_count;
    uint32_t generation;

    union {
        VirtualMemoryObject shared_memory;
        IPCPort ipc_port;
    } object;
} KernelObjectEntry;

void kobjects_init();

bool kobject_resolve(KernelObjectID id, KernelObjectEntry **out);
bool kobject_create_without_init(KernelObjectType type, KernelObjectID *out_id, KernelObjectEntry **out);

void kobject_increase_ref(KernelObjectID id);
void kobject_decrease_ref(KernelObjectID id);

void kobject_reset(KernelObjectID id);

KernelObjectID kobject_find(KernelObjectEntry *entry);
