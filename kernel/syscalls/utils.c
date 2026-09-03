
#include "utils.h"
#include "../types.h"
#include "../vmm.h"
#include "../context_switching.h"
#include <stddef.h>
#include <stdint.h>

bool is_valid_mapped_user_range(uintptr_t ptr, size_t size) {
    if (!is_valid_mappable_user_range(ptr, size)) return false;

    uint64_t flags;

    uint64_t start_page = ptr & PT_ADDRESS_MASK;
    uint64_t end_page = (ptr + size) & PT_ADDRESS_MASK;
    size_t page_count = (end_page - start_page) / PAGE_SIZE + 1;
    
    for (size_t i = 0; i < page_count; i++) { // FIXME: Needs to use VAS
        if (!vmm_get_page_info(ctx_switching_get_active_thread()->owner->user_vas.cpu_address_table, start_page + i * PAGE_SIZE, null, &flags))
            return false;
    }

    uint64_t mask = PT_PRESENT | PT_USER;
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
