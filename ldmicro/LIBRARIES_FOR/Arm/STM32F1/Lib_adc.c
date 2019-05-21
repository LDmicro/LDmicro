/**
 * Copyright (C) JG 2019
 */

#include "stm32f10x.h"

#include "lib_gpio.h"
#include "lib_adc.h"

/**
  * @brief  Configure ADC source, channel and resolution
  * @param  ADCx = ADC1 or ADC2
  * @param  channel = Adc_Channel_0 to Adc_Channel_7
  * @param  resol = ADC_Resolution_8b, ADC_Resolution_10b or ADC_Resolution_12b
  * @retval None
  */
void LibADC_Init(ADC_TypeDef *ADCx, uint8_t channel)
{
    ADC_InitTypeDef ADC_InitStruct;

    if(channel == ADC_Channel_0)
        LibGPIO_Conf(GPIOA, GPIO_PIN_0, GPIO_Mode_AIN, LibGPIO_Speed_Medium);
    if(channel == ADC_Channel_1)
        LibGPIO_Conf(GPIOA, GPIO_Pin_1, GPIO_Mode_AIN, LibGPIO_Speed_Medium);
    if(channel == ADC_Channel_2)
        LibGPIO_Conf(GPIOA, GPIO_Pin_2, GPIO_Mode_AIN, LibGPIO_Speed_Medium);
    if(channel == ADC_Channel_3)
        LibGPIO_Conf(GPIOA, GPIO_Pin_3, GPIO_Mode_AIN, LibGPIO_Speed_Medium);
    if(channel == ADC_Channel_4)
        LibGPIO_Conf(GPIOA, GPIO_Pin_4, GPIO_Mode_AIN, LibGPIO_Speed_Medium);
    if(channel == ADC_Channel_5)
        LibGPIO_Conf(GPIOA, GPIO_Pin_5, GPIO_Mode_AIN, LibGPIO_Speed_Medium);
    if(channel == ADC_Channel_6)
        LibGPIO_Conf(GPIOA, GPIO_Pin_6, GPIO_Mode_AIN, LibGPIO_Speed_Medium);
    if(channel == ADC_Channel_7)
        LibGPIO_Conf(GPIOA, GPIO_Pin_7, GPIO_Mode_AIN, LibGPIO_Speed_Medium);

#if defined(STM32F10X_LD_VL) || defined(STM32F10X_MD_VL) || defined(STM32F10X_HD_VL)
    // ADCCLK = PCLK2/2
    RCC_ADCCLKConfig(RCC_PCLK2_Div2);
#else
    // ADCCLK = PCLK2/4
    RCC_ADCCLKConfig(RCC_PCLK2_Div4);
#endif

    // Enable ADCx clock
    if(ADCx == ADC1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    if(ADCx == ADC2)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);

    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStruct.ADC_ScanConvMode = ENABLE;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_NbrOfChannel = 1;

    // Initialize ADC
    ADC_Init(ADCx, &ADC_InitStruct);

    // Enable ADC
    ADC_Cmd(ADCx, ENABLE);

    // Enable ADCx reset calibration register
    ADC_ResetCalibration(ADCx);
    // Check the end of ADCx reset calibration register
    while(ADC_GetResetCalibrationStatus(ADCx))
        ;

    // Start ADCx calibration
    ADC_StartCalibration(ADCx);
    // Check the end of ADCx calibration
    while(ADC_GetCalibrationStatus(ADCx))
        ;
}

/**
  * @brief  Read from ADC source and channel
  * @param  ADCx = ADC1 or ADC2
  * @param  channel = Adc_Channel_0 to Adc_Channel_7
  * @retval Read value
  */
uint16_t LibADC_Read(ADC_TypeDef *ADCx, uint8_t channel)
{
    ADC_RegularChannelConfig(ADCx, channel, 1, ADC_SampleTime_13Cycles5);

    ADC_SoftwareStartConvCmd(ADCx, ENABLE);
    while(!ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC))
        ; // Wait for end of conversion
    ADC_ClearFlag(ADCx, ADC_FLAG_EOC);

    return ADC_GetConversionValue(ADCx);
}

/**
  * @brief  Stop ADC (all channels)
  * @param  ADCx = ADC1 or ADC2
  * @retval None
  */
void LibADC_Stop(ADC_TypeDef *ADCx)
{
    ADC_Cmd(ADCx, DISABLE);

    // Disable ADCx clock
    if(ADCx == ADC1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
    if(ADCx == ADC2)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, DISABLE);
}
