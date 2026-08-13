
#include "kernel_lib.h"
#include "types.h"
#include <stddef.h>
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

int compare_ascii_utf16(const char *ascii_str, const uint16_t *utf16_str) {
    while (*ascii_str && *utf16_str) {
        if (*utf16_str > 127 || (uint16_t)(*ascii_str) != *utf16_str) {
            return (uint16_t)(*ascii_str) - *utf16_str;
        }
        ascii_str++;
        utf16_str++;
    }
    
    return (uint16_t)(*ascii_str) - *utf16_str;
}

void convert_utf16_to_ascii(const uint16_t *utf16_str, char *ascii_out) {
    uint64_t i = 0;
    
    while (utf16_str[i] != u'\0') {
        if (utf16_str[i] > 127) {
            ascii_out[i] = '?'; // Fallback for non-ASCII
        } else {
            ascii_out[i] = (char)utf16_str[i];
        }
        i++;
    }
    ascii_out[i] = '\0';
}

size_t strnlen(const char str[], size_t maxlen) {
    if (str == null) return 0;

    size_t len;

    for (len = 0; len < maxlen && str[len] != '\0'; len++);

    return len;
}
