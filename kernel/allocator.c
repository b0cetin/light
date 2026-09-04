
#include "allocator.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "pmm.h"
#include "types.h"
#include "vmm.h"
#include <stddef.h>
#include <stdint.h>

// TODO: No support for expanding the heap. Will implement in the future when necessary.

__attribute__((aligned(16)))
typedef struct BlockHeader {
    // Block size. Header size is not counted.
    size_t size;

    bool is_free;
    __attribute__((aligned(16))) struct BlockHeader *prev;
    __attribute__((aligned(16))) struct BlockHeader *next;

    uint64_t checksum;
} BlockHeader;

BlockHeader *first = null;

uint64_t heap_size = 0;
uint64_t heap_capacity = 0;

// void test() {
//     kprintln("TEST: Printing heap memory... (only first 128 bytes)");

//     for (int i = 0; i < 16; i++) {
//         uint64_t *ptr = ((uint64_t*) first) + i;
//         kernel_printf("%lx", *ptr);
//     }

//     size_t test_allocation_size = 5;

//     kprintln("\nTEST: Allocting %ld bytes.", test_allocation_size);

//     void *test_allocation = kmalloc(test_allocation_size);

//     kprintln("TEST: memsetting the allocation to 0xFF.");

//     memset(test_allocation, 0xFF, test_allocation_size);

//     kprintln("TEST: Heap size: %ld bytes, heap capacity: %ld bytes", heap_size, heap_capacity);

//     kprintln("TEST: Printing heap memory... (only first 128 bytes)");

//     for (int i = 0; i < 16; i++) {
//         uint64_t *ptr = ((uint64_t*) first) + i;
//         kernel_printf("%lx", *ptr);
//     }

//     // kernel_println("TEST: Coalescing discard test...");

//     // (((BlockHeader*) test_allocation) - 1)->next += 0xFF; // TEST SUCCESS

//     kprintln("\nTEST: Freeing allocation...");

//     kfree(test_allocation);

//     kprintln("TEST: Heap size: %ld bytes, heap capacity: %ld bytes", heap_size, heap_capacity);

//     kprintln("TEST: Printing heap memory... (only first 128 bytes)");

//     for (int i = 0; i < 16; i++) {
//         uint64_t *ptr = ((uint64_t*) first) + i;
//         kernel_printf("%lx", *ptr);
//     }

//     kprintln("\nTEST: Finished.");
// }

static void set_checksum(BlockHeader *header) {
    uint64_t sum = 0;

    sum += header->is_free;
    sum += header->size;
    sum += (uintptr_t) header->next;
    sum += (uintptr_t) header->prev;

    // Two's complement: Additive inverse
    header->checksum = (uint64_t) -sum;
}

static void checksum(BlockHeader *header) {
    uint64_t sum = 0;

    sum += header->is_free;
    sum += header->size;
    sum += (uintptr_t) header->next;
    sum += (uintptr_t) header->prev;
    sum += header->checksum;

    if (sum != 0)
        PANIC("BlockHeader at %lx has been externally modified!", header);
}

void alloc_init() {
    kprintln("ALLOC: Initializing heap memory...");

    heap_capacity = 4096 * 256;
    heap_size = sizeof(BlockHeader);

    kprintln("ALLOC: Initial heap capacity: %ld pages", heap_capacity / 4096);

    uint64_t block_phys = pmm_alloc(heap_capacity / 4096);

    if (block_phys == 0)
        PANIC("Not enough memory found for heap (size: %ld KiB)", heap_capacity / 1024);

    first = p2v(block_phys);

    memzero(first, heap_capacity);

    first->size = heap_capacity - sizeof(BlockHeader);
    first->is_free = true;
    first->prev = null;
    first->next = null;
    set_checksum(first);

    kprintln("ALLOC: Initialized heap memory with size of %ld KiB.", heap_capacity / 1024);

    // test();
}

#define ALIGN_UP(x, a) (((x) + ((uintptr_t)(a) - 1)) & ~((uintptr_t)(a) - 1))

