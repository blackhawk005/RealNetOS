#ifndef THREADS_H
#define THREADS_H

#define MAX_THREADS 2
#define STACK_SIZE 4096

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

typedef struct {
    context_t ctx;
    unsigned char stack[STACK_SIZE];
    int active;
} thread_t;


void init_threads();
void schedule();


#endif