
#pragma once

#include "bootinfo.h"
#include "types.h"
#include <stdint.h>

void pmm_mark_free(uint64_t physical_address);
void pmm_mark_used(uint64_t physical_address);
bool pmm_is_used(uint64_t physical_address);

void pmm_init(BootInfo *boot_info);

uint8_t *pmm_get_bitmap();
uint64_t pmm_get_bitmap_size();
uint64_t pmm_get_highest_phys_addr();
uint64_t pmm_get_available_memory_size();

void pmm_print_stats();

uint64_t pmm_alloc_page();
uint64_t pmm_alloc(int page_count);

void pmm_free_page(uint64_t physical_address);
void pmm_free(uint64_t physical_address, int page_count);
