#include "../include/uart.h"
#include "../include/threads.h"

extern void exception_vector(void);
extern void user_entry(void);

// 4KB aligned stack for EL0 (user mode)
unsigned char user_stack[4096] __attribute__((aligned(16)));

void delay(void) { for (volatile int i = 0; i < 100000000; i++); }

void kernel_main(void) {
    uart_init();
    uart_puts("RealNetOS Booting...\n");

    // Set vector base for exception handling (SVC, IRQ, etc.)
    asm volatile("msr VBAR_EL1, %0" :: "r"(exception_vector));

    // Initialize scheduler & threads
    init_threads();

    unsigned long spsr = 0;         // EL0t mode
    unsigned long elr = (unsigned long)user_entry;
    unsigned long sp_usr = (unsigned long)(user_stack + sizeof(user_stack));

    uart_puts("Switching to user mode...\n");

    // Move to user mode with eret
    asm volatile(
        "msr SPSR_EL1, %0\n"     // Saved Program Status Register
        "msr ELR_EL1, %1\n"      // Exception Link Register
        "msr SP_EL0, %2\n"       // Set EL0 stack pointer
        "eret\n"                 // Jump to user_entry in EL0
        :
        : "r"(spsr), "r"(elr), "r"(sp_usr)
        : "memory"
    );

    // Never returns here
    while (1) { __asm__ volatile("wfe"); }
}