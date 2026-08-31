.extern syscall_handler

.global syscall_entry
syscall_entry:
    swapgs # Swap whatever the user application used there.
    movq %rsp, %gs:8
    movq %gs:0, %rsp

    pushq %gs:8 # Preserve user stack.

    # NOTE: User stack needs to be preserved in case of syscalls like
    # sys_yield. If the syscall yields to another thread, and that
    # thread makes another syscall, the per-core user stack variable
    # is overwritten. This would lead to popping off to the wrong address.

    # SYSCALL puts user RIP into RCX and user RFLAGS into R11
    pushq %r11 # Preserve user RFLAGS
    pushq %rcx # Preserve user RIP

    cli
    cld

    pushq %rbp
    pushq %rbx
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # The additional registers above are expected to be saved
    # and protected after the syscall.

    pushq %r9 # 7th argument
    movq %r8, %r9
    movq %r10, %r8
    movq %rdx, %rcx
    movq %rsi, %rdx
    movq %rdi, %rsi
    movq %rax, %rdi

    # The syscall is called with x86-64 ABI (Standard), but the
    # function expects System V AMD64 C ABI, so we shift
    # the registers.
  
    call syscall_handler

    addq $0x8, %rsp

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbx
    popq %rbp
    popq %rcx # Restore user RIP
    popq %r11 # Restore user RFLAGS

    popq %gs:8 # Restore user stack

    movq %gs:8, %rsp
    swapgs
    sti
    sysretq
