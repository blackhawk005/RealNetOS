#include "../include/uart.h"
#include "../include/threads.h"
#include "../include/genet.h"
#include "../include/fs/vfs.h"
#include "../include/fs/dummyfs.h"
#include "../include/fs/fat32.h"
#include <stddef.h>


extern void exception_vector(void);
extern thread_t threads[MAX_THREADS];
extern int current;

void kernel_main(void) {
    uart_init();
    uart_puts("RealNetOS Booting...\n");

    register_filesystem(&dummy_fs);
    register_filesystem(&fat32_fs);

    file_t* test_file;
    if (vfs_open("/test", &test_file) == 0) {
        uart_puts("VFS open succeeded\n");
    } else {
        uart_puts("VFS open failed\n");
    }


    uart_puts("Network Stack Starting...\n");
    genet_init();

    

    // Set vector base for exception handling (SVC, IRQ, etc.)
    asm volatile("msr VBAR_EL1, %0" :: "r"(exception_vector));

    // Initialize threads
    init_threads();

    uart_puts("Scheduler starting...\n");

    while (1) {
        // 1. Run the scheduler to pick next thread
        schedule();

        // 2. Get the current thread
        thread_t* t = &threads[current];

        // ===== Signal Check =====
        if (t->pending_signal && t->signal_handlers[t->pending_signal]) {
            t->saved_pc = (unsigned long)t->user_entry;
            t->user_entry = t->signal_handlers[t->pending_signal];
            t->pending_signal = 0;
        }

        // ===== Kernel Thread =====
        if (t->user_stack == NULL) {
            // kernel thread like rx_thread or tx_thread
            void (*entry)(void) = t->user_entry;
            entry(); // just run as function call
            t->state = THREAD_BLOCKED; // block again or exit
            continue;
        }

        // ===== User Thread =====
        unsigned long spsr = 0; // EL0t mode
        unsigned long elr = (unsigned long)t->user_entry;
        unsigned long sp_usr = (unsigned long)(t->user_stack + STACK_SIZE);

        asm volatile(
            "msr SPSR_EL1, %0\n"
            "msr ELR_EL1, %1\n"
            "msr SP_EL0, %2\n"
            "eret\n"
            :
            : "r"(spsr), "r"(elr), "r"(sp_usr)
            : "memory"
        );
    }
}
