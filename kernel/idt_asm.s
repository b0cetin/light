.altmacro

# A macro for interrupts that don't have an error code
.macro ISR_NOERRCODE num
.global isr_\num
isr_\num:
    pushq $0 # Push a dummy error code (0) to keep stack consistent
    pushq $\num
    jmp isr_common
.endm

# A macro for interrupts that do have an error code (CPU pushes it)
.macro ISR_ERRCODE num
.global isr_\num
isr_\num:
    # Error code is already on the stack.
    pushq $\num
    jmp isr_common
.endm

# A macro for context switching timer on 32
.macro ISR_CTX_SWITCH
.global isr_32
isr_32:
    # No codes.
    jmp isr_ctx_switch
.endm

# A macro for manual context switching on 0x81
.macro ISR_CTX_SWITCH_MANUAL
.global isr_129
isr_129:
    # No codes.
    jmp isr_ctx_switch_manual
.endm

# Stubs
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31
ISR_CTX_SWITCH
.set i, 33
.rept 96
    ISR_NOERRCODE %i
    .set i, i+1
.endr
ISR_CTX_SWITCH_MANUAL
.set i, 130
.rept 126
    ISR_NOERRCODE %i
    .set i, i+1
.endr

.macro PUSH_CONTEXT
    pushq %r15; pushq %r14; pushq %r13; pushq %r12; pushq %r11; pushq %r10; pushq %r9; pushq %r8
    pushq %rbp; pushq %rdi; pushq %rsi; pushq %rdx; pushq %rcx; pushq %rbx; pushq %rax
.endm

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

# C handlers
.extern isr_handler
.extern isr_context_switch
.extern lapic_end_of_interrupt

isr_common:
    cld # Clear Direction Flag for ABI compliance

    PUSH_CONTEXT
    SWAPGS_IF_USER 17

    # Pass the stack pointer to C
    movq %rsp, %rdi
    call isr_handler

    SWAPGS_IF_USER 17
    POP_CONTEXT

    addq $16, %rsp # Clean up error code and interrupt number
    iretq

isr_ctx_switch:
    cld # Clear Direction Flag for ABI compliance

    PUSH_CONTEXT
    SWAPGS_IF_USER 15

    # Pass the stack pointer to C
    movq %rsp, %rdi
    call isr_context_switch

    pushq %rax
    call lapic_end_of_interrupt
    popq %rsp # C function returns the stack pointer to switch to

    SWAPGS_IF_USER 15
    POP_CONTEXT
    iretq

isr_ctx_switch_manual:
    cld # Clear Direction Flag for ABI compliance

    PUSH_CONTEXT
    SWAPGS_IF_USER 15

    # Pass the stack pointer to C
    movq %rsp, %rdi
    call isr_context_switch
    movq %rax, %rsp # C function returns the stack pointer to switch to

    SWAPGS_IF_USER 15
    POP_CONTEXT
    iretq

.macro ISR_STUB n
    .quad isr_\n
.endm

.global isr_stub_table
isr_stub_table:
    .set i, 0
    .rept 256
        ISR_STUB %i
        .set i, i+1
    .endr
