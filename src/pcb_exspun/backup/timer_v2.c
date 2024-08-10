#include "timer.h"
#include "misc.h"

#define __TIMER1_SETCHX(n)\
  void timer1_setch##n(uint8_t ccmr, uint8_t ccer, uint16_t ccr){\
    TIM1->CCMR##n = ccmr;\
    TIM1->CCER##n = ccer;\
    TIM1->CCR##n##L = ccr & 0xff;\
    TIM1->CCR##n##H = (ccr >> 8) & 0xff;\
  }



bool timer2_running = FALSE;
bool timer2_isinterval = FALSE;

uint16_t _currenttick;

void __timer2_dmpcb(){}
timer2_update_callback timer2_cb = __timer2_dmpcb;


void timer1_setup(uint16_t prescaler, uint16_t counter){
  //TIM1->CR2 = 0b00010000;

  TIM1->IER = 0;


  TIM1->CR1 = TIM1_CR1_CEN;
}

__TIMER1_SETCHX(1)
__TIMER1_SETCHX(2)


void timer2_setup(uint8_t prescaler){
  TIM2->PSCR = prescaler & 0x0f;
}

void timer2_delay(uint16_t ticks){
  if(!timer2_running){
    timer2_running = TRUE;

    TIM2->CR1 |= TIM2_CR1_CEN | TIM2_CR1_OPM;
    TIM2->IER = 0;

    SETREG_LH(TIM2->ARR, ticks);
    SETREG_LH(TIM2->CNTR, 0);

    while(!(TIM2->SR1 & TIM2_SR1_UIF))
      ;

    TIM2->SR1 = 0;
    TIM2->CR1 &= ~TIM2_CR1_CEN;

    timer2_running = FALSE;
  }
}

void timer2_asyncdelay(uint16_t ticks, timer2_update_callback cb){
  if(!timer2_running){
    timer2_running = TRUE;
    timer2_cb = cb;
    timer2_isinterval = FALSE;

    TIM2->CR1 |= TIM2_CR1_OPM;
    TIM2->IER |= TIM2_IER_UIE;

    SETREG_LH(TIM2->ARR, ticks);
    SETREG_LH(TIM2->CNTR, 0);

    TIM2->CR1 |= TIM2_CR1_CEN;
  }
}

void timer2_interval(uint16_t ticks, timer2_update_callback cb){
  if(!timer2_running){
    timer2_asyncdelay(ticks, cb);
    
    _currenttick = ticks;
    timer2_isinterval = TRUE;
  }
}


bool timer2_isrunning(){
  return timer2_running;
}

void timer2_stop(){
  TIM2->CR1 &= ~TIM2_CR1_CEN;
  timer2_running = FALSE;

  TIM2->SR1 = 0;
}


void timer2_on_overflow(){
  if(TIM2->CR1 & TIM2_CR1_OPM && !timer2_isinterval){
    timer2_running = FALSE;
    TIM2->CR1 &= ~TIM2_CR1_CEN;
  }
  else{
    SETREG_LH(TIM2->CNTR, 0);
    SETREG_LH(TIM2->ARR, _currenttick);
    TIM2->CR1 |= TIM2_CR1_CEN;
  }

  TIM2->SR1 = 0;
  timer2_cb();
}