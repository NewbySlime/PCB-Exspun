#include "I2C.h"
#include "timer.h"
#include "LCD1602A.h"
#include "misc.h"
#include "GPMEM.h"
#include "stdio.h"
#include "ADC.h"

// 100khz i2c clk speed
#define I2C_CLOCK_SPD 0x50


#define INPUT_TIME_CANCEL 2000
#define INPUT_TIME_HOLD 1000
#define INPUT_TIME_REPEAT 500
#define INPUT_REPEAT_TIMES 3

#define OUTPUTPORTD_UVLIGHT 0b00010000
#define OUTPUTPORTD_MOTOR   0b00001000

#define INPUTPORTD_BUTTON1  0b01000000
#define INPUTPORTD_BUTTON2  0b00100000
#define INPUTPORTD_BUTTON3  0b00000100
#define INPUTPORTD_BUTTONS  0b01100100

#define _TIME_DISP_STARTPOS 0x40
#define SETTIMER_INCREMENT 60
#define SETTIMER_MAX 28800
#define SETTIMER_MIN SETTIMER_INCREMENT
#define SETTIMER_MAXREPEAT 21

#define _BAR_YSIZE 8
#define _BAR_XSIZE 5
#define _BAR_MIDDLELENGTH 3
#define _BAR_STARTPOS 0x4B

#define _BAR_START_FILLLEN 2
#define _BAR_MIDDLE_FILLLEN 5
#define _BAR_END_FILLLEN 2

#define _BAR_START_ADDR 0x0
#define _BAR_MIDDLEFULL_ADDR 0x1
#define _BAR_MIDDLEEMPTY_ADDR 0x2
#define _BAR_MIDDLEVAR_ADDR 0x3
#define _BAR_END_ADDR 0x4

static const char _BAR_FILLLEN = 
  _BAR_START_FILLLEN +
  (_BAR_MIDDLE_FILLLEN * _BAR_MIDDLELENGTH) +
  _BAR_END_FILLLEN
;



// strings for displaying progress bar

static const char _BAR_START_FG[] = {
 0x00,0x00,0x02,0x01,0x03,0x00,0x00,0x00 };

static const char _BAR_START_BG[] = {
 0x00,0x03,0x04,0x04,0x04,0x03,0x00,0x00 };

static const char _BAR_MIDDLE_FG[] = {
 0x00,0x00,0x00,0x1f,0x1f,0x00,0x00,0x00 };

static const char _BAR_MIDDLE_BG[] = {
 0x00,0x1f,0x00,0x00,0x00,0x1f,0x00,0x00 };

static const char _BAR_END_FG[] = {
 0x00,0x00,0x08,0x18,0x18,0x00,0x00,0x00 };

static const char _BAR_END_BG[] = {
 0x00,0x18,0x04,0x04,0x04,0x18,0x00,0x00 };


// strings for displaying things

static const char *_TIME_DISP_PADDING = "%-11s";
static const char *_PROG_DISP_CANCELLED = "Cancelled.";
static const char *_PROG_DISP_MAIN_MENU = "Select Function";
static const char *_PROG_DISP_MAIN_MENU_SELECT_NONE = "Only Timer";
static const char *_PROG_DISP_MAIN_MENU_SELECT_EXPOSE = "Exposing PCB";
static const char *_PROG_DISP_MAIN_MENU_SELECT_STIR = "Etching/Stirring";
static const char *_PROG_DISP_CLEARLINE = "                ";
static const char *_PROG_DISP_CLEARLINE_TIME = "           ";
static const char *_PROG_DISP_SETTIMER_TITLE = "Set The Timer";
static const char *_PROG_DISP_DONE = "Done";



typedef enum{
  pc_main_menu        = 1,
  pc_set_timer        = 2,
  pc_startfunction    = 3,
  pc_timer            = 4,
  pc_set_pwm          = 5,
  pc_finishfunction   = 6,

  pc_using_only_timer = 0b00000000,
  pc_using_exposing   = 0b00100000,
  pc_using_stirring   = 0b01000000,
  pc_using__iterator  = 0b00100000,

  pc_function_code    = 0b01100000,
  pc_function_max     = 0b01100000,
  pc_actual_code      = 0b00011111
} program_context;


