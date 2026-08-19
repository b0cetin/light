
#pragma once

#include "vmm.h"

void *load_elf(PML4 *user_address_space, void *content);
