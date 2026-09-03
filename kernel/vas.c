
#include "vas.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "allocator.h"
#include "pmm.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

// The array supplied is copied over, and can be safely freed.
VirtualMemoryObject *vmem_create_slice(uintptr_t *_array, size_t size) {
    uintptr_t *array = kmalloc(sizeof(uintptr_t) * size);
    memcpy(array, _array, sizeof(uintptr_t) * size);

    VirtualMemoryObject *object = kmalloc(sizeof(VirtualMemoryObject));
    object->type = VM_OBJ_SLICE;
    object->slice.count = size;
    object->slice.pages = array;

    return object;
}
VirtualMemoryObject *vmem_create_slice_continuous(uintptr_t phys_start, size_t page_count) {
    uintptr_t *array = kmalloc(sizeof(uintptr_t) * page_count);
    for (size_t i = 0; i < page_count; i++)
        array[i] = phys_start + i * PAGE_SIZE;

    VirtualMemoryObject *object = kmalloc(sizeof(VirtualMemoryObject));
    object->type = VM_OBJ_SLICE;
    object->slice.count = page_count;
    object->slice.pages = array;

    return object;
}

// Returns null if there isn't enough memory available on the system.
VirtualMemoryObject *vmem_create_allocation(size_t page_count, bool free) {
    if (pmm_get_free_page_count() <= page_count) return null;

    uintptr_t *array = kmalloc(sizeof(uintptr_t) * page_count);
    for (size_t i = 0; i < page_count; i++) {
        array[i] = pmm_alloc_page();
        if (free) memzero(p2v(array[i]), PAGE_SIZE);
    }

    VirtualMemoryObject *object = kmalloc(sizeof(VirtualMemoryObject));
    object->type = VM_OBJ_ALLOCATION;
    object->allocation.count = page_count;
    object->allocation.pages = array;
    
    return object;
}

VirtualMemoryObject *vmem_create_mmio(uint64_t phys_start, size_t page_count) {
    VirtualMemoryObject *object = kmalloc(sizeof(VirtualMemoryObject));
    object->type = VM_OBJ_MMIO;
    object->mmio.page_count = page_count;
    object->mmio.phys_start = phys_start;
    return object;
}

uint64_t vmem_get_page_count(VirtualMemoryObject *object) {
    switch (object->type) {
    case VM_OBJ_SLICE:
        return object->slice.count;
    case VM_OBJ_ALLOCATION:
        return object->allocation.count;
    case VM_OBJ_MMIO:
        return object->mmio.page_count;
    default:
        PANIC("vmem_get_page_count: unrecognized object type: %d", object->type);
    }

    return 0;
}

// Writes to `out_region` if a successful match was found.
// Returns false if there was an invalid intersection (or multiple matches).
// Neither result affects the other. Handle independently.
bool vas_get_memory_region(VAS *vas, uint64_t virt_start, uint64_t virt_end, VASRegion **out_region) {
    VASRegion *current = vas->regions;

    uint64_t start = virt_start & PT_ADDRESS_MASK;
    uint64_t end = virt_end & PT_ADDRESS_MASK; // Inclusive.

    bool found_region = false;
    bool found_error = false;

    while (current != null) {
        bool is_start_inside = current->virt_start <= start && start <= current->virt_end;
        bool is_end_inside = current->virt_start <= end && end <= current->virt_end;

        bool is_intersecting_but_not_inside = is_start_inside != is_end_inside;
        bool is_inside = is_start_inside && is_end_inside;

        if (is_inside) {
            if (found_region) found_error = true;
            else { *out_region = current; found_region = true; }
        }

        if (is_intersecting_but_not_inside) found_error = true;

        if (found_region && found_error) break;

        current = current->next;
    }

    // To make sure that we make it clear that no match was found.
    if (!found_region && found_error && out_region != null) *out_region = null;

    return !found_error;
}

