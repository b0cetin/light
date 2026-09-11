
#include "kobjects.h"
#include "allocator.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "vas.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    KernelObjectEntry *entries;
    size_t capacity;
    size_t count;
    KernelObjectID next_id_hint;
} KernelObjectsTable;

KernelObjectsTable table;
#define KOBJECTS_START_COUNT 256

void kobjects_init() {
    table.capacity = KOBJECTS_START_COUNT;
    table.count = 0;
    table.entries = kmalloc(sizeof(KernelObjectsTable) * KOBJECTS_START_COUNT);

    kprintln("KOBJ: Central table initialized!");
}

bool kobject_resolve(KernelObjectID id, KernelObjectEntry **out) {
    if (id >= table.capacity) return false;
    
    KernelObjectEntry *entry = &table.entries[id];
    if (!entry->exists) return false;

    if (out != null) *out = entry;
    return true;
}

// Returns false if out of memory.
bool kobject_create_without_init(KernelObjectType type, KernelObjectID *out_id, KernelObjectEntry **out_entry) {
    if (table.count >= table.capacity) {
        size_t new_capacity = table.capacity * 2;

        KernelObjectEntry *new_objects = krealloc(table.entries, sizeof(KernelObjectEntry) * new_capacity);
        if (new_objects == null) { 
            kprintln("KOBJ: Table full, can't increase table size: Out of memory!");
            return false;
        }

        table.entries = new_objects;
        table.capacity = new_capacity;
    }

    KernelObjectID id = table.next_id_hint;
    if (id == NULL_KOBJECT || table.entries[id].exists) {
        id = id >= table.capacity - 1 ? NULL_KOBJECT + 1 : id + 1;

        bool found = false;
        while (id != table.next_id_hint) {
            if (id == NULL_KOBJECT) { id++; continue; }
            if (!table.entries[id].exists) {
                found = true;
                break;
            }

            id++;
        }

        if (!found) PANIC("kobject_table_add: impossible state encountered and no empty slot was found.");
    }

    KernelObjectEntry *entry = &table.entries[id];

    entry->exists = true;
    entry->type = type;
    entry->generation++;
    entry->ref_count = 0;

    if (out_id != null) *out_id = id;
    if (out_entry != null) *out_entry = entry;
    return true;
}

void kobject_increase_ref(KernelObjectID id) {
    KernelObjectEntry *entry = null;
    assert_msg(kobject_resolve(id, &entry), "kobject_increase_ref: kobject_resolve failed");

    entry->ref_count++;
}

void kobject_decrease_ref(KernelObjectID id) {
    KernelObjectEntry *entry = null;
    assert_msg(kobject_resolve(id, &entry), "kobject_decrease_ref: kobject_resolve failed");

    entry->ref_count--;

    if (entry->ref_count <= 0) {
        switch (entry->type) {
        case KOBJECT_SHAREDMEMORY:
            if (entry->object.shared_memory.map_count <= 0)
                vas_destroy_shared_memory(id);
            return;
        default:
            PANIC("kobject_decrease_ref: unreachable state reached");
        }
    }
}

void kobject_reset(KernelObjectID id) {
    KernelObjectEntry *entry = null;
    assert_msg(kobject_resolve(id, &entry), "kobject_reset: kobject_resolve failed");

    entry->exists = false;
}

KernelObjectID kobject_find(KernelObjectEntry *entry) {
    uintptr_t entry_ptr = (uintptr_t) entry;
    uintptr_t entries_ptr = (uintptr_t) table.entries;

    if (entry_ptr < entries_ptr) return NULL_KOBJECT;

    uintptr_t diff = entry_ptr - entries_ptr;
    if (diff >= table.capacity * sizeof(KernelObjectEntry)) return NULL_KOBJECT;

    return diff / sizeof(KernelObjectEntry);
}
