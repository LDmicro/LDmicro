/**
 * PWM Library for STM32F4xx devices
 */

#ifndef LibPWM_H
#define LibPWM_H 210

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

/**
 * This library allows you to use PWM feature on any timer with supported PWM output
 *

TIMER   |CHANNEL 1            |CHANNEL 2            |CHANNEL 3            |CHANNEL 4
        |PP1    PP2    PP3    |PP1    PP2    PP3    |PP1    PP2    PP3    |PP1    PP2    PP3

TIM 1   |PA8    PE9    -      |PA9    PE10   -      |PA10   PE13   -      |PA11   PE14   -
TIM 2   |PA0    PA5    PA15   |PA1    PB3    -      |PA2    PB10   -      |PA3    PB11   -
TIM 3   |PA6    PB4    PC6    |PA7    PB5    PC7    |PB0    PC8    -      |PB1    PC9    -
TIM 4   |PB6    PD12   -      |PB7    PD13   -      |PB8    PD14   -      |PB9    PD15    -
TIM 5   |PA0    PH10   -      |PA1    PH11   -      |PA2    PH12   -      |PA3    PI0    -
TIM 8   |PC6    PI5    -      |PC7    PI6    -      |PC8    PI7    -      |PC9    PI2    -
TIM 9   |PA2    PE5    -      |PA3    PE6    -      |-      -      -      |-      -      -
TIM 10  |PB8    PF6    -      |-      -      -      |-      -      -      |-      -      -
TIM 11  |PB9    PF7    -      |-      -      -      |-      -      -      |-      -      -
TIM 12  |PB14   PH6    -      |PB15   PH9    -      |-      -      -      |-      -      -
TIM 13  |PA6    PF8    -      |-      -      -      |-      -      -      |-      -      -
TIM 14  |PA7    PF9    -      |-      -      -      |-      -      -      |-      -      -

 *
 * 	- PPx: Pins Pack 1 to 3, for 3 possible channel outputs on timer.
 *
 * Notes on table above:
 * 	- Not all timers are available on all STM32F4xx devices
 * 	- All timers have 16-bit prescaler
 * 	- TIM6 and TIM7 don't have PWM feature, they are only basic timers
 * 	- TIM2 and TIM5 are 32bit timers
 * 	- TIM9 and TIM12 have two PWM channels
 * 	- TIM10, TIM11, TIM13 and TIM14 have only one PWM channel
 * 	- All channels at one timer have the same PWM frequency!
 */

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"

#include "Lib_gpio.h"

/**
 * @brief  PWM Result enumeration
 */
typedef enum {
	LibPWM_Result_Ok = 0,           /*!< Everything OK */
	LibPWM_Result_FrequencyTooHigh, /*!< You select too high frequency for timer for PWM */
	LibPWM_Result_FrequencyTooLow,  /*!< Prescaler value is too big for selected frequency */
	LibPWM_Result_PulseTooHigh,     /*!< Pulse for Output compare is larger than timer period */
	LibPWM_Result_TimerNotValid,    /*!< Selected timer is not valid. This happens when you select TIM6 or TIM7,
                                            because they don't have PWM capability */
	LibPWM_Result_ChannelNotValid,  /*!< Channel is not valid. Some timers don't have all 4 timers available for PWM */
	LibPWM_Result_PinNotValid       /*!< Selected pin is not valid. Most channels have only 2 possible pins for PWM,
                                            but some 3. If you select pin 3 on channel that don't have 3rd pin available
                                            for PWM, this will be returned */
} LibPWM_Result_t;

/**
 * @brief  PWM Timer data
 */