volatile bool _intervalbeep_dobeep = FALSE;
volatile bool _timeset_dir;
volatile uint8_t _timeset_beep = 0;
volatile uint8_t _timeset_intervalbeep = 0;
volatile uint8_t _timeset_repeat = 0;
volatile uint8_t _timerhold_id = 0;
volatile uint8_t _timercancel_id = 0;
volatile uint8_t _timerpwm_id;
volatile uint8_t _timerfunc_id;
volatile uint16_t _pwm_uvlight = 0;
volatile uint16_t _pwm_motor = 0;

volatile bool _pwm_isupdate = FALSE;
volatile uint8_t _pwm_current_func = 0;

uint8_t _input_last = 0;
uint16_t _previous_filllen;

uint16_t _timemax;
volatile uint16_t _timeleft;

uint16_t _timer_settime = SETTIMER_INCREMENT;

volatile program_context _current_context = pc_main_menu;
program_context _function_context = pc_using_only_timer;


void init_tools(){
  timer2_setup(TIM2_PSCR, TIM2_ARR);

  TIM2->CCMR2 |= TIM2_OCMODE_PWM1;
  TIM2->CCMR3 |= TIM2_OCMODE_PWM1;
}

void enable_uvlight(){
  TIM2->CCER1 |= TIM2_CCER1_CC2E;
}

void enable_motor(){
  TIM2->CCER2 |= TIM2_CCER2_CC3E;
}

void disable_uvlight(){
  TIM2->CCER1 &= ~TIM2_CCER1_CC2E;
}

void disable_motor(){
  TIM2->CCER2 &= ~TIM2_CCER2_CC3E;
}

void set_uvlight(uint16_t dutycycle){
  SETREG_LH(TIM2->CCR2, dutycycle);
  _pwm_uvlight = dutycycle;
}

void set_motor(uint16_t dutycycle){
  SETREG_LH(TIM2->CCR3, dutycycle);
  _pwm_motor = dutycycle;
}

void beep_setup(){
  BEEP->CSR = BEEP_SEL | BEEP_DIV;
}

void beep_start(){
  BEEP->CSR |= BEEP_CSR_BEEPEN;
}

void beep_stop(){
  BEEP->CSR &= ~BEEP_CSR_BEEPEN;
}


uint16_t _get_range(uint16_t high, uint16_t middle, uint16_t highrange);
void update_pwmtools(){
  uint16_t _pwm = _get_range(0xffff, adc1_get_conversion(), TIM2_ARR);

  switch(_pwm_current_func){
    break; case pc_using_exposing:
      set_uvlight(_pwm);
    
    break; case pc_using_stirring:
      set_motor(_pwm);
  }
}


// return a string about time
// using gpmem, don't forget to release it
char *get_timestr(uint16_t current_time){
  uint16_t _timeleft_seconds = 0, _timeleft_minutes = 0, _timeleft_hours = 0;

  _timeleft_seconds = current_time % 60;
  _timeleft_minutes = (current_time / 60) % 60;
  _timeleft_hours = current_time / 3600;

  char *gpmem;
  while((gpmem = useGPMEM()) == 0)
    ;

  char *resstr = &gpmem[GPMEM_BUFSIZE/2];

  if(_timeleft_hours > 0){
    sprintf(gpmem, "%dH%c%dM", _timeleft_hours, _timeleft%2? ' ': ':', _timeleft_minutes);

    sprintf(resstr, _TIME_DISP_PADDING, gpmem);
  }
  else if(_timeleft_minutes > 0){
    sprintf(gpmem, "%dM%c%dS", _timeleft_minutes, _timeleft%2? ' ': ':', _timeleft_seconds);

    sprintf(resstr, _TIME_DISP_PADDING, gpmem);
  }
  else{
    sprintf(gpmem, "%dS", _timeleft_seconds);

    sprintf(resstr, _TIME_DISP_PADDING, gpmem);
  }

  return resstr;
}

