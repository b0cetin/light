
#include "include/string.h"
#include <stddef.h>
#include <stdint.h>

void memcpy(void *dest, const void* src, size_t size) {
    char *d = (char *)dest;
    const char *s = (const char *)src;

    for (uint64_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

void memset(void *adr, uint8_t value, size_t size) {
    for (uint64_t i = 0; i < size; i++) {
        ((int8_t *)adr)[i] = value;
    }
}

size_t strnlen(const char str[], size_t maxlen) {
    if (str == NULL) return 0;

    size_t len;

    for (len = 0; len < maxlen && str[len] != '\0'; len++);

    return len;
}
