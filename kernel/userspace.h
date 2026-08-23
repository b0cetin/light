
#pragma once

#include <stdint.h>

#define PROCESS_INIT_PID 0
#define PROCESS_IDLE_PID UINT64_MAX

void start_first_user_process(void);
