.section .text
.global _start
.extern _end
.extern kernel_main
.extern exception_vector

_start:
    msr DAIFSet, #0xf         // Disable all interrupts: Debug, Async, IRQ, FIQ

    ldr x0, =exception_vector // Load exception vector address
    msr VBAR_EL1, x0          // Set vector base address register
    isb                       // Instruction sync barrier (flush pipeline)

    ldr x0, =_end             // _end = end of kernel binary (provided by linker)
    mov sp, x0                // Set stack pointer

    bl kernel_main            // Jump to main kernel logic

1:
    wfe                       // Wait for event (low-power)
    b 1b                      // Infinite loop
