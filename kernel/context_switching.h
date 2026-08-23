
#pragma once

#include "processes.h"

void ctx_switching_switch_next_immediate();

void ctx_switching_init(Thread *idle_thread);
Thread *ctx_switching_get_active_thread();
