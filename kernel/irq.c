#include "../include/uart.h"

void irq_handler(){
    uart_puts("[IRQ] Interrupt Triggered!\n");

    // TODO: Acknowledge specific interrupt controller (Phase 2)

}