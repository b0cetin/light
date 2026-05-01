
// Virtual Memory Manager (VMM)

// TODO: VMM
// Only map using UEFI memory map (and set the correct flags for each usable memory page)
// When mapping IOMMU, disable caching.

#include "vmm.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "pmm.h"
#include <stdint.h>

#define PAGE_SIZE 4096

void load_cr3(uint64_t pml4_addr) {
    asm volatile("mov %0, %%cr3" ::"r"(pml4_addr) : "memory");
}

uint64_t *get_next_level(uint64_t *current_level, uint64_t index) {
    if (current_level[index] & PT_PRESENT) {
        return p2v(current_level[index] & PT_ADDRESS_MASK);
    }

    uint64_t new_table = pmm_alloc_page();
    if (new_table == 0)
        PANIC("VMM: System has run out of memory to create a new page table!");

    memzero(p2v(new_table), PAGE_SIZE);

    current_level[index] = (uint64_t)new_table | PT_PRESENT | PT_RW; // Maybe also add PT_USER? Userspace is not a thing rn, so don't really care.

    return p2v(new_table);
}

void map(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = get_next_level(pml4, pml4_idx);
    uint64_t *pd = get_next_level(pdpt, pdpt_idx);

    if ((*pd & PT_HUGE) > 0) {
        PANIC("VMM: Reached huge PD while traversing with normal mapping.");
    }

    uint64_t *pt = get_next_level(pd, pd_idx);

    pt[pt_idx] = phys | flags;
}

void map_2mb(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;

    uint64_t *pdpt = get_next_level(pml4, pml4_idx);
    uint64_t *pd = get_next_level(pdpt, pdpt_idx);

    pd[pd_idx] = phys | flags | PT_HUGE;
}

void map_range_with_offset(uint64_t *pml4, uint64_t start, uint64_t size, uint64_t offset, uint64_t flags) {
    uint64_t first_page = start & ~(0xFFFULL);
    uint64_t last_page = (start + size + 4095) & ~(0xFFFULL);

    for (uint64_t addr = first_page; addr < last_page; addr += PAGE_SIZE) {
        map(pml4, addr + offset, addr, flags);
    }
}

void map_2mb_range_with_offset(uint64_t *pml4, uint64_t start, uint64_t size, uint64_t offset, uint64_t flags) {
    uint64_t first_huge_page = start & ~0x1FFFFFULL;
    uint64_t last_huge_page = (start + size + 0x1FFFFFULL) & ~0x1FFFFFULL;

    for (uint64_t addr = first_huge_page; addr < last_huge_page; addr += 0x200000) {
        map_2mb(pml4, addr + offset, addr, flags);
    }
}

void vmm_identity_map_range(uint64_t *pml4, uint64_t start, uint64_t size, uint64_t flags) {
    map_range_with_offset(pml4, start, size, 0, flags);
}

uint64_t *pml4 = 0;

void vmm_init() {
    uint64_t pml4_phys = pmm_alloc_page();
    if (pml4_phys == 0)
        PANIC("VMM: PMM returned null for PML4 allocation.");
    pml4 = p2v(pml4_phys);
    memzero(pml4, PAGE_SIZE);
    kernel_println("VMM: Created PML4 on %lx.", pml4);

    map_2mb_range_with_offset(pml4, 0, pmm_get_highest_phys_addr(), HHDM_OFFSET, PT_PRESENT | PT_RW);
    kernel_println("VMM: Physical memory mapped to higher half with offset: %lx (%ld pages)", HHDM_OFFSET, pmm_get_highest_phys_addr() / PAGE_SIZE);

    kernel_println("VMM: Loading CR3...");
    load_cr3(v2p(pml4));

    kernel_println("VMM: Initialization finished.");
}

/*
void vmm_first_init() {
    uint64_t pml4_phys = pmm_alloc_page();
    if (pml4_phys == 0)
        PANIC("VMM: PMM returned null for PML4 allocation.");
    pml4 = p2v(pml4_phys);
    memzero(pml4, PAGE_SIZE);
    kernel_println("VMM: Created PML4 on %lx.", pml4);

    map_2mb_range_with_offset(pml4, 0, pmm_get_highest_phys_addr(), 0, PT_PRESENT | PT_RW);
    kernel_println("VMM: Physical memory mapped identically. (%ld pages)", pmm_get_highest_phys_addr() / PAGE_SIZE);

    map_2mb_range_with_offset(pml4, 0, pmm_get_highest_phys_addr(), HHDM_OFFSET, PT_PRESENT | PT_RW);
    kernel_println("VMM: Physical memory mapped to higher half with offset: %lx (%ld pages)", HHDM_OFFSET, pmm_get_highest_phys_addr() / PAGE_SIZE);

    kernel_println("VMM: Loading CR3...");
    load_cr3(v2p(pml4));

    kernel_println("VMM: First half of initialization finished.");

    pmm_print_stats();

    uint64_t rsp;
    asm volatile("lea (%%rsp), %0" : "=r"(rsp));
    kernel_println("Current RSP: %lx", rsp);
}

void vmm_final_init() {
    // FIXME: Memory leak: Old info remains on memory.

    uint64_t pml4_phys = pmm_alloc_page();
    if (pml4_phys == 0)
        PANIC("VMM: PMM returned null for PML4 allocation.");
    pml4 = p2v(pml4_phys);
    memzero(pml4, PAGE_SIZE);
    kernel_println("VMM: Created new PML4 on %lx.", pml4);

    map_2mb_range_with_offset(pml4, 0, pmm_get_highest_phys_addr(), HHDM_OFFSET, PT_PRESENT | PT_RW);
    kernel_println("VMM: Physical memory mapped to higher half with offset: %lx (%ld pages)", HHDM_OFFSET, pmm_get_highest_phys_addr() / PAGE_SIZE);

    kernel_println("VMM: Loading CR3...");
    load_cr3(v2p(pml4));

    kernel_println("VMM: Initialization finished.");
}
*/
