/**
 * Copyright (C) Tilen Majerle 2015, and JG 2018
 */

#include "Lib_gpio.h"
#include "Lib_pwm.h"

/* Private functions */
void            LibPWM_INT_EnableClock(TIM_TypeDef *TIMx);
LibPWM_Result_t LibPWM_INT_GetTimerProperties(TIM_TypeDef *TIMx, uint32_t *frequency, uint32_t *period);

LibPWM_Result_t LibPWM_INT_InitTIM1Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM2Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM3Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM4Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM5Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM8Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM9Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM10Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM11Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM12Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM13Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);
LibPWM_Result_t LibPWM_INT_InitTIM14Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);

LibPWM_Result_t LibPWM_InitTimer(TIM_TypeDef *TIMx, LibPWM_TIM_t *TIM_Data, double PWMFrequency)
{
    TIM_TimeBaseInitTypeDef TIM_BaseStruct;
    LibTIMER_PROPERTIES_t   Timer_Data;

    /* Check valid timer */
    if(0
#ifdef TIM6
       || TIMx == TIM6
#endif
#ifdef TIM7
       || TIMx == TIM7
#endif
    ) {
        /* Timers TIM6 and TIM7 can not provide PWM feature */
        return LibPWM_Result_TimerNotValid;
    }

    /* Save timer */
    TIM_Data->TIM = TIMx;

    /* Get timer properties */
    LibTIMER_PROPERTIES_GetTimerProperties(TIMx, &Timer_Data);

    /* Check for maximum timer frequency */
    if(PWMFrequency > Timer_Data.TimerFrequency) {
        /* Frequency too high */
        return LibPWM_Result_FrequencyTooHigh;
    } else if(PWMFrequency == 0) {
        /* Not valid frequency */
        return LibPWM_Result_FrequencyTooLow;
    }

    /* Generate settings */
    LibTIMER_PROPERTIES_GenerateDataForWorkingFrequency(&Timer_Data, PWMFrequency);

    /* Check valid data */
    if(Timer_Data.Period == 0) {
        /* Too high frequency */
        return LibPWM_Result_FrequencyTooHigh;
    }

    /* Tests are OK */
    TIM_Data->Frequency = PWMFrequency;
    TIM_Data->Micros = 1000000 / PWMFrequency;
    TIM_Data->Period = Timer_Data.Period;
    TIM_Data->Prescaler = Timer_Data.Prescaler;

    /* Enable clock for Timer */
    LibTIMER_PROPERTIES_EnableClock(TIMx);

    /* Set timer options */
    TIM_BaseStruct.TIM_Prescaler = Timer_Data.Prescaler - 1;
    TIM_BaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_BaseStruct.TIM_Period = Timer_Data.Period - 1;
    TIM_BaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_BaseStruct.TIM_RepetitionCounter = 0;

    /* Initialize timer */
    TIM_TimeBaseInit(TIMx, &TIM_BaseStruct);

    /* Preload enable */
    TIM_ARRPreloadConfig(TIMx, ENABLE);

    if(0
#ifdef TIM1
       || TIMx == TIM1
#endif
#ifdef TIM8
       || TIMx == TIM8
#endif
    ) {
        /* Enable PWM outputs */
        TIM_CtrlPWMOutputs(TIMx, ENABLE);
    }

    /* Start timer */
    TIMx->CR1 |= TIM_CR1_CEN;

    /* Set default values */
    TIM_Data->CH_Init = 0;

    /* Return OK */
    return LibPWM_Result_Ok;
}

