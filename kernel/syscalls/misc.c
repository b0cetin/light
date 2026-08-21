
#include "defined_syscalls.h"
#include "../types.h"
#include "../debugging.h"
#include "../kernel_lib.h"
#include "../allocator.h"

#define PRINT_RREFIX "USER: "
int64_t sys_print(const char* buf) {
    if (buf == null) {
        kprintln("SYSCALLS: sys_print called with null as buffer!");
        return -1;
    }

    size_t len = strnlen(buf, UINT8_MAX);

    char *str = kmalloc(len + sizeof(PRINT_RREFIX) + 1);

    memcpy(str, PRINT_RREFIX, sizeof(PRINT_RREFIX));
    memcpy(str + sizeof(PRINT_RREFIX) - 1, buf, len);

    kprintln(str);

    kfree(str);
    return 0;
}
