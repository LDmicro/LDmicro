/**
 * PWM Library for STM32F10x devices
 */

#ifndef LibPWM_H
#define LibPWM_H 210

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

/**
 *  PWMx    Channel 1   Channel 2   Channel 3   Channel 4
 *
 *  PWM1        PA8         PA9         PA10        PA11
 *  PWM2        PA0         PA1         PA2         PA3
 *  PWM3        PA6         PA7         PB0         PB1
 *  PWM4        PB6         PB7         PB8         PB9
 */

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_tim.h"

#include "Lib_gpio.h"


/**
 * @brief  PWM Timer data
 */
typedef struct {
	TIM_TypeDef* TIM;       /*!< Pointer to timer used */
    uint8_t Is_Init;        /*!< Flag to check if specific channel is already initialized */
    uint32_t Frequency;     /*!< PWM frequency used */
    uint32_t Prescaler;     /*!< Prescaler used for PWM frequency */
	uint32_t Period;        /*!< Period used for PWM frequency  */
} LibPWM_TIM_t;

/**
 * @brief  Channel selection for PWM on specific timer
 */
typedef enum {
	LibPWM_Channel_1 = 0x00, /*!<  Operate with channel 1 */
	LibPWM_Channel_2 = 0x01, /*!<  Operate with channel 2 */
	LibPWM_Channel_3 = 0x02, /*!<  Operate with channel 3 */
	LibPWM_Channel_4 = 0x03  /*!<  Operate with channel 4 */
} LibPWM_Channel_t;


void LibPWM_InitTimer(TIM_TypeDef* TIMx, LibPWM_TIM_t* TIM_Data, double PWMFrequency);

void LibPWM_InitChannel(LibPWM_TIM_t* TIM_Data, LibPWM_Channel_t Channel);

void LibPWM_SetChannel(LibPWM_TIM_t* TIM_Data, LibPWM_Channel_t Channel, uint32_t Pulse);

void LibPWM_SetChannelPercent(LibPWM_TIM_t* TIM_Data, LibPWM_Channel_t Channel, float percent);


/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif


