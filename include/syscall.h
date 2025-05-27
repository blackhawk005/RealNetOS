#ifndef SYSCALL_H
#define SYSCALL_H

typedef enum {
    SYSCALL_SLEEP,
    SYSCALL_YIELD,
    SYSCALL_SEND,
    SYSCALL_RECEIVE
} syscall_id_t;

int syscall_dispatcher(syscall_id_t id, unsigned long arg0, unsigned long arg1);
unsigned long handle_syscall_from_el0(unsigned long syscall_id, unsigned long arg0, unsigned long arg1);

int sys_sleep(unsigned long ticks);
int sys_yield();
int sys_send(int to_id, const char* msg);
int sys_receive(void* out_msg);

#endif