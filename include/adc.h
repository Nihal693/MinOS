#ifndef ADC_H
#define ADC_H

#include <avr/io.h>
#include "systime.h"

void adc_init(void);
uint16_t adc_read(uint8_t);

#endif