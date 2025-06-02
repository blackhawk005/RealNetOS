#include "../include/user/shell.h"
#include "../include/syscall.h"
#include "../include/uart.h"
#include "../include/lib.h"
#include <stddef.h>

#define MAX_CMD_LENGTH 64

static void print_prompt() {
    sys_write(1, "RealNetOS> ");
}

static void handle_command(const char* cmd) {
    if (strcmp(cmd, "help") == 0){
        sys_write(1, "Available commands:\n");
        sys_write(1, "  help            - Show this message\n");
        sys_write(1, "  ticks           - Show system ticks count\n");
        sys_write(1, "  hello           - Print hello message\n");
        sys_write(1, "  clear           - Clear screen (UART)\n");
        sys_write(1, "  exxit           - Exit Shell\n");
    }
    else if (strcmp(cmd, "ticks") == 0){
        char numbuf[32], outbuf[64];
        unsigned long ticks = sys_ticks();
        int_to_buf(ticks, numbuf);
        strcpy(outbuf, "System ticks: ");
        strcat(outbuf, numbuf);
        strcat(outbuf, "\n");
        sys_write(1, outbuf);
    }
    else if (strcmp(cmd, "hello") == 0){
        sys_write(1, "Hello from RealNetOS");
    }
    else if (strcmp(cmd, "clear") == 0){
        sys_write(1, "\033[2J\033[H");          // ANSI escape to clear UART terminal
    }
    else if (strcmp(cmd, "exxit") == 0){
        sys_write(1, "Exiting shell\n");
        return;
    }
    else {
        sys_write(1, "Unknown Command. Try 'help'\n");
    }
}

void shell_entry(){
    char input_buf[MAX_CMD_LENGTH];
    int idx = 0;

    sys_write(1, "RealNetOS CLI Shell\n");
    print_prompt();
    while (1){
        char c;
        if (sys_read(0, &c) == 1){
            if (c == '\r' || c == '\n'){
                input_buf[idx] = '\0';
                sys_write(1, "\n");
                handle_command(input_buf);
                idx = 0;
                print_prompt();
            }
        }
        else if (c == 127 || c == '\b'){
            if (idx > 0) {
                idx--;
                sys_write(1, "\b \b");
            }
        }
        else if (idx < MAX_CMD_LENGTH - 1){
            input_buf[idx++] = c;
            sys_write(1, &c);
        }
    }
}