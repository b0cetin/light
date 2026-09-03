
#include "allocator.h"
#include "context_switching.h"
#include "debugging.h"
#include "defined_syscalls.h"
#include "pmm.h"
#include "processes.h"
#include "shared_memory.h"
#include "syscalls/utils.h"
#include "types.h"
#include "vas.h"
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
    if (__builtin_add_overflow(physical_page_base, size, &end) || __builtin_add_overflow(target_virt, size, &end)) // A possible attack target.
        return SYS_ERR_MAP_MMIO_INVALID_RANGE;

    Process *process = ctx_switching_get_active_thread()->owner;
    if (process->is_ring_0) return -1; // Kernel-space processes don't feature their own address space.

    if (target_virt == 0) {
        target_virt = vas_get_unused_space(&process->user_vas, page_count);

        if (target_virt == 0)
            return SYS_ERR_MAP_MMIO_CANNOT_FIND_SPACE;
    }
    else {
        if (!is_valid_mappable_user_range(target_virt, size))
            return SYS_ERR_MAP_MMIO_INVALID_RANGE;

        VASRegion *conflicting_region = null;
        if (!vas_get_memory_region(&process->user_vas, target_virt, target_virt + size, &conflicting_region) ||
                conflicting_region != null)
            return SYS_ERR_MAP_MMIO_USED;
    }

    VirtualMemoryObject *mmio = vmem_create_mmio(physical_page_base, page_count);
    vas_add_region(&process->user_vas, target_virt, VMEM_PERM_RW, VREGION_REASON_USER_REQUEST, mmio);

    *virtual_address = target_virt;
    return SYS_SUCCESS;
}

int64_t sys_memory_map(void **address, size_t length, Sys_MemoryAccessFlags access) {
    if (!is_valid_mapped_user_range((uintptr_t) address, sizeof(uintptr_t)))
        return -1;

    uintptr_t target_virt = (uintptr_t) *address;

    if (target_virt > 0 && target_virt <= 1024 * 1024) return SYS_ERR_MMAP_INVALID_RANGE; // First MiB is not allowed.

    if (length & 0xFFF || target_virt & 0xFFF)
        return SYS_ERR_MMAP_ARG_UNALIGNED;

    uint64_t page_count = length / PAGE_SIZE;

    if (page_count == 0)
        return SYS_ERR_MMAP_INVALID_RANGE;
    
    uint64_t end;
    if (__builtin_add_overflow(target_virt, length, &end)) // A possible attack target.
        return SYS_ERR_MMAP_INVALID_RANGE;
    
    if ((access & (MMAP_ACCESS_READ | MMAP_ACCESS_WRITE | MMAP_ACCESS_EXEC)) != access)
        return SYS_ERR_MMAP_INVALID_ACCESS;

    Process *process = ctx_switching_get_active_thread()->owner;
    if (process->is_ring_0) return -1; // Kernel-space processes don't feature their own address space.

    if (target_virt == 0) {
        target_virt = vas_get_unused_space(&process->user_vas, page_count);

        if (target_virt == 0)
            return SYS_ERR_MMAP_CANNOT_FIND_SPACE;
    }
    else {
        if (!is_valid_mappable_user_range(target_virt, length))
            return SYS_ERR_MMAP_INVALID_RANGE;

        VASRegion *conflicting_region = null;
        if (!vas_get_memory_region(&process->user_vas, target_virt, target_virt + length, &conflicting_region) ||
                conflicting_region != null)
            return SYS_ERR_MMAP_USED;
    }

    VirtualMemoryObject *object = vmem_create_allocation(page_count, true);
    VASRegionPermission permission = VMEM_PERM_NONE;

    if (object == null)
        return SYS_ERR_MMAP_OUT_OF_MEMORY;

    if (access & MMAP_ACCESS_READ) permission |= VMEM_PERM_READ;
    if (access & MMAP_ACCESS_WRITE) permission |= VMEM_PERM_WRITE;
    if (access & MMAP_ACCESS_EXEC) permission |= VMEM_PERM_EXEC;

    vas_add_region(&process->user_vas, target_virt, permission, VREGION_REASON_USER_REQUEST, object);

    *address = (void*) target_virt;
    return SYS_SUCCESS;
}

int64_t sys_memory_share(void *address, size_t length, Sys_SharedMemoryID *out_id) {
    PANIC("sys_memory_share not implemented.");
    // if (!is_valid_mapped_user_range((uintptr_t) out_id, sizeof(SharedMemoryID*)))
    //     return -1;

    // if (length & 0xFFF || ((uintptr_t) address) & 0xFFF)
    //     return SYS_ERR_MSHARE_ARG_UNALIGNED;

    // if (!is_valid_mapped_user_range((uintptr_t) address, length)) // FIXME: The process should only be able to map what's been mapped with sys_memory_map.
    //     return SYS_ERR_MSHARE_UNMAPPED;

    // uint64_t end;
    // if (__builtin_add_overflow((uintptr_t) address, length, &end)) // A possible attack target.
    //     return SYS_ERR_MMAP_INVALID_RANGE;
    
    // uint64_t page_count = length / PAGE_SIZE;

    // if (page_count == 0)
    //     return SYS_ERR_MMAP_INVALID_RANGE;

    // Process *process = ctx_switching_get_active_thread()->owner;
    // if (process->is_ring_0) return -1;

    // SharedMemoryID id = smem_create(process->user_cr3, (uintptr_t) address, page_count);
    // if (id == SMEM_NULL_ID) {
    //     kprintln("smem_create returned SMEM_NULL_ID!");
    //     return -1;
    // }

    // *out_id = id;
    // return SYS_SUCCESS;
}

int64_t sys_memory_share_map(Sys_SharedMemoryID id, void **address) {
    PANIC("sys_memory_share_map not implemented.");
}
