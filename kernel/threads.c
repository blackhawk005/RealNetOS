#include "../include/uart.h"
#include "../include/threads.h"

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
    threads[0].priority = 1;

    // Thread 2
    threads[1].ctx.sp = (unsigned long)(threads[1].stack + STACK_SIZE);
    threads[1].ctx.lr = (unsigned long)thread_fn2;
    threads[1].active = 1;
    threads[1].priority = 0;    // Higher Priority
}

void schedule(){
    int prev = current;
    int best_priority = 1000;
    int next = current;

    for (int i = 0; i <= MAX_THREADS; i++){
        int idx = (current + i) % MAX_THREADS;
        if (threads[idx].active && threads[idx].priority < best_priority){
            best_priority = threads[idx].priority;
            next = idx;
        }
    }

    if (next != current){
        current = next;
        switch_context(&threads[prev].ctx, &threads[current].ctx);
    }
}