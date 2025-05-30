#include "../include/uart.h"
#include "../include/threads.h"
#include "../include/ipc.h"
#include "../include/syscall.h"
#include <stddef.h>

thread_t threads[MAX_THREADS];
int current = 0;
unsigned long system_ticks = 0;
int last_scheduled[MAX_PRIORITY_LEVELS] = {-1, -1, -1, -1};

extern void switch_context(context_t* old, context_t* new);
extern void rx_thread(void*);
extern void tx_thread(void*);
extern void user3_entry(void);

unsigned char rx_stack[STACK_SIZE] __attribute__((aligned(16)));
unsigned char tx_stack[STACK_SIZE] __attribute__((aligned(16)));

// Dummy user functions
void thread_fn1() {
    while (1) {
        sys_send(1, "Hello from T1");
        sys_sleep(5);
    }
}

void thread_fn2() {
    message_t msg;
    while (1) {
        if (sys_receive(&msg) == 0) {
            uart_puts("T2 received: ");
            uart_puts(msg.data);
            uart_puts("\n");
        }
    }
}

unsigned char user_stack1[4096] __attribute__((aligned(16)));
unsigned char user_stack2[4096] __attribute__((aligned(16)));
unsigned char user_stack3[4096] __attribute__((aligned(16)));

void init_threads(){
    // Thread 1
    // threads[0].ctx.sp = (unsigned long)(threads[0].stack + STACK_SIZE);
    // threads[0].ctx.lr = (unsigned long)thread_fn1;
    // Above not needed since eret transitions directly into EL0
    threads[0].active = 1;
    threads[0].priority = 1;
    threads[0].period = 5;
    threads[0].deadline = system_ticks + threads[0].period;
    threads[0].mailbox.head = 0;
    threads[0].mailbox.tail = 0;
    threads[0].mailbox.count = 0;
    threads[0].user_entry = user1_entry;
    threads[0].user_stack = user_stack1;


    // Thread 2
    // threads[1].ctx.sp = (unsigned long)(threads[1].stack + STACK_SIZE);
    // threads[1].ctx.lr = (unsigned long)thread_fn2;
    // Above not needed since eret transitions directly into EL0
    threads[1].active = 1;
    threads[1].priority = 0;    // Higher Priority
    threads[1].period = 3;
    threads[1].deadline = system_ticks + threads[1].period;    // Will be scheduled first
    threads[1].mailbox.head = 0;
    threads[1].mailbox.tail = 0;
    threads[1].mailbox.count = 0;
    threads[1].user_entry = user2_entry;
    threads[1].user_stack = user_stack1;

    // Thread 3 - RX Thread
    threads[2].active = 1;
    threads[2].priority = 2;
    threads[2].period = 10;
    threads[2].deadline = system_ticks + threads[2].period;
    threads[2].mailbox.head = threads[2].mailbox.tail = threads[2].mailbox.count = 0;
    threads[2].user_entry = user3_entry;
    threads[2].user_stack = user_stack3;

    // Thread 4 - TX Thread
    threads[3].active = 1;
    threads[3].priority = 3;
    threads[3].period = 10;
    threads[3].deadline = system_ticks + threads[3].period;
    threads[3].mailbox.head = threads[3].mailbox.tail = threads[3].mailbox.count = 0;
    threads[3].user_entry = (void (*)(void))tx_thread;
    threads[3].user_stack = NULL;
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

    // Wake sleeping threads whose time has come
    for (int i = 0; i < MAX_THREADS; i++){
        if (threads[i].state == THREAD_SLEEPING && system_ticks >= threads[i].sleep_until){
            threads[i].state = THREAD_RUNNABLE;
        }
    }

    int prev = current;

    // Scan priority levels from high to low
    for (int prio = 0; prio < MAX_PRIORITY_LEVELS; prio++){
        unsigned long best_deadline = ~0UL;
        int selected = -1;

        
        for (int i = 1; i <= MAX_THREADS; i++){
            int idx = (last_scheduled[prio] + i) % MAX_THREADS;     // Round-Robin Effect

            if (threads[idx].active && 
                threads[idx].priority == prio &&
                threads[idx].state == THREAD_RUNNABLE){
                // Picks thread with earliest deadline at this priority
                if (threads[idx].deadline < best_deadline){
                    best_deadline = threads[idx].deadline;
                    selected = idx;
                }
            }
        }

        // If new thread is selected
        if (selected != -1){
            last_scheduled[prio] = selected;
            current = selected;
            switch_context(&threads[prev].ctx, &threads[current].ctx);
            return;
        }
    }
}

void yield(){
    schedule();
}

void sleep(unsigned long ticks){
    threads[current].sleep_until = system_ticks + ticks;
    threads[current].state = THREAD_SLEEPING;
    schedule();
}

// Block the current thread
void wait(){
    threads[current].state = THREAD_BLOCKED;
    schedule();
}

void notify(int thread_id){
    if (thread_id >= 0 && thread_id < MAX_THREADS && threads[thread_id].active){
        if (threads[thread_id].state == THREAD_BLOCKED){
            threads[thread_id].state = THREAD_RUNNABLE;
        }
    }
}

// void wake(int thread_id){
//     if (thread_id >= 0 && thread_id < MAX_THREADS && threads[thread_id].active){
//         threads[thread_id].state = THREAD_RUNNABLE;
//     }
// }

int send(int to_id, const char* data){
    if (to_id < 0 || to_id >= MAX_THREADS){
        return -1;
    }

    mailbox_t* box = &threads[to_id].mailbox;
    if (box->count >= MAILBOX_SIZE){
        // Full
        return -2;
    }

    int pos = box->tail;

    box->messages[pos].from = current;
    for (int i = 0; i < 32 && data[i]; i++){
        box->messages[pos].data[i] = data[i];
    }

    box->tail = (box->tail + 1) % MAILBOX_SIZE;
    box->count++;

    // Wake if blocked on receive
    if (threads[to_id].state == THREAD_BLOCKED){
        threads[to_id].state = THREAD_RUNNABLE;
    }

    return 0;
}

int receive(message_t* out){
    mailbox_t* box = &threads[current].mailbox;

    if (box->count == 0){
        threads[current].state = THREAD_BLOCKED;
        schedule();
        return -1;  // Retry after scheduling
    }

    int pos = box->head;
    *out = box->messages[pos];
    box->head = (box->head + 1) % MAILBOX_SIZE;
    box->count--;
    return 0;
}