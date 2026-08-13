
#pragma once

#include <stddef.h>
#include <stdint.h>

void memcpy(void *dest, const void* src, uint64_t size);

void memset(void *adr, uint8_t value, uint64_t size);

static inline void memzero(void *adr, uint64_t size) {
    memset(adr, 0, size);
}

int compare_ascii_utf16(const char *ascii_str, const uint16_t *utf16_str);
void convert_utf16_to_ascii(const uint16_t *utf16_str, char *ascii_out);

size_t strnlen(const char str[], size_t maxlen);
