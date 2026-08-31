
#pragma once

#include "acpi.h"
#include <stdint.h>

// The end of this struct is followed with variable-length entries.
// The end of the MADT can be found by inspecting the `length` variable at the header.
typedef struct {
    ACPISDTHeader header;
    uint32_t physical_local_interrupt_controller_address;
    uint32_t flags;
} __attribute__ ((packed)) MADT;

typedef enum : uint8_t {
    MADT_TYPE_LAPIC = 0,
    MADT_TYPE_IOAPIC = 1,
    MADT_TYPE_IOAPIC_INTERRUPT_SOURCE_OVERRIDE = 2,
    MADT_TYPE_IOAPIC_NON_MASKABLE_INTERRUPT_SOURCE = 3,
    MADT_TYPE_LAPIC_NON_MASKABLE_INTERRUPTS = 4,
    MADT_TYPE_LAPIC_ADDRESS_OVERRIDE = 5,
    MADT_TYPE_LX2APIC = 9,
} MADTRecordType;

typedef struct {
    MADTRecordType type;
    uint8_t length;
} __attribute__ ((packed)) MADTRecordHeader;

#define LAPIC_ENABLED 0x1
#define LAPIC_ONLINE_CAPABLE (0x1 << 1)
typedef struct {
    MADTRecordHeader header;
    uint8_t processor_uid;
    uint8_t apic_id;
    uint8_t flags;
} __attribute__ ((packed)) MADTLAPICRecord;

typedef struct {
    MADTRecordHeader header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__ ((packed)) MADTIOAPICRecord;

#define MADT_FLAGS_ACTIVE_HIGH_OVERRIDE 0b01
#define MADT_FLAGS_ACTIVE_LOW_OVERRIDE 0b11
#define MADT_FLAGS_ACTIVE_EDGE_TRIGGERED 0b0100
#define MADT_FLAGS_ACTIVE_LEVEL_TRIGGERED 0b1100
typedef struct {
    MADTRecordHeader header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__ ((packed)) MADTIOAPICInterruptSourceOverrideRecord;

typedef struct {
    MADTRecordHeader header;
    uint8_t reserved[2];
    uint64_t physical_address_override;
} __attribute__ ((packed)) MADTLAPICAddressOverrideRecord;

static inline MADT *acpi_find_madt() {
    return (MADT*) acpi_find_table(&((const char[4]) { 'A', 'P', 'I', 'C' }));
}
