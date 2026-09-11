
#pragma once

#include "types.h"
#include "vmm.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    VM_OBJ_MMIO,
    VM_OBJ_ALLOCATION,
    VM_OBJ_SLICE,
    VM_OBJ_SHARED,
} VirtualMemoryObjectType;

// Contains variable-length array.
typedef struct {
    VirtualMemoryObjectType type;
    size_t map_count;

    union {
        struct {
            uint64_t phys_start;
            uint64_t page_count;
        } mmio;

        struct {
            size_t count;
            uintptr_t *pages;
        } slice;

        struct {
            size_t count;
            uintptr_t *pages;
        } allocation;

        struct {
            size_t count;
            uintptr_t *pages;

            void* kobject_entry;
            uint64_t kobject_id;
        } shared;
    };
} VirtualMemoryObject;

typedef enum : uint32_t {
    VREGION_REASON_EXECUTABLE,
    VREGION_REASON_STACK,
    VREGION_REASON_USER_REQUEST,
    VREGION_REASON_SYSTEM_INIT,
} VASRegionReason;

typedef enum : uint32_t {
    VMEM_PERM_NONE = 0,

    VMEM_PERM_READ = 1 << 0,
    VMEM_PERM_WRITE = 1 << 1,
    VMEM_PERM_EXEC = 1 << 2,

    VMEM_PERM_RW = (VMEM_PERM_READ | VMEM_PERM_WRITE),
    VMEM_PERM_RX = (VMEM_PERM_READ | VMEM_PERM_EXEC),
    VMEM_PERM_ALL = (VMEM_PERM_RW | VMEM_PERM_EXEC),
} VASRegionPermission;

typedef struct VASRegion {
    uint64_t virt_start;
    uint64_t virt_end;
    VASRegionPermission permissions;
    VASRegionReason reason;
    VirtualMemoryObject *backing;

    // Doubly linked list, no tree yet
    struct VASRegion *next;
    struct VASRegion *prev;
} VASRegion;

#define VAS_STACK_TOP 0x0000700000000000ULL

typedef struct {
    VASRegion *regions;

    PML4 *cpu_address_table;
    bool is_kernel;

    uint64_t next_stack_top;
} VAS;

VirtualMemoryObject *vmem_create_slice(uintptr_t *array, size_t size);
static inline VirtualMemoryObject *vmem_create_slice_single(uint64_t physical_address) {
    return vmem_create_slice(&physical_address, 1);
}
VirtualMemoryObject *vmem_create_slice_continuous(uint64_t phys_start, size_t page_count);
VirtualMemoryObject *vmem_create_mmio(uint64_t phys_start, size_t page_count);
VirtualMemoryObject *vmem_create_allocation(size_t page_count, bool free);

uint64_t vmem_create_shared(size_t page_count); // Can't use KernelObjectID here.
void vas_destroy_shared_memory(uint64_t kid);

uint64_t vmem_get_page_count(VirtualMemoryObject *object);
uintptr_t vmem_get_phys_page(VirtualMemoryObject *object, size_t index);

bool vas_get_memory_region(VAS *vas, uint64_t virt_start, uint64_t virt_end, VASRegion **out_region);
uint64_t vas_get_unused_space(VAS *vas, uint64_t page_count);

VASRegion *vas_add_region(VAS *vas, uint64_t virt_start, VASRegionPermission permissions, VASRegionReason reason, VirtualMemoryObject *backing);
void vas_remove_region(VAS *vas, VASRegion *region);

void vas_init(VAS *vas);
void vas_destroy(VAS *vas);
