
#pragma once

#include <stddef.h>
#include <stdint.h>

void memcpy(void *dest, const void* src, size_t size);
void memset(void *adr, uint8_t value, size_t size);
static inline void memzero(void *adr, size_t size) {
    memset(adr, 0, size);
}

size_t strnlen(const char str[], size_t maxlen);
