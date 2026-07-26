
#pragma once
#include "types.h"
#include <stdint.h>

typedef enum {
    CPUID_GETVENDORSTRING = 0,
    CPUID_FEATURES = 1,
} CPUIDRequests;

typedef enum {
    CPUID_FEAT_APIC = 1 << 9,
    CPUID_FEAT_MSR = 1 << 5,
} CPUIDFeatures;

static inline void cpuid(CPUIDRequests function, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(function));
}

void cpuid_read_vendor(char result[13]);
bool cpuid_check_apic();
bool cpuid_check_msr();
