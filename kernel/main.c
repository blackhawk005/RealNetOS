#include "../include/uart.h"
#include "../include/threads.h"
#include "../include/genet.h"
#include "../include/fs/vfs.h"
#include "../include/fs/dummyfs.h"
#include "../include/fs/fat32.h"
#include "../include/block/sd.h"
#include "../include/timer.h"
#include "../include/fb.h"
#include <stddef.h>


extern void exception_vector(void);
extern thread_t threads[MAX_THREADS];
extern int current;
extern void el0_test_entry(void);
extern void enter_user_ctx(void* ctx);

#define RUN_USER_EL0 1

// Color helpers for the HDMI console (0x00RRGGBB)
#define COL_WHITE  0xFFFFFFFF
#define COL_CYAN   0x0000FFFF
#define COL_GREEN  0x0000FF00
#define COL_YELLOW 0x00FFFF00

user_ctx_t* get_current_user_ctx(void) {
    return &threads[current].user_ctx;
}

static void boot_banner(void) {
    fb_set_color(COL_CYAN, 0x00000000);
    uart_puts("\n");
    uart_puts("  ____            _ _   _      _    ___  ____  \n");
    uart_puts(" |  _ \\ ___  __ _| | \\ | | ___| |_ / _ \\/ ___| \n");
    uart_puts(" | |_) / _ \\/ _` | |  \\| |/ _ \\ __| | | \\___ \\ \n");
    uart_puts(" |  _ <  __/ (_| | | |\\  |  __/ |_| |_| |___) |\n");
    uart_puts(" |_| \\_\\___|\\__,_|_|_| \\_|\\___|\\__|\\___/|____/ \n");
    uart_puts("   Real-Time Microkernel for Raspberry Pi 4\n\n");
    fb_set_color(COL_WHITE, 0x00000000);
}

static int boot_demo_ls_cb(const char* name, int is_dir, void* ctx) {
    (void)ctx;
    uart_puts("   - ");
    uart_puts(name);
    if (is_dir) uart_puts("/");
    uart_puts("\n");
    return 0;
}

// A short self-running demo so the HDMI screen shows live activity at boot
// (no serial keyboard required). Then control drops into the kernel shell.
static void boot_demo(void) {
    fb_set_color(COL_YELLOW, 0x00000000);
    uart_puts("\n[DEMO] Filesystem (FAT32 on SD) root listing:\n");
    fb_set_color(COL_WHITE, 0x00000000);
    vfs_list_root(boot_demo_ls_cb, NULL);

    fb_set_color(COL_GREEN, 0x00000000);
    uart_puts("\n[DEMO] Real-time scheduler: 8 periodic ticks (period=10)\n");
    fb_set_color(COL_WHITE, 0x00000000);
    const unsigned long period = 10;
    unsigned long next = system_ticks + period;
    unsigned long miss = 0;
    for (int i = 0; i < 8; i++) {
        timer_update_ticks();
        if (system_ticks > next) miss++;
        uart_puts("   tick=");
        uart_puti((int)system_ticks);
        uart_puts(" deadline_misses=");
        uart_puti((int)miss);
        uart_puts("\n");
        for (volatile int j = 0; j < 200000; j++) { }
        while (system_ticks < next) timer_update_ticks();
        next += period;
    }
    fb_set_color(COL_CYAN, 0x00000000);
    uart_puts("\n[DEMO] Done. Type 'help' at the prompt (serial UART).\n\n");
    fb_set_color(COL_WHITE, 0x00000000);
}

