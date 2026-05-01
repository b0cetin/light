
#pragma once

#include "interrupts.h"

#define EXCEPTION_DIVISION 0x00
#define EXCEPTION_INVALID_OPCODE 0x06
#define EXCEPTION_DOUBLE_FAULT 0x08
#define EXCEPTION_GENERAL_PROTECTION_FAULT 0x0D
#define EXCEPTION_PAGE_FAULT 0x0E

void handle_cpu_exception(InterruptRegisters *regs);
