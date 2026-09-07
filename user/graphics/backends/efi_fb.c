
#include "efi_fb.h"
#include "graphics_backend.h"
#include "efi_framebuffer.h"
#include "shared.h"
#include "stdio.h"
#include "syscalls.h"
#include <stdint.h>

static void log(const char * restrict format, ...) {
    char buffer[208] = { 'E', 'F', 'I', '-', 'F', 'B', ':', ' ' };

    va_list args;
    va_start(args, format);
    vsnprintf(&buffer[sizeof(buffer) - 200], sizeof(buffer), format, args);
    va_end(args);

    sys_print(buffer);
}

static void log_error(const char * restrict format, ...) {
    char buffer[213] = { 'E', 'F', 'I', '-', 'F', 'B', ':', ' ', 'E', 'R', 'R', ':', ' ' };

    va_list args;
    va_start(args, format);
    vsnprintf(&buffer[sizeof(buffer) - 200], sizeof(buffer), format, args);
    va_end(args);

    sys_print(buffer);
}

static uint32_t *buffer;

static bool init() {
    buffer = NULL;

    int64_t map_result = sys_map_mmio(efi_fb_phys_base, efi_fb_size, (uintptr_t *)&buffer);
    if (map_result != SYS_SUCCESS) {
        log_error("MMIO mapping of EFI framebuffer failed: %li", map_result);
        return false;
    }

    log("Mapped EFI framebuffer to %lx.", (uint64_t) buffer);
    return true;
}

static void destroy() {
    int64_t unmap_result = sys_memory_unmap(buffer, efi_fb_size, 0);

    if (unmap_result != SYS_SUCCESS) log_error("Unmapping of EFI framebuffer failed: %li", unmap_result);
    else log("Unmapped EFI framebuffer.");
}

static bool poll_properties(DisplayProperties *out_properties) {
    DisplayProperties properties;

    properties.width = efi_fb_width;
    properties.height = efi_fb_height;
    properties.pixel_format = PIXEL_FORMAT_BGRX8888; // FIXME: Get this from bootloader
    properties.refresh_rate = REFRESH_RATE_UNKNOWN;

    if (out_properties != NULL) *out_properties = properties;
    return true;
}

static bool present_cpu_buffer (void *buffer, size_t length, uint64_t stride, DirtyRect dirty_rect) {
    return false;
}

DisplayBackend efi_fb_get() {
    DisplayBackend backend;

    backend.init = init;
    backend.destroy = destroy;
    backend.poll_properties = poll_properties;
    backend.present_cpu_buffer = present_cpu_buffer;

    return backend;
}
