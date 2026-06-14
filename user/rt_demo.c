#include "../include/user/rt_demo.h"
#include "../include/user_syscall.h"
#include "../include/lib.h"

// Simple real-time demo task:
// Periodically prints a marker and ticks.
void start_rt_demo_task(void) {
    const unsigned long period = 20; // 200ms at 100Hz tick
    unsigned long next = u_sys_ticks() + period;
    unsigned long miss = 0;

    while (1) {
        unsigned long now = u_sys_ticks();
        if (now > next) {
            miss++;
        }

        // Minimal output to avoid heavy string ops
        u_sys_write(1, "[RT] tick=");
        char numbuf[32];
        int_to_buf(now, numbuf);
        u_sys_write(1, numbuf);
        u_sys_write(1, " miss=");
        int_to_buf(miss, numbuf);
        u_sys_write(1, numbuf);
        u_sys_write(1, "\n");

        // Simulate some work
        for (volatile int i = 0; i < 20000; i++) { }

        // Sleep until next period
        u_sys_sleep(period);
        next += period;
    }
}
