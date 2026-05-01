.global gdt_flush

gdt_flush:
    lgdt (%rdi)          # Load GDT

    # 1. Reload Data Segments (Safe)
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # 2. Reload CS using a "Long Return"
    # We push the new CS and the address to jump to.
    # Then we use lretq (Long Return 64-bit).
    
    pushq $0x08                 # New Code Segment Selector
    lea .next(%rip), %rax       # Get address of next instruction
    pushq %rax                  # Push it
    lretq                       # Pop RIP, then pop CS.

.next:
    ret
