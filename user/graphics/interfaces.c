
#include "interface.h"
#include "stdio.h"
#include <stdint.h>
#include <stdbool.h>

ServerInterface *interfaces[UINT8_MAX]; // A dynamically-resizible array (or hashmap) needed

void register_interface(ServerInterface *interface) {
    uint16_t id = interface->interface_id;

    if (id > UINT8_MAX) {
        println("register_interface: interface ID too big: %hu", id);
        return;
    }

    if (interfaces[id] != NULL) {
        println("register_interface: interface with ID %hu is already registered", id);
        return;
    }

    interfaces[id] = interface;

    println("Interface \"%s\" registered at ID %hu.", interface->name, id);
}

bool receive_call(pid_t caller, uint64_t header, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t *out) {
    uint16_t interface_id = header & (UINT64_C(0xFFFF) << (16 * 3));

    if (interfaces[interface_id] == NULL)
        return false;

    uint16_t call_num = header & (UINT64_C(0xFFFF) << (16 * 2));
    uint32_t object_id = header & (0xFFFFFFFF);

    uint64_t result = 0;
    if (!interfaces[interface_id]->call(caller, call_num, object_id, arg0, arg1, arg2, arg3, &result))
        return false;

    if (out != NULL) *out = result;
    return true;
}
