#include "../include/lib.h"

#define UART0_BASE 0xFE201000
#define UART0_DR   (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART0_FR   (*(volatile unsigned int *)(UART0_BASE + 0x18))

void uart_init(void) {
    volatile unsigned int* uart_cr = (unsigned int*)(UART0_BASE + 0x30);
    volatile unsigned int* uart_ibrd = (unsigned int*)(UART0_BASE + 0x24);
    volatile unsigned int* uart_fbrd = (unsigned int*)(UART0_BASE + 0x28);
    volatile unsigned int* uart_lcrh = (unsigned int*)(UART0_BASE + 0x2C);
    volatile unsigned int* uart_icr  = (unsigned int*)(UART0_BASE + 0x44);

    *uart_cr = 0;           // Disable UART
    *uart_icr = 0x7FF;      // Clear all interrupts
    *uart_ibrd = 1;         // Integer baud rate divisor
    *uart_fbrd = 40;        // Fractional baud rate divisor
    *uart_lcrh = (3 << 5);  // 8 bits, no parity, 1 stop bit
    *uart_cr = (1 << 0) | (1 << 8) | (1 << 9); // Enable UART, TX, RX
}

void uart_putc(char c) {
    while (UART0_FR & (1 << 5)) {}
    UART0_DR = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}

char uart_getc(void){
    while (UART0_FR & (1 << 4)){}   // WAIT while RX FIFO empty
    return UART0_DR & 0xFF;
}

void uart_puti(int num) {
    char buf[16];
    int_to_str(num, buf);
    uart_puts(buf);
}
