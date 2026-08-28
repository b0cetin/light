
#pragma once

#include "types.h"
#include <stddef.h>
#include <stdint.h>

typedef struct Node {
    uint64_t key;
    uintptr_t address;
    struct Node *next;
} Node;

typedef struct {
    size_t size;
    size_t capacity;
    struct Node **bucket;
} Hashtable;

Hashtable *ht_create();
bool ht_set(Hashtable *table, uint64_t key, uintptr_t address);
bool ht_add(Hashtable *table, uint64_t key, uintptr_t address);
bool ht_lookup(Hashtable *table, uint64_t key, uintptr_t *out);
bool ht_remove(Hashtable *table, uint64_t key);
