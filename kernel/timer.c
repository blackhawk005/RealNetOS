#include "../include/timer.h"
#include "../include/threads.h"
#include <stdint.h>

static uint64_t cnt_freq = 0;
static uint64_t last_cnt = 0;
static uint64_t tick_interval = 0;

void timer_timebase_init(void) {
    uint64_t freq;
    uint64_t cnt;
    asm volatile ("mrs %0, CNTFRQ_EL0" : "=r"(freq));
    asm volatile ("mrs %0, CNTVCT_EL0" : "=r"(cnt));
    cnt_freq = freq;
    tick_interval = (cnt_freq / 100); // 100 Hz tick
    if (tick_interval == 0) tick_interval = 1;
    last_cnt = cnt;
}

void timer_update_ticks(void) {
    if (tick_interval == 0) {
        timer_timebase_init();
    }
    uint64_t now;
    asm volatile ("mrs %0, CNTVCT_EL0" : "=r"(now));
    uint64_t delta = now - last_cnt;
    if (delta >= tick_interval) {
        uint64_t ticks = delta / tick_interval;
        system_ticks += ticks;
        last_cnt += ticks * tick_interval;
    }
}

void timer_init(){
    system_ticks++;
    unsigned long freq = 100000;
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"(0));        // Disable Timer
    asm volatile ("msr cntp_tval_el0, %0" :: "r"(freq));    // Set countdown
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"(1));        // Enable Timer
    asm volatile ("msr DAIFClr, #2");                       // Enable IRQs
}
