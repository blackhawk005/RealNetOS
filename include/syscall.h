#include "signal.h"

#ifndef SYSCALL_H
#define SYSCALL_H

// #define SYSCALL_SIGNAL 4
// #define SYSCALL_KILL 5
// #define SYSCALL_RETURN 6

typedef enum {
    SYSCALL_SLEEP,
    SYSCALL_YIELD,
    SYSCALL_SEND,
    SYSCALL_RECEIVE,
    SYSCALL_SIGNAL,
    SYSCALL_KILL,
    SYSCALL_RETURN
} syscall_id_t;

int syscall_dispatcher(syscall_id_t id, unsigned long arg0, unsigned long arg1);
unsigned long handle_syscall_from_el0(unsigned long syscall_id, unsigned long arg0, unsigned long arg1);

int sys_sleep(unsigned long ticks);
int sys_yield();
int sys_send(int to_id, const char* msg);
int sys_receive(void* out_msg);

int sys_signal(int sig, signalhandler_t handler);
int sys_kill(int pid, int sig);
int sys_return_from_signal(void);

#endif