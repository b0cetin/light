
#include "linker_symbols.h"

extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

uint64_t get_kernel_start() {
    return (uint64_t)&_kernel_start;
}

uint64_t get_kernel_end() {
    return (uint64_t)&_kernel_end;
}
