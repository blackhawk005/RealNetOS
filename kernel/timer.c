#include "../include/timer.h"

void timer_init(){
    unsigned long freq = 100000;
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"(0));        // Disable Timer
    asm volatile ("msr cntp_tval_el0, %0" :: "r"(freq));    // Set countdown
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"(1));        // Enable Timer
    asm volatile ("msr DAIFClr, #2");                       // Enable IRQs
}
