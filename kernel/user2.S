.section .text
.global user2_entry
.type user2_entry, %function

user2_entry:
    mov x0, #0      // pid of user 1
    mov x1, #1      // signal ID = SIG_USER
    mov x8, #3      // syscall ID = SYSCALL_KILL
    svc #0

    b .             // Loop forever after sending

// user1 sets up its signal handler using sys_signal(SIG_USER, handler_func)

// user2 calls sys_kill(0, SIG_USER) to send signal to thread[0]

// The kernel traps into EL1 and detects a pending signal for thread 0

// On next schedule, thread 0:

// sets ELR_EL1 = handler_func

// saves original PC to saved_pc

// jumps into handler

// Handler ends with sys_return_from_signal(), and the kernel resumes execution at saved_pc

