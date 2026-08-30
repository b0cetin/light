
#pragma once

#include "debugging.h"
#include "processor_info.h"
#include <stdint.h>

#define MSR_EFER           0xC0000080
#define MSR_STAR           0xC0000081
#define MSR_LSTAR          0xC0000082
#define MSR_SFMASK         0xC0000084
#define MSR_GS_BASE        0xC0000101
#define MSR_KERNEL_GS_BASE 0xC0000102
#define MSR_IA32_APIC_BASE 0x1B

static inline void msr_ensure()
{
    if (!cpuid_check_msr())
        PANIC("CPU does not support MSR.");
}

static inline uint64_t msr_read(uint32_t msr)
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

static inline void msr_write(uint32_t msr, uint64_t value)
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