uint16_t _get_range(uint16_t high, uint16_t middle, uint16_t rangehigh){
  uint16_t res;
  if(high >= rangehigh)
    res = middle / (high / rangehigh);
  else
    res = middle * rangehigh / high;
  
  return res;
}

void init_input(){
  // using 4, 5, 6 as input floating
  GPIOD->DDR &= ~(INPUTPORTD_BUTTON1 | INPUTPORTD_BUTTON2 | INPUTPORTD_BUTTON3);
  GPIOD->CR1 &= ~(INPUTPORTD_BUTTON1 | INPUTPORTD_BUTTON2 | INPUTPORTD_BUTTON3);
  GPIOD->CR2 = 0;
}

bool _checkif_pwm_func(){
  switch(_pwm_current_func){
    break;
      case pc_using_exposing:
      case pc_using_stirring:
        return TRUE;
  }

  return FALSE;
}

void _iterate_pwm_func(){
  bool keep_looping = TRUE;
  while(keep_looping){
    _pwm_current_func += pc_using__iterator;
    if(_pwm_current_func >= pc_function_max)
      _pwm_current_func = pc_using_only_timer;

    keep_looping = !_checkif_pwm_func();
  }
}

void change_program_context(program_context pc);
void oninput_cancel(){
  uint8_t _current_actual = _current_context & pc_actual_code;

  switch(_current_actual){
    break;
      case pc_timer:
      case pc_set_timer:
        change_program_context(_current_context & ~pc_actual_code | pc_main_menu);
  }
}

void updatedraw_timer(uint16_t time);
void oninput_repeat(){
  uint16_t _actual_code = _current_context & pc_actual_code;

  switch(_actual_code){
    break; case pc_set_timer:{
      uint16_t _iter;
      if(_timeset_repeat <= SETTIMER_MAXREPEAT)
        _timeset_repeat++;

      _iter = SETTIMER_INCREMENT * (1 << (_timeset_repeat / INPUT_REPEAT_TIMES));
      
      if(_timeset_dir){
        if(_iter > (SETTIMER_MAX - _timer_settime))
          _timer_settime = SETTIMER_MAX;
        else
          _timer_settime += _iter;
      }
      else{
        if(_iter > (_timer_settime - SETTIMER_MIN))
          _timer_settime = SETTIMER_MIN;
        else
          _timer_settime -= _iter;
      }

      updatedraw_timer(_timer_settime);
    }
  }
}

void oninput_hold(){
  _timerhold_id = timer1_interval(INPUT_TIME_REPEAT, oninput_repeat);
  
  oninput_repeat();
}

