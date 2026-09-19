
#pragma once

#include <stdint.h>

#define NULL_HANDLE 0
typedef uint64_t HandleID;

#define NULL_KOBJECT 0
typedef uint64_t KernelObjectID;

typedef uint64_t PID;
#define PROCESS_INIT_PID 0
#define PROCESS_IDLE_PID UINT64_MAX
