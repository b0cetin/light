
#pragma once

#include "../types.h"
#include <stddef.h>
#include <stdint.h>

bool is_valid_mapped_user_range(uintptr_t ptr, size_t size);
bool is_valid_mappable_user_range(uintptr_t ptr, size_t size);
