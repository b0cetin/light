.global set_stack_and_jump
set_stack_and_jump:
    mov %rdi, %rsp   # Set new stack
    mov %rsi, %rdi   # Pass boot_info to the next function (RSI -> RDI)

    pushq $0 # GCC expects us to provide a return address in the stack.

    jmp *%rdx        # Jump to the actual kernel main
