
#include "context_switching.h"
#include "debugging.h"
#include "defined_syscalls.h"
#include "handles.h"
#include "kobjects.h"
#include "pmm.h"
#include "processes.h"
#include "syscalls/utils.h"
#include "types.h"
#include "vas.h"
#include "vmm.h"
#include <stddef.h>
#include <stdint.h>

static VASRegionPermission access_flags_to_permissions(Sys_MemoryAccessFlags access) {
    VASRegionPermission permission = VMEM_PERM_NONE;

    if (access & MMAP_ACCESS_READ) permission |= VMEM_PERM_READ;
    if (access & MMAP_ACCESS_WRITE) permission |= VMEM_PERM_WRITE;
    if (access & MMAP_ACCESS_EXEC) permission |= VMEM_PERM_EXEC;

    return permission;
}

// FIXME: This requires actual regulation (such as capabilities or driver manifest), but this works for now.
int64_t sys_map_mmio(uint64_t physical_page_base, uint64_t size, uintptr_t *virtual_address) {
    if (!is_valid_mapped_user_range((uintptr_t) virtual_address, sizeof(uintptr_t)))
        return -1;

    uintptr_t target_virt = *virtual_address;
    uint64_t page_count = size / PAGE_SIZE;

    // checks
    {
        if (target_virt > 0 && target_virt <= 1024 * 1024) return -1; // First MiB is not allowed.

        if (physical_page_base & 0xFFF || size & 0xFFF || target_virt & 0xFFF)
            return SYS_ERR_ARGUMENT_UNALIGNED;

        if (page_count == 0)
            return SYS_ERR_INVALID_RANGE;

        if (physical_page_base <= pmm_get_highest_phys_addr()) // Tries to map to conventional memory.
            return SYS_ERR_INVALID_RANGE;
        
        uint64_t end;
        if (__builtin_add_overflow(physical_page_base, size, &end) || __builtin_add_overflow(target_virt, size, &end)) // A possible attack target.
            return SYS_ERR_INVALID_RANGE;
    }

    Process *process = ctx_switching_get_active_thread()->owner;
    if (process->is_ring_0) return -1; // Kernel-space processes don't feature their own address space.

    if (target_virt == 0) {
        target_virt = vas_get_unused_space(&process->user_vas, page_count);

        if (target_virt == 0)
            return SYS_ERR_CANNOT_FIND_SPACE;
    }
    else {
        if (!is_valid_mappable_user_range(target_virt, size))
            return SYS_ERR_INVALID_RANGE;

        VASRegion *conflicting_region = null;
        if (!vas_get_memory_region(&process->user_vas, target_virt, target_virt + size, &conflicting_region) ||
                conflicting_region != null)
            return SYS_ERR_ADDRESS_RANGE_CLASH;
    }

    VirtualMemoryObject *mmio = vmem_create_mmio(physical_page_base, page_count);
    vas_add_region(&process->user_vas, target_virt, VMEM_PERM_RW, VREGION_REASON_USER_REQUEST, mmio);

    *virtual_address = target_virt;
    return SYS_SUCCESS;
}

static int64_t get_address(VAS *vas, uintptr_t user_provided, size_t length, uintptr_t *out_address) {
    if (length & 0xFFF) return SYS_ERR_ARGUMENT_UNALIGNED;
    if (length == 0) return SYS_ERR_INVALID_RANGE;
    uint64_t page_count = length / PAGE_SIZE;

    if (user_provided == 0) {
        uintptr_t address = vas_get_unused_space(vas, page_count);

        if (address == 0)
            return SYS_ERR_CANNOT_FIND_SPACE;

        if (out_address != null) *out_address = address;
        return SYS_SUCCESS;
    }
    else {
        if (user_provided > 0 && user_provided <= 1024 * 1024) return SYS_ERR_INVALID_RANGE; // First MiB is not allowed.
        
        if (user_provided & 0xFFF)
            return SYS_ERR_ARGUMENT_UNALIGNED;

        uint64_t end;
        if (__builtin_add_overflow(user_provided, length, &end)) // A possible attack target.
            return SYS_ERR_INVALID_RANGE;

        if (end >= HHDM_OFFSET) return SYS_ERR_INVALID_RANGE;

        VASRegion *conflicting_region = null;
        if (!vas_get_memory_region(vas, user_provided, end, &conflicting_region) ||
                conflicting_region != null)
            return SYS_ERR_ADDRESS_RANGE_CLASH;
        
        *out_address = user_provided;
        return SYS_SUCCESS;
    }
}

