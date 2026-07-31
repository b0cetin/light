
#pragma once
#include "bootinfo.h"

void boot_modules_init(BootInfo *boot_info);

ReadModule *boot_modules_get_all();
