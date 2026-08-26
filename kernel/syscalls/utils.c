
#include "utils.h"
#include "../types.h"
#include "../vmm.h"
#include "../context_switching.h"
#include <stddef.h>
#include <stdint.h>

bool is_valid_mapped_user_range(uintptr_t ptr, size_t size) { // TODO: Check for the actual range
    if (!is_valid_mappable_user_range(ptr, size)) return false;

    uint64_t flags, phys;
    
    if (!vmm_get_page_info(ctx_switching_get_active_thread()->owner->user_cr3, ptr, &phys, &flags))
        return false;

    uint64_t mask = PT_PRESENT | PT_USER;// | PT_RW | PT_NX;
    return (flags & mask) == mask; 
}

bool is_valid_mappable_user_range(uintptr_t ptr, size_t size) {
    if (ptr <= 1024 * 1024) return false; // First MiB

    uintptr_t end;
    if (__builtin_add_overflow(ptr, size, &end)) {
        return false;
    }

    if (end >= HHDM_OFFSET) return false;

    return true;
}
