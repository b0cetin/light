
#include "allocator.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "pmm.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

typedef struct PageHeader {
    PageHeader *previous;
    PageHeader *next;
    void *new_alloc_loc;
    AllocHeader *last_alloc;
    uint16_t biggest_gap;
} PageHeader;

typedef struct AllocHeader {
    AllocHeader *previous;
    AllocHeader *next;
    PageHeader *page;
    uint16_t size;
} AllocHeader;

PageHeader *last_header;

PageHeader *alloc_and_create_page(PageHeader *previous, PageHeader *next) {
    PageHeader *page = p2v(pmm_alloc_page());
    memzero(page, 4096);
    page->previous = previous;
    page->next = next;
    page->biggest_gap = 4096 - sizeof(PageHeader) - sizeof(AllocHeader);
    page->new_alloc_loc = page + 1;

    return page;
}

AllocHeader *create_alloc(PageHeader *page, uint64_t content_size) {
    if (page == null) {
        PANIC("Page passed into create_alloc was null.");
    }

    if (page->biggest_gap < content_size) {
        PANIC("create_alloc called when the page's biggest gap (%d) was not sufficient for content size (%d)!", page->biggest_gap, content_size);
    }

    AllocHeader *alloc = page->new_alloc_loc;
    alloc->previous = page->last_alloc;
    alloc->size = content_size;
    if (page->last_alloc != null)
        page->last_alloc->next = alloc;
    page->last_alloc = alloc;
    page->new_alloc_loc += content_size + sizeof(AllocHeader);
    page->biggest_gap -= content_size; // FIXME: This may warrant issues. Maybe implement a `get_biggest_gap` function?

    return alloc;
}

void *malloc(uint64_t size) {
    uint64_t limit = 4096 - sizeof(PageHeader) - sizeof(AllocHeader);

    if (size > limit) {
        kernel_println("ALC: Tried to malloc an object bigger than %d bytes: %d", limit, size);
        return null;
    }

    PageHeader *page;
    AllocHeader *alloc;

    for (page = last_header; true; page = page->previous) {
        if (page == null) {
            page = alloc_and_create_page(last_header, null);

            kernel_println("ALC: Not enough space (%d) found on existing pages. Allocated new: %lx", size, page);
            page->previous = last_header;

            if (last_header != null)
                last_header->next = page;

            last_header = page;
        }

        if (page->biggest_gap >= size) {
            alloc = create_alloc(page, size);
            break;
        }
    }

    return alloc + 1;
}

void free(void *ptr) {
    // AllocHeader *alloc = ptr - sizeof(AllocHeader);
    // PageHeader *page = alloc->page;

    // alloc->

    PANIC("free not implemented");
}