// Checks to see if the next and prev headers are coalesce-able and coalesces if it can.
void coalesce(BlockHeader *free_header) {
    if (!free_header->is_free)
        PANIC("coalesce() called on used block header.");

    checksum(free_header);

    // NOTE: If any of the prev or next pointers are null, then the check just won't fire.
    // Pointers being null is not an issue.

    bool is_next_immediate = (char*) free_header->next == ((char *)(free_header + 1)) + free_header->size;

    if (is_next_immediate && free_header->next->is_free) {
        checksum(free_header->next);

        BlockHeader* next_next = free_header->next->next;
        size_t next_size = free_header->next->size;

        // kprintln("ALLOC: Coalescing free header at %lx with next header at %lx...", free_header, free_header->next);
        // Silence frequent log

        memzero(free_header->next, sizeof(BlockHeader));
        free_header->size += next_size + sizeof(BlockHeader);
        free_header->next = next_next;

        if (next_next != null) {
            next_next->prev = free_header;
            set_checksum(next_next);
        }

        heap_size -= sizeof(BlockHeader);

        set_checksum(free_header);
    }

    // NOTE: Prev pointer being null is a problem here. free_header->prev->size will read garbage memory if it is.

    bool is_prev_immediate = false;
    if (free_header->prev != null) {
        checksum(free_header->prev);
        is_prev_immediate = (char*) free_header->prev == ((char *)(free_header - 1)) - free_header->prev->size;
    }

    if (is_prev_immediate && free_header->prev->is_free) {
        checksum(free_header->prev);

        BlockHeader* current_next = free_header->next;
        size_t current_size = free_header->size;

        // kprintln("ALLOC: Coalescing free header at %lx with prev header at %lx...", free_header, free_header->prev);
        // Silence frequent log

        BlockHeader* prev = free_header->prev;

        memzero(free_header, sizeof(BlockHeader));

        prev->size += current_size + sizeof(BlockHeader);
        prev->next = current_next;

        if (current_next != null) {
            current_next->prev = prev;
            set_checksum(current_next);
        }

        heap_size -= sizeof(BlockHeader);

        set_checksum(prev);
    }
}

void *kmalloc(size_t size) {
    if (size == 0) return null; // Expected behaviour from allocators.

    // NOTE: The header already is 16-byte aligned.

    size_t aligned_size = ALIGN_UP(size, 16);

    // FIXME: This region search fails in finding exact fits or almost-exact fits.
    // Should an exact fit be found, then no splitting should be done and the block
    // should just be repurposed.
    // If region->size > aligned_size && region->size < aligned_size + sizeof(BlockHeader),
    // then the block can most likely be repurposed again, with the extra space being
    // negligable.
    //
    // I won't bother with implementing this for now, as the current allocator
    // just works as it is and I want to get on with my life. :)

    BlockHeader* region = first;
    while (region != null && (!region->is_free || region->size <= aligned_size + sizeof(BlockHeader))) {
        checksum(region);
        region = region->next;
    }

    if (region == null)
        PANIC("Not enough capacity. Ran out of heap memory. TODO.");

    size_t old_size = region->size;
    BlockHeader* old_next = region->next;

    checksum(region);
    if (old_next != null) checksum(old_next);

    region->size = aligned_size;
    region->is_free = false;

    BlockHeader* new_header = (BlockHeader*)((char *)(region + 1) + aligned_size);
    new_header->size = old_size - aligned_size - sizeof(BlockHeader);
    new_header->is_free = true; // Already ensured.
    new_header->next = old_next;
    new_header->prev = region;

    // region->prev left intact.
    region->next = new_header;

    if (old_next != null) {
        old_next->prev = new_header;
        set_checksum(old_next);
    }

    set_checksum(new_header);
    set_checksum(region);

    // NOTE: This implementation doesn't need to touch the previous header
    // as its pointers are still correct.

    memzero(region + 1, aligned_size);

    heap_size += aligned_size + sizeof(BlockHeader);

    return (void *)(region + 1);
}

void kfree(void *ptr) {
    if (ptr == null) return; // Expected free logic from all allocators.

    BlockHeader* header = ((BlockHeader*)ptr) - 1;

    memset(ptr, 0xCC, header->size);

    checksum(header);
    if (header->is_free) PANIC("Double free detected at %p!", ptr);
    header->is_free = true;
    set_checksum(header);

    heap_size -= header->size;

    coalesce(header);
}

void *kcalloc(size_t num, size_t size) {
    return kmalloc(num * size); // kmalloc already zero-initializes memory.
}
