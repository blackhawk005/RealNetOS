#include "../include/user_syscall.h"

static inline long svc_call(long id, long a0, long a1) {
    register long x0 __asm__("x0") = a0;
    register long x1 __asm__("x1") = a1;
    register long x8 __asm__("x8") = id;
    __asm__ volatile ("svc #0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory");
    return x0;
}

int u_sys_sleep(unsigned long ticks){
    return (int)svc_call(0, (long)ticks, 0);
}

int u_sys_yield(void){
    return (int)svc_call(1, 0, 0);
}

int u_sys_send(int to_id, const char* msg){
    return (int)svc_call(2, to_id, (long)msg);
}

int u_sys_receive(void* out_msg){
    return (int)svc_call(3, (long)out_msg, 0);
}

int u_sys_signal(int sig, signalhandler_t handler){
    return (int)svc_call(4, sig, (long)handler);
}

int u_sys_kill(int pid, int sig){
    return (int)svc_call(5, pid, sig);
}

int u_sys_return_from_signal(void){
    return (int)svc_call(6, 0, 0);
}

int u_sys_write(int fd, const char* buf){
    return (int)svc_call(8, fd, (long)buf);
}

int u_sys_read(int fd, char* buf){
    return (int)svc_call(7, fd, (long)buf);
}

int u_sys_tcp_connect(uint32_t dest_ip, uint16_t dest_port) {
    return (int)svc_call(9, (long)dest_ip, (long)dest_port);
}

int u_sys_tcp_send(const char* data, int len) {
    return (int)svc_call(10, (long)data, (long)len);
}

int u_sys_tcp_receive(char* buf, int max_len) {
    return (int)svc_call(11, (long)buf, (long)max_len);
}

unsigned long u_sys_ticks(void){
    return (unsigned long)svc_call(12, 0, 0);
}
