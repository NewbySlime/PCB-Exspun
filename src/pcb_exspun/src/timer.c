#include "timer.h"
#include "misc.h"

#define TIM1_AVAILABLEID 0b11110
#define TIM1_COUNTID 4
#define TIM1_ID1 1
#define TIM1_ID2 2
#define TIM1_ID3 3
#define TIM1_ID4 4
#define TIM1_IDBIT(n) (1 << n)


volatile timer_callback _t1_cb_bypass[TIM1_COUNTID];
volatile timer_callback _t1_cb[TIM1_COUNTID];
volatile uint16_t _t1_ticks[TIM1_COUNTID];
volatile uint8_t _t1_flags = 0;
volatile uint8_t _t1_isinterval = 0;
volatile uint8_t _t1_isbypass = 0;


void _timer1_erasesetup(uint8_t idbit){
  _t1_flags &= ~idbit;
  _t1_isinterval &= ~idbit;
  _t1_isbypass &= ~idbit;

  TIM1->IER &= ~idbit;
  TIM1->SR1 &= ~idbit;
}

uint8_t _timer1_get_availableid(){
  #define _TIMER1_GET_AVAILABLEID_N(n)\
  (!(TIM1->IER & TIM1_IDBIT(n)))\
    return n;

  if _TIMER1_GET_AVAILABLEID_N(TIM1_ID1)
  else if _TIMER1_GET_AVAILABLEID_N(TIM1_ID2)
  else if _TIMER1_GET_AVAILABLEID_N(TIM1_ID3)
  else if _TIMER1_GET_AVAILABLEID_N(TIM1_ID4)

  return 0;
}

void _timer1_set_ccr(uint8_t id, uint16_t n){
  switch(id){
    break; case TIM1_ID1:
      SETREG_LH(TIM1->CCR1, n);
    break; case TIM1_ID2:
      SETREG_LH(TIM1->CCR2, n);
    break; case TIM1_ID3:
      SETREG_LH(TIM1->CCR3, n);
    break; case TIM1_ID4:
      SETREG_LH(TIM1->CCR4, n);
  }
}


void timer1_setup(uint16_t prescaler){
  TIM1->CR1 &= ~TIM1_CR1_CEN;

  SETREG_LH(TIM1->ARR, 0xffff);
  SETREG_LH(TIM1->PSCR, prescaler);
  SETREG_LH(TIM1->CNTR, 0);

  TIM1->CR1 |= TIM1_CR1_CEN;
}


void timer1_delay(uint16_t ticks){
  ticks += GETREG_LH(TIM1->CNTR);

  while(ticks < GETREG_LH(TIM1->CNTR))
    ;
  
  while(ticks > GETREG_LH(TIM1->CNTR))
    ;
}

uint8_t timer1_asyncdelay(uint16_t ticks, timer_callback cb){
  uint8_t id;
  if((id = _timer1_get_availableid())){
    uint8_t _idbit = TIM1_IDBIT(id);
    _timer1_erasesetup(_idbit);

    _t1_cb[id-1] = cb;
    ticks += GETREG_LH(TIM1->CNTR);

    _timer1_set_ccr(id, ticks);
    TIM1->SR1 &= ~_idbit;
    TIM1->IER |= _idbit;
  }

  return id;
}

uint8_t timer1_interval(uint16_t ticks, timer_callback cb){
  uint8_t id;
  if((id = _timer1_get_availableid())){
    uint8_t _idbit = TIM1_IDBIT(id);
    _timer1_erasesetup(_idbit);

    _t1_isinterval |= _idbit;

    _t1_cb[id-1] = cb;
    _t1_ticks[id-1] = ticks;
    
    ticks += GETREG_LH(TIM1->CNTR);

    _timer1_set_ccr(id, ticks);
    TIM1->SR1 &= ~_idbit;
    TIM1->IER |= _idbit;
  }

  return id;
}

void timer1_changetimer(uint8_t id, uint16_t ticks){
  ticks += GETREG_LH(TIM1->CNTR);
  _timer1_set_ccr(id, ticks);
}

void timer1_stop(uint8_t id){
  if(id > 0 && id <= TIM1_COUNTID){
    uint8_t _idbit = TIM1_IDBIT(id);
    TIM1->IER &= ~_idbit;
    TIM1->SR1 &= ~_idbit;

    _t1_flags &= ~_idbit;
    _t1_isinterval &= ~_idbit;

    _t1_cb[id-1] = _dmpfunc_pv;
    _t1_cb_bypass[id-1] = _dmpfunc_pv;
  }
}

void timer1_bypasscheck(uint8_t id, timer_callback cb){
  if(id > 0 && id <= TIM1_COUNTID){
    uint8_t _idbit = TIM1_IDBIT(id);
    
    _t1_isbypass |= _idbit;
    _t1_cb_bypass[id-1] = cb;
  }
}


uint16_t timer1_getticks(){
  return GETREG_LH(TIM1->CNTR);
}


void timer1_on_capcom(){
  #define _TIMER1_ON_CAPCOM_M1(id)\
    (TIM1->SR1 & TIM1_IDBIT(id))\
      _currid = id;

  uint8_t _currid = 0;
  
  if _TIMER1_ON_CAPCOM_M1(TIM1_ID1)
  else if _TIMER1_ON_CAPCOM_M1(TIM1_ID2)
  else if _TIMER1_ON_CAPCOM_M1(TIM1_ID3)
  else if _TIMER1_ON_CAPCOM_M1(TIM1_ID4)

  if(_currid){
    uint8_t _currbitid = TIM1_IDBIT(_currid);
    TIM1->SR1 &= ~_currbitid;

    if(_t1_isinterval & _currbitid){
      uint16_t _newtick = _t1_ticks[_currid-1];

      switch(_currid){
        break; case TIM1_ID1:
          _newtick += GETREG_LH(TIM1->CCR1);
          SETREG_LH(TIM1->CCR1, _newtick);
        break; case TIM1_ID2:
          _newtick += GETREG_LH(TIM1->CCR2);
          SETREG_LH(TIM1->CCR2, _newtick);
        break; case TIM1_ID3:
          _newtick += GETREG_LH(TIM1->CCR3);
          SETREG_LH(TIM1->CCR3, _newtick);
        break; case TIM1_ID4:
          _newtick += GETREG_LH(TIM1->CCR4);
          SETREG_LH(TIM1->CCR4, _newtick);
      }
    }
    else
      TIM1->IER &= ~_currbitid;

    if(_t1_isbypass & _currbitid)
      _t1_cb_bypass[_currid-1]();

    _t1_flags |= _currbitid;
  }
  else
    TIM1->SR1 = 0;
}

void timer1_interruptcheck(){
  for(uint8_t i = 1; i <= TIM1_COUNTID; i++){
    if(_t1_flags & TIM1_IDBIT(i)){
      _t1_flags &= ~TIM1_IDBIT(i);
      _t1_cb[i-1]();
    }
  }
}


void timer2_setup(uint8_t prescaler, uint16_t arr){
  TIM2->CR1 &= ~TIM2_CR1_CEN;

  TIM2->PSCR = prescaler & 0b111;
  SETREG_LH(TIM2->ARR, arr);

  TIM2->IER = 0;
  TIM2->EGR |= TIM2_EGR_UG;
  TIM2->CR1 |= TIM2_CR1_CEN;
}