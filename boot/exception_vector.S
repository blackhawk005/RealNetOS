.section .text
.global exception_vector
.type exception_vector, %function
.extern irq_handler
.extern handle_syscall_from_el0
.extern schedule_from_el0
.extern get_current_user_ctx
.extern current
.align 11  // 2KB alignment required by VBAR_EL1

.macro VEC label
    b \label
    .space 124
.endm

// Minimal UART putc for exception diagnostics
.macro UART_PUTC reg
    ldr x9, =0xFE201018       // UART0_FR
1:
    ldr w10, [x9]
    tbnz w10, #5, 1b          // TXFF
    ldr x9, =0xFE201000       // UART0_DR
    str w\reg, [x9]
.endm

.macro UART_PUTNIB reg
    and w12, w\reg, #0xF
    cmp w12, #9
    add w12, w12, #'0'
    b.le 2f
    add w12, w12, #('A' - '0' - 10)
2:
    UART_PUTC 12
.endm

.macro UART_PUTHEX64 reg
    // Print 64-bit reg as 16 hex nibbles, high to low
    mov x11, \reg
    lsr x13, x11, #60
    UART_PUTNIB 13
    lsr x13, x11, #56
    UART_PUTNIB 13
    lsr x13, x11, #52
    UART_PUTNIB 13
    lsr x13, x11, #48
    UART_PUTNIB 13
    lsr x13, x11, #44
    UART_PUTNIB 13
    lsr x13, x11, #40
    UART_PUTNIB 13
    lsr x13, x11, #36
    UART_PUTNIB 13
    lsr x13, x11, #32
    UART_PUTNIB 13
    lsr x13, x11, #28
    UART_PUTNIB 13
    lsr x13, x11, #24
    UART_PUTNIB 13
    lsr x13, x11, #20
    UART_PUTNIB 13
    lsr x13, x11, #16
    UART_PUTNIB 13
    lsr x13, x11, #12
    UART_PUTNIB 13
    lsr x13, x11, #8
    UART_PUTNIB 13
    lsr x13, x11, #4
    UART_PUTNIB 13
    UART_PUTNIB 11
.endm

exception_vector:
    // 0x000: Synchronous EL1t
    VEC el1_sync_handler
    // 0x080: IRQ EL1t
    VEC el1_irq_handler
    // 0x100: FIQ EL1t
    VEC el1_fiq_handler
    // 0x180: SError EL1t
    VEC el1_serr_handler

    // 0x200: Synchronous EL1h
    VEC el1_sync_handler
    // 0x280: IRQ EL1h
    VEC el1_irq_handler
    // 0x300: FIQ EL1h
    VEC el1_fiq_handler
    // 0x380: SError EL1h
    VEC el1_serr_handler

    // 0x400: Synchronous EL0t (SVC from user mode)
    VEC el0_sync_handler
    // 0x480: IRQ EL0t
    VEC el0_irq_handler
    // 0x500: FIQ EL0t
    VEC el0_fiq_handler
    // 0x580: SError EL0t
    VEC el0_serr_handler

    // 0x600: Synchronous EL0h
    VEC el0_sync_handler
    // 0x680: IRQ EL0h
    VEC el0_irq_handler
    // 0x700: FIQ EL0h
    VEC el0_fiq_handler
    // 0x780: SError EL0h
    VEC el0_serr_handler

// ----------------------------------------
// EL1 IRQ HANDLER
el1_sync_handler:
    mov w0, #'K'
    UART_PUTC 0
    mrs x4, ESR_EL1
    lsr x4, x4, #26           // EC
    mov w5, w4
    lsr w5, w5, #4
    UART_PUTNIB 5
    UART_PUTNIB 4
    mov w0, #' '
    UART_PUTC 0
    mrs x4, ESR_EL1
    UART_PUTHEX64 x4
    mov w0, #' '
    UART_PUTC 0
    mrs x4, ELR_EL1
    UART_PUTHEX64 x4
    mov w0, #' '
    UART_PUTC 0
    mrs x4, FAR_EL1
    UART_PUTHEX64 x4
    b .

el1_irq_handler:
    // Acknowledge IRQ (for now just log)
    ldr x0, =irq_handler  // load address of a c function (irq_handler in irq.c)
    blr x0  // Branch and link to the address in x0

    eret  // return from exception : this will restore state and retur to interrupt thread

