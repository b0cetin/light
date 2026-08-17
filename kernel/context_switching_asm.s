
# switch_to_context_immediately(uint64_t rsp)

.global switch_to_context_immediately
switch_to_context_immediately:
    # Argument provides the stack pointer to switch to
    movq %rdi, %rsp
    
    movb $0x20, %al
    outb %al, $0x20

    # Restore registers
    popq %rax; popq %rbx; popq %rcx; popq %rdx; popq %rsi; popq %rdi; popq %rbp
    popq %r8;  popq %r9;  popq %r10; popq %r11; popq %r12; popq %r13; popq %r14; popq %r15

    # Return from interrupt
    iretq
