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

    // 0x400: Synchronous EL0t (SVC from user mode)
    b el0_sync_handler

    // 0x480: IRQ EL0t
    b .

    // 0x500: FIQ EL0t
    b .

    // 0x580: SError EL0t
    b .

    // Padding remaining entries
    .space 0x400  // pad remaining vector entries

// ----------------------------------------
// EL1 IRQ HANDLER
el1_irq_handler:
    // Acknowledge IRQ (for now just log)
    ldr x0, =irq_handler  // load address of a c function (irq_handler in irq.c)
    blr x0  // Branch and link to the address in x0

    eret  // return from exception : this will restore state and retur to interrupt thread

// ----------------------------------------
// EL0 SYNC HANDLER — HANDLE SVC #0
el0_sync_handler:
    // Convention: syscall ID = x8, args in x0/x1

    mov x10, x8     // syscall ID
    mov x11, x0     // arg0
    mov x12, x1     // arg1

    bl handle_syscall_from_el0  // dispatches to C code

    eret            // return to EL0
