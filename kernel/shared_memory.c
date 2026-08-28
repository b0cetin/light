
#include "shared_memory.h"
#include "debugging.h"
#include "utils/hashtables/u64toaddr_hashtable.h"
#include "vmm.h"
#include <stdint.h>

Hashtable *table;

void smem_init() {
    table = ht_create();
}



SharedMemoryKey next_id = 1; // FIXME: A more secure way to identify needed



SharedMemoryKey smem_create(PML4 *address_space, uintptr_t base, uint64_t page_count) {
    PANIC("smem_create not implemented.");


    // SharedMemoryEntry *entry = allocate_entry();
    // if (entry == null) return SMEM_NULL_ID;

    // if (base & 0xFFF)
    //     PANIC("Address supplied into smem_create is not page-aligned!");

    // if (page_count <= 0)
    //     PANIC("Page count is 0 in smem_create!");

    // entry->physical_pages = kmalloc(page_count * sizeof(uint64_t));
    // entry->page_count = page_count;
    // entry->map_flags = 0;

    // for (uint64_t i = 0; i < page_count; i++) {
    //     uint64_t phys = 0, flags = 0;
    //     vmm_get_page_info(address_space, base + PAGE_SIZE * i, &phys, &flags);

    //     entry->physical_pages[i] = phys;
    //     entry->map_flags |= flags; // FIXME: Inconsistent behaviour.
    // }

    // entry->reference_count = 1;

    // return entry->id;
}

// Returns the number of bytes mapped.
uint64_t smem_map(PML4 *address_space, uintptr_t base, SharedMemoryKey key) {
    PANIC("smem_map not implemented.");
}
