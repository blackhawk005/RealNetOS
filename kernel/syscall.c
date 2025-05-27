#include "../include/syscall.h"
#include "../include/threads.h"
#include "../include/ipc.h"

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