
.global enter_userspace
.type enter_userspace, @function

# System V ABI calling convention:
# %rdi = kernel_rsp

enter_userspace:
    movq %rdi, %rsp

    # Restore registers
    popq %rax; popq %rbx; popq %rcx; popq %rdx; popq %rsi; popq %rdi; popq %rbp
    popq %r8;  popq %r9;  popq %r10; popq %r11; popq %r12; popq %r13; popq %r14; popq %r15

    iretq
