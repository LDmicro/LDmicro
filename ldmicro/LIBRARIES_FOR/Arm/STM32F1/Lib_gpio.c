/**
 * Copyright (C) Tilen Majerle 2015, and JG 2019
 */

#include "stm32f10x_rcc.h"
#include "lib_gpio.h"

void LibGPIO_Conf(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, LibGPIO_Mode_t GPIO_Mode, LibGPIO_Speed_t GPIO_Speed)
{
    GPIO_InitTypeDef gpioInit; // structure de configuration des ports

    if(GPIOx == GPIOA)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); /// activation horloge Port
    if(GPIOx == GPIOB)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); /// activation horloge Port
    if(GPIOx == GPIOC)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); /// activation horloge Port

    gpioInit.GPIO_Mode = GPIO_Mode;   // entrees ou sorties avec ou sans pull-up / down
    gpioInit.GPIO_Pin = GPIO_Pin;     // broches a configurer
    gpioInit.GPIO_Speed = GPIO_Speed; // frequence

    GPIO_Init(GPIOx, &gpioInit); /// configuration Port
}

uint16_t LibGPIO_GetPortSource(GPIO_TypeDef *GPIOx)
{
    // Get port source number
    // Offset from GPIOA                       Difference between 2 GPIO addresses
    return ((uint32_t)GPIOx - (GPIOA_BASE)) / ((GPIOB_BASE) - (GPIOA_BASE));
}
