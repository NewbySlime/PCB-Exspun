#ifndef ADC_HEADER
#define ADC_HEADER

#include "stm8s.h"

void adc1_turnon();
void adc1_turnoff();

void adc1_set_schmitt(uint8_t channel, bool set);
void adc1_change_channel(uint8_t channel);

void adc1_use_continous();
void adc1_use_single();

uint16_t adc1_get_conversion();

#endif