
#include "reader.h"
#include <efi.h>
#include <efilib.h>
#include <elf.h>

EFI_STATUS open_volume(EFI_HANDLE image, EFI_FILE_HANDLE *out_handle) {
    EFI_STATUS status;

    EFI_LOADED_IMAGE *loaded_image;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_HANDLE volume;

    status = uefi_call_wrapper(BS->HandleProtocol, 3, image, &gEfiLoadedImageProtocolGuid, (void **)&loaded_image);
    if (EFI_ERROR(status)) {
        Print(L"LoadedImage failed: %r\n", status);
        return status;
    }

    status = uefi_call_wrapper(BS->HandleProtocol, 3, loaded_image->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void **)&fs);
    if (EFI_ERROR(status)) {
        Print(L"SimpleFS failed: %r\n", status);
        return status;
    }

    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &volume);
    if (EFI_ERROR(status)) {
        Print(L"OpenVolume failed: %r\n", status);
        return status;
    }

    Print(L"Volume opened.\n");
    
    *out_handle = volume;
    return EFI_SUCCESS;
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

ReadResult read_into_buffer(CHAR16 *path, EFI_FILE_HANDLE volume) {
    EFI_STATUS status;

    EFI_FILE_HANDLE kernel_handle;

    Print(L"Opening file: %s\n", path);
    status = uefi_call_wrapper(volume->Open, 5, volume, &kernel_handle, path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        Print(L"Could not open file: %r\n", status);
        return (ReadResult) { status, 0, 0 };
    }

    UINT64 read_size = file_size(kernel_handle);
    
    void *location = AllocatePool(read_size);
    if (location == NULL) {
        Print(L"Allocation failed!\n");
        return (ReadResult) { EFI_LOAD_ERROR, 0, 0 };
    }

    status = uefi_call_wrapper(kernel_handle->Read, 3, kernel_handle, &read_size, location);
    if (EFI_ERROR(status)) {
        Print(L"Could not read kernel file: %r\n", status);
        return (ReadResult) { status, 0, 0 };
    }

    status = uefi_call_wrapper(kernel_handle->Close, 1, kernel_handle);
    if (EFI_ERROR(status)) {
        Print(L"Could not close kernel file: %r\n", status);
        return (ReadResult) { status, 0, 0 };
    }

    Print(L"Read at %llx, size %lld.\n", location, read_size);
    return (ReadResult) { EFI_SUCCESS, location, read_size };
}
