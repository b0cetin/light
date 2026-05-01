
#pragma once

#include "bootinfo.h"
#include <stdint.h>

#define COLOR_WHITE 0xFFFFFF
#define COLOR_BLACK 0x000000
#define COLOR_RED 0xFF0000
#define COLOR_GREEN 0x00FF00
#define COLOR_BLUE 0x0000FF

typedef uint32_t Color;

void fb_init(BootInfo *bootInfo);

void fb_clear(Color color);

void fb_draw_pixel(uint64_t x, uint64_t y, Color color);
void fb_draw_rect(uint64_t x, uint64_t y, uint64_t w, uint64_t h, Color color);

void fb_draw_text(uint64_t x, uint64_t y, const char *text, Color color);

static inline Color rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}