void schedule_from_el0(void) {
    timer_update_ticks();

    // 1. Run the scheduler to pick next thread
    schedule();

    // 2. Get the current thread
    thread_t* t = &threads[current];

    // ===== Signal Check =====
    if (t->pending_signal && t->signal_handlers[t->pending_signal]) {
        t->saved_pc = (unsigned long)t->user_entry;
        t->user_entry = t->signal_handlers[t->pending_signal];
        t->user_ctx.elr_el1 = 0;
        t->pending_signal = 0;
    }

    // ===== Kernel Thread =====
    if (t->user_stack == NULL) {
        void (*entry)(void) = t->user_entry;
        entry();
        t->state = THREAD_BLOCKED;
        return;
    }

    // ===== User Thread =====
    static int printed[MAX_THREADS] = {0};
    static int printed_ctx[MAX_THREADS] = {0};
    if (!printed[current]) {
        uart_puts("[K] Dispatching thread ");
        uart_puti(current);
        uart_puts("\n");
        printed[current] = 1;
    }

    // EL0t, no masks (let SVC work, no timer IRQs yet)
    // Force EL0t AArch64 with interrupts masked for a clean return
    unsigned long spsr = 0x3C0;
    unsigned long elr = t->user_ctx.elr_el1 ? t->user_ctx.elr_el1 : (unsigned long)t->user_entry;
    unsigned long sp_usr = t->user_ctx.sp_el0 ? t->user_ctx.sp_el0 : (unsigned long)(t->user_stack + STACK_SIZE);

    t->user_ctx.elr_el1 = elr;
    t->user_ctx.sp_el0 = sp_usr;

    uart_puts("[K] ERET -> EL0\n");
    if (!printed_ctx[current]) {
        uart_puts("[K] ctx v=");
        uart_puti((int)t->user_ctx.valid);
        uart_puts(" elr=");
        uart_puthex64(elr);
        uart_puts(" sp=");
        uart_puthex64(sp_usr);
        uart_puts(" entry=");
        uart_puthex64((unsigned long)t->user_entry);
        uart_puts(" ustack=");
        uart_puthex64((unsigned long)t->user_stack);
        uart_puts("\n");
        printed_ctx[current] = 1;
    }
    if (t->user_ctx.valid) {
        asm volatile("msr SPSR_EL1, %0" :: "r"(spsr));
        enter_user_ctx(&t->user_ctx);
    } else {
        static int printed_regs[MAX_THREADS] = {0};
        if (!printed_regs[current]) {
            asm volatile("msr SPSR_EL1, %0" :: "r"(spsr));
            asm volatile("msr ELR_EL1, %0" :: "r"(elr));
            asm volatile("msr SP_EL0, %0" :: "r"(sp_usr));
            unsigned long spsr_chk, elr_chk, sp_chk;
            asm volatile("mrs %0, SPSR_EL1" : "=r"(spsr_chk));
            asm volatile("mrs %0, ELR_EL1" : "=r"(elr_chk));
            asm volatile("mrs %0, SP_EL0" : "=r"(sp_chk));
            uart_puts("[K] regs spsr=");
            uart_puthex64(spsr_chk);
            uart_puts(" elr=");
            uart_puthex64(elr_chk);
            uart_puts(" sp=");
            uart_puthex64(sp_chk);
            uart_puts("\n");
            printed_regs[current] = 1;
        }
        asm volatile(
            "msr SPSR_EL1, %0\n"
            "msr ELR_EL1, %1\n"
            "msr SP_EL0, %2\n"
            "mov x0, %3\n"
            "eret\n"
            :
            : "r"(spsr), "r"(elr), "r"(sp_usr), "r"(t->user_ctx.x0_ret)
            : "memory"
        );
    }
}

void kernel_main(void) {
    uart_init();
    fb_init(1024, 768);
    boot_banner();
    uart_puts("[BOOT] UART OK\n");
    // Print current EL for sanity
    unsigned long cur_el;
    asm volatile("mrs %0, CurrentEL" : "=r"(cur_el));
    uart_puts("[K] CurrentEL=");
    uart_puti((int)((cur_el >> 2) & 0x3));
    uart_puts("\n");
    uart_puts("RealNetOS Booting...\n");

    register_filesystem(&dummy_fs);
    register_filesystem(&fat32_fs);

    int sd_ok = (sd_init() == 0);
    if (!sd_ok) {
        uart_puts("SD init failed\n");
    } else {
        uart_puts("SD init ok\n");
    }
    if (sd_ok) {
        if (vfs_mount(&fat32_fs) == 0) {
            uart_puts("FAT32 mounted.\n");
        } else {
            uart_puts("FAT32 mount failed.\n");
        }
    } else {
        uart_puts("FAT32 mount skipped (no SD)\n");
    }

    file_t* test_file;
    if (vfs_open("/test", &test_file) == 0) {
        char buf[64];
        int n = vfs_read(test_file, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            uart_puts("VFS read /test: ");
            uart_puts(buf);
        } else {
            uart_puts("VFS read /test: <empty>\n");
        }
    } else {
        uart_puts("VFS open /test failed\n");
    }

    file_t* log_file;
    if (vfs_create("/log", &log_file) == 0) {
        const char* msg = "boot log entry\n";
        vfs_write(log_file, msg, 16);
        uart_puts("VFS write /log ok\n");
    } else {
        uart_puts("VFS create /log failed\n");
    }


    uart_puts("Network Stack Starting...\n");
    genet_init();
    timer_timebase_init();

    // Set vector base for exception handling (SVC, IRQ, etc.)
    asm volatile("msr VBAR_EL1, %0" :: "r"(exception_vector));

    // Initialize threads
    init_threads();
    // threads[0].user_entry = el0_test_entry;

    uart_puts("Scheduler starting...\n");

    // Self-running on-screen demo (RT scheduling + FAT32), then the shell.
    boot_demo();

    while (1) {
        schedule_from_el0();
    }
}
