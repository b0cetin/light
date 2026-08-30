
#pragma once

#include "bootinfo.h"
#include <stdint.h>

typedef struct {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oemid[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__ ((packed)) ACPISDTHeader;

void acpi_init(BootInfo *boot_info);

ACPISDTHeader *acpi_find_table(const char (*signature)[4]);
