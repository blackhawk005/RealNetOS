#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char* s);
char uart_getc(void);
int uart_try_getc(char* out);
void uart_puti(int num);
void uart_puthex64(unsigned long value);

#endif
