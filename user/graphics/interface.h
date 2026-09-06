
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <syscalls.h>

typedef bool (*InterfaceCall) (pid_t caller, uint16_t call, uint32_t object, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t *out);

typedef struct {
    uint16_t interface_id;
    char *name;
    InterfaceCall call;
} ServerInterface;

extern void register_interface(ServerInterface *interface);
extern bool receive_call(pid_t caller, uint64_t header, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t *out);
