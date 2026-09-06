
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "shared.h"

typedef uint64_t DisplayRefreshRate;
#define REFRESH_RATE_UNKNOWN 0

typedef struct {
    uint64_t width;
    uint64_t height;
    DisplayRefreshRate refresh_rate;
    PixelFormat pixel_format;
} DisplayProperties;

typedef bool (*DspPollProperties) (DisplayProperties *out_properties);
typedef bool (*DspInitialize) (void);
typedef void (*DspDestruct) (void);
typedef bool (*DspPresentCPUBuffer) (void *buffer, size_t length, uint64_t stride, DirtyRect dirty_rect);

typedef struct {
    DspInitialize init;
    DspDestruct destroy;
    DspPollProperties poll_properties;
    // The given buffer must be mapped inside the graphics server. The buffer must still cover the full canvas. The length is used as a memory guard.
    DspPresentCPUBuffer present_cpu_buffer;
} DisplayBackend;
