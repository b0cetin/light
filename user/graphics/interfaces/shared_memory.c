
#include "shared.h"
#include "shared.h"
#include "interface.h"
#include "interfaces/interfaces.h"
#include "syscalls.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

MemoryPool pools[UINT8_MAX];
uint32_t next_pool_id = 0; // FIXME: We desperately need a hashmap and allocator in userspace.

MemoryBuffer buffers[UINT8_MAX];
uint32_t next_buffer_id = 0; // FIXME: Same here.

static int64_t create_pool(pid_t caller, SharedMemoryID smem_id) {
    if (next_pool_id >= UINT8_MAX) {
        log_error("create_pool: out of pool space");
        exit(-1);
    }

    void *address;
    int64_t map_result = sys_memory_share_map(smem_id, &address, MMAP_ACCESS_READ | MMAP_ACCESS_WRITE);
    if (map_result < 0) return -1;
    size_t size = map_result;

    uint32_t id = next_pool_id++;

    pools[id].exists = true;
    pools[id].target = caller;
    pools[id].base = address;
    pools[id].size = size;

    log("Created memory pool %u for %lu with size %lu!", id, caller, size);

    return id;
}

static int64_t destroy_pool(pid_t caller, uint32_t object) {
    if (object >= UINT8_MAX) return -1;
    if (!pools[object].exists || pools[object].target != caller) return -1;

    if (sys_memory_unmap(pools[object].base, pools[object].size, 0) != SYS_SUCCESS)
    {
        log_error("destroy_pool: cannot unmap memory pool %u");
        exit(-1);
    }

    log("Destroyed memory pool %u!", object);

    return 0;
}

static int64_t create_buffer(pid_t caller, uint32_t object, uint64_t offset, uint64_t stride, uint64_t height, PixelFormat format) {
    if (object >= UINT8_MAX) return -1;
    if (!pools[object].exists || pools[object].target != caller) return -1;

    MemoryPool *pool = &pools[object];

    size_t size = stride;
    uint64_t end = 0;
    if (__builtin_mul_overflow(size, height, &size) || __builtin_mul_overflow(size, get_pixel_format_size_per_pixel(format), &size)) return -1;
    if (__builtin_add_overflow(offset, size, &end)) return -1;
    if (end > pool->size) return -1;

    if (next_buffer_id >= UINT8_MAX) {
        log_error("create_buffer: out of buffer space");
        exit(-1);
    }

    uint32_t id = next_buffer_id++;
    MemoryBuffer *buffer = &buffers[id];

    buffer->pool = object;
    buffer->exists = true;
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
    interface_record.interface_id = INTERFACE_ID_SHARED_MEMORY;
    interface_record.name = name;
    interface_record.call = call;
    return &interface_record;
}
