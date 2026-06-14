#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include <stdint.h>
#include "signal.h"

int u_sys_sleep(unsigned long ticks);
int u_sys_yield(void);
int u_sys_send(int to_id, const char* msg);
int u_sys_receive(void* out_msg);

int u_sys_signal(int sig, signalhandler_t handler);
int u_sys_kill(int pid, int sig);
int u_sys_return_from_signal(void);

int u_sys_write(int fd, const char* buf);
int u_sys_read(int fd, char* buf);

int u_sys_tcp_connect(uint32_t dest_ip, uint16_t dest_port);
int u_sys_tcp_send(const char* data, int len);
int u_sys_tcp_receive(char* buf, int max_len);

unsigned long u_sys_ticks(void);

#endif
