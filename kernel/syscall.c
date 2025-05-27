#include "../include/syscall.h"
#include "../include/threads.h"
#include "../include/ipc.h"

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