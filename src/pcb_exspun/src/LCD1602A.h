#ifndef LCD1602A_HEADER
#define LCD1602A_HEADER

#include "stm8s_conf.h"

// for starting codes

#define LCD_CLEARDISPLAY_CODE 0b1
#define LCD_RETURNHOME_CODE 0b10
#define LCD_ENTRYMODE_CODE 0b100
#define LCD_DISPLAYCONTROL_CODE 0b1000
#define LCD_CDSHIFT_CODE 0b10000
#define LCD_FUNCTIONSET_CODE 0b100000
#define LCD_SETCGRAM_GETADDR8(n) ((n & 0b111) << 3) | 0b01000000
#define LCD_SETCGRAM_GETADDR10(n) ((n & 0b11) << 4) | 0b01000000
#define LCD_SETCGRAM_GETCODE(addr) (addr & 0b00111111) | 0b01000000
#define LCD_SETDDRAM_GETCODE(addr) (addr & 0b01111111) | 0b10000000
#define LCD_READBUSYADDRESS_GETCODE(addr) (addr & 0b01111111)

// for entry mode

#define LCD_ENTRYMODE_SH 0b1
#define LCD_ENTRYMODE_ID 0b10

// for display control

#define LCD_DISPLAYCONTROL_B 0b1
#define LCD_DISPLAYCONTROL_C 0b10
#define LCD_DISPLAYCONTROL_D 0b100

// for cursor display shift

#define LCD_CDSHIFT_RL 0b100
#define LCD_CDSHIFT_SC 0b1000

// for function set

#define LCD_FUNCTIONSET_DEFAULT LCD_FUNCTIONSET_CODE | LCD_FUNCTIONSET_F

#define LCD_FUNCTIONSET_F 0b100
#define LCD_FUNCTIONSET_N 0b1000
#define LCD_FUNCTIONSET_DL 0b10000

// for read busy address

#define LCD_READBUSYADDRESS_BF 0b10000000

bool LCD_write(const char *str, uint16_t len);
bool LCD_read(char *buffer, uint16_t len);

bool LCD_clearDisplay();
bool LCD_returnHome();
bool LCD_entryMode(uint8_t code);
bool LCD_displayControl(uint8_t code);
bool LCD_cdShift(uint8_t code);
bool LCD_functionSet(uint8_t code);
bool LCD_setCGRAM(uint8_t address);
bool LCD_setDDRAM(uint8_t address);
bool LCD_readBusyAddress(uint8_t code, uint8_t address);

bool LCD_useAddress(uint8_t address);
bool LCD_initLCD();


#endif