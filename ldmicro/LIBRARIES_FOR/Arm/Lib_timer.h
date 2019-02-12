/**
 * Timer Library for STM32F4xx devices
 */


#include "stm32f4xx_tim.h"

#include "misc.h"


/// TIM2 et TIM5 sont des timers 32 bits
/// TIM3 et TIM4 sont des timers 16 bits

void LibTimer_Init(TIM_TypeDef * TIMx, uint16_t prediv, uint32_t period);

void LibTimer_Interrupts(TIM_TypeDef * TIMx, FunctionalState able);
