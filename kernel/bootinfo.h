
#pragma once

#include <stdint.h>

typedef struct {
    uint64_t PhysicalFramebufferBase;
    uint64_t FramebufferSize;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    uint32_t PixelsPerScanLine;

    void *MMap;
    uint64_t MMapSize;
    uint64_t DescriptorSize;

    void *PhysicalKernelBase;
} BootInfo;
