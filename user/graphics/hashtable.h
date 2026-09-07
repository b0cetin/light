
#pragma once

#include <stdbool.h>
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
bool ht_set(Hashtable *table, uint64_t key, void *ptr);
bool ht_add(Hashtable *table, uint64_t key, void *ptr);
bool ht_lookup(Hashtable *table, uint64_t key, void **out);
bool ht_remove(Hashtable *table, uint64_t key);
