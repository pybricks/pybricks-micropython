// tary, 22:14 2012/12/28
#ifndef __UART_H__
#define __UART_H__

int uart_init(void);
int putchar(int c);
int getchar(void);
int peekchar(void);
int uart_gets(char *line, int n);

#endif//__UART_H__
