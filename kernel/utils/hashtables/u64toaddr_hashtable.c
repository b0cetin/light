
#include "u64toaddr_hashtable.h"
#include "allocator.h"
#include "debugging.h"
#include "types.h"

#include <stddef.h>
#include <stdint.h>

#define INITIAL_CAPACITY 16

// NOTE: This is done like this because SSE is disabled kernel-wide. (0.75)
#define COMPUTE_LOAD_THRESHOLD(capacity) (capacity * 3 / 4)

// source: https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
// based on splitmix64
static uint64_t hash(uint64_t x) {
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

Hashtable *ht_create() {
    Hashtable *table = kmalloc(sizeof(Hashtable));
    if (table == null) return null;

    table->capacity = INITIAL_CAPACITY;
    table->size = 0;

    table->bucket = kcalloc(table->capacity, sizeof(Node));

    if (!table->bucket) {
        kfree(table);
        return null;
    }

    return table;
}

static bool ht_resize(Hashtable *table, size_t new_capacity) {
    if (COMPUTE_LOAD_THRESHOLD(new_capacity) <= table->size) {
        kprintln("HTABLE: New requested resize capacity %ld is too small for a table with %ld many items!", new_capacity, table->size);
        return false;
    }

    Node **old_bucket = table->bucket;
    size_t old_capacity = table->capacity;

    table->bucket = kcalloc(new_capacity, sizeof(Node*));
    table->capacity = new_capacity;

    for (size_t i = 0; i < old_capacity; i++) {
        Node *node = old_bucket[i];
        if (node == null) continue;

        size_t index = hash(node->key) % new_capacity;
        table->bucket[index] = node;
    }

    kfree(old_bucket);

    kprintln("HTABLE: Table resized from %ld to %ld!", old_capacity, new_capacity);

    return true;
}

static bool ht_set_internal(Hashtable *table, uint64_t key, uintptr_t address, bool only_add) {
    if (table->size >= COMPUTE_LOAD_THRESHOLD(table->capacity)) {
        if (!ht_resize(table, table->capacity * 2))
            return false;
    }

    size_t index = hash(key) % table->capacity;
    Node *current = table->bucket[index];

    while (current != null) {
        if (current->key == key) {
            if (only_add) return false;

            current->address = address;
            return true;
        }

        current = current->next;
    }

    // This key needs to be inserted.

    Node* new_node = kmalloc(sizeof(Node));
    if (new_node == null) return false;

    new_node->key = key;
    new_node->address = address;

    // Instead of inserting at the end, we insert at the beginning.
    // This assumes that the last ones to be inserted are usually going to be looked for first.

    new_node->next = table->bucket[index];
    table->bucket[index] = new_node;
    table->size++;

    return true;
}

bool ht_set(Hashtable *table, uint64_t key, uintptr_t address) {
    return ht_set_internal(table, key, address, false);
}

bool ht_lookup(Hashtable *table, uint64_t key, uintptr_t *out) {
    size_t index = hash(key) % table->capacity;
    Node *current = table->bucket[index];

    while (current != null) {
        if (current->key == key) {
            if (out != null) *out = current->address;
            return true;
        }

        current = current->next;
    }

    // No such key found.

    return false;
}

bool ht_add(Hashtable *table, uint64_t key, uintptr_t address) {
    return ht_set_internal(table, key, address, true);
}

bool ht_remove(Hashtable *table, uint64_t key) {
    size_t index = hash(key) % table->capacity;

    Node *previous = null;
    Node *current = table->bucket[index];

    while (current != null) {
        if (current->key == key) {
            if (previous != null) {
                previous->next = current->next;
            }
            else {
                table->bucket[index] = null;
            }

            kfree(current);
            return true;
        }

        previous = current;
        current = current->next;
    }

    return false;
}