int64_t sys_memory_map(void **address, size_t length, Sys_MemoryAccessFlags access) {
    if (!is_valid_mapped_user_range((uintptr_t) address, sizeof(void*)))
        return SYS_ERR_ARGUMENT_POINTER_INVALID;

    uintptr_t user_provided = (uintptr_t) *address;

    Process *process = ctx_switching_get_active_thread()->owner;
    if (process->is_ring_0) PANIC("sys_memory_map called from a kernel-space process!");
    VAS *vas = &process->user_vas;

    uintptr_t target_address = 0;
    uint64_t status = get_address(vas, user_provided, length, &target_address);
    if (status != SYS_SUCCESS) return status;

    uint64_t page_count = length / PAGE_SIZE;

    VirtualMemoryObject *object = vmem_create_allocation(page_count, true);
    if (object == null) return SYS_ERR_OUT_OF_MEMORY;
    
    assert(vas_add_region(vas, target_address, access_flags_to_permissions(access), VREGION_REASON_USER_REQUEST, object));

    return SYS_SUCCESS;
}

int64_t sys_memory_unmap(void *address, size_t length, Sys_MemoryUnmapFlags flags) {
    uint64_t virt_start = (uint64_t) address;
    uint64_t virt_end = 0;

    if (length <= 0)
        return SYS_ERR_INVALID_RANGE;

    if (__builtin_add_overflow(virt_start, length, &virt_end))
        return SYS_ERR_INVALID_RANGE;

    if (flags & ~(MUNMAP_FLAGS_INCLUSIVE))
        return SYS_ERR_ARGUMENT_INVALID;

    Process *process = ctx_switching_get_active_thread()->owner;
    VAS *vas = &process->user_vas;

    if (process->is_ring_0) return -1;

    static const uint64_t REGIONS_ARRAY_LIMIT = 32;
    VASRegion *regions_to_remove[REGIONS_ARRAY_LIMIT];
    uint64_t regions_array_i = 0;

    uint64_t range_start = virt_start & PT_ADDRESS_MASK;
    uint64_t range_end = 0; // Combat overflow
    if (__builtin_add_overflow(virt_end, PAGE_SIZE - 1, &range_end))
        range_end = UINT64_MAX;
    else range_end = range_end & PT_ADDRESS_MASK;
    
    VASRegion *current = vas->regions;
    while (current != null) {
        if (current->reason != VREGION_REASON_USER_REQUEST) {
            current = current->next;
            continue;
        }

        bool is_start_inside = range_start <= current->virt_start && current->virt_start <= range_end;
        bool is_end_inside = range_start <= current->virt_end && current->virt_end <= range_end;

        bool is_intersecting_but_not_inside = is_start_inside != is_end_inside;
        bool is_inside = is_start_inside && is_end_inside;

        if (is_inside || (is_intersecting_but_not_inside && flags & MUNMAP_FLAGS_INCLUSIVE)) {
            if (regions_array_i >= REGIONS_ARRAY_LIMIT)
                PANIC("sys_memory_unmap: reached regions to remove array limit.");

            regions_to_remove[regions_array_i] = current;
            regions_array_i++;
        }

        current = current->next;
    }

    bool any_region_found = regions_array_i > 0;

    for (uint64_t i = 0; i < regions_array_i; i++) {
        vas_remove_region(vas, regions_to_remove[i]);
    }

    return any_region_found ? 0 : SYS_MUNMAP_SUCCESS_NOOP;
}