void updatedraw_timer(uint16_t time);
void updatedraw_pwmbar(bool _forcedraw);
void change_display_function_select(program_context pc, uint8_t ddram_address);
void oninput_different(){
  uint8_t _context = _current_context & pc_actual_code;
  uint8_t _inputs = GPIOD->IDR & INPUTPORTD_BUTTONS;
  switch(_context){
    break; case pc_main_menu:{
      if(_inputs & INPUTPORTD_BUTTON3){
        _function_context += pc_using__iterator;

        if(_function_context >= pc_function_max)
          _function_context = pc_using_only_timer;
      }
      else if(_inputs & INPUTPORTD_BUTTON1){
        if(_function_context >= pc_using__iterator)
          _function_context -= pc_using__iterator;
        else
          _function_context = pc_function_max - pc_using__iterator;
      }
      else if(_inputs & INPUTPORTD_BUTTON2){
        change_program_context(_function_context | pc_set_timer);
        break;
      }
      
      change_display_function_select(_function_context, 0x40);
    }

    break; case pc_set_timer:{
      if(_inputs & (INPUTPORTD_BUTTON1 | INPUTPORTD_BUTTON3)){
        if(_inputs & INPUTPORTD_BUTTON3)
          _timeset_dir = TRUE;
        
        if(_inputs & INPUTPORTD_BUTTON1)
          _timeset_dir = FALSE;
          
        if(!_timerhold_id){
          _timerhold_id = timer1_asyncdelay(INPUT_TIME_HOLD, oninput_hold);

          _timeset_repeat = 0;
          oninput_repeat();
        }
      }
      else if(_timerhold_id){
        timer1_stop(_timerhold_id);
        _timerhold_id = 0;
      }
      else if(_inputs & INPUTPORTD_BUTTON2){
        change_program_context(_current_context & ~pc_actual_code | pc_startfunction);
        break;
      }

      if((_inputs & INPUTPORTD_BUTTON1) && (_inputs & INPUTPORTD_BUTTON3))
        _timercancel_id = timer1_asyncdelay(INPUT_TIME_CANCEL, oninput_cancel);
      
      if(_timercancel_id && (!(_inputs & INPUTPORTD_BUTTON1) || !(_inputs & INPUTPORTD_BUTTON3))){
        timer1_stop(_timercancel_id);
        _timercancel_id = 0;
      }

      updatedraw_timer(_timer_settime);
    }

    break; case pc_timer:{
      if(!_timercancel_id && _inputs & INPUTPORTD_BUTTON2){
        change_program_context(_current_context & ~pc_actual_code | pc_set_pwm);
        break;
      }

      if((_inputs & INPUTPORTD_BUTTON1) && (_inputs & INPUTPORTD_BUTTON3))
        _timercancel_id = timer1_asyncdelay(INPUT_TIME_CANCEL, oninput_cancel);
      
      if(_timercancel_id && (!(_inputs & INPUTPORTD_BUTTON1) || !(_inputs & INPUTPORTD_BUTTON3))){
        timer1_stop(_timercancel_id);
        _timercancel_id = 0;
      }
    }

    break; case pc_set_pwm:{
      if(!(_inputs & INPUTPORTD_BUTTON2)){
        change_program_context(_current_context & ~pc_actual_code | pc_timer);
        break;
      }
      
      _pwm_isupdate = (_inputs & INPUTPORTD_BUTTON1) > 0;
      if(_inputs & INPUTPORTD_BUTTON3){
        _iterate_pwm_func();

        change_display_function_select(_pwm_current_func, 0x0);
        updatedraw_pwmbar(TRUE);
      }
    }

    break; case pc_finishfunction:{
      change_program_context(_current_context & ~pc_actual_code | pc_main_menu);
    }
  }

  _input_last = _inputs;
}


void init_bardraw(){
  LCD_setCGRAM(LCD_SETCGRAM_GETADDR8(_BAR_MIDDLEEMPTY_ADDR));
  LCD_write(_BAR_MIDDLE_BG, _BAR_YSIZE);

  char *gpmem;
  while((gpmem = useGPMEM()) == 0)
    ;
  
  memcpy(gpmem, _BAR_MIDDLE_BG, _BAR_YSIZE);
  for(uint16_t i = 0; i < _BAR_YSIZE; i++)
    gpmem[i] |= _BAR_MIDDLE_FG[i];
  
  LCD_setCGRAM(LCD_SETCGRAM_GETADDR8(_BAR_MIDDLEFULL_ADDR));
  LCD_write(gpmem, _BAR_YSIZE);

  releaseGPMEM();
}

void _get_barfill(uint16_t emptyfill, char *dst, char *bg, char *fg){
  memcpy(dst, bg, _BAR_YSIZE);
  for(uint16_t ibar = 0; ibar < _BAR_YSIZE; ibar++)
    dst[ibar] |= fg[ibar] & (0xff << emptyfill);
}

