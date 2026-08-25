
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

    bootInfo->memory_map.PhysicalMMapBase = (uint64_t) MemoryMap;
    bootInfo->memory_map.MMapSize = MemoryMapSize;
    bootInfo->memory_map.DescriptorSize = DescriptorSize;
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

void set_gop_into_boot_info(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, BootInfo *bootInfo) {
    bootInfo->framebuffer.PhysicalFramebufferBase = gop->Mode->FrameBufferBase;
    bootInfo->framebuffer.FramebufferSize = gop->Mode->FrameBufferSize;
    bootInfo->framebuffer.HorizontalResolution = gop->Mode->Info->HorizontalResolution;
    bootInfo->framebuffer.VerticalResolution = gop->Mode->Info->VerticalResolution;
    bootInfo->framebuffer.PixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;
}

ReadResult read_kernel_file(EFI_FILE_HANDLE volume) {
    ReadResult read_result = read_into_buffer(L"\\kernel\\kernel.elf", volume);
    if (EFI_ERROR(read_result.status))
    {
        Print(L"Error reading kernel ELF file.\n");
        while (1) {}
    }

    return read_result;
}

unsigned int get_size_of_string(const CHAR16 *str) {
    unsigned int size = 0;
    while (str[size] != '\0') {
        size++;
    }
    return size; // returns number of bytes
}

ReadResult read_file(EFI_FILE_HANDLE volume, CHAR16 *path, ReadModule read_modules[READ_MODULE_COUNT]) {
    ReadResult read_result = read_into_buffer(path, volume);
    if (EFI_ERROR(read_result.status))
    {
        Print(L"Error reading ELF file.\n");
        while (1) {}
    }

    int free_module = -1;

    for (int i = 0; i < READ_MODULE_COUNT; i++) {
        if (read_modules[i].is_read)
            continue;

        free_module = i;
        break;
    }

    if (free_module == -1) {
        Print(L"Tried to load too many files!\n");
        while (1) {}
    }

    read_modules[free_module].is_read = 1;
    read_modules[free_module].physical_location = (uint64_t) read_result.location;
    read_modules[free_module].size = read_result.size;

    uint32_t path_size = get_size_of_string(path);

    if (path_size > READ_MODULE_PATH_SIZE) {
        Print(L"File size is too big! %d > %d\n", path_size, READ_MODULE_PATH_SIZE);
        while (1) {}
    }

    uefi_call_wrapper(BS->CopyMem, 3, read_modules[free_module].path, path, path_size * 2);

    return read_result;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);

    EFI_FILE_HANDLE volume;
    if(EFI_ERROR(open_volume(ImageHandle, &volume)))
        while(1) {}

    ReadModule read_modules[READ_MODULE_COUNT] = {0};

    ReadResult kernel_result = read_kernel_file(volume);
    read_file(volume, L"\\user\\init.elf", read_modules);
    read_file(volume, L"\\user\\keyboard.elf", read_modules);
    read_file(volume, L"\\user\\graphics.elf", read_modules);

    Print(L"Loading kernel elf file...\n");
    UINT64 kernel_entry_ptr = load_elf_file(kernel_result.location);

    Print(L"Freeing kernel elf file...\n");
    uefi_call_wrapper(BS->FreePages, 2, kernel_result.location, ((kernel_result.size + 0xFFF) & ~0xFFF) / 4096);

    Print(L"Beginning boot info construction...\n");
    BootInfo bootInfo;
    uefi_call_wrapper(BS->CopyMem, 3, bootInfo.modules, read_modules, sizeof(read_modules));

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    if (EFI_ERROR(get_gop(&gop)))
        while(1) {}

    set_gop_into_boot_info(gop, &bootInfo);

    init_mapping();

    Print(L"Jumping to kernel entry at 0x%llx\n", kernel_entry_ptr);
    exit_boot_services(ImageHandle, &bootInfo);

    BootInfo *boot_info_ptr = (BootInfo *)((uint64_t)&bootInfo + HHDM_OFFSET);

    __asm__ volatile("cli\n\t"
                     "mov %0, %%rdi\n\t" // Move BootInfo pointer to RDI (System V ABI)
                     "jmp *%1"           // Raw jump to kernel entry
                     :
                     : "r"(boot_info_ptr), "r"(kernel_entry_ptr)
                     : "rdi");

    return EFI_LOAD_ERROR;
}
