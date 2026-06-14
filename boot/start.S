.section .text
.global _start
.extern __bss_start
.extern __bss_end
.extern __stack_top
.extern kernel_main
.extern exception_vector

_start:
    msr DAIFSet, #0xf         // Disable all interrupts: Debug, Async, IRQ, FIQ

    // Set up a temporary SP in case we are in EL2
    ldr x0, =__stack_top
    mov sp, x0

    // Clear .bss
    ldr x0, =__bss_start
    ldr x1, =__bss_end
    mov x2, #0
1:
    cmp x0, x1
    b.ge 2f
    str x2, [x0], #8
    b 1b
2:

    // If running in EL2 (QEMU raspi4b), drop to EL1
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #2
    b.ne el1_entry

    // Configure EL2 to drop into EL1h (AArch64) with a clean state
    mov x0, #(1 << 31)        // HCR_EL2.RW = 1 (EL1 AArch64)
    msr HCR_EL2, x0

    // Allow EL1 access to physical timers
    mov x0, #0x3
    msr CNTHCTL_EL2, x0
    mov x0, #0
    msr CNTVOFF_EL2, x0

    // Mask all exceptions and set EL1h mode
    mov x0, #(0b111 << 6) | 0b0101
    msr SPSR_EL2, x0

    adr x0, el1_entry
    msr ELR_EL2, x0
    eret

el1_entry:
    // If truly in EL1, switch to SP_EL1 and set stack
    mrs x0, CurrentEL
    lsr x0, x0, #2
    cmp x0, #1
    b.ne 1f
    msr SPSel, #1
    ldr x0, =__stack_top
    mov sp, x0
1:
    // Set EL1 stack (fallback)
    ldr x0, =__stack_top
    mov sp, x0

    // Unmask exceptions at EL1 (allow SVC/IRQ/FIQ/Debug)
    msr DAIFClr, #0xf


    // Set a known-safe SCTLR_EL1 (MMU/caches off, RES1 bits set)
    ldr x0, =0x30D00800
    msr SCTLR_EL1, x0
    isb

    ldr x0, =exception_vector // Load exception vector address
    msr VBAR_EL1, x0          // Set vector base address register
    isb                       // Instruction sync barrier (flush pipeline)

    bl kernel_main            // Jump to main kernel logic

1:
    wfe                       // Wait for event (low-power)
    b 1b                      // Infinite loop