LibPWM_Result_t LibPWM_InitChannel(LibPWM_TIM_t *TIM_Data, LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
#ifdef TIM1
    if(TIM_Data->TIM == TIM1) {
        return LibPWM_INT_InitTIM1Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM2
    if(TIM_Data->TIM == TIM2) {
        return LibPWM_INT_InitTIM2Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM3
    if(TIM_Data->TIM == TIM3) {
        return LibPWM_INT_InitTIM3Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM4
    if(TIM_Data->TIM == TIM4) {
        return LibPWM_INT_InitTIM4Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM5
    if(TIM_Data->TIM == TIM5) {
        return LibPWM_INT_InitTIM5Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM8
    if(TIM_Data->TIM == TIM8) {
        return LibPWM_INT_InitTIM8Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM9
    if(TIM_Data->TIM == TIM9) {
        return LibPWM_INT_InitTIM9Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM10
    if(TIM_Data->TIM == TIM10) {
        return LibPWM_INT_InitTIM10Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM11
    if(TIM_Data->TIM == TIM11) {
        return LibPWM_INT_InitTIM11Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM12
    if(TIM_Data->TIM == TIM12) {
        return LibPWM_INT_InitTIM12Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM13
    if(TIM_Data->TIM == TIM13) {
        return LibPWM_INT_InitTIM13Pins(Channel, PinsPack);
    }
#endif
#ifdef TIM14
    if(TIM_Data->TIM == TIM14) {
        return LibPWM_INT_InitTIM14Pins(Channel, PinsPack);
    }
#endif

    /* Timer is not valid */
    return LibPWM_Result_TimerNotValid;
}

LibPWM_Result_t LibPWM_SetChannel(LibPWM_TIM_t *TIM_Data, LibPWM_Channel_t Channel, uint32_t Pulse)
{
    TIM_OCInitTypeDef TIM_OCStruct;
    uint8_t           ch = (uint8_t)Channel;

    /* Check pulse length */
    if(Pulse >= TIM_Data->Period) {
        /* Pulse too high */
        return LibPWM_Result_PulseTooHigh;
    }

    /* Common settings */
    TIM_OCStruct.TIM_Pulse = Pulse;
    TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_Low;

    /* Save pulse value */
    TIM_Data->CH_Periods[ch] = Pulse;

    /* Go to bits */
    ch = 1 << ch;

    /* Select proper channel */
    switch(Channel) {
        case LibPWM_Channel_1:
            /* Check if initialized */
            if(!(TIM_Data->CH_Init & ch)) {
                TIM_Data->CH_Init |= ch;

                /* Init channel */
                TIM_OC1Init(TIM_Data->TIM, &TIM_OCStruct);
                TIM_OC1PreloadConfig(TIM_Data->TIM, TIM_OCPreload_Enable);
            }

            /* Set pulse */
            TIM_Data->TIM->CCR1 = Pulse;
            break;
        case LibPWM_Channel_2:
            /* Check if initialized */
            if(!(TIM_Data->CH_Init & ch)) {
                TIM_Data->CH_Init |= ch;

                /* Init channel */
                TIM_OC2Init(TIM_Data->TIM, &TIM_OCStruct);
                TIM_OC2PreloadConfig(TIM_Data->TIM, TIM_OCPreload_Enable);
            }

            /* Set pulse */
            TIM_Data->TIM->CCR2 = Pulse;
            break;
        case LibPWM_Channel_3:
            /* Check if initialized */
            if(!(TIM_Data->CH_Init & ch)) {
                TIM_Data->CH_Init |= ch;

                /* Init channel */
                TIM_OC3Init(TIM_Data->TIM, &TIM_OCStruct);
                TIM_OC3PreloadConfig(TIM_Data->TIM, TIM_OCPreload_Enable);
            }

            /* Set pulse */
            TIM_Data->TIM->CCR3 = Pulse;
            break;
        case LibPWM_Channel_4:
            /* Check if initialized */
            if(!(TIM_Data->CH_Init & ch)) {
                TIM_Data->CH_Init |= ch;

                /* Init channel */
                TIM_OC4Init(TIM_Data->TIM, &TIM_OCStruct);
                TIM_OC4PreloadConfig(TIM_Data->TIM, TIM_OCPreload_Enable);
            }

            /* Set pulse */
            TIM_Data->TIM->CCR4 = Pulse;
            break;
        default:
            break;
    }

    /* Return everything OK */
    return LibPWM_Result_Ok;
}

LibPWM_Result_t LibPWM_SetChannelPercent(LibPWM_TIM_t *TIM_Data, LibPWM_Channel_t Channel, float percent)
{
    /* Check input value */
    if(percent > 100) {
        return LibPWM_SetChannel(TIM_Data, Channel, TIM_Data->Period);
    } else if(percent <= 0) {
        return LibPWM_SetChannel(TIM_Data, Channel, 0);
    }

    /* Set channel value */
    return LibPWM_SetChannel(TIM_Data, Channel, (uint32_t)((float)(TIM_Data->Period - 1) * percent) / 100);
}

LibPWM_Result_t LibPWM_SetChannelMicros(LibPWM_TIM_t *TIM_Data, LibPWM_Channel_t Channel, uint32_t micros)
{
    /* If we choose too much micro seconds that we have valid */
    if(micros > TIM_Data->Micros) {
        /* Too high pulse */
        return LibPWM_Result_PulseTooHigh;
    }

    /* Set PWM channel */
    return LibPWM_SetChannel(TIM_Data, Channel, (uint32_t)((TIM_Data->Period - 1) * micros) / TIM_Data->Micros);
}

/* Private functions */
LibPWM_Result_t LibPWM_INT_InitTIM1Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_8, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM1);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOE
                    LibGPIO_InitAlternate(GPIOE, GPIO_PIN_9, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM1);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_2:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_9, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM1);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOE
                    LibGPIO_InitAlternate(GPIOE, GPIO_PIN_10, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM1);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_3:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_10, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM1);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOE
                    LibGPIO_InitAlternate(GPIOE, GPIO_PIN_13, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM1);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_4:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_11, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM1);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOE
                    LibGPIO_InitAlternate(GPIOE, GPIO_PIN_14, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM1);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM2Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_0, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM2);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_5, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM2);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_3:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_15, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM2);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_2:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_1, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM2);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_3, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM2);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_3:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_2, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM2);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_10, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM2);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_4:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_3, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM2);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_11, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM2);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM3Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_6, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_4, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_3:
