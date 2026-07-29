.extern syscall_handler

.global syscall_entry
syscall_entry:
    swapgs # Swap whatever the user application used there.
    movq %rsp, %gs:8
    movq %gs:0, %rsp

    # TODO: Consider interrupts in the future.

    # SYSCALL puts user RIP into RCX and user RFLAGS into R11
    pushq %r11 # Preserve user RFLAGS
    pushq %rcx # Preserve user RIP
    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # The additional registers above are expected to be saved
    # and protected after the syscall.

    movq %r8, %r9
    movq %r10, %r8
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq %rax, %rdi

    # The syscall is called with x86-64 ABI (Standard), but the
    # function expects System V AMD64 C ABI, so we shift
    # the registers.

    # System V C ABI expects argument 4 in RCX.
    # SYSCALL used RCX for RIP, so incoming user arg 4 was sent in R10.
    # We move R10 to RCX before calling our C function.

    movq %r10, %rcx             
    call syscall_handler

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    popq %rcx # Restore user RIP
    popq %r11 # Restore user RFLAGS

    movq %gs:8, %rsp
    swapgs
    sysretq
