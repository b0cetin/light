
#pragma once

#include "kobjects.h"
#include "stdint.h"
#include "types.h"

#define NULL_HANDLE 0
typedef uint64_t HandleID;

typedef struct {
    KernelObjectID object_id;
    uint32_t generation;
} HandleTableEntry;

typedef struct {
    HandleTableEntry* entries;
    size_t count;
    size_t capacity;
    HandleID next_id_hint;
} HandleTable;

bool handle_table_create(HandleTable *table);
void handle_table_destroy(HandleTable *table);

bool handle_add(HandleTable *table, KernelObjectID kobject, HandleID *out_handle);
bool handle_exists(HandleTable *table, HandleID id);
bool handle_remove(HandleTable *table, HandleID handle);
bool handle_copy(HandleTable *from, HandleID from_handle, HandleTable *to, HandleID *out_to_handle);

bool handle_resolve(HandleTable *table, HandleID handle, KernelObjectID *out_kid);