// ----------------------------------------
// EL0 SYNC HANDLER — HANDLE SVC #0
el0_sync_handler:
    // Ensure we're using SP_EL1 in the handler
    msr SPSel, #1
    // Check EC; only handle SVC64 here
    mrs x10, ESR_EL1
    lsr x11, x10, #26
    cmp x11, #0x15            // EC = SVC from A64
    b.eq 1f
    // Not an SVC: report ESR/ELR/FAR and halt
    mov w0, #'U'
    UART_PUTC 0
    mov w0, #' '
    UART_PUTC 0
    mov w0, #'E'      // ESR_EL1
    UART_PUTC 0
    UART_PUTHEX64 x10
    mov w0, #' '
    UART_PUTC 0
    mov w0, #'P'      // SPSR_EL1
    UART_PUTC 0
    mrs x12, SPSR_EL1
    UART_PUTHEX64 x12
    mov w0, #' '
    UART_PUTC 0
    mov w0, #'L'      // ELR_EL1
    UART_PUTC 0
    mrs x12, ELR_EL1
    UART_PUTHEX64 x12
    mov w0, #' '
    UART_PUTC 0
    mov w0, #'F'      // FAR_EL1
    UART_PUTC 0
    mrs x12, FAR_EL1
    UART_PUTHEX64 x12
    mov w0, #' '
    UART_PUTC 0
    mov w0, #'S'      // SP_EL0
    UART_PUTC 0
    mrs x12, SP_EL0
    UART_PUTHEX64 x12
    b .
1:
    // Advance ELR to avoid re-executing SVC
    mrs x4, ELR_EL1
    add x4, x4, #4
    msr ELR_EL1, x4
    // Force EL0t AArch64, interrupts masked, before return
    mov x4, #0x3C0
    msr SPSR_EL1, x4

    // Save all user regs x0-x30 on EL1 stack (before any C calls)
    sub sp, sp, #256
    stp x0,  x1,  [sp, #0]
    stp x2,  x3,  [sp, #16]
    stp x4,  x5,  [sp, #32]
    stp x6,  x7,  [sp, #48]
    stp x8,  x9,  [sp, #64]
    stp x10, x11, [sp, #80]
    stp x12, x13, [sp, #96]
    stp x14, x15, [sp, #112]
    stp x16, x17, [sp, #128]
    stp x18, x19, [sp, #144]
    stp x20, x21, [sp, #160]
    stp x22, x23, [sp, #176]
    stp x24, x25, [sp, #192]
    stp x26, x27, [sp, #208]
    stp x28, x29, [sp, #224]
    str x30, [sp, #240]

    // Get pointer to current user_ctx
    bl get_current_user_ctx
    mov x19, x0              // keep ctx pointer in callee-saved reg

    // Save all registers x0-x30 into user_ctx using caller-saved temps
    ldp x0, x1, [sp, #0]
    stp x0, x1, [x19, #0]
    ldp x0, x1, [sp, #16]
    stp x0, x1, [x19, #16]
    ldp x0, x1, [sp, #32]
    stp x0, x1, [x19, #32]
    ldp x0, x1, [sp, #48]
    stp x0, x1, [x19, #48]
    ldp x0, x1, [sp, #64]
    stp x0, x1, [x19, #64]
    ldp x0, x1, [sp, #80]
    stp x0, x1, [x19, #80]
    ldp x0, x1, [sp, #96]
    stp x0, x1, [x19, #96]
    ldp x0, x1, [sp, #112]
    stp x0, x1, [x19, #112]
    ldp x0, x1, [sp, #128]
    stp x0, x1, [x19, #128]
    ldp x0, x1, [sp, #144]
    stp x0, x1, [x19, #144]
    ldp x0, x1, [sp, #160]
    stp x0, x1, [x19, #160]
    ldp x0, x1, [sp, #176]
    stp x0, x1, [x19, #176]
    ldp x0, x1, [sp, #192]
    stp x0, x1, [x19, #192]
    ldp x0, x1, [sp, #208]
    stp x0, x1, [x19, #208]
    ldp x0, x1, [sp, #224]
    stp x0, x1, [x19, #224]
    ldr x0, [sp, #240]
    str x0, [x19, #240]

    // Save ELR/SP
    mrs x5, ELR_EL1
    mrs x6, SP_EL0
    str x6, [x19, #248]
    str x5, [x19, #256]

    // Restore syscall args and id
    ldr x0, [sp, #0]
    ldr x1, [sp, #8]
    ldr x8, [sp, #64]
    add sp, sp, #256

    // Convention: syscall ID = x8, args in x0/x1
    mov x2, x1      // arg1 -> x2
    mov x1, x0      // arg0 -> x1
    mov x0, x8      // syscall ID -> x0

    bl handle_syscall_from_el0  // dispatches to C code; return in x0
    str x0, [x19, #264]         // save return value for x0
    mov x5, #1
    str x5, [x19, #272]         // valid = 1
    b schedule_from_el0         // tail-branch to avoid stack growth


el1_fiq_handler:
    b .

el1_serr_handler:
    b .

el0_irq_handler:
    b .

el0_fiq_handler:
    b .

el0_serr_handler:
    // Report EL0 SError
    mov w0, #'S'
    UART_PUTC 0
    mrs x4, ESR_EL1
    lsr x4, x4, #26           // EC
    mov w5, w4
    lsr w5, w5, #4
    UART_PUTNIB 5
    UART_PUTNIB 4
    b .
