
#pragma once

#include <stdint.h>

#define READ_MODULE_PATH_SIZE 32

typedef struct {
    uint8_t is_read;
    uint16_t path[READ_MODULE_PATH_SIZE];
    uint64_t physical_location;
    uint64_t size;
} ReadModule;
