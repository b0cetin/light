
#pragma once

#include <syscalls.h>

#define CLIENT_REGISTRY_POLL_CALL_NUM UINT64_MAX

void client_registry_respond(pid_t client);
