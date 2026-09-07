
#pragma once

#include "syscalls.h"
#include "shared.h"

typedef struct {
    pid_t target;
    void *base;
    size_t size;
} MemoryPool;
typedef uint32_t MemoryPoolID;

typedef struct {
    MemoryPoolID pool;

    uint64_t offset;
    size_t size;

    uint64_t stride;
    uint64_t height;
    PixelFormat format;
} MemoryBuffer;
typedef uint32_t MemoryBufferID;

MemoryPool *get_memory_pool(MemoryPoolID id);
MemoryBuffer *get_memory_buffer(MemoryBufferID id);
