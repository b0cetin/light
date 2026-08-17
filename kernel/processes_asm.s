.extern kernel_stack_top

# void processes_exit_trampoline(void *function, 2 64-bit args)
.global processes_exit_trampoline
processes_exit_trampoline:
    cli

    movq %rdi, %rax # function

    movq %rsi, %rdi; movq %rdx, %rsi # shift arguments

    movq kernel_stack_top(%rip), %rsp
    andq $-16, %rsp # Force 16-byte alignment if needed (it shouldn't be.)

    call *%rax

    hlt # Should not be reached.
