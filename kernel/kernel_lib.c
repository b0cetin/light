
#include "kernel_lib.h"
#include <stdint.h>

void memcpy(void *dest, const void* src, uint64_t size) {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    for (uint64_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

void memset(void *adr, uint8_t value, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        ((int8_t *)adr)[i] = value;
    }
}
