
#pragma once

#include <stdint.h>

#define READ_MODULE_PATH_SIZE 32
#define READ_MODULE_COUNT 16

typedef struct __attribute__((packed)) {
    uint8_t is_read;
    uint16_t path[READ_MODULE_PATH_SIZE];
    uint64_t physical_location;
    uint64_t size;
    
    uint8_t padding[7];
} ReadModule;

typedef struct {
    struct {
        uint64_t PhysicalFramebufferBase;
        uint64_t FramebufferSize;
        uint32_t HorizontalResolution;
        uint32_t VerticalResolution;
        uint32_t PixelsPerScanLine;
    } framebuffer;

    struct {
        uint64_t PhysicalMMapBase;
        uint64_t MMapSize;
        uint64_t DescriptorSize;
    } memory_map;

    uint64_t physical_xsdp_address;

    ReadModule modules[READ_MODULE_COUNT];
} BootInfo;
