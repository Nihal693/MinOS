#include "adc.h"


void adc_init(void){
    // wake up adc module
    ADCSRA |= (1U << 0) | (1U << 1) | (1U << 2);
    ADCSRA |= (1U << 7);
}

uint16_t adc_read(uint8_t channel){
    if(channel < 8){
        ADMUX = channel | (1U << 6);
    }
    else{
        ADMUX = channel | (1U << 6) | (1U << 7);
    }
    uint32_t lastTime;
    lastTime = sysTime();
    while(sysTime() - lastTime >= 5){}

    ADCSRA |= (1U << 6);
    while(ADCSRA & (1U << 6)){

    }

    return ADC;
}