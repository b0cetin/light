
#include "fb_graphics.h"
#include "debugging.h"
#include "font8x8_basic.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

uint32_t *fb;
uint64_t fb_size;
uint32_t width;
uint32_t height;
uint32_t stride;

bool initialized;


uint32_t fb_width() {
    return width;
}
uint32_t fb_height() {
    return height;
}

void fb_init(BootInfo *bootInfo) {
    if (bootInfo->PhysicalFramebufferBase == 0) {
        kernel_println("FBG: There is no framebuffer available.");
        return;
    }

    fb = p2v(bootInfo->PhysicalFramebufferBase);
    fb_size = bootInfo->FramebufferSize;
    width = bootInfo->HorizontalResolution;
    height = bootInfo->VerticalResolution;
    stride = bootInfo->PixelsPerScanLine;

    kernel_println("FBG: Framebuffer location: %lx", fb);
    kernel_println("FBG: Framebuffer size: %ld", fb_size);
    kernel_println("FBG: Framebuffer dimensions: %dx%d (stride: %d)", width, height, stride);

    kernel_println("FBG: Mapping framebuffer memory as MMIO...");
    
    vmm_kmap_mmio(bootInfo->PhysicalFramebufferBase, fb_size);

    initialized = true;

    kernel_println("FBG: Framebuffer initialized.");
}

void fb_clear(Color color) {
    if (!initialized)
        return;

    for (uint64_t i = 0; i < (width * height); i++) {
        fb[i] = color;
    }
}

inline void fb_draw_pixel(uint64_t x, uint64_t y, Color color) {
    if (!initialized)
        return;

    if (x >= width || y >= height)
        return;

    fb[y * stride + x] = color;
}

void fb_draw_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, Color color) {
    if (!initialized || w == 0 || h == 0)
        return;

    uint64_t x_end = x + w;
    uint64_t y_end = y + h;

    if (x >= width || y >= height)
        return;

    if (x_end > width)
        x_end = width;
    if (y_end > height)
        y_end = height;

    for (uint64_t yy = y; yy < y_end; yy++) {
        Color *row = fb + yy * stride + x;

        for (uint64_t xx = x; xx < x_end; xx++) {
            *row++ = color;
        }
    }
}

// WARN: Horribly unoptimized.
void fb_draw_text(uint64_t x, uint64_t y, const char *text, Color color) {
    if (!initialized)
        return;

    uint64_t x_offset = 0;

    while (*text) {
        const uint8_t *bitmap = (uint8_t *)font8x8_basic[(uint8_t)*text++];

        for (uint32_t wy = 0; wy < 8; wy++) {
            uint8_t row = bitmap[wy];

            for (uint32_t wx = 0; wx < 8; wx++) {
                if (row & (1 << wx)) {
                    uint64_t px = x + x_offset + wx * 2;
                    uint64_t py = y + wy * 2;

                    if (px < width && py < height) {
                        fb[py * stride + px] = color;
                        fb[py * stride + px + 1] = color;
                        fb[(py + 1) * stride + px] = color;
                        fb[(py + 1) * stride + px + 1] = color;
                    }
                }
            }
        }

        x_offset += 16;
    }
}
