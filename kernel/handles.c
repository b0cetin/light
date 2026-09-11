
#include "handles.h"
#include "allocator.h"
#include "debugging.h"
#include "kobjects.h"
#include "types.h"
#include <stdint.h>

bool handle_table_create(HandleTable *table) {
    table->capacity = 16;
    table->count = 0;
    table->next_id_hint = NULL_HANDLE + 1;
    table->entries = kmalloc(sizeof(HandleTableEntry) * table->capacity);

    return table->entries != null;
}

void handle_table_destroy(HandleTable *table) {
    for (size_t i = 0; i < table->capacity; i++) {
        HandleTableEntry *entry = &table->entries[i];

        KernelObjectID id = entry->object_id;
        if (entry->object_id != NULL_KOBJECT) kobject_decrease_ref(id);

        entry->object_id = NULL_KOBJECT;
        entry->generation = 0;
    }

    kfree(table->entries);
    table->entries = null;
    table->capacity = 0;
    table->count = 0;
    table->next_id_hint = 0;
}

// Returns false when not enough memory was available to complete the operation,
// or the given kernel object ID could not be resolved.
bool handle_add(HandleTable *table, KernelObjectID kobject, HandleID *out_handle) {
    assert(table); assert(kobject != NULL_KOBJECT);

    KernelObjectEntry *entry = null;
    if (!kobject_resolve(kobject, &entry)) return false;
    
    if (table->count >= table->capacity) {
        size_t new_capacity = table->capacity * 2;

        HandleTableEntry *new_objects = krealloc(table->entries, sizeof(HandleTableEntry) * new_capacity);
        if (new_objects == null) { 
            kprintln("HANDLES: Table full, can't increase table size: Out of memory!");
            return false;
        }

        table->entries = new_objects;
        table->capacity = new_capacity;
    }

    HandleID id = table->next_id_hint;
    if (id == NULL_HANDLE || table->entries[id].object_id != NULL_KOBJECT) {
        id = id >= table->capacity - 1 ? 0 : id + 1;

        while (id != table->next_id_hint) {
            if (id == NULL_HANDLE) { id++; continue; }
            if (table->entries[id].object_id != NULL_KOBJECT) goto add;
        }

        PANIC("kobject_table_add: impossible state encountered and no empty slot was found.");
    }

    add:

    table->entries[id].object_id = kobject;
    table->entries[id].generation = entry->generation;
    kobject_increase_ref(id);

    if (out_handle != null) *out_handle = id;
    return true;
}

bool handle_exists(HandleTable *table, HandleID handle) {
    assert(table);
    if (handle >= table->capacity) return false;
    if (table->entries[handle].object_id == NULL_KOBJECT) return false;

    KernelObjectID id = table->entries[handle].object_id;

    KernelObjectEntry *entry = null;
    if (!kobject_resolve(id, (KernelObjectEntry**) &entry)) return false;

    return table->entries[handle].generation == entry->generation;
}

bool handle_remove(HandleTable *table, HandleID handle) {
    assert(table);
    if (!handle_exists(table, handle)) return false;

    HandleTableEntry *entry = &table->entries[handle];

    KernelObjectID kobject = entry->object_id;
    entry->object_id = NULL_KOBJECT;
    entry->generation = 0;

    kobject_decrease_ref(kobject);
    return true;
}

bool handle_copy(HandleTable *from, HandleID from_id, HandleTable *to, HandleID *out_to_id) {
    assert(from); assert(from_id != NULL_HANDLE); assert(to);

    KernelObjectID kobject = NULL_KOBJECT;
    if (!handle_resolve(from, from_id, &kobject)) return false;

    return handle_add(to, kobject, out_to_id);
}

bool handle_resolve(HandleTable *table, HandleID handle, KernelObjectID *out_kid) {
    assert(table);
    assert(handle != NULL_HANDLE);

    if (handle >= table->capacity) return false;

    KernelObjectID id = table->entries[handle].object_id;

    KernelObjectEntry *entry = null;
    if (!kobject_resolve(id, &entry)) return false;

    if (table->entries[handle].generation != entry->generation) return false;

    if (out_kid != null) *out_kid = id;
    return true;
}
