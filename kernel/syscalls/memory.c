
#include "context_switching.h"
#include "defined_syscalls.h"
#include "pmm.h"
#include "processes.h"
#include "syscalls/utils.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

#define VAS_SEARCH_START 0x40000000
#define VAS_SEARCH_END 0x0000900000000000ULL

// FIXME: This requires actual regulation (such as capabilities or driver manifest), but this works for now.
int64_t sys_map_mmio(uint64_t physical_page_base, uint64_t size, uintptr_t *virtual_address) {
    if (!is_valid_mapped_user_range((uintptr_t) virtual_address, sizeof(uintptr_t)))
        return -1;

    uintptr_t target_virt = *virtual_address;

    if (target_virt > 0 && target_virt <= 1024 * 1024) return -1; // First MiB is not allowed.

    if (physical_page_base & 0xFFF || size & 0xFFF || target_virt & 0xFFF)
        return SYS_ERR_MAP_MMIO_ARG_UNALIGNED;

    uint64_t page_count = size / PAGE_SIZE;

    if (page_count == 0)
        return SYS_ERR_MAP_MMIO_INVALID_RANGE;

    if (physical_page_base < pmm_get_highest_phys_addr()) // Tries to map to conventional memory.
        return SYS_ERR_MAP_MMIO_INVALID_RANGE;
    
    uint64_t end;
    if (__builtin_add_overflow(physical_page_base, size, &end)) // A possible attack target.
        return SYS_ERR_MAP_MMIO_INVALID_RANGE;

    Process *process = ctx_switching_get_active_thread()->owner;
    if (process->is_ring_0) return -1; // Kernel-space processes don't feature their own address space.

    if (target_virt == 0) {
        uint64_t found_pages = 0;

        uint64_t addr;
        for (addr = VAS_SEARCH_START; found_pages < page_count && addr < VAS_SEARCH_END; addr += PAGE_SIZE) {
            if (!vmm_get_page_info(process->user_cr3, addr, null, null))
            {
                found_pages++;

                if (found_pages == page_count) {
                    // Calculate the starting address of this contiguous block
                    target_virt = addr - (page_count - 1) * PAGE_SIZE;
                    break;
                }
            }
            else {
                found_pages = 0;
            }
        }

        if (found_pages != page_count)
            return SYS_ERR_MAP_MMIO_CANNOT_FIND_SPACE;
    }
    else {
        if (!is_valid_mappable_user_range(target_virt, size))
            return SYS_ERR_MAP_MMIO_INVALID_RANGE;

        for (uint64_t i = 0; i < page_count; i++) {
            uint64_t virt = target_virt + i * PAGE_SIZE;

            if (vmm_get_page_info(process->user_cr3, virt, null, null))
                return SYS_ERR_MAP_MMIO_USED;
        }
    }

    for (uint64_t i = 0; i < page_count; i++) {
        uint64_t virt = target_virt + i * PAGE_SIZE;
        uint64_t phys = physical_page_base + i * PAGE_SIZE;

        vmm_map(process->user_cr3, virt, phys, PT_USER | PT_PWT | PT_PCD | PT_NX | PT_RW);
    }

    *virtual_address = target_virt;

    return SYS_SUCCESS;
}
