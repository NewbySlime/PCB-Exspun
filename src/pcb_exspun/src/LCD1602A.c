#include "LCD1602A.h"
#include "I2C.h"
#include "timer.h"

#include "misc.h"


#define LCD_RS 0b1
#define LCD_RW 0b10
#define LCD_E 0b100
#define LCD_BT 0b1000

#define LCD_I2C_CLOCKINSEND(conf, byte, close)\
  i2c_manual_sendbyte((byte & 0xf0) | conf | LCD_E, FALSE);\
  i2c_manual_sendbyte((byte & 0xf0) | conf, FALSE);\
  i2c_manual_sendbyte((byte << 4) | conf | LCD_E, FALSE);\
  i2c_manual_sendbyte((byte << 4) | conf, close)

#define LCD_I2C_CLOCKINSENDHALF(conf, halfbyte, close)\
  i2c_manual_sendbyte((halfbyte << 4) | conf | LCD_E, FALSE);\
  i2c_manual_sendbyte((halfbyte << 4) | conf, close)


uint8_t _LCD_curraddr;
uint8_t _LCD_defaultConf = LCD_BT;


bool LCD_write(const char *str, uint16_t len){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;

  uint8_t conf = _LCD_defaultConf | LCD_RS;
  for(uint16_t iter = 0; iter < len; iter++){
    uint8_t c = str[iter];
    LCD_I2C_CLOCKINSEND(conf, c, FALSE);
  }

  i2c_manual_stop();
  return TRUE;
}

// note: not tested
bool LCD_read(char *buffer, uint16_t len){
  uint8_t conf = _LCD_defaultConf | LCD_RS | LCD_RW;
  for(uint16_t iter = 0; iter < len; iter++){
    buffer[iter] = 0;
    uint8_t _tmp = 0;

    if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
      return;
    LCD_I2C_CLOCKINSENDHALF(conf, 0, TRUE);

    if(!i2c_manual_start(_LCD_curraddr, TRUE, FALSE))
      return;
    i2c_manual_readbyte(&_tmp, TRUE);
    buffer[iter] |= _tmp & 0xf0;

    if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
      return;
    LCD_I2C_CLOCKINSENDHALF(conf, 0, TRUE);

    if(!i2c_manual_start(_LCD_curraddr, TRUE, FALSE))
      return;
    i2c_manual_readbyte(&_tmp, TRUE);
    buffer[iter] |= (_tmp >> 4) & 0xf;
  }

  return TRUE;
}


bool LCD_clearDisplay(){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;

  LCD_I2C_CLOCKINSEND(_LCD_defaultConf, LCD_CLEARDISPLAY_CODE, TRUE);

  timer1_delay(2);

  return TRUE;
}

bool LCD_returnHome(){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;

  LCD_I2C_CLOCKINSEND(_LCD_defaultConf, LCD_RETURNHOME_CODE, TRUE);
  
  timer1_delay(2);

  return TRUE;
}

bool LCD_entryMode(uint8_t code){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;

  code |= LCD_ENTRYMODE_CODE;
  LCD_I2C_CLOCKINSEND(_LCD_defaultConf, code, TRUE);

  timer1_delay(1);

  return TRUE;
}

bool LCD_displayControl(uint8_t code){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;
  
  code |= LCD_DISPLAYCONTROL_CODE;
  LCD_I2C_CLOCKINSEND(_LCD_defaultConf, code, TRUE);

  timer1_delay(1);

  return TRUE;
}

bool LCD_cdShift(uint8_t code){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;
  
  code |= LCD_CDSHIFT_CODE;
  LCD_I2C_CLOCKINSEND(_LCD_defaultConf, code, TRUE);
  
  timer1_delay(1);

  return TRUE;
}

bool LCD_functionSet(uint8_t code){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;
  
  code |= LCD_FUNCTIONSET_CODE;
  LCD_I2C_CLOCKINSEND(_LCD_defaultConf, code, TRUE);

  timer1_delay(1);

  return TRUE;
}

bool LCD_setCGRAM(uint8_t address){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;

  address = LCD_SETCGRAM_GETCODE(address);
  LCD_I2C_CLOCKINSEND(_LCD_defaultConf, address, TRUE);

  timer1_delay(1);

  return TRUE;
}

bool LCD_setDDRAM(uint8_t address){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;
  
  address = LCD_SETDDRAM_GETCODE(address);
  LCD_I2C_CLOCKINSEND(_LCD_defaultConf, address, TRUE);

  timer1_delay(1);
  
  return TRUE;
}

bool LCD_readBusyAddress(uint8_t code, uint8_t address){
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return TRUE;

  address = LCD_READBUSYADDRESS_GETCODE(address) | code;
  LCD_I2C_CLOCKINSEND(_LCD_defaultConf, address, TRUE);

  return TRUE;
}


bool LCD_useAddress(uint8_t address){
  _LCD_curraddr = address;
}

bool LCD_initLCD(){
  timer1_delay(17);
  if(!i2c_manual_start(_LCD_curraddr, FALSE, FALSE))
    return FALSE;

  uint8_t _currentbyte = 0b0011;
  LCD_I2C_CLOCKINSENDHALF(0, _currentbyte, FALSE);

  timer1_delay(6);
  LCD_I2C_CLOCKINSENDHALF(0, _currentbyte, FALSE);

  LCD_I2C_CLOCKINSENDHALF(0, _currentbyte, FALSE);

  _currentbyte = 0b0010;
  LCD_I2C_CLOCKINSENDHALF(0, _currentbyte, TRUE);

  LCD_functionSet(LCD_FUNCTIONSET_DEFAULT);
  LCD_displayControl(LCD_DISPLAYCONTROL_CODE);
  LCD_clearDisplay();
  LCD_entryMode(LCD_ENTRYMODE_CODE);

  return TRUE;
}