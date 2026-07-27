
#include "cpu_exceptions.h"
#include "debugging.h"
#include <stdint.h>

void gp_fault() {
    kernel_println("EXCEPTION: General Protection Fault!");
    // TODO: Print details.
}

void page_fault(InterruptRegisters *regs) {
    kernel_println("Page fault!\n");

    // For Page Faults, the CR2 register holds the address that caused the crash
    uint64_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));
    kernel_println("Fault address: %lx", fault_addr);

    uint64_t error_code = regs->error_code;

    kernel_println("Error code: %ld", error_code);
    kernel_println("Present: %d     Write:      %d     User:       %d", error_code & 0b1, (error_code & 0b10) > 0 ? 1 : 0, (error_code & 0b100) > 0 ? 1 : 0);
    kernel_println("RWrite:  %d     INSTRFetch: %d     ProtectKey: %d", (error_code & 0b1000) > 0 ? 1 : 0, (error_code & 0b10000) > 0 ? 1 : 0,
                   (error_code & 0b100000) > 0 ? 1 : 0);
    kernel_println("ShStack: %d     SGX:        %d", (error_code & 0b1000000) > 0 ? 1 : 0, (error_code & 0x0E) > 0 ? 1 : 0);
    kernel_println("\nSGX: Software Guard Extensions");
    kernel_println("If this bit is set, it is most likely because the software tried to access unmapped memory.");
}

void handle_cpu_exception(InterruptRegisters *regs) {
    kernel_println("\nCPU EXCEPTION: From instruction at %lx", regs->rip);

    switch (regs->interrupt_number) {
    case EXCEPTION_DIVISION:
        kernel_println("Division Error: A DIV or IDIV instruction occured with divisor being zero, or the division result was too big to be stored.");
        break;
    case EXCEPTION_INVALID_OPCODE:
        kernel_println("Invalid Opcode: Tried to execute an invalid or undefined opcode, or an instruction with invalid prefixes.");
        break;
    case EXCEPTION_DOUBLE_FAULT:
        PANIC("Double Fault: A CPU exception wasn't handled by the IDT, or calling exception handler failed. Unrecoverable.");
        break;
    case EXCEPTION_GENERAL_PROTECTION_FAULT:
        gp_fault();
        break;
    case EXCEPTION_PAGE_FAULT:
        page_fault(regs);
        break;
    default:
        kernel_println("Unhandled CPU exception: %d", regs->interrupt_number);
        break;
    }

    PANIC("Fatal CPU exception from interrupt %d", regs->interrupt_number);
}
