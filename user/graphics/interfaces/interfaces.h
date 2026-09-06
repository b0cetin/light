
#pragma once

#include "interface.h"

#define INTERFACE_ID_COMPOSITOR 0
#define INTERFACE_ID_SHARED_MEMORY 1

ServerInterface *interfaces_shared_memory();
ServerInterface *interfaces_compositor();
