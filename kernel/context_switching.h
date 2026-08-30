
#pragma once

#include "processes.h"

void ctx_switching_switch_next_destructive();
void ctx_switching_switch_to_now(Thread *target);

void ctx_switching_init(Thread *init_thread, Thread *idle_thread);
Thread *ctx_switching_get_active_thread();
