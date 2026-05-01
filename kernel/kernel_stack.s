.global set_stack_and_jump
set_stack_and_jump:
    mov %rdi, %rsp   # Set new stack
    mov %rsi, %rdi   # Pass boot_info to the next function (RSI -> RDI)
    jmp *%rdx        # Jump to the actual kernel main
