
#include "defined_syscalls.h"
#include "../debugging.h"

int64_t sys_interrupt_control(IRQCTLRequest request, uint64_t vector) {
    PANIC("sys_interrupt_control not implemented.");
}
