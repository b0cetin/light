
#include "context_switching.h"
#include "defined_syscalls.h"
#include "../types.h"
#include "../debugging.h"

int64_t sys_print(const char* buf) {
    if (buf == null) {
        kprintln("SYSCALLS: sys_print called with null as buffer!");
        return -1;
    }

    char *name = ctx_switching_get_active_thread()->owner->name;
    kprintln("USER \"%s\": %s", name, buf);

    return 0;
}
