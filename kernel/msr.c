
#include "msr.h"
#include "debugging.h"
#include "processor_info.h"
#include <stdint.h>

bool is_msr_checked = false;

void msr_ensure_support()
{
    if (is_msr_checked)
        return;

    if (!cpuid_check_msr())
        PANIC("CPU does not support MSR.");

    is_msr_checked = true;
}

void msr_get(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    msr_ensure_support();
   __asm__ volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void msr_set(uint32_t msr, uint32_t lo, uint32_t hi)
{
    msr_ensure_support();
   __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}
