/**
 * Copyright (C) Tilen Majerle 2014, and JG 2016
 */

#include "stm32f4xx.h" // JG

#include "lib_gpio.h" // JG

#include "lib_adc.h"

/// initialisation du CAN1 ou CAN2
/// canal 4 => broche PA4 (#7)
/// canal 5 => broche PA5 (#8)
/// canal 6 => broche PA6 (#9)

/// initialisation du CAN3
/// canal 6 => broche PF8 (#11)
/// canal 7 => broche PF9 (#12)
/// canal 8 => broche PF10 (#13)

/**
  * @brief  Configure ADC source, channel and resolution
  * @param  ADCx = ADC1, ADC2 or ADC3
  * @param  channel = ADC_Channel_0 to ADC_Channel_15
  * @param  resol = ADC_Resolution_8b, ADC_Resolution_10b or ADC_Resolution_12b
  * @retval None
  */
void LibADC_Init(ADC_TypeDef *ADCx, uint8_t channel, uint32_t resol)
{
    CAN_Channel_t ch = (CAN_Channel_t)channel;
    if(ch == CAN_Channel_0)
        CAN_INT_Channel_0_Init(ADCx);
    if(ch == CAN_Channel_1)
        CAN_INT_Channel_1_Init(ADCx);
    if(ch == CAN_Channel_2)
        CAN_INT_Channel_2_Init(ADCx);
    if(ch == CAN_Channel_3)
        CAN_INT_Channel_3_Init(ADCx);
    if(ch == CAN_Channel_4)
        CAN_INT_Channel_4_Init(ADCx);
    if(ch == CAN_Channel_5)
        CAN_INT_Channel_5_Init(ADCx);
    if(ch == CAN_Channel_6)
        CAN_INT_Channel_6_Init(ADCx);
    if(ch == CAN_Channel_7)
        CAN_INT_Channel_7_Init(ADCx);
    if(ch == CAN_Channel_8)
        CAN_INT_Channel_8_Init(ADCx);
    if(ch == CAN_Channel_9)
        CAN_INT_Channel_9_Init(ADCx);
    if(ch == CAN_Channel_10)
        CAN_INT_Channel_10_Init(ADCx);
    if(ch == CAN_Channel_11)
        CAN_INT_Channel_11_Init(ADCx);
    if(ch == CAN_Channel_12)
        CAN_INT_Channel_12_Init(ADCx);
    if(ch == CAN_Channel_13)
        CAN_INT_Channel_13_Init(ADCx);
    if(ch == CAN_Channel_14)
        CAN_INT_Channel_14_Init(ADCx);
    if(ch == CAN_Channel_15)
        CAN_INT_Channel_15_Init(ADCx);
    /* Init ADC */
    LibADC_InitADC(ADCx, resol);
}

void LibADC_InitADC(ADC_TypeDef *ADCx, uint32_t resol) // modified by JG
{
    ADC_InitTypeDef       ADC_InitStruct;
    ADC_CommonInitTypeDef ADC_CommonInitStruct;

    /* Init ADC settings */
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_ExternalTrigConv = DISABLE;
    ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStruct.ADC_NbrOfConversion = 1;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;

    /* Enable clock and fill resolution settings */
    if(ADCx == ADC1) {
        /* Enable ADC clock */
        RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

        /* Set resolution */
        ADC_InitStruct.ADC_Resolution = resol; // modified by JG
    }
    if(ADCx == ADC2) {
        /* Enable ADC clock */
        RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;

        /* Set resolution */
        ADC_InitStruct.ADC_Resolution = resol; // modified by JG
    }
    if(ADCx == ADC3) {
        /* Enable ADC clock */
        RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;

        /* Set resolution */
        ADC_InitStruct.ADC_Resolution = resol; // modified by JG
    }

    /* Set common ADC settings */
    ADC_CommonInitStruct.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStruct.ADC_Prescaler = ADC_Prescaler_Div4;
    ADC_CommonInitStruct.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_8Cycles;
    ADC_CommonInit(&ADC_CommonInitStruct);

    /* Initialize ADC */
    ADC_Init(ADCx, &ADC_InitStruct);

    /* Enable ADC */
    ADCx->CR2 |= ADC_CR2_ADON;
}

/**
  * @brief  Read from ADC source and channel
  * @param  ADCx = ADC1, ADC2 or ADC3
  * @param  channel = ADC_Channel_0 to ADC_Channel_15
  * @retval Read value
  */
uint16_t LibADC_Read(ADC_TypeDef *ADCx, uint8_t channel)
{
    uint32_t timeout = 0xFFF;

    ADC_RegularChannelConfig(ADCx, channel, 1, ADC_SampleTime_15Cycles);

    /* Start software conversion */
    ADCx->CR2 |= (uint32_t)ADC_CR2_SWSTART;

    /* Wait till done */
    while(!(ADCx->SR & ADC_SR_EOC)) {
        if(timeout-- == 0x00) {
            return 0;
        }
    }

    /* Return result */
    return ADCx->DR;
}

void LibADC_EnableVbat(void)
{
    /* Enable VBAT */
    ADC->CCR |= ADC_CCR_VBATE;
}

void LibADC_DisableVbat(void)
{
    /* Disable VBAT */
    ADC->CCR &= ~ADC_CCR_VBATE;
}

