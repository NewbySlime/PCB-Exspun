#ifndef TIMER_HEADER
#define TIMER_HEADER

#include "stm8s.h"

typedef void(*timer2_update_callback)();


void timer1_setup(uint16_t prescaler, uint16_t counter);


void timer2_setup(uint8_t prescaler);

void timer2_delay(uint16_t ticks);
void timer2_asyncdelay(uint16_t ticks, timer2_update_callback cb);
void timer2_interval(uint16_t ticks, timer2_update_callback cb);

bool timer2_isrunning();
void timer2_stop();

void timer2_on_overflow();

#endif