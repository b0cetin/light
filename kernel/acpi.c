
#include "acpi.h"
#include "allocator.h"
#include "bootinfo.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

typedef struct {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_address; // Deprecated since RDSP

    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__ ((packed)) XSDP;

static char* sdp_signature = "RSD PTR ";

typedef struct {
  ACPISDTHeader header;
  uint64_t entries[];
} __attribute__((packed)) XSDT;

XSDT *xsdt;

void acpi_init(BootInfo *boot_info) {
    XSDP *xdsp = p2v(boot_info->physical_xsdp_address);

    if (memcmp(xdsp->signature, sdp_signature, 8) != 0)
        PANIC("ACPI SDP signature check failed.");

    uint8_t checksum = 0;
    for (uint8_t i = 0; i < 20; i++) {
        checksum += *(((uint8_t*) xdsp) + i);
    }

    if (checksum != 0)
        PANIC("ACPI RSDP checksum isn't valid!");

    if (xdsp->revision != 2)
        PANIC("ACPI RSDP revision invalid: %d, expected 2.", xdsp->revision);

    for (uint8_t i = 0; i < sizeof(XSDP); i++) {
        checksum += *(((uint8_t*) xdsp) + i);
    }

    if (checksum != 0)
        PANIC("ACPI XSDP checksum isn't valid!");

    xsdt = p2v(xdsp->xsdt_address);

    kprintln("ACPI: XSDT declared at %lx.", xsdt);
}

static bool checksum(ACPISDTHeader *tableHeader)
{
    uint8_t sum = 0;

    for (uint32_t i = 0; i < tableHeader->length; i++)
        sum += ((uint8_t *) tableHeader)[i];

    return sum == 0;
}

ACPISDTHeader *acpi_find_table(const char (*signature)[4]) {
    uint32_t entry_count = (xsdt->header.length - sizeof(ACPISDTHeader)) / 8;

    for (uint32_t i = 0; i < entry_count; i++) {
        ACPISDTHeader *header = p2v(xsdt->entries[i]);
        
        if (memcmp(header->signature, signature, sizeof(header->signature)) == 0) {
            if (checksum(header))
                return header;
            else {
                char *printout = kmalloc(5);
                memcpy(printout, signature, 4);
                kprintln("ACPI: Checksum for table \"%s\" in XSDT failed!", printout);
                kfree(printout);
            }
        }
    }

    return null;
}
