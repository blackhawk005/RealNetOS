#include "../include/uart.h"
#include "../include/lib.h"
#include "../include/threads.h"
#include "../include/timer.h"
#include "../include/fs/vfs.h"

#define KSH_MAX_CMD 64

static void ksh_prompt(void) {
    uart_puts("RealNetOS# ");
}

static size_t ksh_strlen(const char* s) {
    size_t i = 0;
    while (s[i]) i++;
    return i;
}

static int ksh_cmd_is(const char* cmd, const char* name) {
    int i = 0;
    while (name[i] && cmd[i] == name[i]) i++;
    if (name[i] != '\0') return 0;
    return (cmd[i] == '\0');
}

static int ksh_cmd_is_prefix(const char* cmd, const char* name) {
    int i = 0;
    while (name[i] && cmd[i] == name[i]) i++;
    if (name[i] != '\0') return 0;
    return (cmd[i] == '\0' || cmd[i] == ' ');
}

static int ksh_ls_cb(const char* name, int is_dir, void* ctx) {
    (void)ctx;
    uart_puts(name);
    if (is_dir) uart_puts("/");
    uart_puts("\n");
    return 0;
}

static void ksh_handle(char* cmd) {
    if (ksh_cmd_is(cmd, "help")) {
        uart_puts("Available commands:\n");
        uart_puts("  help            - Show this message\n");
        uart_puts("  ticks           - Show system ticks count\n");
        uart_puts("  hello           - Print hello message\n");
        uart_puts("  clear           - Clear screen (UART)\n");
        uart_puts("  ls              - List files in root\n");
        uart_puts("  cat <file>      - Print file content\n");
        uart_puts("  write <f> <txt> - Write text to file\n");
        uart_puts("  mkdir <dir>     - Create directory\n");
        uart_puts("  rm <path>       - Remove file or empty dir\n");
        uart_puts("  rt              - Run RT timing demo\n");
        uart_puts("  rt-stress       - Run RT demo with intentional misses\n");
        uart_puts("  exit            - Exit shell loop\n");
        return;
    }

    if (ksh_cmd_is(cmd, "ticks")) {
        uart_puts("System ticks: ");
        uart_puti((int)system_ticks);
        uart_puts("\n");
        return;
    }

    if (ksh_cmd_is(cmd, "hello")) {
        uart_puts("Hello from RealNetOS\n");
        return;
    }

    if (ksh_cmd_is(cmd, "clear")) {
        uart_puts("\033[2J\033[H");
        return;
    }

    if (ksh_cmd_is(cmd, "rt")) {
        const unsigned long period = 20; // 200ms at 100Hz tick
        unsigned long next = system_ticks + period;
        unsigned long miss = 0;
        uart_puts("[RT] demo start (period=20 ticks)\n");
        for (int i = 0; i < 25; i++) {
            timer_update_ticks();
            unsigned long now = system_ticks;
            if (now > next) {
                miss++;
            }
            uart_puts("[RT] tick=");
            uart_puti((int)now);
            uart_puts(" miss=");
            uart_puti((int)miss);
            uart_puts("\n");
            // Simulate some work
            for (volatile int j = 0; j < 20000; j++) { }
            // Busy wait until next period (tick-based)
            while (system_ticks < next) {
                timer_update_ticks();
            }
            next += period;
        }
        uart_puts("[RT] demo done\n");
        return;
    }

    if (ksh_cmd_is(cmd, "rt-stress")) {
        const unsigned long period = 20; // 200ms at 100Hz tick
        unsigned long next = system_ticks + period;
        unsigned long miss = 0;
        uart_puts("[RT] stress start (period=20 ticks)\n");
        for (int i = 0; i < 25; i++) {
            timer_update_ticks();
            unsigned long now = system_ticks;
            if (now > next) {
                miss++;
            }
            uart_puts("[RT] tick=");
            uart_puti((int)now);
            uart_puts(" miss=");
            uart_puti((int)miss);
            uart_puts("\n");
            // Heavy work to overrun period
            for (volatile int j = 0; j < 400000; j++) {
                timer_update_ticks();
            }
            // Check for overrun after work
            timer_update_ticks();
            now = system_ticks;
            if (now > next) {
                miss++;
            }
            // Wait until next period
            while (system_ticks < next) {
                timer_update_ticks();
            }
            next += period;
        }
        uart_puts("[RT] stress done\n");
        return;
    }

    if (ksh_cmd_is(cmd, "exit")) {
        uart_puts("Exiting shell\n");
        return;
    }

    // Parse commands with arguments
    if (ksh_cmd_is_prefix(cmd, "ls")) {
        char* p = cmd + 2;
        while (*p == ' ') p++;
        if (*p == '\0') {
            if (vfs_list_root(ksh_ls_cb, NULL) != 0) {
                uart_puts("ls failed\n");
            }
        } else {
            if (vfs_list_dir(p, ksh_ls_cb, NULL) != 0) {
                uart_puts("ls failed\n");
            }
        }
        return;
    }

    if (ksh_cmd_is_prefix(cmd, "cat")) {
        char* p = cmd + 3;
        while (*p == ' ') p++;
        if (*p == '\0') {
            uart_puts("usage: cat <file>\n");
            return;
        }
        file_t* f;
        if (vfs_open(p, &f) != 0) {
            uart_puts("cat: open failed\n");
            return;
        }
        char buf[64];
        int n;
        while ((n = vfs_read(f, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            uart_puts(buf);
        }
        uart_puts("\n");
        return;
    }

    if (ksh_cmd_is_prefix(cmd, "write")) {
        char* p = cmd + 5;
        while (*p == ' ') p++;
        if (*p == '\0') {
            uart_puts("usage: write <file> <text>\n");
            return;
        }
        char* file = p;
        while (*p && *p != ' ') p++;
        if (*p == '\0') {
            uart_puts("usage: write <file> <text>\n");
            return;
        }
        *p++ = '\0';
        while (*p == ' ') p++;
        if (*p == '\0') {
            uart_puts("usage: write <file> <text>\n");
            return;
        }
        char* text = p;

        file_t* f;
        if (vfs_open(file, &f) != 0) {
            if (vfs_create(file, &f) != 0) {
                uart_puts("write: create failed\n");
                return;
            }
        }
        int n = vfs_write(f, text, (size_t)ksh_strlen(text));
        if (n < 0) {
            uart_puts("write: failed\n");
        } else {
            uart_puts("write: ok\n");
        }
        return;
    }

    if (ksh_cmd_is_prefix(cmd, "mkdir")) {
        char* p = cmd + 5;
        while (*p == ' ') p++;
        if (*p == '\0') {
            uart_puts("usage: mkdir <dir>\n");
            return;
        }
        if (vfs_mkdir(p) != 0) {
            uart_puts("mkdir: failed\n");
        } else {
            uart_puts("mkdir: ok\n");
        }
        return;
    }

    if (ksh_cmd_is_prefix(cmd, "rm")) {
        char* p = cmd + 2;
        while (*p == ' ') p++;
        if (*p == '\0') {
            uart_puts("usage: rm <path>\n");
            return;
        }
        if (vfs_unlink(p) != 0) {
            uart_puts("rm: failed\n");
        } else {
            uart_puts("rm: ok\n");
        }
        return;
    }

    uart_puts("Unknown Command. Try 'help'\n");
}

void kernel_shell_entry(void) {
    char buf[KSH_MAX_CMD];
    int idx = 0;

    uart_puts("RealNetOS Kernel Shell\n");
    ksh_prompt();

    while (1) {
        timer_update_ticks();
        char c;
        if (uart_try_getc(&c)) {
            if (c == '\r' || c == '\n') {
                buf[idx] = '\0';
                uart_puts("\n");
                if (idx > 0) {
                    ksh_handle(buf);
                }
                idx = 0;
                ksh_prompt();
                continue;
            }
            if (c == 127 || c == '\b') {
                if (idx > 0) {
                    idx--;
                    uart_puts("\b \b");
                }
                continue;
            }
            if (idx < KSH_MAX_CMD - 1) {
                buf[idx++] = c;
                uart_putc(c);
            }
        }
    }
}
