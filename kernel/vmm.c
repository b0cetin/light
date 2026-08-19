
// Virtual Memory Manager (VMM)

#include "vmm.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "pmm.h"
#include <stdint.h>

// Low physical attribute bits must be cleared to avoid corrupting the physical target frame.
#define PAGE_4KB_MASK 0x000FFFFFFFFFF000ULL

static inline void load_cr3(uint64_t pml4_addr) {
    asm volatile("mov %0, %%cr3" ::"r"(pml4_addr) : "memory");
}

static inline void invlpg(uint64_t virt) { // Invalidates TLB entries.
    asm volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

// If create_if_needed is set to false, then if the next level cannot be found,
// the function will return 0.
static uint64_t *get_next_level(uint64_t *current_level, uint64_t index, bool create_if_needed) {
    if (current_level[index] & PT_PRESENT) {
        return p2v(current_level[index] & PT_ADDRESS_MASK);

        // Immediately skip if table already exists.

        // NOTE: I didn't implement upgrading as the default option is
        // the most permissive one.
    }

    if (!create_if_needed) return 0;

    uint64_t new_table = pmm_alloc_page();
    if (new_table == 0)
        PANIC("VMM: System has run out of memory to create a new page table!");

    memzero(p2v(new_table), PAGE_SIZE);

    current_level[index] = (uint64_t)new_table | PT_PRESENT | PT_RW | PT_USER;

    // NOTE: This configuration is the most permissive configuration an entry
    // can have above PT. To not worry about the pages above, this is how
    // it's done. This way, only the PT declares the final decision on what
    // page is what.

    return p2v(new_table);
}

void vmm_map(PML4 *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = get_next_level(pml4, pml4_idx, true);
    if (pdpt[pdpt_idx] & PT_HUGE) PANIC("VMM: Reached 1GB huge page in PDPT during page traversal.");

    uint64_t *pd = get_next_level(pdpt, pdpt_idx, true);

    if (pd[pd_idx] & PT_HUGE) {
        PANIC("VMM: Reached huge PD while traversing with normal mapping.");
    }

    uint64_t *pt = get_next_level(pd, pd_idx, true);

    pt[pt_idx] = (phys & PAGE_4KB_MASK) | flags | PT_PRESENT;

    invlpg(virt); // Invalidate CPU caching.
}

void map_range_with_offset(PML4 *pml4, uint64_t start, uint64_t size, uint64_t offset, uint64_t flags) {
    uint64_t first_page = start & ~(0xFFFULL);
    uint64_t last_page = (start + size + 4095) & ~(0xFFFULL);

    for (uint64_t addr = first_page; addr < last_page; addr += PAGE_SIZE) {
        vmm_map(pml4, addr + offset, addr, flags);
    }
}

PML4 *kernel_pml4 = 0;

void vmm_kmap_mmio(uint64_t phys, uint64_t size) {
    // PT_PCD (Cache Disable) and PT_PWT (Write Through) are recommended for MMIO
    // to prevent the CPU from caching video memory reads/writes incorrectly.
    uint64_t flags = PT_PRESENT | PT_RW | PT_PCD | PT_PWT; 
    map_range_with_offset(kernel_pml4, phys, size, HHDM_OFFSET, flags);
}

PML4 *vmm_create_user_address_space() {
    uint64_t pml4_phys = pmm_alloc_page();
    if (pml4_phys == 0)
        PANIC("VMM: PMM returned null for PML4 allocation for user address space.");

    PML4 *plm4 = p2v(pml4_phys);
    memzero(plm4, PAGE_SIZE);

    memcpy(&plm4[256], &kernel_pml4[256], 256 * sizeof(uint64_t));

    return plm4;

    // NOTE: A possible optimization here is setting the kernel tables
    // as PT_GLOBAL to not force the processor to invalidate its cache.
    //
    // I didn't really bother with this for now. Seems like a good
    // way to begin optimization in the future.
}

// Will destroy all user tables and free all memory.
void vmm_destroy_user_address_space_and_free_memory(PML4 *plm4) {
    if (plm4 == kernel_pml4)
        PANIC("vmm_destroy_user_address_space() called on kernel PLM4.");

    for (uint16_t i = 0; i < 256; i++) {
        if (!(plm4[i] & PT_PRESENT)) continue;

        uint64_t *pdpt = p2v(plm4[i] & PT_ADDRESS_MASK);

        for (uint16_t j = 0; j < 512; j++) {
            if (!(pdpt[j] & PT_PRESENT)) continue;

            uint64_t *pd = p2v(pdpt[j] & PT_ADDRESS_MASK);

            for (uint16_t k = 0; k < 512; k++) {
                if (!(pd[k] & PT_PRESENT)) continue;

                uint64_t *pt = p2v(pd[k] & PT_ADDRESS_MASK);

                for (uint16_t l = 0; l < 512; l++) {
                    if (!(pt[l] & PT_PRESENT)) continue;

                    uint64_t phys_frame = pt[l] & PT_ADDRESS_MASK;

                    pmm_free_page(phys_frame);

                    // NOTE: Reference counting will be essential before freeing
                    // once shared memory between processes become a thing.
                }

                memzero(pt, 4096);
                pmm_free_page(v2p(pt));
            }

            memzero(pd, 4096);
            pmm_free_page(v2p(pd));
        }

        memzero(pdpt, 4096);
        pmm_free_page(v2p(pdpt));
    }

    memzero(plm4, 4096);
    pmm_free_page(v2p(plm4));

    kprintln("VMM: Destroyed user PLM4 on %lx.", plm4);
}

void vmm_switch_to_user_address_space(PML4 *plm4) {
    load_cr3(v2p(plm4));
}

void vmm_switch_to_kernel_address_space() {
    load_cr3(v2p(kernel_pml4));
}

void vmm_init() {
    uint64_t pml4_phys = pmm_alloc_page();
    if (pml4_phys == 0)
        PANIC("VMM: PMM returned null for PML4 allocation.");

    kernel_pml4 = p2v(pml4_phys);
    memzero(kernel_pml4, PAGE_SIZE);

    kprintln("VMM: Created PML4 on %lx.", kernel_pml4);

    map_range_with_offset(kernel_pml4, 0, pmm_get_highest_phys_addr(), HHDM_OFFSET, PT_PRESENT | PT_RW);
    kprintln("VMM: Physical memory mapped to higher half with offset: %lx (%ld pages)", HHDM_OFFSET, pmm_get_highest_phys_addr() / PAGE_SIZE);

    kprintln("VMM: Loading CR3...");
    load_cr3(v2p(kernel_pml4));

    kprintln("VMM: Initialization finished.");
}

bool vmm_get_page_info(PML4 *pml4, uint64_t virt, uint64_t *out_phys, uint64_t *out_flags) {
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx = (virt >> 21) & 0x1FF;
    uint64_t pt_idx = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = get_next_level(pml4, pml4_idx, false);
    if (pdpt == 0) return false;
    
    if (pdpt[pdpt_idx] & PT_HUGE) PANIC("VMM: Reached 1GB huge page in PDPT during page traversal.");

    uint64_t *pd = get_next_level(pdpt, pdpt_idx, false);
    if (pd == 0) return false;

    if (pd[pd_idx] & PT_HUGE) {
        PANIC("VMM: Reached huge PD while traversing with normal mapping.");
    }

    uint64_t *pt = get_next_level(pd, pd_idx, false);
    if (pt == 0) return false;

    *out_phys = (pt[pt_idx] & PAGE_4KB_MASK) | (virt & 0xFFF);;
    *out_flags = pt[pt_idx] & (~PAGE_4KB_MASK);

    return true;
}

// Previous 2mb functionality has been removed.

// #define PAGE_2MB_MASK 0x000FFFFFFFFFE00000ULL
//
// void map_2mb(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
//     uint64_t pml4_idx = (virt >> 39) & 0x1FF;
//     uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
//     uint64_t pd_idx = (virt >> 21) & 0x1FF;

//     uint64_t *pdpt = get_next_level(pml4, pml4_idx);
//     if (pdpt[pdpt_idx] & PT_HUGE) PANIC("VMM: Reached 1GB huge page in PDPT during page traversal.");

//     uint64_t *pd = get_next_level(pdpt, pdpt_idx);

//     pd[pd_idx] = (phys & PAGE_2MB_MASK) | flags | PT_HUGE;
// }
//
// void map_2mb_range_with_offset(uint64_t *pml4, uint64_t start, uint64_t size, uint64_t offset, uint64_t flags) {
//     uint64_t first_huge_page = start & ~0x1FFFFFULL;
//     uint64_t last_huge_page = (start + size + 0x1FFFFFULL) & ~0x1FFFFFULL;

//     for (uint64_t addr = first_huge_page; addr < last_huge_page; addr += 0x200000) {
//         map_2mb(pml4, addr + offset, addr, flags);
//     }
// }
