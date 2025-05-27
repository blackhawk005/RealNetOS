.section .text
.global user1_entry, user_signal_handler
.type user1_entry, %function

user1_entry:
    mov x0, #1
    ldr x1, =msg
    mov x8, #2        // SYSCALL_SEND
    svc #0

    mov x0, #3
    mov x8, #0        // SYSCALL_SLEEP
    svc #0

    b .


user_signal_handler:
    ldr x0, =msg
    mov x8, #2       // SYSCALL_SEND
    svc #0

    mov x8, #6       // SYSCALL_RETURN
    svc #0

.section .rodata
msg:
    .asciz "From User 1\n"

handler:
    .word user_signal_handler