#define VAS_SEARCH_START 0x40000000
#define VAS_SEARCH_END 0x0000900000000000ULL
// Returns 0 if no space is found.
uint64_t vas_get_unused_space(VAS *vas, uint64_t page_count) { // FIXME: Old implementation directly copied from sys_map_mmio
    uint64_t found_pages = 0;

    uint64_t addr;
    for (addr = VAS_SEARCH_START; found_pages < page_count && addr < VAS_SEARCH_END; addr += PAGE_SIZE) {
        if (!vmm_get_page_info(vas->cpu_address_table, addr, null, null))
        {
            found_pages++;

            if (found_pages == page_count) {
                // Calculate the starting address of this contiguous block
                return addr - (page_count - 1) * PAGE_SIZE;
            }
        }
        else {
            found_pages = 0;
        }
    }

    return 0;
}

// Returns 0 if access is invalid.
uintptr_t vmem_get_phys_page(VirtualMemoryObject *object, size_t index) {
    switch (object->type) {
    case VM_OBJ_MMIO:
        if (object->mmio.page_count <= index) return 0;
        return object->mmio.phys_start + index * PAGE_SIZE;
    case VM_OBJ_SLICE:
        if (object->slice.count <= index) return 0;
        return object->slice.pages[index];
    case VM_OBJ_ALLOCATION:
        if (object->allocation.count <= index) return 0;
        return object->allocation.pages[index];
    default:
        PANIC("vmem_get_page: unrecognized object type: %d", object->type);
    }

    return 0;
}

// Returns number of pages mapped.
VASRegion *vas_add_region(VAS *vas, uint64_t virt_start, VASRegionPermission permissions, VASRegionReason reason, VirtualMemoryObject *backing) {
    if (backing == null)
        PANIC("vas_add_region with null backing!");

    uint64_t virt_end = virt_start + vmem_get_page_count(backing) * PAGE_SIZE;

    if (virt_end >= HHDM_OFFSET)
        PANIC("vas_add_region: mapping virt end reaches higher half.");

    VASRegion *clashing_region = null;
    if (!vas_get_memory_region(vas, virt_start, virt_end, &clashing_region) || clashing_region != null) // Perhaps this check can be optimized away?
        PANIC("vas_add_region on VAS with conflicting region already added!");

    if (virt_start & ~PT_ADDRESS_MASK || virt_end & ~PT_ADDRESS_MASK)
        PANIC("vas_add_region with unaligned range!");

    VASRegion *region = kmalloc(sizeof(VASRegion));
    region->virt_start = virt_start;
    region->virt_end = virt_end;
    region->permissions = permissions;
    region->reason = reason;
    region->backing = backing;

    region->next = null;
    region->prev = null;

    if (vas->regions == null) vas->regions = region;
    else {
        VASRegion *current = vas->regions;
        while (current->next != null) current = current->next;
        current->next = region;
        region->prev = current;
    }

    backing->ref_count++;

    uint64_t cpu_mapping_flags = PT_PRESENT;
    if (!vas->is_kernel) cpu_mapping_flags |= PT_USER;
    if (!(permissions & VMEM_PERM_READ)) PANIC("vas_add_region: unsupported permission !VMEM_PERM_READ");
    if (permissions & VMEM_PERM_WRITE) cpu_mapping_flags |= PT_RW;
    if (!(permissions & VMEM_PERM_EXEC)) cpu_mapping_flags |= PT_NX;

    switch (backing->type) {
    case VM_OBJ_MMIO: {
        cpu_mapping_flags |= PT_PWT | PT_PCD;

        for (uint64_t i = 0; i < backing->mmio.page_count; i++)
            vmm_map(vas->cpu_address_table, virt_start + i * PAGE_SIZE, backing->mmio.phys_start + i * PAGE_SIZE, cpu_mapping_flags);

        break;
    }
    case VM_OBJ_SLICE:
        for (uint64_t i = 0; i < backing->slice.count; i++)
            vmm_map(vas->cpu_address_table, virt_start + i * PAGE_SIZE, backing->slice.pages[i], cpu_mapping_flags);
        break;
    case VM_OBJ_ALLOCATION:
        for (uint64_t i = 0; i < backing->allocation.count; i++)
            vmm_map(vas->cpu_address_table, virt_start + i * PAGE_SIZE, backing->allocation.pages[i], cpu_mapping_flags);
        break;
    default:
        PANIC("vas_add_region called with backing featuring an unsupported backing type: %d", backing->type);
    }

    return region;
}

