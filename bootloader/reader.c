
#include <efi.h>
#include <efilib.h>
#include <elf.h>

EFI_FILE_HANDLE get_volume(EFI_HANDLE image) {
    EFI_STATUS status;

    EFI_LOADED_IMAGE *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_HANDLE volume;

    Print(L"Getting LoadedImage...\n");
    status = uefi_call_wrapper(BS->HandleProtocol, 3, image, &gEfiLoadedImageProtocolGuid, (void **)&loaded_image);
    if (EFI_ERROR(status)) {
        Print(L"LoadedImage failed: %r\n", status);
        while (1)
            ;
    }

    Print(L"Getting SimpleFS...\n");
    status = uefi_call_wrapper(BS->HandleProtocol, 3, loaded_image->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&fs);
    if (EFI_ERROR(status)) {
        Print(L"SimpleFS failed: %r\n", status);
        while (1)
            ;
    }

    Print(L"Opening volume...\n");
    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &volume);
    if (EFI_ERROR(status)) {
        Print(L"OpenVolume failed: %r\n", status);
        while (1)
            ;
    }

    return volume;
}

UINT64 file_size(EFI_FILE_HANDLE FileHandle) {
    UINT64 ret;
    EFI_FILE_INFO *FileInfo; /* file information structure */
    /* get the file's size */
    FileInfo = LibFileInfo(FileHandle);
    ret = FileInfo->FileSize;
    FreePool(FileInfo);
    return ret;
}

void read_kernel_into_buffer(EFI_HANDLE ImageHandle, void **kernel_buf, UINT64 *kernel_size) {
    EFI_STATUS status;

    Print(L"Getting volume...\n");
    EFI_FILE_HANDLE volume = get_volume(ImageHandle);

    CHAR16 *kernel_name = L"\\kernel\\kernel.elf";
    EFI_FILE_HANDLE kernel_handle;

    Print(L"Opening file...\n");
    Print(kernel_name);
    Print(L"\n");
    status = uefi_call_wrapper(volume->Open, 5, volume, &kernel_handle, kernel_name, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        Print(L"Could not open kernel file: %r\n", status);
        while (1)
            ;
    }

    UINT64 read_size = *kernel_size = file_size(kernel_handle);
    Print(L"Allocating pool...\n");
    *kernel_buf = AllocatePool(read_size);
    if (*kernel_buf == NULL) {
        Print(L"Allocation failed!\n");
        while (1)
            ;
    }

    Print(L"Reading...\n");
    status = uefi_call_wrapper(kernel_handle->Read, 3, kernel_handle, &read_size, *kernel_buf);
    if (EFI_ERROR(status)) {
        Print(L"Could not read kernel file: %r\n", status);
        while (1)
            ;
    }

    Print(L"Closing...\n");
    status = uefi_call_wrapper(kernel_handle->Close, 1, kernel_handle);
    if (EFI_ERROR(status)) {
        Print(L"Could not close kernel file: %r\n", status);
        while (1)
            ;
    }
}
