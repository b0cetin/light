
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "syscalls.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 4096

__attribute__((aligned(16)))
typedef struct BlockHeader {
    // Block size. Header size is not counted.
    size_t size;

    bool is_free;
    __attribute__((aligned(16))) struct BlockHeader *prev;
    __attribute__((aligned(16))) struct BlockHeader *next;

    uint64_t checksum;
} BlockHeader;

BlockHeader *first = NULL;

bool is_heap_initialized = false;
uint64_t heap_size = 0;
uint64_t heap_overhead = 0;
uint64_t heap_capacity = 0;

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

    if (sum != 0) {
        println("stdlib: BlockHeader at %lx has been externally modified!", header);
        sys_exit(-1);
    }
}

void heap_init() {
    heap_capacity = PAGE_SIZE * 256;
    heap_size = 0;
    heap_overhead = sizeof(BlockHeader);


    first = NULL;
    if (sys_memory_map((void**) &first, heap_capacity, MMAP_ACCESS_READ | MMAP_ACCESS_WRITE) != SYS_SUCCESS) {
        println("stdlib: not enough memory found for heap (size: %ld KiB)", heap_capacity / 1024);
        sys_exit(-1);
    }

    memzero(first, heap_capacity);

    first->size = heap_capacity - sizeof(BlockHeader);
    first->is_free = true;
    first->prev = NULL;
    first->next = NULL;
    set_checksum(first);

    heap_overhead = sizeof(BlockHeader);
}

#define ALIGN_UP(x, a) (((x) + ((uintptr_t)(a) - 1)) & ~((uintptr_t)(a) - 1))

// Checks to see if the next and prev headers are coalesce-able and coalesces if it can.
void coalesce(BlockHeader *free_header) {
    if (!free_header->is_free) {
        println("stdlib: coalesce() called on used block header.");
        sys_exit(-1);
    }

    checksum(free_header);

    // NOTE: If any of the prev or next pointers are null, then the check just won't fire.
    // Pointers being null is not an issue.

    bool is_next_immediate = (char*) free_header->next == ((char *)(free_header + 1)) + free_header->size;

    if (is_next_immediate && free_header->next->is_free) {
        checksum(free_header->next);

        BlockHeader* next_next = free_header->next->next;
        size_t next_size = free_header->next->size;
        memzero(free_header->next, sizeof(BlockHeader));
        free_header->size += next_size + sizeof(BlockHeader);
        free_header->next = next_next;

        if (next_next != NULL) {
            next_next->prev = free_header;
            set_checksum(next_next);
        }

        set_checksum(free_header);

        heap_overhead -= sizeof(BlockHeader);
    }

    // NOTE: Prev pointer being null is a problem here. free_header->prev->size will read garbage memory if it is.

    bool is_prev_immediate = false;
    if (free_header->prev != NULL) {
        checksum(free_header->prev);
        is_prev_immediate = (char*) free_header->prev == ((char *)(free_header - 1)) - free_header->prev->size;
    }

    if (is_prev_immediate && free_header->prev->is_free) {
        checksum(free_header->prev);

        BlockHeader* current_next = free_header->next;
        size_t current_size = free_header->size;

        BlockHeader* prev = free_header->prev;

        memzero(free_header, sizeof(BlockHeader));

        prev->size += current_size + sizeof(BlockHeader);
        prev->next = current_next;

        if (current_next != NULL) {
            current_next->prev = prev;
            set_checksum(current_next);
        }

        set_checksum(prev);

        heap_overhead -= sizeof(BlockHeader);
    }
}

BlockHeader *expand_heap(size_t aligned_size) {
    uint64_t page_count = (aligned_size + sizeof(BlockHeader) + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t allocation_size = page_count * PAGE_SIZE;

    void *allocation = NULL;
    if (sys_memory_map(&allocation, allocation_size, MMAP_ACCESS_READ | MMAP_ACCESS_WRITE) != SYS_SUCCESS)
        return NULL;

    BlockHeader *header = allocation;
    
    header->size = allocation_size - sizeof(BlockHeader);
    header->is_free = true;
    header->prev = NULL;
    header->next = NULL;
    set_checksum(header);

    if (first != NULL) {
        BlockHeader *last = first;
        checksum(last);
        while (last->next != NULL) { last = last->next; checksum(last); }
        last->next = header;
        header->prev = last;
    }
    else {
        first = header;
    }

    heap_capacity += allocation_size;
    heap_overhead += sizeof(BlockHeader);

    return header;
}

#define MIN_USEFUL_BLOCK 32
void split_block_if_worthwhile(BlockHeader *region, size_t aligned_size) {
    size_t remainder = region->size - aligned_size;
    if (remainder <= sizeof(BlockHeader) + MIN_USEFUL_BLOCK) {
        return;
    }

    checksum(region);
    if (region->next != NULL) checksum(region->next);

    size_t old_size = region->size;
    BlockHeader* old_next = region->next;

    region->size = aligned_size;

    BlockHeader* new_header = (BlockHeader*)((char *)(region + 1) + aligned_size);
    new_header->size = old_size - aligned_size - sizeof(BlockHeader);
    new_header->is_free = true; // Already ensured.
    new_header->next = old_next;
    new_header->prev = region;

    // region->prev left intact.
    region->next = new_header;

    if (old_next != NULL) {
        old_next->prev = new_header;
        set_checksum(old_next);
    }

    set_checksum(new_header);
    set_checksum(region);

    heap_overhead += sizeof(BlockHeader);

    // NOTE: This implementation doesn't need to touch the previous header
    // as its pointers are still correct.
}

void *malloc(size_t size) {
    if (size == 0) return NULL; // Expected behaviour from allocators.

    if (!is_heap_initialized)
        heap_init();

    // NOTE: The header already is 16-byte aligned.

    size_t aligned_size = ALIGN_UP(size, 16);

    BlockHeader* region = first;
    while (region != NULL) {
        checksum(region);
        if (region->is_free && region->size >= aligned_size) break;
        region = region->next;
    }

    if (region == NULL) {
        region = expand_heap(aligned_size);
        if (region == NULL) return NULL;
    }

    split_block_if_worthwhile(region, aligned_size);
    region->is_free = false;
    set_checksum(region);

    memzero(region + 1, aligned_size);

    heap_size += region->size;

    return (void *)(region + 1);
}

void free(void *ptr) {
    if (ptr == NULL) return; // Expected free logic from all allocators.

    BlockHeader* header = ((BlockHeader*)ptr) - 1;

    memset(ptr, 0xCC, header->size);

    checksum(header);

    if (header->is_free) {
        println("stdlib: double free detected at %llu!", ptr);
        sys_exit(-1);
    }

    header->is_free = true;
    set_checksum(header);

    heap_size -= header->size;

    coalesce(header);
}

void *calloc(size_t num, size_t size) {
    return malloc(num * size); // malloc already zero-initializes memory.
}
