#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

volatile static uint32_t ms_counter = 0;
volatile static uint8_t fractAcc = 0;

void timer0_init(void){

    //Waveform generation - PWM
    TCCR0A |= (1U << WGM01) | (1U << WGM00);
    TCCR0B &= ~(1U << WGM02);

    //Prescaler
    TCCR0B |= ((1U << CS00) | (1U << CS01));
    TCCR0B &= ~(1U << CS02);

    //Interrupt
    TIMSK0 |= (1U << TOIE0);

    sei();
}

void timer1_init(){
    //Waveform generation - PWM
    TCCR1A |= (1U << WGM10) | (1U << WGM11);
    TCCR1B |= (1U << WGM12);
    TCCR1B &= ~(1U << WGM13);

    // Prescaler
    TCCR1B |= (1U << CS10);

}

void timer2_init(){
    //Waveform generation - PWM
    TCCR2A |= (1U << WGM21) | (1U << WGM20);
    TCCR2B &= ~(1U << WGM22);
    //Prescaler
    TCCR2B |= (1U << CS20);
}

ISR(TIMER0_OVF_vect){
    uint32_t m = ms_counter;
    uint32_t f = fractAcc;
    m++;
    f += 24;

    if(f >= 250){
        m++;
        f -= 250;
    }

    ms_counter = m;
}

uint32_t sysTime(void){
    uint32_t time;
    cli();
    time = ms_counter;
    sei();
    return time;
}