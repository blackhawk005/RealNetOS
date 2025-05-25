#include "threads.h"
#include "../include/uart.h"

thread_t threads[MAX_THREADS];
int current = 0;

extern void switch_context(context_t* old, context_t* new);

// Dummy user functions
void thread_fn1(){
    while (1){
        uart_puts("[Thread 1]\n");
    }
}

void thread_fn2(){
    while (1){
        uart_puts("[Thread 2]\n");
    }
}

void init_threads(){
    // Thread 1
    threads[0].ctx.sp = (unsigned long)(threads[0].stack + STACK_SIZE);
    threads[0].ctx.lr = (unsigned long)thread_fn1;
    threads[0].active = 1;

    // Thread 2
    threads[1].ctx.sp = (unsigned long)(threads[1].stack + STACK_SIZE);
    threads[1].ctx.lr = (unsigned long)thread_fn2;
    threads[1].active = 1;
}

void schedule(){
    int prev = current;
    current = (current + 1) % MAX_THREADS;
    switch_context(&threads[prev].ctx, &threads[current].ctx);
}