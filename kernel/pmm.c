
// BITMAP Physical Memory Manager (PMM)

// TODO: PMM
// "Highest address" displays 1 TB? Find a fix. Don't map unusable memory, and don't count it.
// You can recycle boot services and bootloader code.

#include "pmm.h"
#include "bootinfo.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "linker_symbols.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct {
    uint32_t Type;
    uint32_t Pad;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

uint8_t *bitmap;
uint64_t bitmap_size;

// statistics
uint64_t highest_physical_address_available;
uint64_t total_available_memory;
uint64_t free_pages_remaining;

void pmm_mark_free(uint64_t physical_address) {
    uint64_t page = physical_address / 4096;
    bitmap[page / 8] &= ~(1 << (page % 8));
    free_pages_remaining++;
}

void pmm_mark_used(uint64_t physical_address) {
    uint64_t page = physical_address / 4096;
    bitmap[page / 8] |= (1 << (page % 8));
    free_pages_remaining--;
}

bool pmm_is_used(uint64_t addr) {
    uint64_t page = addr / 4096;
    return (bitmap[page / 8] & (1 << (page % 8))) != 0;
}

bool should_skip_descriptor(EFI_MEMORY_DESCRIPTOR *desc) {
    return desc->Type == EfiReservedMemoryType || desc->Type == EfiUnusableMemory;
}

uint8_t *get_bitmap_address(BootInfo *boot_info, uint64_t *out_bitmap_size, uint64_t *out_highest_phys_addr) {
    uint64_t highest_phys_addr = 0;
    for (uint64_t i = 0; i < boot_info->MMapSize; i += boot_info->DescriptorSize) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((uint64_t)boot_info->MMap + i);
        uint64_t end_addr = desc->PhysicalStart + desc->NumberOfPages * 4096;

        if (should_skip_descriptor(desc))
            continue;

        // kernel_println("(%ld): %d: %lx -> %lx (attr: %d, size: %ld pages)", i / boot_info->DescriptorSize, desc->Type, desc->PhysicalStart, end_addr,
        //                desc->Attribute, desc->NumberOfPages);

        if (end_addr > highest_phys_addr)
            highest_phys_addr = end_addr;
    }
    uint64_t total_pages = (highest_phys_addr + 4095) / 4096;
    uint64_t bitmap_size = total_pages / 8;

    kernel_println("PMM: Highest physical address available: %lx", highest_phys_addr);
    kernel_println("PMM: Total pages: %ld pages.", total_pages);
    kernel_println("PMM: Bitmap size: %ld bytes.", bitmap_size);

    uint64_t bitmap_loc = 0;
    bool space_found = false;

    for (uint64_t i = 0; i < boot_info->MMapSize; i += boot_info->DescriptorSize) {
        EFI_MEMORY_DESCRIPTOR *descriptor = (EFI_MEMORY_DESCRIPTOR *)((uint64_t)boot_info->MMap + i);

        if (descriptor->PhysicalStart == 0)
            continue;

        if (descriptor->Type != EfiConventionalMemory)
            continue;

        if (descriptor->NumberOfPages * 4096 < bitmap_size)
            continue;

        bitmap_loc = (uint64_t)p2v(descriptor->PhysicalStart);
        space_found = true;
        break;
    }

    if (!space_found) {
        PANIC("Could not find enough space in memory for physical memory table.");
    }

    kernel_println("PMM: Bitmap address found at %lx.", bitmap_loc);

    *out_bitmap_size = bitmap_size;
    *out_highest_phys_addr = highest_phys_addr;
    return (uint8_t *)bitmap_loc;
}

void mark_available_pages_free(BootInfo *boot_info) {
    uint64_t total_freed = 0;

    for (uint64_t i = 0; i < boot_info->MMapSize; i += boot_info->DescriptorSize) {
        EFI_MEMORY_DESCRIPTOR *descriptor = (EFI_MEMORY_DESCRIPTOR *)((uint64_t)boot_info->MMap + i);

        if (descriptor->Type != EfiConventionalMemory)
            continue;

        // kernel_println("%d: %lx, %ld", descriptor->Type, descriptor->PhysicalStart, descriptor->NumberOfPages * 4096);

        for (uint64_t j = 0; j < descriptor->NumberOfPages; j++) {
            pmm_mark_free(descriptor->PhysicalStart + j * 4096);
            total_freed++;
        }
    }

    kernel_println("PMM: %ld pages marked free in bitmap.", total_freed);
}

void pmm_init(BootInfo *boot_info) {
    bitmap = get_bitmap_address(boot_info, &bitmap_size, &highest_physical_address_available);

    memset(bitmap, 0xFF, bitmap_size);

    mark_available_pages_free(boot_info);
    total_available_memory = free_pages_remaining * 4096;

    for (int i = 0; i < 16; i++) {
        pmm_mark_used(i * 4096); // Ensure the first 16 pages are unusable.
    }

    // Lock bitmap
    for (uint64_t i = 0; i < (bitmap_size + 4095) / 4096; i++) {
        pmm_mark_used(v2p((void *)bitmap) + (i * 4096));
    }

    // Lock kernel // TODO: Very suspicious. Fix needed.
    for (uint64_t i = 0; i < (get_kernel_end() - get_kernel_start() + 4095) / 4096; i++) {
        pmm_mark_used((uint64_t)boot_info->PhysicalKernelBase + (i * 4096));
    }

    if (!pmm_is_used(v2p(bitmap)))
        PANIC("PMM: Failed to lock bitmap!");

    if (!pmm_is_used((uint64_t)boot_info->PhysicalKernelBase))
        PANIC("PMM: Failed to lock kernel memory!");
}

void pmm_print_stats() {
    kernel_println("--- PMM Statistics ---");
    kernel_println("Highest addr: %lx (%ld MiB)", highest_physical_address_available, highest_physical_address_available / 4096 / 4096);
    kernel_println("Total RAM:    %ld MB", total_available_memory / 1024 / 1024);
    kernel_println("Free RAM:     %ld MB", free_pages_remaining * 4 / 1024);
}

uint64_t pmm_alloc_page() {
    for (uint64_t byte = 0; byte < bitmap_size; byte++) {
        if (bitmap[byte] != 0xFF) { // If this byte isn't full
            for (int bit = 0; bit < 8; bit++) {
                if (!(bitmap[byte] & (1 << bit))) {
                    uint64_t addr = (byte * 8 + bit) * 4096;
                    pmm_mark_used(addr);
                    return addr;
                }
            }
        }
    }

    return 0; // Out of memory!
}

void pmm_free_page(uint64_t physical_address) {
    if (physical_address == 0 || (uint64_t)physical_address >= highest_physical_address_available)
        return;

    pmm_mark_free((uint64_t)physical_address);
}

uint8_t *pmm_get_bitmap() {
    return bitmap;
}

uint64_t pmm_get_bitmap_size() {
    return bitmap_size;
}

uint64_t pmm_get_highest_phys_addr() {
    return highest_physical_address_available;
}

uint64_t pmm_get_available_memory_size() {
    return total_available_memory;
}
