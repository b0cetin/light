
#pragma once

#include <efi.h>

void read_kernel_into_buffer(EFI_HANDLE ImageHandle, void **kernel_buf, UINT64 *kernel_size);