uint16_t LibADC_ReadVbat(ADC_TypeDef *ADCx)
{
    uint32_t result;
    /* Read battery voltage data */
    /* Start conversion */
    ADC_RegularChannelConfig(ADCx, ADC_Channel_Vbat, 1, ADC_SampleTime_112Cycles);

    /* Start software conversion */
    ADCx->CR2 |= (uint32_t)ADC_CR2_SWSTART;

    /* Wait till done */
    while(!(ADCx->SR & ADC_SR_EOC))
        ;

    /* Get result */
    result = ADCx->DR;

    /* Convert to voltage */
    result = result * ADC_VBAT_MULTI * ADC_SUPPLY_VOLTAGE / 0xFFF;

    /* Return value in mV */
    return (uint16_t)result;
}

/* Private functions */
void CAN_INT_Channel_0_Init(ADC_TypeDef *ADCx)
{
    CAN_INT_InitPin(GPIOA, GPIO_PIN_0);
}

void CAN_INT_Channel_1_Init(ADC_TypeDef *ADCx)
{
    CAN_INT_InitPin(GPIOA, GPIO_PIN_1);
}

void CAN_INT_Channel_2_Init(ADC_TypeDef *ADCx)
{
    CAN_INT_InitPin(GPIOA, GPIO_PIN_2);
}

void CAN_INT_Channel_3_Init(ADC_TypeDef *ADCx)
{
    CAN_INT_InitPin(GPIOA, GPIO_PIN_3);
}

void CAN_INT_Channel_4_Init(ADC_TypeDef *ADCx)
{
    if(ADCx == ADC1 || ADCx == ADC2) {
        CAN_INT_InitPin(GPIOA, GPIO_PIN_4);
    }
    if(ADCx == ADC3) {
        CAN_INT_InitPin(GPIOF, GPIO_PIN_6);
    }
}

void CAN_INT_Channel_5_Init(ADC_TypeDef *ADCx)
{
    if(ADCx == ADC1 || ADCx == ADC2) {
        CAN_INT_InitPin(GPIOA, GPIO_PIN_5);
    }
    if(ADCx == ADC3) {
        CAN_INT_InitPin(GPIOF, GPIO_PIN_7);
    }
}

void CAN_INT_Channel_6_Init(ADC_TypeDef *ADCx)
{
    if(ADCx == ADC1 || ADCx == ADC2) {
        CAN_INT_InitPin(GPIOA, GPIO_PIN_6);
    }
    if(ADCx == ADC3) {
        CAN_INT_InitPin(GPIOF, GPIO_PIN_8);
    }
}

void CAN_INT_Channel_7_Init(ADC_TypeDef *ADCx)
{
    if(ADCx == ADC1 || ADCx == ADC2) {
        CAN_INT_InitPin(GPIOA, GPIO_PIN_7);
    }
    if(ADCx == ADC3) {
        CAN_INT_InitPin(GPIOF, GPIO_PIN_9);
    }
}

void CAN_INT_Channel_8_Init(ADC_TypeDef *ADCx)
{
    if(ADCx == ADC1 || ADCx == ADC2) {
        CAN_INT_InitPin(GPIOB, GPIO_PIN_0);
    }
    if(ADCx == ADC3) {
        CAN_INT_InitPin(GPIOF, GPIO_PIN_10);
    }
}

void CAN_INT_Channel_9_Init(ADC_TypeDef *ADCx)
{
    if(ADCx == ADC1 || ADCx == ADC2) {
        CAN_INT_InitPin(GPIOB, GPIO_PIN_1);
    }
    if(ADCx == ADC3) {
        CAN_INT_InitPin(GPIOF, GPIO_PIN_3);
    }
}

void CAN_INT_Channel_10_Init(ADC_TypeDef *ADCx)
{
    CAN_INT_InitPin(GPIOC, GPIO_PIN_0);
}

void CAN_INT_Channel_11_Init(ADC_TypeDef *ADCx)
{
    CAN_INT_InitPin(GPIOC, GPIO_PIN_1);
}

void CAN_INT_Channel_12_Init(ADC_TypeDef *ADCx)
{
    CAN_INT_InitPin(GPIOC, GPIO_PIN_2);
}

void CAN_INT_Channel_13_Init(ADC_TypeDef *ADCx)
{
    CAN_INT_InitPin(GPIOC, GPIO_PIN_3);
}

void CAN_INT_Channel_14_Init(ADC_TypeDef *ADCx)
{
    if(ADCx == ADC1 || ADCx == ADC2) {
        CAN_INT_InitPin(GPIOC, GPIO_PIN_4);
    }
    if(ADCx == ADC3) {
        CAN_INT_InitPin(GPIOF, GPIO_PIN_4);
    }
}

void CAN_INT_Channel_15_Init(ADC_TypeDef *ADCx)
{
    if(ADCx == ADC1 || ADCx == ADC2) {
        CAN_INT_InitPin(GPIOC, GPIO_PIN_5);
    }
    if(ADCx == ADC3) {
        CAN_INT_InitPin(GPIOF, GPIO_PIN_5);
    }
}

void CAN_INT_InitPin(GPIO_TypeDef *GPIOx, uint16_t PinX)
{
    /* Enable GPIO */
    LibGPIO_Init(GPIOx, PinX, LibGPIO_Mode_AN, LibGPIO_OType_PP, LibGPIO_PuPd_DOWN, LibGPIO_Speed_Medium);
}
