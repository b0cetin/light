
#include "processor_info.h"
#include <stdint.h>

void read_cpu_vendor_id(char result[13]) {
    uint32_t *res = (uint32_t *)result;
    cpuid(CPUID_GETVENDORSTRING, res, res + 1, res + 2, res + 3);
    result[12] = '\0';
}
