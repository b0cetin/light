
#pragma once

#include <stddef.h>

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

typedef struct { uint64_t x, y, width, height; } DirtyRect;
