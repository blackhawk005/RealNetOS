.section .text
.global exception_vector
.type exception_vector, %function

// Each entry is 128 bytes per ARMv8-A architecture
// We only implement IRQs from EL1 for now

exception_vector:
    // 0x000: Synchronous EL1t
    b .

    // 0x080: IRQ EL1t
    b el1_irq_handler

    // 0x100: FIQ EL1t
    b .

    // 0x180: SError EL1t
    b .

    // 0x200: Synchronous EL1h
    b .

    // 0x280: IRQ EL1h
    b el1_irq_handler

    // 0x300: FIQ EL1h
    b .

    // 0x380: SError EL1h
    b .

el1_irq_handler:
    // Acknowledge IRQ (for now just log)
    ldr x0, =irq_handler
    blr x0

    eret  // return from exception
