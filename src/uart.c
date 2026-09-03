#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "porg_memo.h"

#define BUF_LEN 65

char buf[BUF_LEN];
volatile uint8_t head = 0, tail = 0;

void uart_init(){
    UBRR0L = 103U;
    UBRR0H = 0U;

    UCSR0B |= (1U << 3) | (1U << 4);

}

void print_flash(const char *str){
    uint8_t idx = 0;
    uint8_t newhead;
    char ch;
    while((ch = (read_prog_byte(str + idx)))){
        newhead = (head + 1) % BUF_LEN;
        while(newhead == tail);
        //cli();
        buf[head] = ch;
        head = newhead;
        //sei();
        idx++;
    }
    UCSR0B |= (1U << 5);
}

void print(const char* str){
    uint8_t idx = 0;
    uint8_t newhead;
    char ch;
    while((ch = (str[idx]))){
        newhead = (head + 1) % BUF_LEN;
        while(newhead == tail);
        //cli();
        buf[head] = ch;
        head = newhead;
        //sei();
        idx++;
    }
    UCSR0B |= (1U << 5);
}

void print_char(const char ch){
    uint8_t newhead = (head + 1) % BUF_LEN;
    //head = head % BUF_LEN;
    while(tail == newhead);
    
    buf[head] = ch;
    head = newhead;
    

    UCSR0B |= (1U << 5);
}

char recieve(){
    while(!(UCSR0A & (1U << RXC0)));
    return UDR0;
}

ISR(USART_UDRE_vect){
    if(head != tail){
        char ch = buf[tail];
        UDR0 = ch;
        tail = (tail + 1) % BUF_LEN;
    }
    else{
        UCSR0B &= ~(1U << 5);
    }
}