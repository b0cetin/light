
#pragma once

#include "vmm.h"
#include <stddef.h>

void *load_elf(PML4 *user_address_space, void *content, size_t len);
