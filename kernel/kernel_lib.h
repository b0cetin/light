
#pragma once

#include <stdint.h>

void memcpy(void *dest, const void* src, uint64_t size);

void memset(void *adr, uint8_t value, uint64_t size);

static inline void memzero(void *adr, uint64_t size) {
    memset(adr, 0, size);
}