#ifdef GPIOC
                    LibGPIO_InitAlternate(GPIOC, GPIO_PIN_10, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_2:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_7, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_5, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_3:
#ifdef GPIOC
                    LibGPIO_InitAlternate(GPIOC, GPIO_PIN_7, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_3:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_0, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOC
                    LibGPIO_InitAlternate(GPIOC, GPIO_PIN_8, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_4:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_1, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOC
                    LibGPIO_InitAlternate(GPIOC, GPIO_PIN_9, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM3);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM4Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_6, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM4);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOD
                    LibGPIO_InitAlternate(GPIOD, GPIO_PIN_12, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM4);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_2:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_7, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM4);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOD
                    LibGPIO_InitAlternate(GPIOD, GPIO_PIN_13, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM4);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_3:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_8, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM4);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOD
                    LibGPIO_InitAlternate(GPIOD, GPIO_PIN_14, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM4);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_4:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_9, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM4);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOD
                    LibGPIO_InitAlternate(GPIOD, GPIO_PIN_15, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM4);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM5Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_0, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM5);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOH
                    LibGPIO_InitAlternate(GPIOH, GPIO_PIN_10, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM5);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_2:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_1, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM5);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOH
                    LibGPIO_InitAlternate(GPIOH, GPIO_PIN_11, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM5);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_3:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_2, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM5);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOH
                    LibGPIO_InitAlternate(GPIOH, GPIO_PIN_12, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM5);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_4:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_3, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM5);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOI
                    LibGPIO_InitAlternate(GPIOI, GPIO_PIN_0, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM5);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM8Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOC
                    LibGPIO_InitAlternate(GPIOC, GPIO_PIN_6, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM8);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOI
                    LibGPIO_InitAlternate(GPIOI, GPIO_PIN_5, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM8);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_2:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOC
                    LibGPIO_InitAlternate(GPIOC, GPIO_PIN_7, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM8);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOI
                    LibGPIO_InitAlternate(GPIOI, GPIO_PIN_6, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM8);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_3:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOC
                    LibGPIO_InitAlternate(GPIOC, GPIO_PIN_8, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM8);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOI
                    LibGPIO_InitAlternate(GPIOI, GPIO_PIN_7, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM8);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_4:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOC
                    LibGPIO_InitAlternate(GPIOC, GPIO_PIN_9, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM8);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOI
                    LibGPIO_InitAlternate(GPIOI, GPIO_PIN_8, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM8);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM9Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_2, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM9);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOE
                    LibGPIO_InitAlternate(GPIOE, GPIO_PIN_5, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM9);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_2:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_3, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM9);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOE
                    LibGPIO_InitAlternate(GPIOE, GPIO_PIN_6, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM9);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM10Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_8, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM10);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOF
                    LibGPIO_InitAlternate(GPIOF, GPIO_PIN_6, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM10);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM11Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_9, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM11);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOF
                    LibGPIO_InitAlternate(GPIOF, GPIO_PIN_7, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM11);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM12Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_14, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM12);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOH
                    LibGPIO_InitAlternate(GPIOH, GPIO_PIN_6, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM12);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        case LibPWM_Channel_2:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOB
                    LibGPIO_InitAlternate(GPIOB, GPIO_PIN_15, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM12);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOH
                    LibGPIO_InitAlternate(GPIOH, GPIO_PIN_9, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM12);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM13Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_1, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM13);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOF
                    LibGPIO_InitAlternate(GPIOF, GPIO_PIN_8, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM13);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibPWM_Result_t LibPWM_INT_InitTIM14Pins(LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack)
{
    LibPWM_Result_t result = LibPWM_Result_PinNotValid;

    switch(Channel) {
        case LibPWM_Channel_1:
            switch(PinsPack) {
                case LibPWM_PinsPack_1:
#ifdef GPIOA
                    LibGPIO_InitAlternate(GPIOA, GPIO_PIN_7, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM14);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                case LibPWM_PinsPack_2:
#ifdef GPIOF
                    LibGPIO_InitAlternate(GPIOF, GPIO_PIN_9, GPIO_OType_PP, GPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_TIM14);
                    result = LibPWM_Result_Ok;
#endif
                    break;
                default:
                    result = LibPWM_Result_PinNotValid;
                    break;
            }
            break;
        default:
            result = LibPWM_Result_ChannelNotValid;
            break;
    }

    return result;
}

