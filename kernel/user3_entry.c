#include "../include/syscall.h"
#include "../include/uart.h"

void user3_entry(void){
    uart_puts("[EchoServer] Starting...\n");

    uint32_t dest_ip = (192 << 24) | (168 << 16) | (1 << 8) | 100;  // e.g., 192.168.1.100
    uint16_t dest_port = 8000;

    if (sys_tcp_connect(dest_ip, dest_port) < 0){
        uart_puts("[EchoServer] TCP connect failed\n");
        return;
    }

    uart_puts("[EchoServer] Connected.\n");

    char buffer[512];
    int len;

    while (1) {
        len = sys_tcp_receive(buffer, sizeof(buffer));
        if (len > 0) {
            uart_puts("[EchoServer] Received: ");
            for (int i = 0; i < len; i++) {
                uart_putc(buffer[i]);
            }
            uart_puts("\n");

            sys_tcp_send(buffer, len);
            uart_puts("[EchoServer] Echoed back.\n");
        }
    }
}