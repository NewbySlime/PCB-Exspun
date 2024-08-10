#ifndef TIMER_HEADER
#define TIMER_HEADER

#include "stm8s.h"

typedef void(*timer_callback)();


void timer1_setup(uint16_t prescaler);

void timer1_delay(uint16_t ticks);
uint8_t timer1_asyncdelay(uint16_t ticks, timer_callback cb);
uint8_t timer1_interval(uint16_t ticks, timer_callback cb);

void timer1_changetimer(uint8_t id, uint16_t ticks);
void timer1_stop(uint8_t id);
void timer1_bypasscheck(uint8_t id, timer_callback cb);

uint16_t timer1_getticks();

void timer1_on_capcom();
void timer1_interruptcheck();


void timer2_setup(uint8_t prescaler, uint16_t arr);

#endif