LibTIMER_PROPERTIES_Result_t LibTIMER_PROPERTIES_GetTimerProperties(TIM_TypeDef *TIMx, LibTIMER_PROPERTIES_t *Timer_Data)
{
    RCC_ClocksTypeDef RCC_ClocksStruct;

    /* Get clocks */
    RCC_GetClocksFreq(&RCC_ClocksStruct);

    /* All timers have 16-bit prescaler */
    Timer_Data->MaxPrescaler = 0xFFFF;

    if(0 /* 32bit timers with PCLK2 max frequency */
#ifdef TIM2
       || TIMx == TIM2
#endif
#ifdef TIM5
       || TIMx == TIM5
#endif
    ) {
        Timer_Data->TimerFrequency = RCC_ClocksStruct.PCLK2_Frequency; /* Clock */
        Timer_Data->MaxPeriod = 0xFFFFFFFF;                            /* Max period */

        /* Timer valid */
        return LibTIMER_PROPERTIES_Result_Ok;
    } else if(0 /* 16bit timers with HCLK clock frequency */
#ifdef TIM1
              || TIMx == TIM1
#endif
#ifdef TIM8
              || TIMx == TIM8
#endif
#ifdef TIM9
              || TIMx == TIM9
#endif
#ifdef TIM10
              || TIMx == TIM10
#endif
#ifdef TIM11
              || TIMx == TIM11
#endif
    ) {
        Timer_Data->TimerFrequency = RCC_ClocksStruct.HCLK_Frequency; /* Clock */
        Timer_Data->MaxPeriod = 0xFFFF;                               /* Max period */

        /* Timer valid */
        return LibTIMER_PROPERTIES_Result_Ok;
    } else if(0 /* 16bit timers with PCLK2 clock frequency */
#ifdef TIM3
              || TIMx == TIM3
#endif
#ifdef TIM4
              || TIMx == TIM4
#endif
#ifdef TIM6
              || TIMx == TIM6
#endif
#ifdef TIM7
              || TIMx == TIM7
#endif
#ifdef TIM12
              || TIMx == TIM12
#endif
#ifdef TIM13
              || TIMx == TIM13
#endif
#ifdef TIM14
              || TIMx == TIM14
#endif
    ) {
        Timer_Data->TimerFrequency = RCC_ClocksStruct.PCLK2_Frequency; /* Clock */
        Timer_Data->MaxPeriod = 0xFFFF;                                /* Max period */

        /* Timer valid */
        return LibTIMER_PROPERTIES_Result_Ok;
    }

    /* Timer is not valid */
    return LibTIMER_PROPERTIES_Result_TimerNotValid;
}

LibTIMER_PROPERTIES_Result_t LibTIMER_PROPERTIES_GenerateDataForWorkingFrequency(LibTIMER_PROPERTIES_t *Timer_Data, double frequency)
{
    if(frequency > Timer_Data->TimerFrequency) {
        /* Reset values */
        Timer_Data->Prescaler = 0;
        Timer_Data->Period = 0;
        Timer_Data->Frequency = 0;

        /* Frequency too high */
        return LibTIMER_PROPERTIES_Result_FrequencyTooHigh;
    } else if(frequency == 0) {
        /* Reset values */
        Timer_Data->Prescaler = 0;
        Timer_Data->Period = 0;
        Timer_Data->Frequency = 0;

        /* Not valid frequency */
        return LibTIMER_PROPERTIES_Result_FrequencyTooLow;
    }

    /* Fix for 16/32bit timers */
    if(Timer_Data->MaxPeriod <= 0xFFFF) {
        Timer_Data->MaxPeriod++;
    }

    /* Get minimum prescaler and maximum resolution for timer */
    Timer_Data->Prescaler = 0;
    do {
        /* Get clock */
        Timer_Data->Period = (Timer_Data->TimerFrequency / (Timer_Data->Prescaler + 1));
        /* Get period */
        Timer_Data->Period = (Timer_Data->Period / frequency);
        /* Increase prescaler value */
        Timer_Data->Prescaler++;
    } while(Timer_Data->Period > (Timer_Data->MaxPeriod) && Timer_Data->Prescaler <= (Timer_Data->MaxPrescaler + 1));
    /* Check for too low frequency */
    if(Timer_Data->Prescaler > (Timer_Data->MaxPrescaler + 1)) {
        /* Reset values */
        Timer_Data->Prescaler = 0;
        Timer_Data->Period = 0;
        Timer_Data->Frequency = 0;

        /* Prescaler too high, frequency is too low for use */
        return LibTIMER_PROPERTIES_Result_FrequencyTooLow;
    }

    /* Set frequency */
    Timer_Data->Frequency = frequency;

    /* Return OK */
    return LibTIMER_PROPERTIES_Result_Ok;
}

LibTIMER_PROPERTIES_Result_t LibTIMER_PROPERTIES_EnableClock(TIM_TypeDef *TIMx)
{
#ifdef TIM1
    if(TIMx == TIM1) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    }
#endif
#ifdef TIM2
    if(TIMx == TIM2) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    }
#endif
#ifdef TIM3
    if(TIMx == TIM3) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    }
#endif
#ifdef TIM4
    if(TIMx == TIM4) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    }
#endif
#ifdef TIM5
    if(TIMx == TIM5) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
    }
#endif
#ifdef TIM6
    if(TIMx == TIM6) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    }
#endif
#ifdef TIM7
    if(TIMx == TIM7) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    }
#endif
#ifdef TIM8
    if(TIMx == TIM8) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
    }
#endif
#ifdef TIM9
    if(TIMx == TIM9) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM9EN;
    }
#endif
#ifdef TIM10
    if(TIMx == TIM10) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;
    }
#endif
#ifdef TIM11
    if(TIMx == TIM11) {
        RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;
    }
#endif
#ifdef TIM12
    if(TIMx == TIM12) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM12EN;
    }
#endif
#ifdef TIM13
    if(TIMx == TIM13) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM13EN;
    }
#endif
#ifdef TIM14
    if(TIMx == TIM14) {
        RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
    }
#endif

    /* Return OK */
    return LibTIMER_PROPERTIES_Result_Ok;
}

LibTIMER_PROPERTIES_Result_t LibTIMER_PROPERTIES_DisableClock(TIM_TypeDef *TIMx)
{
#ifdef TIM1
    if(TIMx == TIM1) {
        RCC->APB2ENR &= ~RCC_APB2ENR_TIM1EN;
    }
#endif
#ifdef TIM2
    if(TIMx == TIM2) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
    }
#endif
#ifdef TIM3
    if(TIMx == TIM3) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM3EN;
    }
#endif
#ifdef TIM4
    if(TIMx == TIM4) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM4EN;
    }
#endif
#ifdef TIM5
    if(TIMx == TIM5) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM5EN;
    }
#endif
#ifdef TIM6
    if(TIMx == TIM6) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM6EN;
    }
#endif
#ifdef TIM7
    if(TIMx == TIM7) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM7EN;
    }
#endif
#ifdef TIM8
    if(TIMx == TIM8) {
        RCC->APB2ENR &= ~RCC_APB2ENR_TIM8EN;
    }
#endif
#ifdef TIM9
    if(TIMx == TIM9) {
        RCC->APB2ENR &= ~RCC_APB2ENR_TIM9EN;
    }
#endif
#ifdef TIM10
    if(TIMx == TIM10) {
        RCC->APB2ENR &= ~RCC_APB2ENR_TIM10EN;
    }
#endif
#ifdef TIM11
    if(TIMx == TIM11) {
        RCC->APB2ENR &= ~RCC_APB2ENR_TIM11EN;
    }
#endif
#ifdef TIM12
    if(TIMx == TIM12) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM12EN;
    }
#endif
#ifdef TIM13
    if(TIMx == TIM13) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM13EN;
    }
#endif
#ifdef TIM14
    if(TIMx == TIM14) {
        RCC->APB1ENR &= ~RCC_APB1ENR_TIM14EN;
    }
#endif

    /* Return OK */
    return LibTIMER_PROPERTIES_Result_Ok;
}
