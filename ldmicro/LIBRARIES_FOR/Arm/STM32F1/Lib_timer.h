/**
 * Timer Library for STM32F10x devices
 */

#include "stm32f10x_tim.h"

#include "misc.h"

/// TIM1 est un timer pour la PWM
/// TIM2, TIM3 et TIM4 sont des timers 16 bits

void LibTimer_Init(TIM_TypeDef *TIMx, uint16_t prediv, uint32_t period);

void LibTimer_Interrupts(TIM_TypeDef *TIMx, FunctionalState able);

void LibTimer_Stop(TIM_TypeDef *TIMx);
