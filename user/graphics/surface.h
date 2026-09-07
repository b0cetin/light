
#pragma once

#include "shared.h"
#include "shared_memory.h"
#include <syscalls.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct ListedDirtyRect { DirtyRect dirty_rect; struct ListedDirtyRect *next; } ListedDirtyRect;

typedef struct {
    MemoryBufferID buffer;

    uint64_t x, y;
    uint64_t width, height;

    bool visible;

    // This is transfered by pointer copy, and before transfer the current's damage_rects must be freed.
    ListedDirtyRect *damage_rects;
} SurfaceState;

typedef struct Surface {
    pid_t owner;

    SurfaceState current;
    SurfaceState pending;

    struct Surface *next;
    struct Surface *prev;
} Surface;

typedef uint32_t SurfaceID;

Surface *get_surface(SurfaceID id);
