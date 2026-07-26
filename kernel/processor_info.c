
#include "processor_info.h"
#include <stdint.h>

void cpuid_read_vendor(char result[13]) {
    uint32_t unused;
    uint32_t *res = (uint32_t *)result;
    cpuid(CPUID_GETVENDORSTRING, &unused, res, res + 2, res + 1);
    result[13] = '\0';
}

bool cpuid_check_apic() {
    uint32_t eax, edx, unused;
    cpuid(CPUID_FEATURES, &eax, &unused, &unused, &edx);
    return edx & CPUID_FEAT_APIC;
}

bool cpuid_check_msr() {
    uint32_t edx, unused;
    cpuid(CPUID_FEATURES, &unused, &unused, &unused, &edx);
    return edx & CPUID_FEAT_MSR;
}