typedef struct {
	TIM_TypeDef* TIM;       /*!< Pointer to timer used */
	uint32_t Period;        /*!< Period used, set on initialization for PWM */
	uint32_t Prescaler;     /*!< Prescaler used for PWM frequency */
	uint32_t Frequency;     /*!< PWM frequency used */
	uint32_t Micros;        /*!< Microseconds used for one period.
                                    This is not useful in large pwm frequency, but good for controlling servos or similar,
                                    Where you need exact time of pulse high */
	uint32_t CH_Periods[4]; /*!< Array of periods for PWM compare */
	uint8_t CH_Init;        /*!< Flag to check if specific channel is already initialized */
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

/**
 * @brief  Pin selected for corresponding channel on specific channel
 */
typedef enum {
	LibPWM_PinsPack_1 = 0x00, /*!< Pinspack 1 from pinout table */
	LibPWM_PinsPack_2,        /*!< Pinspack 2 from pinout table */
	LibPWM_PinsPack_3         /*!< Pinspack 3 from pinout table */
} LibPWM_PinsPack_t;


/**
 * @brief  Initializes specific timer for PWM capability
 * @param  *TIMx: Pointer to selected timer, you want to use for PWM
 * @param  *TIM_Data: Pointer to blank @ref LibPWM_TIM_t structure. Here will init function save all data for specific timer
 * @param  PWMFrequency: Select custom frequency for PWM
 * @retval Member of @ref LibPWM_Result_t
 */
LibPWM_Result_t LibPWM_InitTimer(TIM_TypeDef* TIMx, LibPWM_TIM_t* TIM_Data, double PWMFrequency);

/**
 * @brief  Initializes channel used for specific timer
 * @param  *TIM_Data: Pointer to struct with already initialized timer for PWM
 * @param  Channel: Select channel you will use on specific timer. This parameter can be a value of @ref LibPWM_Channel_t
 * @param  PinsPack: Select which pinspack you will use for pin. This parameter can be a value of @ref LibPWM_PinsPack_t
 * @retval Member of @ref LibPWM_Result_t
 */
LibPWM_Result_t LibPWM_InitChannel(LibPWM_TIM_t* TIM_Data, LibPWM_Channel_t Channel, LibPWM_PinsPack_t PinsPack);

/**
 * @brief  Sets PWM value for specific timer and channel
 * @param  *TIM_Data: Pointer to struct with already initialized timer for PWM
 * @param  Channel: Select channel you will use on specific timer. This parameter can be a value of @ref LibPWM_Channel_t
 * @param  Pulse: Pulse, to be set for compare match
 * @retval Member of @ref LibPWM_Result_t
 */
LibPWM_Result_t LibPWM_SetChannel(LibPWM_TIM_t* TIM_Data, LibPWM_Channel_t Channel, uint32_t Pulse);

/**
 * @brief  Sets PWM value for specific timer and channel with percentage feature
 * @param  *TIM_Data: Pointer to struct with already initialized timer for PWM
 * @param  Channel: Select channel you will use on specific timer. This parameter can be a value of @ref LibPWM_Channel_t
 * @param  percent: Percentage from 0 to 100, to set PWM duty cycle
 * @retval Member of @ref LibPWM_Result_t
 */
LibPWM_Result_t LibPWM_SetChannelPercent(LibPWM_TIM_t* TIM_Data, LibPWM_Channel_t Channel, float percent);

/**
 * @brief  Sets PWM value for specific timer and channel with pulse high time feature.
 * @note   You have to specify amount of micro seconds pulse will be high for specific timer and channel.
 *         This method is not so good on PWM large than 100k, because you cannot set nice and correct settings, with microseconds
 *         accuracy. It is perfect, and was designed for low frequencies, eg. use with servos, where you have exact amount of time
 *         for servo's rotation.
 * @param  *TIM_Data: Pointer to struct with already initialized timer for PWM
 * @param  Channel: Select channel you will use on specific timer. This parameter can be a value of @ref LibPWM_Channel_t
 * @param  micros: Microseconds for pulse high on PWM. Cannot be large than timer period in micros.
 *		      PWM 1kHz = Timer period = 1000000 / 1000 = 1000us. This parameter can not be greater than 1000us in this case.
 * @retval Member of @ref LibPWM_Result_t
 */
LibPWM_Result_t LibPWM_SetChannelMicros(LibPWM_TIM_t* TIM_Data, LibPWM_Channel_t Channel, uint32_t micros);

/**
 * @brief  Timer result enumeration
 */
typedef enum {
	LibTIMER_PROPERTIES_Result_Ok,               /*!< Everything OK */
	LibTIMER_PROPERTIES_Result_Error,            /*!< An error occurred */
	LibTIMER_PROPERTIES_Result_TimerNotValid,    /*!< Timer is not valid */
	LibTIMER_PROPERTIES_Result_FrequencyTooHigh, /*!< Frequency for timer is too high */
	LibTIMER_PROPERTIES_Result_FrequencyTooLow   /*!< Frequency for timer is too low */
} LibTIMER_PROPERTIES_Result_t;

/**
 * @brief  Structure for timer data
 */
typedef struct {
	uint32_t TimerFrequency; /*!< timer's working frequency */
	uint32_t MaxPeriod;      /*!< Max timer period */
	uint32_t MaxPrescaler;   /*!< Max timer prescaler */
	uint32_t Period;         /*!< Timer's working period */
	uint32_t Prescaler;      /*!< Timer's working prescaler */
	uint32_t Frequency;      /*!< Timer's reload frequency */
} LibTIMER_PROPERTIES_t;

/**
 * @}
 */

/**
 * @defgroup LibTIMER_PROPERTIES_Functions
 * @brief    Library Functions
 * @{
 */

/**
 * @brief  Gets timer properties
 * @param  *TIMx: Timer used to get settings for
 * @param  *Timer_Data: Pointer to @ref LibTIMER_PROPERTIES_t structure to store data to
 * @retval Member of @ref LibTIMER_PROPERTIES_Result_t
 */
LibTIMER_PROPERTIES_Result_t LibTIMER_PROPERTIES_GetTimerProperties(TIM_TypeDef* TIMx, LibTIMER_PROPERTIES_t* Timer_Data);

/**
 * @brief  Generates period and prescaler for given timer frequency
 * @param  *Timer_Data: Pointer to @ref LibTIMER_PROPERTIES_t structure
 * @param  frequency: Frequency used to generate period and prescaler values
 * @retval Member of @ref LibTIMER_PROPERTIES_Result_t
 */
LibTIMER_PROPERTIES_Result_t LibTIMER_PROPERTIES_GenerateDataForWorkingFrequency(LibTIMER_PROPERTIES_t* Timer_Data, double frequency);

/**
 * @brief  Enables timer clock
 * @param  *TIMx: Timer used to enable clock for it
 * @retval Member of @ref LibTIMER_PROPERTIES_Result_t
 */
LibTIMER_PROPERTIES_Result_t LibTIMER_PROPERTIES_EnableClock(TIM_TypeDef* TIMx);

/**
 * @brief  Disables timer clock
 * @param  *TIMx: Timer used to disable clock for it
 * @retval Member of @ref LibTIMER_PROPERTIES_Result_t
 */
LibTIMER_PROPERTIES_Result_t LibTIMER_PROPERTIES_DisableClock(TIM_TypeDef* TIMx);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif


