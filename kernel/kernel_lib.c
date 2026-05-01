
#include "kernel_lib.h"
#include <stdint.h>

void memset(void *adr, uint8_t value, uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        ((int8_t *)adr)[i] = value;
    }
}
