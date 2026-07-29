
.global enter_userspace
.type enter_userspace, @function

# System V ABI calling convention:
# %rdi = entry_point
# %rsi = user_rsp

enter_userspace:
    /* 1. Load User Data Selector into DS and ES.
          GDT Index 3 (0x18) with RPL 3 (0x3) = 0x1B */
    movw $0x1B, %ax # 0x1B -> "GDT Entry #3, using the GDT, running at Ring 3 privilege."
    movw %ax, %ds # Hold kernels permissions, will invoke General Protection Fault.
    movw %ax, %es # Hold kernels permissions, will invoke General Protection Fault.

    /* 2. Set up RCX (User RIP) */
    movq %rdi, %rcx

    /* 3. Set up R11 (User RFLAGS)
          0x202 = Bit 1 (Always 1) + Bit 9 (IF - Enable Interrupts in Ring 3) */
    movq $0x202, %r11

    /* 4. Switch to the User Stack Pointer */
    movq %rsi, %rsp

    /* 5. Swap GS base MSRs so user mode gets user GS,
          and KERNEL_GS_BASE holds our kernel per-CPU struct */
    swapgs

    /* 6. Return to Ring 3!
          CS becomes 0x23 (Index 4 | RPL 3)
          SS becomes 0x1B (Index 3 | RPL 3)
          RIP becomes RCX
          RFLAGS becomes R11 */
    sysretq