void updatedraw_bar(uint16_t high, uint16_t middle, bool _forcedraw){
  uint16_t filllen = _get_range(high, middle, _BAR_FILLLEN);

  if(!_forcedraw && filllen == _previous_filllen)
    return;
  
  _previous_filllen = filllen;

  uint8_t _currentddram = _BAR_STARTPOS;
  char *gpmem;
  while((gpmem = useGPMEM()) == 0)
    ;


  // start bar
  _get_barfill(
    _BAR_START_FILLLEN - MIN(filllen, _BAR_START_FILLLEN),
    gpmem,
    _BAR_START_BG,
    _BAR_START_FG
  ); filllen -= MIN(filllen, _BAR_START_FILLLEN);

  LCD_setCGRAM(LCD_SETCGRAM_GETADDR8(_BAR_START_ADDR));
  LCD_write(gpmem, _BAR_YSIZE);

  LCD_setDDRAM(_currentddram); _currentddram++;
  gpmem[0] = _BAR_START_ADDR;
  LCD_write(gpmem, 1);
  

  // middle bar(s)
  for(uint16_t ibar = 0; ibar < _BAR_MIDDLELENGTH; ibar++){
    uint8_t _cgram;
    if(filllen >= _BAR_MIDDLE_FILLLEN)
      _cgram = _BAR_MIDDLEFULL_ADDR;
    else if(filllen == 0)
      _cgram = _BAR_MIDDLEEMPTY_ADDR;
    else{
      _cgram = _BAR_MIDDLEVAR_ADDR;

      _get_barfill(
        _BAR_MIDDLE_FILLLEN - MIN(filllen, _BAR_MIDDLE_FILLLEN),
        gpmem,
        _BAR_MIDDLE_BG,
        _BAR_MIDDLE_FG
      );

      LCD_setCGRAM(LCD_SETCGRAM_GETADDR8(_BAR_MIDDLEVAR_ADDR));
      LCD_write(gpmem, _BAR_YSIZE);
    }

    filllen -= MIN(filllen, _BAR_MIDDLE_FILLLEN);
    LCD_setDDRAM(_currentddram); _currentddram++;
    LCD_write(&_cgram, 1);
  }

  
  // end bar
  _get_barfill(
    (_BAR_XSIZE - _BAR_END_FILLLEN) + _BAR_END_FILLLEN - MIN(filllen, _BAR_END_FILLLEN),
    gpmem,
    _BAR_END_BG,
    _BAR_END_FG
  ); filllen -= MIN(filllen, _BAR_END_FILLLEN);

  LCD_setCGRAM(LCD_SETCGRAM_GETADDR8(_BAR_END_ADDR));
  LCD_write(gpmem, _BAR_YSIZE);

  LCD_setDDRAM(_currentddram); _currentddram++;
  gpmem[0] = _BAR_END_ADDR;
  LCD_write(gpmem, 1);


  releaseGPMEM();
}


void updatedraw_timer(uint16_t current_time){
  LCD_setDDRAM(_TIME_DISP_STARTPOS);
  LCD_write(_PROG_DISP_CLEARLINE_TIME, strlen(_PROG_DISP_CLEARLINE_TIME));

  LCD_setDDRAM(_TIME_DISP_STARTPOS);
  const char* timestr = get_timestr(current_time);
  LCD_write(timestr, strlen(timestr));
  releaseGPMEM();
}

void updatedraw_pwmbar(bool _forcedrawbar){
  uint16_t _pwmdc = 0;
  switch(_pwm_current_func){
    break; case pc_using_exposing:
      _pwmdc = _pwm_uvlight;
    
    break; case pc_using_stirring:
      _pwmdc = _pwm_motor;
  }

  uint16_t _range = _get_range(TIM2_ARR, _pwmdc, 100);
  _range = MIN(_range, 100);

  char *gpmem;
  while(!(gpmem = useGPMEM()))
    ;

  sprintf(gpmem, "%d%%", _range);

  LCD_setDDRAM(0x40);
  LCD_write(_PROG_DISP_CLEARLINE_TIME, strlen(_PROG_DISP_CLEARLINE_TIME));
  LCD_setDDRAM(0x40);
  LCD_write(gpmem, strlen(gpmem));
  releaseGPMEM();

  updatedraw_bar(TIM2_ARR, _pwmdc, _forcedrawbar);
}

void updatedraw_timer_bar(bool _forcedraw){
  updatedraw_timer(_timeleft);
  updatedraw_bar(_timemax, _timeleft, _forcedraw);
}


void onfinish_timer();
void on1s_timer_bypass(){
  _timeleft--;
}

void onupdatelcd_pwm(){
  uint8_t _actualcontext = _current_context & pc_actual_code;
  if(_actualcontext == pc_set_pwm){
    updatedraw_pwmbar(FALSE);

    if(_pwm_isupdate)
      update_pwmtools();
  }
}

