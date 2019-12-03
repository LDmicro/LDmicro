/**
 * Copyright (C) Tilen Majerle 2015, and JG 2016
 */

#include "stm32f4xx_rcc.h"
#include "lib_gpio.h"

/* Private variables */
static uint16_t GPIO_UsedPins[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* Private functions */
void LibGPIO_INT_EnableClock(GPIO_TypeDef *GPIOx);
void LibGPIO_INT_DisableClock(GPIO_TypeDef *GPIOx);
void LibGPIO_INT_Init(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, LibGPIO_Mode_t GPIO_Mode, LibGPIO_OType_t GPIO_OType, LibGPIO_PuPd_t GPIO_PuPd, LibGPIO_Speed_t GPIO_Speed);

void LibGPIO_Conf(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, LibGPIO_Mode_t GPIO_Mode, LibGPIO_OType_t GPIO_Type, LibGPIO_Speed_t GPIO_Speed)
{
    GPIO_InitTypeDef gpioInit; // structure de configuration des ports

    if(GPIOx == GPIOA)
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); /// activation horloge Port
    if(GPIOx == GPIOB)
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); /// activation horloge Port
    if(GPIOx == GPIOC)
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); /// activation horloge Port
    if(GPIOx == GPIOD)
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); /// activation horloge Port
    if(GPIOx == GPIOE)
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE); /// activation horloge Port
    if(GPIOx == GPIOF)
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE); /// activation horloge Port
    if(GPIOx == GPIOG)
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE); /// activation horloge Port

    gpioInit.GPIO_Mode = GPIO_Mode;   // entrees ou sorties
    gpioInit.GPIO_Pin = GPIO_Pin;     // broches a configurer
    gpioInit.GPIO_Speed = GPIO_Speed; // frequence

    if(GPIO_Mode == GPIO_Mode_IN) {
        gpioInit.GPIO_PuPd = GPIO_Type;      // entrees avec ou sans pull-up / down
        gpioInit.GPIO_OType = GPIO_OType_OD; // sans push-pull
    } else {
        gpioInit.GPIO_PuPd = GPIO_PuPd_NOPULL; // sorties sans pull-up / down
        gpioInit.GPIO_OType = GPIO_Type;       // sorties avec ou sans push-pull
    }

    GPIO_Init(GPIOx, &gpioInit); /// configuration Port
}

void LibGPIO_Init(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, LibGPIO_Mode_t GPIO_Mode, LibGPIO_OType_t GPIO_OType, LibGPIO_PuPd_t GPIO_PuPd, LibGPIO_Speed_t GPIO_Speed)
{
    /* Check input */
    if(GPIO_Pin == 0x00)
        return;

    /* Enable clock for GPIO */
    LibGPIO_INT_EnableClock(GPIOx);

    /* Do initialization */
    LibGPIO_INT_Init(GPIOx, GPIO_Pin, GPIO_Mode, GPIO_OType, GPIO_PuPd, GPIO_Speed);
}

void LibGPIO_InitAlternate(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, LibGPIO_OType_t GPIO_OType, LibGPIO_PuPd_t GPIO_PuPd, LibGPIO_Speed_t GPIO_Speed, uint8_t Alternate)
{
    uint32_t pinpos;

    /* Check input */
    if(GPIO_Pin == 0x00)
        return;

    /* Enable GPIOx clock */
    LibGPIO_INT_EnableClock(GPIOx);

    /* Set alternate functions for all pins */
    for(pinpos = 0; pinpos < 0x10; pinpos++) {
        /* Check pin */
        if((GPIO_Pin & (1 << pinpos)) == 0)
            continue;

        /* Set alternate function */
        GPIOx->AFR[pinpos >> 0x03] = (GPIOx->AFR[pinpos >> 0x03] & ~(0x0F << (4 * (pinpos & 0x07)))) | (Alternate << (4 * (pinpos & 0x07)));
    }

    /* Do initialization */
    LibGPIO_INT_Init(GPIOx, GPIO_Pin, LibGPIO_Mode_AF, GPIO_OType, GPIO_PuPd, GPIO_Speed);
}

void LibGPIO_DeInit(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint8_t i;
    uint8_t ptr = LibGPIO_GetPortSource(GPIOx);

    /* Go through all pins */
    for(i = 0x00; i < 0x10; i++) {
        /* Pin is set */
        if(GPIO_Pin & (1 << i)) {
            /* Set 11 bits combination for analog mode */
            GPIOx->MODER |= (0x03 << (2 * i));

            /* Pin is not used */
            GPIO_UsedPins[ptr] &= ~(1 << i);
        }
    }
}

void LibGPIO_SetPinAsInput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint8_t i;
    /* Go through all pins */
    for(i = 0x00; i < 0x10; i++) {
        /* Pin is set */
        if(GPIO_Pin & (1 << i)) {
            /* Set 00 bits combination for input */
            GPIOx->MODER &= ~(0x03 << (2 * i));
        }
    }
}

void LibGPIO_SetPinAsOutput(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint8_t i;
    /* Go through all pins */
    for(i = 0x00; i < 0x10; i++) {
        /* Pin is set */
        if(GPIO_Pin & (1 << i)) {
            /* Set 01 bits combination for output */
            GPIOx->MODER = (GPIOx->MODER & ~(0x03 << (2 * i))) | (0x01 << (2 * i));
        }
    }
}

