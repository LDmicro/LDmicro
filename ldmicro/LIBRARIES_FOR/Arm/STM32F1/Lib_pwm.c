/**
 * Copyright (C) JG 2019
 */

#include "Lib_gpio.h"
#include "Lib_pwm.h"

/**
 * @brief  Initializes specific timer for PWM capability
 * @param  *TIMx: Pointer to selected timer, you want to use for PWM
 * @param  *TIM_Data: Pointer to blank @ref LibPWM_TIM_t structure. Here will init function save all data for specific timer
 * @param  PWMFrequency: Select custom frequency for PWM
 * @retval None
 */
void LibPWM_InitTimer(TIM_TypeDef *TIMx, LibPWM_TIM_t *TIM_Data, double PWMFrequency)
{
    TIM_TimeBaseInitTypeDef timerInitStruct;

    /// APB2 for TIM1 and APB1 for others !!!
    if(TIMx == TIM1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    if(TIMx == TIM2)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    if(TIMx == TIM3)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    if(TIMx == TIM4)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    // Remember Timer configuration
    TIM_Data->TIM = TIMx;
    TIM_Data->Is_Init = 1;
    TIM_Data->Frequency = PWMFrequency;
    TIM_Data->Prescaler = 0;
    TIM_Data->Period = 0xFFFF; /// Max period (16 bits)

    // Get minimum prescaler and maximum resolution
    do {
        TIM_Data->Period = (SystemCoreClock / PWMFrequency) / (TIM_Data->Prescaler + 1) - 1;
        TIM_Data->Prescaler++;
    } while((TIM_Data->Period > 0xFFFF) && (TIM_Data->Prescaler <= 0x10000));

    TIM_Data->Prescaler--;
    if(TIM_Data->Prescaler > 0xFFFF) // PWMFrequency is too low
    {
        TIM_Data->Prescaler = 0xFFFF;
        TIM_Data->Period = 100;
    }

    TIM_TimeBaseStructInit(&timerInitStruct);

    timerInitStruct.TIM_Prescaler = TIM_Data->Prescaler;
    timerInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    timerInitStruct.TIM_Period = TIM_Data->Period;
    timerInitStruct.TIM_ClockDivision = 0;
    timerInitStruct.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIMx, &timerInitStruct);

    // Activate Timer + Pwm
    TIM_ARRPreloadConfig(TIMx, ENABLE);
    TIM_CtrlPWMOutputs(TIMx, ENABLE);
    TIM_Cmd(TIMx, ENABLE);
}

/**
 * @brief  Initializes channel used for specific timer
 * @param  *TIM_Data: Pointer to struct with already initialized timer for PWM
 * @param  Channel: Select channel you will use on specific timer. This parameter can be a value of @ref LibPWM_Channel_t
 * @retval None
 */
void LibPWM_InitChannel(LibPWM_TIM_t *TIM_Data, LibPWM_Channel_t Channel)
{
    TIM_OCInitTypeDef TIM_OCStruct;

    // Check if Timer initialized and channel is valid
    if(TIM_Data->Is_Init != 1)
        return;
    if(Channel > LibPWM_Channel_4)
        return;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    if(TIM_Data->TIM == TIM1) // PA8 to PA11
        LibGPIO_Conf(GPIOA, GPIO_PIN_8 << Channel, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
    if(TIM_Data->TIM == TIM2) // PA0 to PA3
        LibGPIO_Conf(GPIOA, GPIO_PIN_0 << Channel, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
    if(TIM_Data->TIM == TIM3) // PA6 to PA7 + PB0 to PB1
    {
        if(Channel <= LibPWM_Channel_2)
            LibGPIO_Conf(GPIOA, GPIO_PIN_6 << Channel, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
        else if(Channel <= LibPWM_Channel_4)
            LibGPIO_Conf(GPIOB, GPIO_PIN_0 << (Channel - 2), GPIO_Mode_AF_PP, GPIO_Speed_50MHz);
    }
    if(TIM_Data->TIM == TIM4) // PB6 to PB9
        LibGPIO_Conf(GPIOB, GPIO_PIN_6 << Channel, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);

    TIM_OCStructInit(&TIM_OCStruct);

    // PWM mode 1 = Set on compare match
    // PWM mode 2 = Clear on compare match
    TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCStruct.TIM_Pulse = 0;
    TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_High;

    switch(Channel) {
        case 0:
            TIM_OC1Init(TIM_Data->TIM, &TIM_OCStruct);
            TIM_OC1PreloadConfig(TIM_Data->TIM, TIM_OCPreload_Enable);
            break;
        case 1:
            TIM_OC2Init(TIM_Data->TIM, &TIM_OCStruct);
            TIM_OC2PreloadConfig(TIM_Data->TIM, TIM_OCPreload_Enable);
            break;
        case 2:
            TIM_OC3Init(TIM_Data->TIM, &TIM_OCStruct);
            TIM_OC3PreloadConfig(TIM_Data->TIM, TIM_OCPreload_Enable);
            break;
        case 3:
            TIM_OC4Init(TIM_Data->TIM, &TIM_OCStruct);
            TIM_OC4PreloadConfig(TIM_Data->TIM, TIM_OCPreload_Enable);
            break;
    }
}

/**
 * @brief  Sets PWM value for specific timer and channel
 * @param  *TIM_Data: Pointer to struct with already initialized timer for PWM
 * @param  Channel: Select channel you will use on specific timer. This parameter can be a value of @ref LibPWM_Channel_t
 * @param  Pulse: Pulse, to be set for compare match
 * @retval None
 */
void LibPWM_SetChannel(LibPWM_TIM_t *TIM_Data, LibPWM_Channel_t Channel, uint32_t Pulse)
{
    // Check if Timer initialized and channel is valid
    if(TIM_Data->Is_Init != 1)
        return;
    if(Channel > LibPWM_Channel_4)
        return;

    // Check pulse length
    if(Pulse > TIM_Data->Period)
        Pulse = TIM_Data->Period;

    // Select proper channel
    switch(Channel) {
        case LibPWM_Channel_1:
            // Set pulse
            TIM_Data->TIM->CCR1 = Pulse;
            break;
        case LibPWM_Channel_2:
            // Set pulse
            TIM_Data->TIM->CCR2 = Pulse;
            break;
        case LibPWM_Channel_3:
            // Set pulse
            TIM_Data->TIM->CCR3 = Pulse;
            break;
        case LibPWM_Channel_4:
            // Set pulse
            TIM_Data->TIM->CCR4 = Pulse;
            break;
        default:
            break;
    }
}

/**
 * @brief  Sets PWM value for specific timer and channel with percentage feature
 * @param  *TIM_Data: Pointer to struct with already initialized timer for PWM
 * @param  Channel: Select channel you will use on specific timer. This parameter can be a value of @ref LibPWM_Channel_t
 * @param  percent: Percentage from 0 to 100, to set PWM duty cycle
 * @retval None
 */
void LibPWM_SetChannelPercent(LibPWM_TIM_t *TIM_Data, LibPWM_Channel_t Channel, float percent)
{
    // Check if Timer initialized and channel is valid
    if(TIM_Data->Is_Init != 1)
        return;
    if(Channel > LibPWM_Channel_4)
        return;

    // Check input value
    if(percent > 100)
        percent = 100;

    // Set channel value
    LibPWM_SetChannel(TIM_Data, Channel, (uint32_t)((float)(TIM_Data->Period) * percent) / 100);
}