void onupdatelcd_timer(){
  uint8_t _actualcontext = _current_context & pc_actual_code;
  if(_actualcontext == pc_timer)
    updatedraw_timer_bar(FALSE);
  
  if(_timeleft == 0){
    timer1_stop(_timerfunc_id);
    onfinish_timer();
  }
}

void start_timer(uint16_t seconds){
  _timeleft = seconds;
  _timemax = seconds;

  while(!(_timerfunc_id = timer1_interval(1000, onupdatelcd_timer)))
    ;

  timer1_bypasscheck(_timerfunc_id, on1s_timer_bypass);
}

void change_program_context(program_context pc);
void onfinish_timer(){
  change_program_context(_current_context & ~pc_actual_code | pc_finishfunction);
}


void display_clear(){
  LCD_clearDisplay();
  
  // somehow it needs to delay
  timer1_delay(50);
}

void change_display_function_select(program_context pc, uint8_t ddram_address){
  LCD_setDDRAM(ddram_address);
  LCD_write(_PROG_DISP_CLEARLINE, strlen(_PROG_DISP_CLEARLINE));

  const char *_funcstr = "?";
  uint8_t _funccode = pc & pc_function_code;
  switch(_funccode){
    break; case pc_using_only_timer:
      _funcstr = _PROG_DISP_MAIN_MENU_SELECT_NONE;
    
    break; case pc_using_exposing:
      _funcstr = _PROG_DISP_MAIN_MENU_SELECT_EXPOSE;
    
    break; case pc_using_stirring:
      _funcstr = _PROG_DISP_MAIN_MENU_SELECT_STIR;
  }

  LCD_setDDRAM(ddram_address);
  LCD_write(_funcstr, strlen(_funcstr));
}

void onbeepdone();
void onbeepinterval();
void change_program_context(program_context pc){
  uint8_t _actual_code = pc & pc_actual_code;
  uint8_t _last_actual_code = _current_context & pc_actual_code;

  switch(_last_actual_code){
    break; case pc_set_timer:
      timer1_stop(_timercancel_id);
      timer1_stop(_timerhold_id);

      _timercancel_id = 0;
      _timerhold_id = 0;
    
    break; case pc_timer:
      timer1_stop(_timercancel_id);
      _timercancel_id = 0;

    break; case pc_set_pwm:
      timer1_stop(_timerpwm_id);
      _timerpwm_id = 0;
      _pwm_isupdate = FALSE;
    
    break; case pc_finishfunction:
      timer1_stop(_timeset_beep);
      timer1_stop(_timeset_intervalbeep);

      _timeset_beep = 0;
      _timeset_intervalbeep = 0;
  }

  bool _changecode = TRUE;
  switch(_actual_code){
    break; case pc_main_menu:{
      // check current context
      if(_last_actual_code == pc_timer || _last_actual_code == pc_set_pwm){
        uint16_t _funccode = _current_context & pc_function_code;

        switch(_funccode){
          break; case pc_using_exposing:
            disable_uvlight();
          
          break; case pc_using_stirring:
            disable_motor();
        }

        timer1_stop(_timerfunc_id);
        display_clear();

        LCD_setDDRAM(0);
        LCD_write(_PROG_DISP_CANCELLED, strlen(_PROG_DISP_CANCELLED));

        timer1_delay(3000);
      }

      display_clear();
      LCD_setDDRAM(0);
      LCD_write(_PROG_DISP_MAIN_MENU, strlen(_PROG_DISP_MAIN_MENU));

      change_display_function_select(pc, 0x40);
    }

    break; case pc_set_timer:{
      display_clear();

      LCD_setDDRAM(0);
      LCD_write(_PROG_DISP_SETTIMER_TITLE, strlen(_PROG_DISP_SETTIMER_TITLE));

      updatedraw_timer(_timer_settime);
    }

    break; case pc_startfunction:{
      start_timer(_timer_settime);

      uint8_t _funccode = pc & pc_function_code;
      switch(_funccode){
        break; case pc_using_exposing:
          enable_uvlight();

        break; case pc_using_stirring:
          enable_motor();
      }

      _pwm_current_func = _funccode;
      if(!_checkif_pwm_func())
        _iterate_pwm_func();

      change_program_context(_current_context & ~pc_actual_code | pc_timer);
      _changecode = FALSE;
    }

    break; case pc_timer:{
      if(_last_actual_code == pc_set_pwm){
        timer1_stop(_timerpwm_id);
        _timerpwm_id = 0;
      }

      _timercancel_id = 0;

      change_display_function_select(_current_context, 0x0);

      updatedraw_timer_bar(TRUE);
    }

    break; case pc_set_pwm:{
      if(!_timerpwm_id)
        _timerpwm_id = timer1_interval(300, onupdatelcd_pwm);
      
      change_display_function_select(_pwm_current_func, 0x0);

      updatedraw_pwmbar(TRUE);
    }

    break; case pc_finishfunction:{
      if(_last_actual_code == pc_set_pwm){
        timer1_stop(_timerpwm_id);
        _timerpwm_id = 0;
      }


      if(!_timeset_beep){
        _timeset_beep = timer1_asyncdelay(BEEPDONE_TIME, _dmpfunc_pv);

        timer1_bypasscheck(_timeset_beep, onbeepdone);
      }
      
      if(!_timeset_intervalbeep){
        _timeset_intervalbeep = timer1_interval(BEEPDONE_INTERVAL, _dmpfunc_pv);
        _intervalbeep_dobeep = TRUE;

        timer1_bypasscheck(_timeset_intervalbeep, onbeepinterval);

        onbeepinterval();
      }

      uint16_t _funccode = _current_context & pc_function_code;

      switch(_funccode){
        break; case pc_using_exposing:
          disable_uvlight();
        
        break; case pc_using_stirring:
          disable_motor();
      }

      LCD_setDDRAM(0);
      LCD_write(_PROG_DISP_CLEARLINE, strlen(_PROG_DISP_CLEARLINE));

      LCD_setDDRAM(0);
      LCD_write(_PROG_DISP_DONE, strlen(_PROG_DISP_DONE));
    }
  }

  if(_changecode)
    _current_context = pc;
}

