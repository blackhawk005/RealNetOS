#define UART0_BASE 0xFE201000
#define UART0_DR   (*(volatile unsigned int *)(UART0_BASE + 0x00))
#define UART0_FR   (*(volatile unsigned int *)(UART0_BASE + 0x18))

void uart_putc(char c) {
    while (UART0_FR & (1 << 5)) {}
    UART0_DR = c;
}

void uart_puts(const char* s) {
    while (*s) uart_putc(*s++);
}
