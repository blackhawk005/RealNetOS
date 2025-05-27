#include "../include/threads.h"
#include "../include/uart.h"
#include "../include/timer.h"
#include "../include/mem.h"
#include "../include/page.h"

// void kernel_main(){
//     uart_puts("Starting Scheduler...\n");
//     while (1);

//     // void* ptr1 = kmalloc(64);
//     // void* ptr2 = kmalloc(128);

//     // uart_puts("Allocated heap memory.\n");

//     // page_allocator_init();

//     // void* p1 = alloc_page();
//     // void* p2 = alloc_page();
//     // free_page(p1);
//     // void* p3 = alloc_page();

//     // init_threads();

//     // timer_init();
//     // schedule();
// }