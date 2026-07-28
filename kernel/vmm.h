
#pragma once

#include <stdint.h>

typedef uint64_t PLM4;

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

#define PAGE_SIZE 4096

void vmm_init();

#define HHDM_OFFSET 0xFFFF800000000000ULL
static inline void *p2v(uint64_t phys) {
    return (void *)(phys + HHDM_OFFSET);
}
static inline uint64_t v2p(void *virt) {
    return (uint64_t)virt - HHDM_OFFSET;
}

void vmm_map(PLM4 *pml4, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_kmap_mmio(uint64_t phys, uint64_t size);

PLM4 *vmm_create_user_address_space();
void vmm_destroy_user_address_space(PLM4 *plm4);

void vmm_switch_to_user_address_space(PLM4 *plm4);
void vmm_switch_to_kernel_address_space();
