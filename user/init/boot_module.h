
#pragma once

#include <stdint.h>

#define BOOT_MODULE_PATH_SIZE 64 
#define BOOT_MODULE_COUNT 16

typedef struct __attribute__((packed)) {
    uint8_t is_read;
    char path[BOOT_MODULE_PATH_SIZE];
    uint64_t address;
    uint64_t size;

    uint8_t padding[7];
} BootModule;
