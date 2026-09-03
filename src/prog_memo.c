#include "porg_memo.h"

char read_prog_byte(const char* p){
    char byte;
    __asm__ volatile("lpm %0, %a1" : "=&r" (byte) : "z" (p));
    return byte;
}