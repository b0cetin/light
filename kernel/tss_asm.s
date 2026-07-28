
.global tss_flush
tss_flush:
    mov $0x28, %ax   # Selector 0x28 = Index 5 * 8 bytes
    ltr %ax          # Load Task Register
    ret
