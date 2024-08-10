#include "I2C.h"
#include "timer.h"
#include "misc.h"


#define POLLING_STATUS(reg, n)\
  while(!(reg & n) & !_i2c_checkerr(___i2c_dumpfunc))\
    ;\
  \
  if(_lasterrcode)\
    return FALSE


/*        VARIABLES       */

volatile uint8_t _lasterrcode;

volatile uint8_t _addr;
volatile uint8_t *_buf;
volatile uint16_t _buflen, _bufiter;

volatile bool _sb_send;
bool _manually_started;

void ___i2c_dumpfunc(uint16_t _n){}
volatile i2c_callback _cb = ___i2c_dumpfunc;



/*        FUNCTIONS       */

bool _i2c_checkerr(i2c_callback cb){
  _lasterrcode = I2C->SR2;
  if(_lasterrcode){
    if(_lasterrcode & I2C_SR2_OVR)
      I2C->DR = _buf[_bufiter-1];

    if(_lasterrcode & I2C_SR2_AF){
      I2C->CR2 |= I2C_CR2_STOP;
      cb(_bufiter);
    }

    if(_lasterrcode & I2C_SR2_BERR)
      cb(_bufiter-1);

    I2C->SR2 = 0;

    return TRUE;
  }

  return FALSE;
}

void i2c_setup(uint16_t clkspd){
  I2C->CR1 = I2C_CR1_PE;

  I2C->FREQR = 0b10;

  I2C->OARL = I2C_CURRENT_ADDRESS << 1;
  I2C->OARH = I2C_OARH_ADDCONF;

  I2C->CCRL = clkspd & 0xff;
  I2C->CCRH = ((clkspd >> 8) & 0xf);
}

bool i2c_senddata(uint8_t address, uint8_t *data, uint16_t datalen){
  if(I2C->SR3 & I2C_SR3_BUSY)
    return FALSE;

  _addr = address << 1;
  _buf = data;
  _buflen = datalen;
  _bufiter = 0;
  _sb_send = FALSE;

  I2C->ITR = I2C_ITR_ITBUFEN | I2C_ITR_ITEVTEN | I2C_ITR_ITERREN;
  I2C->CR2 |= I2C_CR2_START | I2C_CR2_ACK;

  return TRUE;
}

bool i2c_readdata(uint8_t address, uint8_t *buf, uint16_t buflen){
  if(I2C->SR3 & I2C_SR3_BUSY)
    return FALSE;

  _addr = (address << 1) | 1;
  _buf = buf;
  _buflen = buflen;
  _bufiter = 0;
  _sb_send = FALSE;

  I2C->ITR = I2C_ITR_ITBUFEN | I2C_ITR_ITEVTEN | I2C_ITR_ITERREN;
  I2C->CR2 |= I2C_CR2_START | I2C_CR2_ACK;

  return TRUE;
}


void i2c_setcallback(i2c_callback cb){
  _cb = cb;
}


i2c_status i2c_current_status(){
  if(_lasterrcode & I2C_SR2_AF)
    return I2C_STATUS_NACK;
  
  if(_lasterrcode & I2C_SR2_BERR)
    return I2C_STATUS_BUSERROR;
    
  i2c_status _status;
  if(I2C->SR3 & I2C_SR3_BUSY)
    _status = I2C_STATUS_BUSY;
  
  _status |= (I2C->SR3 & I2C_SR3_TRA) * I2C_STATUS_SENDREAD;

  return _status;
}


void i2c_on_interrupt(){
  uint8_t regist = I2C->SR1;
  if(regist){
    if(regist & I2C_SR1_TXE)
      if(_bufiter < _buflen)
        I2C->DR = _buf[_bufiter++];
      else
        I2C->CR2 |= I2C_CR2_STOP;
    
    if(regist & I2C_SR1_RXNE){
      if(_bufiter < _buflen){
        _buf[_bufiter++] = I2C->DR;
        
        if(_bufiter >= (_buflen-1))
          I2C->CR2 &= ~I2C_CR2_ACK;
      }
      else{
        I2C->CR2 |= I2C_CR2_STOP;
        _cb(_bufiter);
      }
    }
    
    if(regist & I2C_SR1_ADDR && _sb_send){
      // transmitter
      if(I2C->SR3 & I2C_SR3_TRA){
        if(_bufiter < _buflen)
          I2C->DR = _buf[0];
        else{
          I2C->CR2 |= I2C_CR2_STOP;
          _cb(_bufiter);
        }
      }
      // receiver
      else{
        if(_bufiter >= _buflen){
          I2C->CR2 |= I2C_CR2_STOP;
          _cb(_bufiter);
        }
      }

      _sb_send = FALSE;
    }
    
    if(regist & I2C_SR1_SB){
      I2C->DR = _addr;
      _sb_send = TRUE;
    }
  }

  _i2c_checkerr(_cb);
}


bool i2c_manual_start(uint8_t address, bool isRecv, bool close){
  if(I2C->SR3 & I2C_SR3_BUSY)
    return FALSE;
  
  _manually_started = TRUE;
  I2C->ITR = 0;

  I2C->CR2 |= I2C_CR2_START | I2C_CR2_ACK;
  POLLING_STATUS(I2C->SR1, I2C_SR1_SB);
  
  I2C->DR = (address << 1) | isRecv;
  POLLING_STATUS(I2C->SR1, I2C_SR1_ADDR);
  I2C->SR3;

  if(close)
    i2c_manual_stop();

  return TRUE;
}

bool i2c_manual_sendbyte(uint8_t byte, bool close){
  if(!_manually_started || !(I2C->SR3 & I2C_SR3_TRA))
    return FALSE;
  
  I2C->DR = byte;
  POLLING_STATUS(I2C->SR1, I2C_SR1_TXE);

  if(close)
    i2c_manual_stop();

  return TRUE;
}

bool i2c_manual_readbyte(uint8_t *byte, bool close){
  if(!_manually_started || (I2C->SR3 & I2C_SR3_TRA))
    return FALSE;
  
  if(close)
    I2C->CR2 &= ~I2C_CR2_ACK;
  
  POLLING_STATUS(I2C->SR1, I2C_SR1_RXNE);
  *byte = I2C->DR;

  if(close)
    i2c_manual_stop();
  
  return TRUE;
}

bool i2c_manual_stop(){
  if(!_manually_started)
    return FALSE;
  
  I2C->CR2 |= I2C_CR2_STOP;
  while(I2C->SR3 & I2C_SR3_BUSY)
    ;

  _manually_started = FALSE;

  return TRUE;
}