#ifndef UART_H
#define UART_H

void print_flash(const char *str);
void print(const char* str);
void print_char(const char ch);
char recieve();
void uart_init();

#endif