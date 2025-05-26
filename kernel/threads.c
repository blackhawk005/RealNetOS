#include "../include/uart.h"
#include "../include/threads.h"

thread_t threads[MAX_THREADS];
int current = 0;
unsigned long system_ticks = 0;
int last_scheduled[MAX_PRIORITY_LEVELS] = {-1, -1, -1, -1};

extern void switch_context(context_t* old, context_t* new);

// Dummy user functions
void thread_fn1(){
    while (1){
        uart_puts("[Thread 1]\n");
        for (volatile int i = 0; i < 1000000; ++i);
    }
}

void thread_fn2(){
    while (1){
        uart_puts("[Thread 2]\n");
        for (volatile int i = 0; i < 1000000; ++i);
    }
}

void init_threads(){
    // Thread 1
    threads[0].ctx.sp = (unsigned long)(threads[0].stack + STACK_SIZE);
    threads[0].ctx.lr = (unsigned long)thread_fn1;
    threads[0].active = 1;
    threads[0].priority = 1;
    threads[0].period = 5;
    threads[0].deadline = system_ticks + threads[0].period;

    // Thread 2
    threads[1].ctx.sp = (unsigned long)(threads[1].stack + STACK_SIZE);
    threads[1].ctx.lr = (unsigned long)thread_fn2;
    threads[1].active = 1;
    threads[1].priority = 0;    // Higher Priority
    threads[1].period = 3;
    threads[1].deadline = system_ticks + threads[1].period;    // Will be scheduled first
}

// Ensures expired threads are re-scheduled
void update_deadlines(){
    for (int i  = 0; i < MAX_THREADS; i++){
        if (threads[i].active && threads[i].deadline <= system_ticks){
            threads[i].deadline = system_ticks + threads[i].period;
        }
    }
}

void schedule(){
    update_deadlines();

    int prev = current;

    // Scan priority levels from high to low
    for (int prio = 0; prio < MAX_PRIORITY_LEVELS; prio++){
        unsigned long best_deadline = ~0UL;
        int selected = -1;

        
        for (int i = 1; i <= MAX_THREADS; i++){
            int idx = (last_scheduled[prio] + i) % MAX_THREADS;     // Round-Robin Effect

            if (threads[idx].active && threads[idx].priority == prio){
                // Picks thread with earliest deadline at this priority
                if (threads[idx].deadline < best_deadline){
                    best_deadline = threads[idx].deadline;
                    selected = idx;
                }
            }
        }

        if (selected != -1){
            last_scheduled[prio] = selected;
            current = selected;
            switch_context(&threads[prev].ctx, &threads[current].ctx);
            return;
        }
    }
}