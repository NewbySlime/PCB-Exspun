#include "ADC.h"
#include "misc.h"


void adc1_turnon(){
  ADC1->CR1 |= ADC1_CR1_ADON;
}

void adc1_turnoff(){
  ADC1->CR1 &= ~ADC1_CR1_ADON;
}


void adc1_set_schmitt(uint8_t channel, bool set){
  if(channel < 8){
    if(set)
      ADC1->TDRL &= ~(1 << channel);
    else
      ADC1->TDRL |= (1 << channel);
  }
  else{
    channel -= 8;
    if(set)
      ADC1->TDRH &= ~(1 << channel);
    else
      ADC1->TDRH |= (1 << channel);
  }
}

void adc1_change_channel(uint8_t channel){
  ADC1->CSR &= ~0b1111;
  ADC1->CSR |= channel & 0b1111;
}


void adc1_use_continous(){
  ADC1->CR1 |= ADC1_CR1_CONT | ADC1_CR1_ADON;
}

void adc1_use_single(){
  ADC1->CR1 &= ~ADC1_CR1_CONT;
}


uint16_t adc1_get_conversion(){
  if(!(ADC1->CR1 & ADC1_CR1_CONT)){
    ADC1->CR1 |= ADC1_CR1_ADON;
    while(!(ADC1->CSR & ADC1_CSR_EOC))
      ;

    ADC1->CSR &= ~ADC1_CSR_EOC;
  }
  
  return GETREG_LH(ADC1->DR);
}