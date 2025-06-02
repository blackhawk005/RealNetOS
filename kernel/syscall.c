#include "../include/syscall.h"
#include "../include/threads.h"
#include "../include/ipc.h"
#include "../include/uart.h"
#include "../include/net/tcp.h"

extern thread_t threads[MAX_THREADS];
extern int current;

int syscall_dispatcher(syscall_id_t id, unsigned long arg0, unsigned long arg1){
    switch (id) {
        case SYSCALL_SLEEP:
            sleep(arg0);
            return 0;
        case SYSCALL_YIELD:
            yield();
            return 0;
        case SYSCALL_SEND:
            return send((int)arg0, (const char*)arg1);
        case SYSCALL_RECEIVE:
            return receive((message_t*)arg0);
        case SYSCALL_SIGNAL:
            threads[current].signal_handlers[arg0] = (signalhandler_t)arg1;
            return 0;
        case SYSCALL_KILL:
            if (arg0 >= 0 && arg0 < MAX_THREADS){
                threads[arg0].pending_signal = arg1;
            }
            return 0;
        case SYSCALL_RETURN:
            return sys_return_from_signal();
        case SYSCALL_WRITE:
            return sys_write((int)arg0, (const char*)arg1);
        case SYSCALL_READ:
            return sys_read((int)arg0, (char*)arg1);
        case SYSCALL_TCP_CONNECT:
            return tcp_connect((uint32_t)arg0, (uint16_t)arg1);
        case SYSCALL_TCP_SEND:
            return tcp_send((const char*)arg0, (int)arg1);
        case SYSCALL_TCP_RECEIVE:
            return tcp_receive((char*)arg0, (int*)arg1);
        case SYSCALL_TICKS:
            return system_ticks;
        default:
            return -1;
    }
}

int sys_sleep(unsigned long ticks){
    return syscall_dispatcher(SYSCALL_SLEEP, ticks, 0);
}

int sys_yield(){
    return syscall_dispatcher(SYSCALL_YIELD, 0, 0);
}

int sys_send(int to_id, const char* msg){
    return syscall_dispatcher(SYSCALL_SEND, to_id, (unsigned long)msg);
}

int sys_receive(void* out_msg){
    return syscall_dispatcher(SYSCALL_RECEIVE, (unsigned long)out_msg, 0);
}

int sys_return_from_signal(){
    threads[current].user_entry = (void*)threads[current].saved_pc;
    return 0;
}

int sys_write(int fd, const char* buf){
    if (fd == 1){
        uart_puts(buf);
        return 0;
    }

    return -1;
}

int sys_read(int fd, char* buf){
    if (fd == 0){
        char c = uart_getc();
        *buf = c;
        return 1;
    }
    return -1;
}

int sys_tcp_connect(uint32_t dest_ip, uint16_t dest_port) {
    return syscall_dispatcher(SYSCALL_TCP_CONNECT, dest_ip, dest_port);
}

int sys_tcp_send(const char* data, int len) {
    return syscall_dispatcher(SYSCALL_TCP_SEND, (unsigned long)data, len);
}

int sys_tcp_receive(char* buf, int max_len) {
    return syscall_dispatcher(SYSCALL_TCP_RECEIVE, (unsigned long)buf, max_len);
}

unsigned long sys_ticks(){
    register unsigned long ret __asm__("x0");
    register unsigned long syscall_id __asm__("x8") = SYSCALL_TICKS;
    __asm__ volatile ("svc #0" : "=r"(ret) : "r"(syscall_id) : "memory");
    return ret;
}