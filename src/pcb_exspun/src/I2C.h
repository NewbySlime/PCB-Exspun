#ifndef I2C_HEADER
#define I2C_HEADER

#include "stm8s.h"

typedef void(*i2c_callback)(uint16_t bytesrw);

typedef enum{
  I2C_STATUS_NACK,
  I2C_STATUS_BUSERROR,
  I2C_STATUS_BUSY         = 0b01000000,
  // if 0, it means sending
  // if 1, it means reading
  I2C_STATUS_SENDREAD     = 0b10000000
} i2c_status;

void i2c_setup(uint16_t clkspd);

bool i2c_senddata(uint8_t address, uint8_t *data, uint16_t datalen);
bool i2c_readdata(uint8_t address, uint8_t *buf, uint16_t buflen);

void i2c_setcallback(i2c_callback cb);

i2c_status i2c_current_status();

void i2c_on_interrupt();

bool i2c_manual_start(uint8_t address, bool isRecv, bool close);
bool i2c_manual_sendbyte(uint8_t byte, bool close);
bool i2c_manual_readbyte(uint8_t *byte, bool close);
bool i2c_manual_stop();

#endif