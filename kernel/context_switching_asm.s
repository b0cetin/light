
# switch_to_context_immediately(uint64_t rsp)

.macro POP_CONTEXT
    popq %rax; popq %rbx; popq %rcx; popq %rdx; popq %rsi; popq %rdi; popq %rbp
    popq %r8;  popq %r9;  popq %r10; popq %r11; popq %r12; popq %r13; popq %r14; popq %r15
.endm

.macro SWAPGS_IF_USER reg_count
    movq ((\reg_count * 8) + 8)(%rsp), %rax // Peak into SS, not RIP.
    test $0b11, %rax
    jz 1f
    swapgs
1:
.endm

.global switch_to_context_immediately
switch_to_context_immediately:
    # Argument provides the stack pointer to switch to
    movq %rdi, %rsp

    SWAPGS_IF_USER 15
    POP_CONTEXT

    iretq
