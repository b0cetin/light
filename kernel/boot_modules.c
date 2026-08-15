
#include "boot_modules.h"
#include "bootinfo.h"
#include "debugging.h"
#include "kernel_lib.h"

ReadModule *modules;

void boot_modules_init(BootInfo *boot_info) {
    kprintln("BOOT: Listing loaded modules now.");

    for (int i = 0; i < READ_MODULE_COUNT; i++) {
        ReadModule *this = &boot_info->modules[i];

        if (!this->is_read) continue;

        char module_path[50];

        convert_utf16_to_ascii(this->path, module_path);

        kprintln("BOOT: Module %d: %s", i, module_path);
    }

    modules = boot_info->modules;

    kprintln("Boot modules initialized.");
}

ReadModule *boot_modules_get_all() {
    return modules;
}
