#include "../include/uart.h"
#include "../include/timer.h"
#include "../include/threads.h"

void irq_handler(){
    uart_puts("[IRQ] Interrupt Triggered!\n");

    // Reset Timer
    timer_init();

    // Perform a context switch
    schedule();

    // TODO: Acknowledge specific interrupt controller (Phase 2)

}