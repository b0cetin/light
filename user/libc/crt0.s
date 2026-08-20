.global _start
.extern main

_start:
    xor %rbp, %rbp

    call main

    movq %rax, %rdi
    movq $2, %rax # sys_exit
    syscall
    
    hlt
