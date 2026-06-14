.section .text
.global el0_test_entry
.type el0_test_entry, %function

el0_test_entry:
    // Trigger SVC, then loop in EL0 without MMIO
    mov x8, #1          // SYSCALL_YIELD
    svc #0
1:
    nop
    b 1b
