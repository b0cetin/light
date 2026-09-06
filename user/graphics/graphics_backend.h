
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint64_t DisplayRefreshRate;
#define REFRESH_RATE_UNKNOWN 0

typedef enum { PIXEL_FORMAT_XRGB8888, PIXEL_FORMAT_BGRX8888 } PixelFormat;

typedef struct {
    uint64_t width;
    uint64_t height;
    DisplayRefreshRate refresh_rate;
    PixelFormat pixel_format;
} DisplayProperties;

typedef struct { uint64_t x, y, width, height; } DisplayRect;

typedef bool (*DspPollProperties) (DisplayProperties *out_properties);
typedef bool (*DspInitialize) (void);
typedef void (*DspDestruct) (void);
typedef bool (*DspPresentCPUBuffer) (void *buffer, size_t length, uint64_t stride, DisplayRect dirty_rect);

typedef struct {
    DspInitialize init;
    DspDestruct destroy;
    DspPollProperties poll_properties;
    // The given buffer must be mapped inside the graphics server. The buffer must still cover the full canvas. The length is used as a memory guard.
    DspPresentCPUBuffer present_cpu_buffer;
} DisplayBackend;
