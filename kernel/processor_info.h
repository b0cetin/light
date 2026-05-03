
#pragma once
#include <stdint.h>

typedef enum {
    CPUID_GETVENDORSTRING = 0,
} CPUIDRequests;

static inline void cpuid(CPUIDRequests function, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(function));
}

void read_cpu_vendor_id(char result[13]);
