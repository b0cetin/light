
#include "shared_memory.h"
#include "allocator.h"
#include "debugging.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

SharedMemoryEntry array[128]; // TODO: Probably a dynamic array is needed

SharedMemoryID next_id = 1; // FIXME: A more secure way to identify needed

static SharedMemoryEntry *allocate_entry() {
    for (int i = 0; i < 128; i++) {
        if (array[i].id == SMEM_NULL_ID) {
            array[i].id = next_id++;
            array[i].physical_pages = null;
            array[i].page_count = 0;
            array[i].reference_count = 0;
            array[i].map_flags = 0;
            return &array[i];
        }
    }

    return null;
}

static SharedMemoryEntry *find_entry(SharedMemoryID id) { // O(n)
    for (int i = 0; i < 128; i++)
        if (array[i].id == id)
            return &array[i];

    return null;
}

SharedMemoryID smem_create(PML4 *address_space, uintptr_t base, uint64_t page_count) {
    SharedMemoryEntry *entry = allocate_entry();
    if (entry == null) return SMEM_NULL_ID;

    if (base & 0xFFF)
        PANIC("Address supplied into smem_create is not page-aligned!");

    if (page_count <= 0)
        PANIC("Page count is 0 in smem_create!");

    entry->physical_pages = kmalloc(page_count * sizeof(uint64_t));
    entry->page_count = page_count;
    entry->map_flags = 0;

    for (uint64_t i = 0; i < page_count; i++) {
        uint64_t phys = 0, flags = 0;
        vmm_get_page_info(address_space, base + PAGE_SIZE * i, &phys, &flags);

        entry->physical_pages[i] = phys;
        entry->map_flags |= flags; // FIXME: Inconsistent behaviour.
    }

    entry->reference_count = 1;

    return entry->id;
}

// Returns the number of bytes mapped.
uint64_t smem_map(PML4 *address_space, uintptr_t base, SharedMemoryID id) {
    SharedMemoryEntry *entry = allocate_entry();
    if (entry == null) return 0;

    if (base & 0xFFF)
        PANIC("Address supplied into smem_map is not page-aligned!");

    entry->reference_count++;

    for (uint64_t i = 0; i < entry->page_count; i++) {
        vmm_map(address_space, base + PAGE_SIZE * i, entry->physical_pages[i], entry->map_flags);
    }

    return entry->page_count * PAGE_SIZE;
}
