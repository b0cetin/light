.global _start
.extern main

_start:
    call main
    movq %rax, %rdi
    movq $2, %rax
    syscall
