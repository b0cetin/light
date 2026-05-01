ENTRY(_start)

SECTIONS {
    . = 0xFFFF800000100000; /* 1 MiB after HHDM */

    _kernel_start = .; /* Virtual Address */

    .text : AT(ADDR(.text) - 0xFFFF800000000000) {
        *(.text*)
    }

    .rodata : AT(ADDR(.rodata) - 0xFFFF800000000000)  {
        *(.rodata*)
    }

    .data : AT(ADDR(.data) - 0xFFFF800000000000)  {
        *(.data*)
    }

    .bss : AT(ADDR(.bss) - 0xFFFF800000000000)  {
        *(.bss*)
        *(COMMON)
    }

    _kernel_end = .; /* Virtual Address */
}
