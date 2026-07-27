
#pragma once

#include <stddef.h>
#include <stdint.h>

void alloc_init();
void *kmalloc(size_t size);
void kfree(void *ptr);
