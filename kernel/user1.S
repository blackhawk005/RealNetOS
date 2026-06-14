.extern start_sensor_polling_task

.section .text
.global user1_entry, user_signal_handler
.type user1_entry, %function
.type user_signal_handler, %function

user1_entry:
    // Direct UART write to prove EL0 code executes
    ldr x2, =0xFE201000     // UART0_DR
    mov w3, #'u'
    str w3, [x2]

    // Print startup banner via syscall to verify EL0 SVC path
    mov x0, #1              // fd = 1
    ldr x1, =msg_start
    mov x8, #8              // SYSCALL_WRITE
    svc #0

    // Register signal handler: sys_signal(SIG_USER, handler)
    mov x0, #1              // signal ID = SIG_USER
    ldr x1, =handler        // handler function address
    mov x8, #4              // SYSCALL_SIGNAL
    svc #0

    // Send a message
    mov x0, #1
    ldr x1, =msg_main
    mov x8, #2              // SYSCALL_SEND
    svc #0

    // Sleep
    mov x0, #3
    mov x8, #0              // SYSCALL_SLEEP
    svc #0

    b start_sensor_polling_task

    b .                     // loop forever

// ----------------------------------------
// Signal Handler
user_signal_handler:
    ldr x0, =msg_signal
    mov x8, #2              // SYSCALL_SEND
    svc #0

    mov x8, #6              // SYSCALL_RETURN
    svc #0

// ----------------------------------------
.section .rodata
msg_main:
    .asciz "From User 1 (normal)\n"

msg_signal:
    .asciz "From User 1 (signal)\n"

msg_start:
    .asciz "[U1] EL0 started\n"

handler:
    .quad user_signal_handler
