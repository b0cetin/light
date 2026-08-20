
#include "elf_loader.h"
#include "debugging.h"
#include "kernel_lib.h"
#include "pmm.h"
#include "types.h"
#include "vmm.h"
#include <stdint.h>

// Identifier flags
#define ELF_CLASS_64_BIT 2
#define ELF_ENDIANNESS_LITTLE_ENDIAN 1
#define ELF_VERSION 1
#define ELF_OS_ABI_SYSTEMV 0

// Other constants
#define ELF_MACHINE_AMD64 0x3E

typedef enum : uint16_t {
    ELFTYPE_RELOCATABLE = 1,
    ELFTYPE_EXECUTABLE = 2,
    EFLTYPE_SHARED = 3,
    ELFTYPE_CORE = 4,
} ELFType;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t class;
    uint8_t endianness;
    uint8_t version;
    uint8_t os_abi;

    uint64_t padding; // ABI version ignored and merged into here.
} ELFHeaderIdentifier;

typedef struct __attribute__((packed)) {
    ELFHeaderIdentifier identifier;

    ELFType type;
    uint16_t machine;
    uint32_t version;

    uint64_t entry;

    uint64_t program_header_table_offset;
    uint64_t section_header_table_offset;

    uint32_t flags; // Can be ignored.

    uint16_t header_size;
    uint16_t program_header_table_entry_size;
    uint16_t program_header_table_entry_count;
    uint16_t section_header_table_entry_size;
    uint16_t section_header_table_entry_count;

    uint16_t section_header_string_table_entry_index;
} ELFHeader;

typedef enum : uint32_t {
    ELF_PT_NULL = 0,
    ELF_PT_LOAD = 1,
    ELF_PT_DYNAMIC = 2,
    ELF_PT_INTERPRET = 3,
    ELF_PT_NOTE = 4,
} ELFProgramEntryType;

// Program Entry Flags
#define PROGRAM_ENTRY_FLAG_EXECUTABLE 0x1
#define PROGRAM_ENTRY_FLAG_WRITABLE 0x2
#define PROGRAM_ENTRY_FLAG_READABLE 0x4

typedef struct __attribute__((packed)) {
    ELFProgramEntryType type;
    uint32_t flags;

    uint64_t data_offset;

    uint64_t virtual_address;
    uint64_t physical_address; // On environments where physical address is relevant. (not here)

    uint64_t file_size;
    uint64_t mem_size;

    uint64_t alignment;
} ELFProgramHeader;

bool check_elf_validity(void *elf, size_t length) {
    if (length < sizeof(ELFHeaderIdentifier)) return false;

    uint8_t *magic = elf;

    if (magic[0] != 0x7F || magic[1] != 'E' || magic[2] != 'L' || magic[3] != 'F') {
        kprintln("ELF: Cannot load elf file as the magic doesn't match.");
        return false;
    }

    {
        ELFHeaderIdentifier *identifier = elf;

        if (identifier->class != ELF_CLASS_64_BIT)
        {
            kprintln("ELF: Cannot load elf file as it's not 64-bit.");
            return false;
        }

        if (identifier->endianness != ELF_ENDIANNESS_LITTLE_ENDIAN)
        {
            kprintln("ELF: Cannot load elf file as it's not little endian.");
            return false;
        }

        if (identifier->version != ELF_VERSION)
        {
            kprintln("ELF: Cannot load elf file because of unknown ELF version.");
            return false;
        }

        if (identifier->os_abi != ELF_OS_ABI_SYSTEMV)
        {
            kprintln("ELF: Cannot load elf file as it follows the wrong OS ABI.");
            return false;
        }
    }

    ELFHeader *header = elf;

    if (header->type != ELFTYPE_EXECUTABLE)
    {
        kprintln("ELF: Cannot load elf file as it's not an executable.");
        return false;
    }

    if (header->machine != ELF_MACHINE_AMD64)
    {
        kprintln("ELF: Cannot load elf file as it's compiled for an incompatible machine architecture.");
        return false;
    }

    if (header->version != ELF_VERSION)
    {
        kprintln("ELF: Cannot load elf file because of unknown ELF version.");
        return false;
    }

    return true;
}

