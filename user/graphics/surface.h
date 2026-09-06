
#pragma once

#include "shared.h"
#include <syscalls.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    pid_t owner;

    MemoryBufferID buffer;

    uint64_t x, y;
    uint64_t width, height;

    bool visible;

    // This is transfered by pointer copy, and before transfer the current's damage_rects must be freed.
    ListedDirtyRect *damage_rects;
} SurfaceState;

typedef struct Surface {
    SurfaceState current;
    SurfaceState pending;

    struct Surface *next;
    struct Surface *prev;
} Surface;
