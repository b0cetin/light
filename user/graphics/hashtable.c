
#include "hashtable.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define INITIAL_CAPACITY 16

// FIXME: This is done like this because I don't think I have SSE context switching yet. (0.75)
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
    Hashtable *table = malloc(sizeof(Hashtable));
    if (table == NULL) return NULL;

    table->capacity = INITIAL_CAPACITY;
    table->size = 0;

    table->bucket = calloc(table->capacity, sizeof(Node));

    if (!table->bucket) {
        free(table);
        return NULL;
    }

    return table;
}

static bool ht_resize(Hashtable *table, size_t new_capacity) {
    if (COMPUTE_LOAD_THRESHOLD(new_capacity) <= table->size)
        return false;

    Node **old_bucket = table->bucket;
    size_t old_capacity = table->capacity;

    table->bucket = calloc(new_capacity, sizeof(Node*));
    table->capacity = new_capacity;

    for (size_t i = 0; i < old_capacity; i++) {
        Node *node = old_bucket[i];
        if (node == NULL) continue;

        size_t index = hash(node->key) % new_capacity;
        table->bucket[index] = node;
    }

    free(old_bucket);

    return true;
}

static bool ht_set_internal(Hashtable *table, uint64_t key, uintptr_t address, bool only_add) {
    if (table->size >= COMPUTE_LOAD_THRESHOLD(table->capacity)) {
        if (!ht_resize(table, table->capacity * 2))
            return false;
    }

    size_t index = hash(key) % table->capacity;
    Node *current = table->bucket[index];

    while (current != NULL) {
        if (current->key == key) {
            if (only_add) return false;

            current->address = address;
            return true;
        }

        current = current->next;
    }

    // This key needs to be inserted.

    Node* new_node = malloc(sizeof(Node));
    if (new_node == NULL) return false;

    new_node->key = key;
    new_node->address = address;

    // Instead of inserting at the end, we insert at the beginning.
    // This assumes that the last ones to be inserted are usually going to be looked for first.

    new_node->next = table->bucket[index];
    table->bucket[index] = new_node;
    table->size++;

    return true;
}

bool ht_set(Hashtable *table, uint64_t key, void *ptr) {
    return ht_set_internal(table, key, (uintptr_t) ptr, false);
}

bool ht_lookup(Hashtable *table, uint64_t key, void **out) {
    size_t index = hash(key) % table->capacity;
    Node *current = table->bucket[index];

    while (current != NULL) {
        if (current->key == key) {
            if (out != NULL) *out = (void*) current->address;
            return true;
        }

        current = current->next;
    }

    // No such key found.

    return false;
}

bool ht_add(Hashtable *table, uint64_t key, void *ptr) {
    return ht_set_internal(table, key, (uintptr_t) ptr, true);
}

bool ht_remove(Hashtable *table, uint64_t key) {
    size_t index = hash(key) % table->capacity;

    Node *previous = NULL;
    Node *current = table->bucket[index];

    while (current != NULL) {
        if (current->key == key) {
            if (previous != NULL) {
                previous->next = current->next;
            }
            else {
                table->bucket[index] = NULL;
            }

            free(current);
            return true;
        }

        previous = current;
        current = current->next;
    }

    return false;
}
