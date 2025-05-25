#include "../include/threads.h"
#include "../include/uart.h"
#include "../include/timer.h"

void kernel_main(){
    uart_puts("Starting Scheduler...\n");
    init_threads();

    timer_init();
    schedule();
}