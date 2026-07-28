
#pragma once

#include "debugging.h"
#include "processor_info.h"
#include <stdint.h>

static inline void msr_ensure()
{
    if (!cpuid_check_msr())
        PANIC("CPU does not support MSR.");
}

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t lo;
    uint32_t hi;

    asm volatile (
        "rdmsr"
        : "=a"(lo), "=d"(hi)
        : "c"(msr)
    );

    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t lo = (uint32_t)value;
    uint32_t hi = (uint32_t)(value >> 32);

    asm volatile (
        "wrmsr"
        :
        : "c"(msr), "a"(lo), "d"(hi)
        : "memory"
    );
}