static void remove_region_internal(VAS *vas, VASRegion *region, bool perform_unmap) {
    if (perform_unmap) {
        switch (region->backing->type) {
        case VM_OBJ_MMIO: {
            for (uint64_t i = 0; i < region->backing->mmio.page_count; i++)
                vmm_unmap(vas->cpu_address_table, region->virt_start + i * PAGE_SIZE);
            break;
        }
        case VM_OBJ_SLICE:
            for (uint64_t i = 0; i < region->backing->slice.count; i++)
                vmm_unmap(vas->cpu_address_table, region->virt_start + i * PAGE_SIZE);
            break;
        case VM_OBJ_ALLOCATION:
            for (uint64_t i = 0; i < region->backing->allocation.count; i++)
                vmm_unmap(vas->cpu_address_table, region->virt_start + i * PAGE_SIZE);
            break;
        default:
            PANIC("vas_remove_region called with backing featuring an unsupported backing type: %d", region->backing->type);
        }
    }

    region->backing->ref_count--;

    if (region->backing->ref_count <= 0) {
        switch (region->backing->type) {
        case VM_OBJ_ALLOCATION:
            kprintln("VAS: Freeing allocation memory object with %ld pages!", region->backing->allocation.count);

            for (uint64_t i = 0; i < region->backing->allocation.count; i++)
                pmm_free_page(region->backing->allocation.pages[i]);

            kfree(region->backing->allocation.pages);
            break;
        case VM_OBJ_SLICE:
            kprintln("VAS: Destroying slice memory object with %ld pages!", region->backing->slice.count);
            kfree(region->backing->slice.pages);
            break;
        case VM_OBJ_MMIO:
            kprintln("VAS: Destroying MMIO memory object at %lx with %ld pages!", region->backing->mmio.phys_start, region->backing->mmio.page_count);
            break;
        default:
            PANIC("vas_remove_region called with backing featuring an unsupported backing type: %d", region->backing->type);
        }

        kfree(region->backing);
        region->backing = null;
    }

    if (region->prev != null) region->prev->next = region->next;
    if (region->next != null) region->next->prev = region->prev;
    if (vas->regions == region) vas->regions = region->next;

    kfree(region);
}

// Will free the region. Keep track of your pointers.
void vas_remove_region(VAS *vas, VASRegion *region) {
    // ensurance (perhaps can be removed?)
    {
        bool is_within_vas = false;

        VASRegion *current = vas->regions;
        while (current != null) {
            if (current == region) {
                is_within_vas = true;
                break;
            }

            current = current->next;
        }

        if (!is_within_vas) PANIC("vas_remove_region: region is not inside vas");
    }

    remove_region_internal(vas, region, true);
}

void vas_init(VAS *vas) {
    if (vas->is_kernel) PANIC("Cannot init kernel VAS yet!");

    vas->cpu_address_table = vmm_create_user_address_space();
    vas->next_stack_top = VAS_STACK_TOP;
}

void vas_destroy(VAS *vas) {
    kprintln("VAS: Destroying VAS on %lx!", (uintptr_t) vas);

    while (vas->regions != null)
        remove_region_internal(vas, vas->regions, false);

    vmm_destroy_user_address_space(vas->cpu_address_table);

    vas->next_stack_top = 0;
}
