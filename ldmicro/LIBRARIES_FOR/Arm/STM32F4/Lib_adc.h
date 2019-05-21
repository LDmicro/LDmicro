/**
 * ADC Library for STM32F4xx devices
 */

#ifndef CAN_H
#define CAN_H 120

/**
    CHANNEL   ADC1   	ADC2   	  ADC3

    0         PA0    	PA0    	  PA0
    1         PA1    	PA1    	  PA1
    2         PA2    	PA2    	  PA2
    3         PA3    	PA3    	  PA3
    4         PA4    	PA4       PF6
    5         PA5    	PA5       PF7
    6         PA6    	PA6       PF8
    7         PA7    	PA7       PF9
    8         PB0    	PB0       PF10
    9         PB1    	PB1    	  PF3
    10        PC0    	PC0    	  PC0
    11        PC1    	PC1    	  PC1
    12        PC2    	PC2    	  PC2
    13        PC3    	PC3    	  PC3
    14        PC4    	PC4    	  PF4
    15        PC5    	PC5    	  PF5
 */

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_adc.h"

/**
 * Default resolution for ADC1
 */
#ifndef TM_ADC1_RESOLUTION
#define TM_ADC1_RESOLUTION		ADC_Resolution_12b
#endif

/**
 * Default resolution for ADC2
 */
#ifndef TM_ADC2_RESOLUTION
#define TM_ADC2_RESOLUTION		ADC_Resolution_12b
#endif

/**
 * Default resolution for ADC3
 */
#ifndef TM_ADC3_RESOLUTION
#define TM_ADC3_RESOLUTION		ADC_Resolution_12b
#endif

/**
 * Default supply voltage in mV
 */
#ifndef ADC_SUPPLY_VOLTAGE
#define ADC_SUPPLY_VOLTAGE		3300
#endif

/**
 * Multipliers for VBAT measurement
 */

#if defined (STM32F40_41xxx)
#define ADC_VBAT_MULTI			2
#endif
#if defined (STM32F427_437xx) || defined (STM32F429_439xx) || defined (STM32F401xx) || defined (STM32F411xE)
#define ADC_VBAT_MULTI			4
#endif


/**
 * @brief  ADC available channels
 */
typedef enum
    {
	CAN_Channel_0,  /*!< Operate with ADC channel 0 */
	CAN_Channel_1,  /*!< Operate with ADC channel 1 */
	CAN_Channel_2,  /*!< Operate with ADC channel 2 */
	CAN_Channel_3,  /*!< Operate with ADC channel 3 */
	CAN_Channel_4,  /*!< Operate with ADC channel 4 */
	CAN_Channel_5,  /*!< Operate with ADC channel 5 */
	CAN_Channel_6,  /*!< Operate with ADC channel 6 */
	CAN_Channel_7,  /*!< Operate with ADC channel 7 */
	CAN_Channel_8,  /*!< Operate with ADC channel 8 */
	CAN_Channel_9,  /*!< Operate with ADC channel 9 */
	CAN_Channel_10, /*!< Operate with ADC channel 10 */
	CAN_Channel_11, /*!< Operate with ADC channel 11 */
	CAN_Channel_12, /*!< Operate with ADC channel 12 */
	CAN_Channel_13, /*!< Operate with ADC channel 13 */
	CAN_Channel_14, /*!< Operate with ADC channel 14 */
	CAN_Channel_15, /*!< Operate with ADC channel 15 */
	CAN_Channel_16, /*!< Operate with ADC channel 16 */
	CAN_Channel_17, /*!< Operate with ADC channel 17 */
	CAN_Channel_18  /*!< Operate with ADC channel 18 */
    } CAN_Channel_t;


void LibADC_InitADC(ADC_TypeDef* ADCx, uint32_t resol);		// modified by JG

void LibADC_Init(ADC_TypeDef* ADCx, uint8_t channel, uint32_t resol);		// modified by JG

uint16_t LibADC_Read(ADC_TypeDef* ADCx, uint8_t channel);

void LibADC_EnableVbat(void);

void LibADC_DisableVbat(void);

uint16_t LibADC_ReadVbat(ADC_TypeDef* ADCx);


/**
 * Private functions
 */
void CAN_INT_Channel_0_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_1_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_2_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_3_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_4_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_5_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_6_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_7_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_8_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_9_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_10_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_11_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_12_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_13_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_14_Init(ADC_TypeDef* ADCx);
void CAN_INT_Channel_15_Init(ADC_TypeDef* ADCx);
void CAN_INT_InitPin(GPIO_TypeDef* GPIOx, uint16_t PinX);

#endif
