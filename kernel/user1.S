.extern start_sensor_polling_task

.section .text
.global user1_entry, user_signal_handler
.type user1_entry, %function
.type user_signal_handler, %function

user1_entry:
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

handler:
    .word user_signal_handler
