#ifndef PROG
#define PROG

#define prog __attribute__((__progmem__))
#define Pmem(str) ({static const prog char c[] = (str); &c[0];})

char read_prog_byte(const char* p);

#endif