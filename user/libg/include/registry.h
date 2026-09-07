
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint16_t interface_id;
    uint64_t target_pid;
} GRegistryItem;

typedef struct {
    size_t item_count;
    GRegistryItem *items;
} GRegistry;

bool poll_registry(GRegistry *out_registry);