int64_t sys_memory_share_create(void **address, size_t length, Sys_MemoryAccessFlags access, sys_handle_t *out_handle) {
    if (!is_valid_mapped_user_range((uintptr_t) address, sizeof(void**)))
        return SYS_ERR_ARGUMENT_POINTER_INVALID;

    uintptr_t user_provided = (uintptr_t) *address;

    if (out_handle != NULL && !is_valid_mapped_user_range((uintptr_t) out_handle, sizeof(sys_handle_t*)))
        return SYS_ERR_ARGUMENT_POINTER_INVALID;

    Process *process = ctx_switching_get_active_thread()->owner;
    assert_msg(!process->is_ring_0, "sys_memory_share_create called from a kernel-space process!");
    VAS *vas = &process->user_vas;
    HandleTable *table = &process->handle_table;

    uintptr_t target_address = 0;
    uint64_t status = get_address(vas, user_provided, length, &target_address);
    if (status != SYS_SUCCESS) return status;

    uint64_t page_count = length / PAGE_SIZE;

    KernelObjectID kobject = vmem_create_shared(page_count);
    if (kobject == NULL_KOBJECT) return SYS_ERR_OUT_OF_MEMORY;
    KernelObjectEntry *entry = null;
    assert(kobject_resolve(kobject, &entry));
    assert(entry->type == KOBJECT_SHAREDMEMORY);

    HandleID handle = NULL_HANDLE;
    if (!handle_add(table, kobject, &handle)) {
        vas_destroy_shared_memory(kobject);
        return SYS_ERR_OUT_OF_MEMORY;
    }

    assert(vas_add_region(vas, target_address, access_flags_to_permissions(access), VREGION_REASON_USER_REQUEST, &entry->object.shared_memory));

    if (out_handle != NULL) *out_handle = handle;

    return SYS_SUCCESS;
}

int64_t sys_memory_share_map(sys_handle_t handle, void **address, Sys_MemoryAccessFlags access) {
    if (!is_valid_mapped_user_range((uintptr_t) address, sizeof(void**)))
        return SYS_ERR_ARGUMENT_POINTER_INVALID;

    uintptr_t user_provided = (uintptr_t) *address;

    Process *process = ctx_switching_get_active_thread()->owner;
    VAS *vas = &ctx_switching_get_active_thread()->owner->user_vas;
    assert_msg(!process->is_ring_0, "sys_memory_share_map called from a kernel-space process!");
    HandleTable *table = &process->handle_table;

    KernelObjectID kid = NULL_KOBJECT;
    if (!handle_resolve(table, handle, &kid))
        return SYS_ERR_HANDLE_INVALID;

    KernelObjectEntry *entry = null;
    if (!kobject_resolve(kid, &entry) || entry->type != KOBJECT_SHAREDMEMORY)
        return SYS_ERR_HANDLE_INVALID;

    VirtualMemoryObject *object = &entry->object.shared_memory;
    size_t page_count = vmem_get_page_count(object);
    size_t length = page_count * PAGE_SIZE;

    uintptr_t target_address = 0;
    uint64_t status = get_address(vas, user_provided, length, &target_address);
    if (status != SYS_SUCCESS) return status;

    VASRegionPermission permissions = access_flags_to_permissions(access);
    vas_add_region(vas, target_address, permissions, VREGION_REASON_USER_REQUEST, object);

    return page_count * PAGE_SIZE;
}

int64_t sys_memory_share_remove(sys_handle_t handle) {
    if (handle == NULL_HANDLE) return SYS_ERR_HANDLE_INVALID;

    HandleTable *table = &ctx_switching_get_active_thread()->owner->handle_table;
    
    KernelObjectID kid = NULL_KOBJECT;
    if (!handle_resolve(table, handle, &kid) || kid == NULL_KOBJECT) return SYS_ERR_HANDLE_INVALID;
    KernelObjectEntry *entry = null;
    if (!kobject_resolve(kid, &entry) || entry == null) return SYS_ERR_HANDLE_INVALID;

    if (entry->type != KOBJECT_SHAREDMEMORY) return SYS_ERR_HANDLE_INVALID;
    if (!handle_remove(table, handle)) return SYS_ERR_HANDLE_INVALID;

    return SYS_SUCCESS;
}
