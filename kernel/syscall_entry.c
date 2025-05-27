#include "../include/syscall.h"

unsigned long handle_syscall_from_el0(unsigned long syscall_id,
                                      unsigned long arg0,
                                      unsigned long arg1) {
    return syscall_dispatcher((syscall_id_t)syscall_id, arg0, arg1);
}