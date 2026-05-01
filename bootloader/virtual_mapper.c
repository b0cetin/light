
#include "virtual_mapper.h"
#include <efi.h>
#include <efilib.h>
#include <elf.h>
#include <stdint.h>

typedef uint64_t PTEntry;

#define PT_PRESENT (1ULL << 0)
#define PT_RW (1ULL << 1)   // Read/Write
#define PT_USER (1ULL << 2) // User Mode
#define PT_PWT (1ULL << 3)  // Write Through
#define PT_PCD (1ULL << 4)  // Cache Disable
#define PT_ACCESSED (1ULL << 5)
#define PT_DIRTY (1ULL << 6)
#define PT_HUGE (1ULL << 7) // 2MB or 1GB pages
#define PT_NX (1ULL << 63)  // No Execute

#define PT_ADDRESS_MASK 0x000FFFFFFFFFF000ULL // Bits 12 to 51
void *p2v(uint64_t phys) {
    return (void *)(phys + HHDM_OFFSET);
}
uint64_t v2p(void *virt) {
    return (uint64_t)virt - HHDM_OFFSET;
}

void load_cr3(uint64_t pml4_addr) {
    asm volatile("mov %0, %%cr3" ::"r"(pml4_addr) : "memory");
}

EFI_PHYSICAL_ADDRESS allocate_page() {
    EFI_PHYSICAL_ADDRESS page_addr;
    EFI_STATUS status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, 1, &page_addr);

    if (EFI_ERROR(status)) {
        Print(L"Allocation failed: %r", status);
        while (1)
            __asm__("hlt");
    }

    uefi_call_wrapper(BS->SetMem, 3, (void *)page_addr, 4096, 0);

    return page_addr;
}

uint64_t *get_next_level(uint64_t *current_level, uint64_t index) {
    if (current_level[index] & PT_PRESENT) {
        return (uint64_t *)(current_level[index] & PT_ADDRESS_MASK);
    }

    EFI_PHYSICAL_ADDRESS new_table = allocate_page();

    current_level[index] = (uint64_t)new_table | PT_PRESENT | PT_RW; // Maybe also add PT_USER? Userspace is not a thing rn, so don't really care.

    return (uint64_t *)(uint64_t)new_table;
}

void map_2mb(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;

    uint64_t *pdpt = get_next_level(pml4, pml4_idx);
    uint64_t *pd = get_next_level(pdpt, pdpt_idx);

    // Note: phys must be 2MiB aligned!
    pd[pd_idx] = phys | flags | PT_HUGE;
}

void map_2mb_range_with_offset(uint64_t *pml4, uint64_t start, uint64_t size, uint64_t offset, uint64_t flags) {
    // Ensure we are 2MB aligned for huge pages
    uint64_t first_huge_page = start & ~0x1FFFFFULL;
    uint64_t last_huge_page = (start + size + 0x1FFFFFULL) & ~0x1FFFFFULL;

    // Increment by 2 MiB (0x200000) instead of 4 KiB
    for (uint64_t addr = first_huge_page; addr < last_huge_page; addr += 0x200000) {
        map_2mb(pml4, addr + offset, addr, flags);
    }
}

uint64_t *pml4;

void calculate_custom_virtual_mappings() {
    pml4 = (UINT64 *)allocate_page();

    uint64_t last_address = (uint64_t)4 * 4096 * 4096 * 4096; // 4 GiB
    Print(L"Identity mapping...\n");
    map_2mb_range_with_offset(pml4, 0, last_address, 0, PT_PRESENT | PT_RW);
    Print(L"HHDM mapping...\n");
    map_2mb_range_with_offset(pml4, 0, last_address, HHDM_OFFSET, PT_PRESENT | PT_RW);
}

void enable_custom_virtual_mappings() {
    load_cr3((uint64_t)pml4);
}
