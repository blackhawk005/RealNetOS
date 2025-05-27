.section .text
.global user_entry
.type user_entry, %function

user_entry:
    // Call: syscall(SYSCALL_SEND, 1, "USR")
    mov x0, #1          // arg0: thread_id
    ldr x1, =msg        // arg1: msg
    mov x8, #2          // syscall ID: SYSCALL_SEND
    svc #0

    // call: syscall(SYSCALL_SLEEP, 2)
    mov x0, #2
    mov x8, #0          // SYSCALL_SLEEP
    svc #0

    b .

.section .rodata
msg:
    .asciz "USR\n"