
#include "hashtable.h"
#include "shared_memory.h"
#include "interface.h"
#include "interfaces/interfaces.h"
#include "syscalls.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void log(const char * restrict format, ...) {
    char buffer[205] = { 'S', 'H', 'M', ':', ' ' };

    va_list args;
    va_start(args, format);
    vsnprintf(&buffer[sizeof(buffer) - 200], sizeof(buffer), format, args);
    va_end(args);

    sys_print(buffer);
}

static void log_error(const char * restrict format, ...) {
    char buffer[210] = { 'S', 'H', 'M', ':', ' ', 'E', 'R', 'R', ':', ' ' };

    va_list args;
    va_start(args, format);
    vsnprintf(&buffer[sizeof(buffer) - 200], sizeof(buffer), format, args);
    va_end(args);

    sys_print(buffer);
}

extern void exit(int64_t status);

static ServerInterface interface_record;
static char* name = "shared_memory";

Hashtable *pools;
static uint32_t next_pool_id = 0;

Hashtable *buffers;
static uint32_t next_buffer_id = 0;

static bool is_initialized = false;

MemoryPool *get_memory_pool(MemoryPoolID id) {
    MemoryPool *pool = NULL;
    ht_lookup(pools, id, (void**) &pool);
    return pool;
}
MemoryBuffer *get_memory_buffer(MemoryBufferID id) {
    MemoryBuffer *buffer = NULL;
    ht_lookup(buffers, id, (void**) &buffer);
    return buffer;
}

static int64_t create_pool(pid_t caller, SharedMemoryID smem_id) {
    if (next_pool_id >= UINT32_MAX) {
        log_error("create_pool: out of object ids");
        return -1;
    }

    void *address;
    int64_t map_result = sys_memory_share_map(smem_id, &address, MMAP_ACCESS_READ | MMAP_ACCESS_WRITE);
    if (map_result < 0) return -1;
    size_t size = map_result;

    uint32_t id = next_pool_id++;

    MemoryPool *pool = malloc(sizeof(MemoryPool));
    if (pool == NULL) {
        log_error("create_pool: could not allocate memory pool struct");
        return -1;
    }
    if (!ht_add(pools, next_pool_id, pool)) {
        log_error("create_pool: could not add object id %u to hashtable", next_pool_id);
        return -1;
    }

    pool->target = caller;
    pool->base = address;
    pool->size = size;

    log("Created memory pool %u for %lu with size %lu!", id, caller, size);

    return id;
}

static int64_t destroy_pool(pid_t caller, uint32_t object) {
    MemoryPool *pool = get_memory_pool(object);
    if (pool == NULL || pool->target != caller) return -1;

    if (sys_memory_unmap(pool->base, pool->size, 0) != SYS_SUCCESS)
    {
        log_error("destroy_pool: cannot unmap memory pool %u");
        exit(-1); // This is not an operation that can be ignored.
    }

    log("Destroyed memory pool %u!", object);
    return 0;
}

static int64_t create_buffer(pid_t caller, uint32_t object, uint64_t offset, uint64_t stride, uint64_t height, PixelFormat format) {
    MemoryPool *pool = get_memory_pool(object);
    if (pool == NULL || pool->target != caller) return -1;

    size_t size = stride;
    uint64_t end = 0;
    if (__builtin_mul_overflow(size, height, &size) || __builtin_mul_overflow(size, get_pixel_format_size_per_pixel(format), &size)) return -1;
    if (__builtin_add_overflow(offset, size, &end)) return -1;
    if (end > pool->size) return -1;

    if (next_buffer_id >= UINT32_MAX) {
        log_error("create_buffer: out of object ids");
        exit(-1);
    }

    uint32_t id = next_buffer_id++;

    MemoryBuffer *buffer = malloc(sizeof(MemoryBuffer));
    if (buffer == NULL) {
        log_error("create_buffer: could not allocate memory pool struct");
        return -1;
    }
    if (!ht_add(buffers, id, buffer)) {
        log_error("create_buffer: could not add object id %u to hashtable", next_pool_id);
        return -1;
    }

    buffer->pool = object;
    buffer->offset = offset;
    buffer->size = size;
    buffer->stride = stride;
    buffer->height = height;
    buffer->format = format;

    return id;
}

static bool call (pid_t caller, uint16_t call, uint32_t object, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t *out) {
    switch (call) {
    case 0:
        *out = create_pool(caller, arg0);
        return true;
    case 1:
        *out = destroy_pool(caller, object);
        return true;
    case 2:
        *out = create_buffer(caller, object, arg0, arg1, arg2, arg3);
        return true;
    default:
        return false;
    }
}

ServerInterface *interfaces_shared_memory() {
    if (!is_initialized) {
        buffers = ht_create();
        pools = ht_create();
        is_initialized = true;
    }

    interface_record.interface_id = INTERFACE_ID_SHARED_MEMORY;
    interface_record.name = name;
    interface_record.handler = sys_get_pid();
    interface_record.call = call;
    return &interface_record;
}
