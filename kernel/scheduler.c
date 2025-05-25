#include "threads.h"
#include "../include/uart.h"

void kernel_main(){
    uart_puts("Starting Scheduler...\n");
    init_threads();
    schedule();
}