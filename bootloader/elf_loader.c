
#include "elf_loader.h"
#include <efi.h>
#include <efilib.h>
#include <elf.h>

UINT64 load_elf_file(void *file_buf) {
    EFI_STATUS status;

    Print(L"file_buf address: 0x%p\n", file_buf);
    if (file_buf == NULL) {
        Print(L"Error: file_buf is NULL!\n");
        while (1)
            ;
    }

    Elf64_Ehdr *header = (Elf64_Ehdr *)file_buf; // buffer = the file you read

    Print(L"Checking validity...\n");
    if (header->e_ident[0] != 0x7F || header->e_ident[1] != 'E') {
        Print(L"Not a valid ELF file\n");
        return 0;
    }
    Print(L"Valid.\n");

    Elf64_Phdr *ph = (Elf64_Phdr *)((UINT8 *)file_buf + header->e_phoff);

    for (int i = 0; i < header->e_phnum; i++, ph++) {
        if (ph->p_type != PT_LOAD)
            continue;

        // Use UINT8* for math to avoid void* arithmetic issues
        UINT8 *src = (UINT8 *)file_buf + ph->p_offset;

        // Calculate how many pages we need, accounting for the offset within the first page
        uint64_t start_addr = ph->p_paddr;
        uint64_t end_addr = start_addr + ph->p_memsz;
        uint64_t aligned_start = start_addr & ~0xFFF;
        uint64_t aligned_end = (end_addr + 0xFFF) & ~0xFFF;
        uint64_t num_pages = (aligned_end - aligned_start) / 4096;

        EFI_PHYSICAL_ADDRESS segment = aligned_start;

        Print(L"Allocating %ld pages at 0x%llx\n", num_pages, aligned_start);

        status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, num_pages, &segment);

        if (EFI_ERROR(status)) {
            Print(L"Allocation failed: %r\n", status);
            while (1)
                ;
        }

        // Zero the entire allocated range first (covers .bss and alignment padding)
        uefi_call_wrapper(BS->SetMem, 3, (void *)aligned_start, num_pages * 4096, 0);

        // Copy the actual data to the EXACT p_vaddr
        Print(L"Copying %ld bytes from %p to 0x%llx\n", ph->p_filesz, src, ph->p_paddr);
        uefi_call_wrapper(BS->CopyMem, 3, (void *)ph->p_paddr, src, ph->p_filesz);
    }

    return header->e_entry;
}
