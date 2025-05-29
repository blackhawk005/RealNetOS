#include "ipc.h"
#include "signal.h"

#ifndef THREADS_H
#define THREADS_H

#define MAX_THREADS 4
#define MAX_PRIORITY_LEVELS 4 // Eg 0 (highest) to 3 (lowest)
#define STACK_SIZE 4096

extern int last_scheduled[MAX_PRIORITY_LEVELS];
extern unsigned long system_ticks;
extern void user1_entry(void);
extern void user2_entry(void);

typedef struct {
    unsigned long x19;  // Callee-saved
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;   // x29
    unsigned long lr;   // x30
    unsigned long sp;   // Stack pointer
} context_t;

typedef enum {
    THREAD_RUNNABLE,
    THREAD_SLEEPING,
    THREAD_BLOCKED
} thread_state_t;

typedef struct {
    context_t ctx;
    unsigned char stack[STACK_SIZE];
    int active;
    int priority;               // Lower number = higher priority
    unsigned long deadline;     // Lower = higher urgency (Timestamp)
    unsigned long period;       // For periodic deadline update
    thread_state_t state;
    unsigned long sleep_until;
    mailbox_t mailbox;
    unsigned char* user_stack;
    void (*user_entry)(void);
    unsigned int pending_signal;
    signalhandler_t signal_handlers[MAX_SIGNALS];
    unsigned long saved_pc;
} thread_t;

void yield();
void sleep(unsigned long ticks);
// void wake(int thread_id);
void wait();
void notify(int thread_id);

void init_threads();
void schedule();


#endif