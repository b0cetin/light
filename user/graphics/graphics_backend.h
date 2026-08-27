
#pragma once

#include <stdint.h>

typedef uint64_t SurfaceID;
typedef uint64_t BufferID;

typedef SurfaceID (*GfxCreateSurface)(uint64_t width, uint64_t height);
// Create buffer?
typedef void (*GfxAttachBuffer) (SurfaceID surface, BufferID buffer, uint64_t pitch);
typedef void (*GfxCommit) (SurfaceID surface);
typedef void (*GfxDestroySurface) (SurfaceID surface);

typedef struct {
    GfxCreateSurface create_surface;
    GfxAttachBuffer attach_buffer;
    GfxCommit commit;
    GfxDestroySurface destroy_surface;
} GraphicsBackend;
