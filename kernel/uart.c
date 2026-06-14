#include "../include/lib.h"
#include "../include/fb.h"

#define UART0_BASE 0xFE201000
#define UART0_DR   (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART0_FR   (*(volatile unsigned int *)(UART0_BASE + 0x18))

#define GPIO_BASE  0xFE200000
#define GPFSEL1    (*(volatile unsigned int *)(GPIO_BASE + 0x04))
#define GPPUPPDN0  (*(volatile unsigned int *)(GPIO_BASE + 0xE4))

static void delay(int count) {
    while (count--) {
        asm volatile("nop");
    }
}

void uart_init(void) {
    volatile unsigned int* uart_cr = (unsigned int*)(UART0_BASE + 0x30);
    volatile unsigned int* uart_ibrd = (unsigned int*)(UART0_BASE + 0x24);
    volatile unsigned int* uart_fbrd = (unsigned int*)(UART0_BASE + 0x28);
    volatile unsigned int* uart_lcrh = (unsigned int*)(UART0_BASE + 0x2C);
    volatile unsigned int* uart_icr  = (unsigned int*)(UART0_BASE + 0x44);

    // Map GPIO14/15 to ALT0 (TXD0/RXD0)
    unsigned int r = GPFSEL1;
    r &= ~((7 << 12) | (7 << 15));
    r |= (4 << 12) | (4 << 15);
    GPFSEL1 = r;

    // Disable pulls for GPIO14/15 using GPPUPPDN0
    r = GPPUPPDN0;
    r &= ~((3 << 28) | (3 << 30));
    GPPUPPDN0 = r;
    delay(150);

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
    if (fb_is_ready()) {
        fb_putc(c);
    }
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}

char uart_getc(void){
    while (UART0_FR & (1 << 4)){}   // WAIT while RX FIFO empty
    return UART0_DR & 0xFF;
}

int uart_try_getc(char* out) {
    if (UART0_FR & (1 << 4)) {
        return 0;
    }
    *out = UART0_DR & 0xFF;
    return 1;
}

void uart_puti(int num) {
    char buf[16];
    int_to_str(num, buf);
    uart_puts(buf);
}

void uart_puthex64(unsigned long value) {
    for (int i = 60; i >= 0; i -= 4) {
        unsigned long nib = (value >> i) & 0xF;
        char c = (nib < 10) ? ('0' + nib) : ('A' + (nib - 10));
        uart_putc(c);
    }
}
