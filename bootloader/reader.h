
#pragma once

#include <efi.h>

typedef struct ReadResult {
    EFI_STATUS status;
    void *location;
    UINT64 size;
} ReadResult;

EFI_STATUS open_volume(EFI_HANDLE image, EFI_FILE_HANDLE *out_handle);
ReadResult read_into_buffer(CHAR16 *path, EFI_FILE_HANDLE volume);
