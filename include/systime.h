#ifndef SYSTIME_H
#define SYSTIME_H

#include <stdint.h>

void timer0_init(void);
void timer1_init();
void timer2_init();
uint32_t sysTime(void);

#endif