// Returns the entry point according to the given user address space.
void *load_elf(PML4 *user_address_space, void *content, size_t len) {
    if (!check_elf_validity(content, len)) return null;

    if (len < sizeof(ELFHeader)) return null;
    ELFHeader *header = content;

    if (len < header->program_header_table_offset) return null;
    ELFProgramHeader *program_header = (ELFProgramHeader*)((char*)content + header->program_header_table_offset);

    // TODO: Keep track of pages allocated to this process.
    // The PML4 already does that in some way, but a dedicated
    // method would probably be a more ideal approach.

    if (len < (uint64_t) header->program_header_table_entry_size * header->program_header_table_entry_count
        + header->program_header_table_offset)
        return null;
    
    if (header->program_header_table_entry_size != sizeof(ELFProgramHeader))
        return null;

    for (uint16_t i = 0; i < header->program_header_table_entry_count; i++, program_header = (ELFProgramHeader*) ((char*)program_header + header->program_header_table_entry_size)) {
        switch (program_header->type) {
            case ELF_PT_NULL: continue;
            case ELF_PT_NOTE: continue;
            case ELF_PT_INTERPRET:
                kprintln("ELF: Program table entry PT_INTERPRET found. Skipping...");
                continue;
            case ELF_PT_DYNAMIC:
                PANIC("Elf program requests dynamic linking. Currently not supported. This is a panic and not a warning because I haven't implemented cleaning up on error.");
                return null;
            case ELF_PT_LOAD: {
                if (program_header->mem_size == 0) continue;

                uint64_t mapping_flags = PT_USER;

                if (!(program_header->flags & PROGRAM_ENTRY_FLAG_EXECUTABLE)) mapping_flags |= PT_NX;
                if (program_header->flags & PROGRAM_ENTRY_FLAG_WRITABLE) mapping_flags |= PT_RW;

                uint64_t aligned_start = program_header->virtual_address & ~0xFFF;
                uint64_t aligned_end = ((program_header->virtual_address + program_header->mem_size) + 0xFFF) & ~0xFFF;
                uint64_t num_pages = (aligned_end - aligned_start) / 4096;
                uint64_t page_offset = program_header->virtual_address & 0xFFFUL;

                uint64_t bytes_copied = 0;

                uint64_t end;
                if (__builtin_add_overflow(program_header->data_offset, program_header->file_size, &end))
                    return null; // Guard against program sections that are out of range.
                if (len < end) return null;

                if (program_header->virtual_address + program_header->mem_size < program_header->virtual_address)
                    return null; // Check for address overflows

                if (program_header->file_size > program_header->mem_size)
                    return null; // File size can never exceed memory size

                for (uint64_t i = 0; i < num_pages; i++) {
                    uint64_t physical_address = pmm_alloc_page();
                    uint64_t address = aligned_start + i * PAGE_SIZE;

                    vmm_map(user_address_space, address, physical_address, mapping_flags);
                    memzero(p2v(physical_address), PAGE_SIZE);

                    kprintln("ELF: Allocating page from %lx, mapped to %lx", address, aligned_start + i * PAGE_SIZE);

                    if (bytes_copied < program_header->file_size) {
                        uint64_t dest_offset = i == 0 ? page_offset : 0;
                        uint64_t amount_to_copy = PAGE_SIZE - dest_offset;

                        memcpy(p2v(physical_address) + dest_offset, (uint8_t*) ((char*)content + program_header->data_offset) + bytes_copied, amount_to_copy);
                        
                        bytes_copied += amount_to_copy;
                    }
                    else {
                        kprintln("ELF: No data on file to copy.");
                    }
                }

                break;
            }
        }
    }

    kprintln("ELF: Successfully loaded ELF file.");

    return (void*) header->entry;
}
