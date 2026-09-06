
#pragma once

#include <stddef.h>

#include "syscalls.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum { PIXEL_FORMAT_XRGB8888, PIXEL_FORMAT_BGRX8888 } PixelFormat;
static inline size_t get_pixel_format_size_per_pixel(PixelFormat format) {
    switch (format) {
    case PIXEL_FORMAT_BGRX8888:
    case PIXEL_FORMAT_XRGB8888:
        return 4;
    }

    return 0;
}

typedef struct {
    bool exists;
    pid_t target;
    void *base;
    size_t size;
} MemoryPool;
typedef uint32_t MemoryPoolID;

typedef struct {
    bool exists;

    MemoryPoolID pool;

    uint64_t offset;
    size_t size;

    uint64_t stride;
    uint64_t height;
    PixelFormat format;
} MemoryBuffer;
typedef uint32_t MemoryBufferID;

typedef struct { uint64_t x, y, width, height; } DirtyRect;
typedef struct ListedDirtyRect { DirtyRect dirty_rect; struct ListedDirtyRect *next; } ListedDirtyRect;
