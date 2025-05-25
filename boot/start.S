.section .text
.global _start
.extern _end

_start:
    msr DAIFSet, #0xf     // disable interrupts
    ldr x0, =_end
    mov sp, x0            // use RAM just above kernel image
    bl kernel_main

1:
    wfe
    b 1b
