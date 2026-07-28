
#include "bootinfo.h"
#include "elf_loader.h"
#include "reader.h"
#include "virtual_mapper.h"
#include <efi.h>
#include <efilib.h>
#include <elf.h>
#include <stdint.h>

void exit_boot_services(EFI_HANDLE ImageHandle, BootInfo *bootInfo) {
    EFI_STATUS status = EFI_SUCCESS;

    UINTN MemoryMapSize = 0;
    EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;

    do {
        if (EFI_ERROR(status)) {
            // If it failed, the MapKey is invalid.
            // You MUST get the memory map again and retry ExitBootServices once.
            Print(L"ExitBootServices failed: %r. Retrying...\n", status);
        }

        // 1. Ask for the size required
        status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MemoryMapSize, NULL, &MapKey, &DescriptorSize, &DescriptorVersion);

        // 2. Add some "wiggle room" (the allocation itself increases the map size)
        MemoryMapSize += 2 * DescriptorSize;
        MemoryMap = AllocatePool(MemoryMapSize);

        // 3. Get the actual map
        status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
        if (EFI_ERROR(status)) {
            Print(L"Failed to get memory map: %r\n", status);
            while (1)
                ;
        }

        status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, MapKey);
    } while (EFI_ERROR(status));

    bootInfo->MMap = MemoryMap;
    bootInfo->MMapSize = MemoryMapSize;
    bootInfo->DescriptorSize = DescriptorSize;
}

EFI_STATUS get_gop(EFI_GRAPHICS_OUTPUT_PROTOCOL **gop) {
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_STATUS status;

    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gop_guid, NULL, (void **)gop);
    if (EFI_ERROR(status)) {
        Print(L"Unable to locate GOP: %r\n", status);
    }

    return status;
}

void set_gop_into_boot_info(BootInfo *bootInfo) {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;

    if (get_gop(&gop) == EFI_SUCCESS) {
        bootInfo->PhysicalFramebufferBase = gop->Mode->FrameBufferBase;
        bootInfo->FramebufferSize = gop->Mode->FrameBufferSize;
        bootInfo->HorizontalResolution = gop->Mode->Info->HorizontalResolution;
        bootInfo->VerticalResolution = gop->Mode->Info->VerticalResolution;
        bootInfo->PixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;

        Print(L"Framebuffer base: %llx\n", bootInfo->PhysicalFramebufferBase);
    }
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    uint64_t kernel_size;
    void *kernel_buf;

    Print(L"Reading kernel...\n");
    read_kernel_into_buffer(ImageHandle, &kernel_buf, &kernel_size);

    Print(L"Beginning loading...\n");
    UINT64 kernel_entry_ptr = load_elf_file(kernel_buf);

    Print(L"Beginning custom memory mapping for high offset...\n");
    calculate_custom_virtual_mappings();

    Print(L"Beginning boot info construction...\n");
    BootInfo bootInfo;

    set_gop_into_boot_info(&bootInfo);

    Print(L"Jumping to kernel entry at 0x%llx\n", kernel_entry_ptr);

    exit_boot_services(ImageHandle, &bootInfo);

    bootInfo.PhysicalKernelBase = kernel_buf;

    enable_custom_virtual_mappings();

    BootInfo *boot_info_ptr = (BootInfo *)((uint64_t)&bootInfo + HHDM_OFFSET);

    __asm__ volatile("cli\n\t"
                     "mov %0, %%rdi\n\t" // Move BootInfo pointer to RDI (System V ABI)
                     "jmp *%1"           // Raw jump to kernel entry
                     :
                     : "r"(boot_info_ptr), "r"(kernel_entry_ptr)
                     : "rdi");

    return EFI_LOAD_ERROR;
}
