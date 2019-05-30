/**
 * ADC Library for STM32F10x devices
 */

#ifndef CAN_H
#define CAN_H 120

/**
    CHANNEL   ADC1   	ADC2

    0         PA0    	PA0
    1         PA1    	PA1
    2         PA2    	PA2
    3         PA3    	PA3
    4         PA4    	PA4
    5         PA5    	PA5
    6         PA6    	PA6
    7         PA7    	PA7
    Other channels not supported...
 */

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_adc.h"


void LibADC_Init(ADC_TypeDef* ADCx, uint8_t channel);

uint16_t LibADC_Read(ADC_TypeDef* ADCx, uint8_t channel);

void LibADC_Stop(ADC_TypeDef* ADCx);

#endif
