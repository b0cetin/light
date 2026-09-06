
#include "interface.h"
#include "interfaces/interfaces.h"
#include "syscalls.h"
#include <stdio.h>

static void log(const char * restrict format, ...) {
    char buffer[205] = { 'S', 'H', 'M', ':', ' ' };

    va_list args;
    va_start(args, format);
    vsnprintf(&buffer[sizeof(buffer) - 200], sizeof(buffer), format, args);
    va_end(args);

    sys_print(buffer);
}

static void log_error(const char * restrict format, ...) {
    char buffer[210] = { 'S', 'H', 'M', ':', ' ', 'E', 'R', 'R', ':', ' ' };

    va_list args;
    va_start(args, format);
    vsnprintf(&buffer[sizeof(buffer) - 200], sizeof(buffer), format, args);
    va_end(args);

    sys_print(buffer);
}

extern void exit(int64_t status);

static ServerInterface interface_record;
static char* name = "compositor";

static bool call (pid_t caller, uint16_t call, uint32_t object, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t *out) {
    log("aa");
    log_error("aa");
    switch (call) {
    default:
        return false;
    }
}

ServerInterface *interfaces_compositor() {
    interface_record.interface_id = INTERFACE_ID_COMPOSITOR;
    interface_record.name = name;
    interface_record.call = call;
    return &interface_record;
}
