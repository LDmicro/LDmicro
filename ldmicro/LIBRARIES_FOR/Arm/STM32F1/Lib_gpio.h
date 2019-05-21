/**
 * GPIO Library for STM32F10x devices
 */

#ifndef LibGPIO_H
#define LibGPIO_H 150

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

/**
 * GPIO library can be used for GPIO pins.
 * It features fast initialization methods as well pin input/output methods.
 * It can be used as replacement for STD/HAL drivers GPIO library.
 */

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"

/**
 * @brief GPIO Pins declarations
 */

#ifndef GPIO_PIN_0
#define GPIO_PIN_0		((uint16_t)0x0001)
#define GPIO_PIN_1		((uint16_t)0x0002)
#define GPIO_PIN_2		((uint16_t)0x0004)
#define GPIO_PIN_3		((uint16_t)0x0008)
#define GPIO_PIN_4		((uint16_t)0x0010)
#define GPIO_PIN_5		((uint16_t)0x0020)
#define GPIO_PIN_6		((uint16_t)0x0040)
#define GPIO_PIN_7		((uint16_t)0x0080)
#define GPIO_PIN_8		((uint16_t)0x0100)
#define GPIO_PIN_9		((uint16_t)0x0200)
#define GPIO_PIN_10		((uint16_t)0x0400)
#define GPIO_PIN_11		((uint16_t)0x0800)
#define GPIO_PIN_12		((uint16_t)0x1000)
#define GPIO_PIN_13		((uint16_t)0x2000)
#define GPIO_PIN_14		((uint16_t)0x4000)
#define GPIO_PIN_15		((uint16_t)0x8000)
#define GPIO_PIN_ALL	((uint16_t)0xFFFF)
#endif

#define GPIO_Pin_Low    ((uint16_t)0x00FF)
#define GPIO_Pin_Hig    ((uint16_t)0xFF00)

/**
 * @brief GPIO Mode enumeration
 */
typedef enum {
    LibGPIO_Mode_AIN = GPIO_Mode_AIN,
    LibGPIO_Mode_IN_FLOATING = GPIO_Mode_IN_FLOATING,
    LibGPIO_Mode_IPD = GPIO_Mode_IPD,
    LibGPIO_Mode_IPU = GPIO_Mode_IPU,
    LibGPIO_Mode_Out_OD = GPIO_Mode_Out_OD,
    LibGPIO_Mode_Out_PP = GPIO_Mode_Out_PP,
    LibGPIO_Mode_AF_OD = GPIO_Mode_AF_OD,
    vGPIO_Mode_AF_PP = GPIO_Mode_AF_PP,
  } LibGPIO_Mode_t;

/**
 * @brief  GPIO Speed enumeration
 */
typedef enum {
	LibGPIO_Speed_Low = GPIO_Speed_2MHz,        /*!< GPIO Speed Low */
	LibGPIO_Speed_Medium = GPIO_Speed_10MHz,    /*!< GPIO Speed Medium */
	LibGPIO_Speed_Fast = GPIO_Speed_50MHz,      /*!< GPIO Speed Fast */
} LibGPIO_Speed_t;


/**
 * @defgroup LibGPIO_Functions
 * @brief    GPIO Functions
 * @{
 */

/**
 * @brief  Configures GPIO port
 * @note   This function also enables clock for GPIO port
 * @param  GPIOx: Pointer to GPIOx port you will use for initialization
 * @param  GPIO_Pin: GPIO pin(s) you will use for initialization
 * @param  GPIO_Mode: Select GPIO mode and type
 * @param  GPIO_Speed: Select GPIO speed. This parameter can be a value of @ref LibGPIO_Speed_t enumeration
 * @retval None
 */
void LibGPIO_Conf(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, LibGPIO_Mode_t GPIO_Mode, LibGPIO_Speed_t GPIO_Speed);

/**
 * @brief  Gets port source from desired GPIOx PORT
 * @note   Meant for private use, unless you know what are you doing
 * @param  GPIOx: GPIO PORT for calculating port source
 * @retval Calculated port source for GPIO
 */
uint16_t LibGPIO_GetPortSource(GPIO_TypeDef* GPIOx);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