void LibGPIO_SetPinAsAnalog(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint8_t i;
    /* Go through all pins */
    for(i = 0x00; i < 0x10; i++) {
        /* Pin is set */
        if(GPIO_Pin & (1 << i)) {
            /* Set 11 bits combination for analog mode */
            GPIOx->MODER |= (0x03 << (2 * i));
        }
    }
}

void LibGPIO_SetPinAsAlternate(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint8_t i;

    /* Set alternate functions for all pins */
    for(i = 0; i < 0x10; i++) {
        /* Check pin */
        if((GPIO_Pin & (1 << i)) == 0)
            continue;

        /* Set alternate mode */
        GPIOx->MODER = (GPIOx->MODER & ~(0x03 << (2 * i))) | (0x02 << (2 * i));
    }
}

void LibGPIO_SetPullResistor(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, LibGPIO_PuPd_t GPIO_PuPd)
{
    uint8_t pinpos;

    /* Go through all pins */
    for(pinpos = 0; pinpos < 0x10; pinpos++) {
        /* Check if pin available */
        if((GPIO_Pin & (1 << pinpos)) == 0)
            continue;

        /* Set GPIO PUPD register */
        GPIOx->PUPDR = (GPIOx->PUPDR & ~(0x03 << (2 * pinpos))) | ((uint32_t)(GPIO_PuPd << (2 * pinpos)));
    }
}

void LibGPIO_Lock(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint32_t d;

    /* Set GPIO pin with 16th bit set to 1 */
    d = 0x00010000 | GPIO_Pin;

    /* Write to LCKR register */
    GPIOx->LCKR = d;
    GPIOx->LCKR = GPIO_Pin;
    GPIOx->LCKR = d;

    /* Read twice */
    (void)GPIOx->LCKR;
    (void)GPIOx->LCKR;
}

uint16_t LibGPIO_GetPinSource(uint16_t GPIO_Pin)
{
    uint16_t pinsource = 0;

    /* Get pinsource */
    while(GPIO_Pin > 1) {
        /* Increase pinsource */
        pinsource++;
        /* Shift right */
        GPIO_Pin >>= 1;
    }

    /* Return source */
    return pinsource;
}

uint16_t LibGPIO_GetPortSource(GPIO_TypeDef *GPIOx)
{
    /* Get port source number */
    /* Offset from GPIOA                       Difference between 2 GPIO addresses */
    return ((uint32_t)GPIOx - (GPIOA_BASE)) / ((GPIOB_BASE) - (GPIOA_BASE));
}

/* Private functions */
void LibGPIO_INT_EnableClock(GPIO_TypeDef *GPIOx)
{
    /* Set bit according to the 1 << portsourcenumber */
    RCC->AHB1ENR |= (1 << LibGPIO_GetPortSource(GPIOx));
}

void LibGPIO_INT_DisableClock(GPIO_TypeDef *GPIOx)
{
    /* Clear bit according to the 1 << portsourcenumber */
    RCC->AHB1ENR &= ~(1 << LibGPIO_GetPortSource(GPIOx));
}

void LibGPIO_INT_Init(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, LibGPIO_Mode_t GPIO_Mode, LibGPIO_OType_t GPIO_OType, LibGPIO_PuPd_t GPIO_PuPd, LibGPIO_Speed_t GPIO_Speed)
{
    uint8_t pinpos;
    uint8_t ptr = LibGPIO_GetPortSource(GPIOx);

    /* Go through all pins */
    for(pinpos = 0; pinpos < 0x10; pinpos++) {
        /* Check if pin available */
        if((GPIO_Pin & (1 << pinpos)) == 0)
            continue;

        /* Pin is used */
        GPIO_UsedPins[ptr] |= 1 << pinpos;

        /* Set GPIO PUPD register */
        GPIOx->PUPDR = (GPIOx->PUPDR & ~(0x03 << (2 * pinpos))) | ((uint32_t)(GPIO_PuPd << (2 * pinpos)));

        /* Set GPIO MODE register */
        GPIOx->MODER = (GPIOx->MODER & ~((uint32_t)(0x03 << (2 * pinpos)))) | ((uint32_t)(GPIO_Mode << (2 * pinpos)));

        /* Set only if output or alternate functions */
        if(GPIO_Mode == LibGPIO_Mode_OUT || GPIO_Mode == LibGPIO_Mode_AF) {
            /* Set GPIO OTYPE register */
            GPIOx->OTYPER = (GPIOx->OTYPER & ~(uint16_t)(0x01 << pinpos)) | ((uint16_t)(GPIO_OType << pinpos));

            /* Set GPIO OSPEED register */
            GPIOx->OSPEEDR = (GPIOx->OSPEEDR & ~((uint32_t)(0x03 << (2 * pinpos)))) | ((uint32_t)(GPIO_Speed << (2 * pinpos)));
        }
    }
}

uint16_t LibGPIO_GetUsedPins(GPIO_TypeDef *GPIOx)
{
    /* Return used */
    return GPIO_UsedPins[LibGPIO_GetPortSource(GPIOx)];
}

uint16_t LibGPIO_GetFreePins(GPIO_TypeDef *GPIOx)
{
    /* Return free pins */
    return ~GPIO_UsedPins[LibGPIO_GetPortSource(GPIOx)];
}