void init_itcpriority(){
  // set itc for i2c (19)
  ITC->ISPR5 &= 0b00111111;
  ITC->ISPR5 |= 0b01000000;

  // set itc for tim1 capcom (12)
  ITC->ISPR4 &= 0b11111100;
}

void reinit_display(){
  LCD_displayControl(LCD_DISPLAYCONTROL_D);
  LCD_entryMode(LCD_ENTRYMODE_ID);
  LCD_functionSet(LCD_FUNCTIONSET_N);
  LCD_returnHome();
}


void onbeepdone(){
  timer1_stop(_timeset_beep);
  timer1_stop(_timeset_intervalbeep);

  _timeset_beep = 0;
  _timeset_intervalbeep = 0;

  beep_stop();
}

void onbeepinterval(){
  if(_intervalbeep_dobeep)
    beep_start();
  else
    beep_stop();
  
  _intervalbeep_dobeep = !_intervalbeep_dobeep;
}


int main(){
  disableInterrupts();
  init_itcpriority();

  init_input();

  i2c_setup(I2C_CLOCK_SPD);

  timer1_setup(TIM1_PSCR);

  beep_setup();
  
  init_tools();

  adc1_turnon();
  adc1_change_channel(2);
  adc1_set_schmitt(2, FALSE);
  enableInterrupts();

  CLK->HSITRIMR = 0;
  CLK->CKDIVR = 0;

  LCD_useAddress(LCD_I2C_ADDRESS);
  LCD_initLCD();
  reinit_display();

  init_bardraw();

  timer1_delay(2000);

  _current_context = 0;
  change_program_context(pc_main_menu);

  while(TRUE){
    if(_input_last != (GPIOD->IDR & INPUTPORTD_BUTTONS))
      oninput_different();

    timer1_interruptcheck();
  }
}


INTERRUPT_HANDLER(__i2c_on_interrupt, ITC_IRQ_I2C){
  i2c_on_interrupt();
}

INTERRUPT_HANDLER(__timer1_on_capcom, ITC_IRQ_TIM1_CAPCOM){
  timer1_on_capcom();
}