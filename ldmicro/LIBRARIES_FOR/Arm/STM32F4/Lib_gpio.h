/**
 * GPIO Library for STM32F4xx devices
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

#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"

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
	LibGPIO_Mode_IN = 0x00,  /*!< GPIO Pin as General Purpose Input */
	LibGPIO_Mode_OUT = 0x01, /*!< GPIO Pin as General Purpose Output */
	LibGPIO_Mode_AF = 0x02,  /*!< GPIO Pin as Alternate Function */
	LibGPIO_Mode_AN = 0x03,  /*!< GPIO Pin as Analog */
} LibGPIO_Mode_t;

/**
 * @brief GPIO Output type enumeration
 */
typedef enum {
	LibGPIO_OType_PP = 0x00, /*!< GPIO Output Type Push-Pull */
	LibGPIO_OType_OD = 0x01  /*!< GPIO Output Type Open-Drain */
} LibGPIO_OType_t;

/**
 * @brief  GPIO Speed enumeration
 */
typedef enum {
	LibGPIO_Speed_Low = 0x00,    /*!< GPIO Speed Low */
	LibGPIO_Speed_Medium = 0x01, /*!< GPIO Speed Medium */
	LibGPIO_Speed_Fast = 0x02,   /*!< GPIO Speed Fast */
	LibGPIO_Speed_High = 0x03    /*!< GPIO Speed High */
} LibGPIO_Speed_t;

/**
 * @brief GPIO pull resistors enumeration
 */
typedef enum {
	LibGPIO_PuPd_NOPULL = 0x00, /*!< No pull resistor */
	LibGPIO_PuPd_UP = 0x01,     /*!< Pull up resistor enabled */
	LibGPIO_PuPd_DOWN = 0x02    /*!< Pull down resistor enabled */
} LibGPIO_PuPd_t;

/**
 * @} LibGPIO_Typedefs
 */

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
 * @param  GPIO_Mode: Select GPIO mode. This parameter can be a value of @ref LibGPIO_Mode_t enumeration
 * @param  GPIO_OType: Select GPIO Output type.
           This parameter can be a value of @ref LibGPIO_OType_t enumeration for outputs
           This parameter can be a value of @ref LibGPIO_PuPd_t enumeration for inputs
 * @param  GPIO_Speed: Select GPIO speed. This parameter can be a value of @ref LibGPIO_Speed_t enumeration
 * @retval None
 */
void LibGPIO_Conf(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, LibGPIO_Mode_t GPIO_Mode, LibGPIO_OType_t GPIO_Type, LibGPIO_Speed_t GPIO_Speed);

/**
 * @brief  Initializes GPIO pins(s)
 * @note   This function also enables clock for GPIO port
 * @param  GPIOx: Pointer to GPIOx port you will use for initialization
 * @param  GPIO_Pin: GPIO pin(s) you will use for initialization
 * @param  GPIO_Mode: Select GPIO mode. This parameter can be a value of @ref LibGPIO_Mode_t enumeration
 * @param  GPIO_OType: Select GPIO Output type. This parameter can be a value of @ref LibGPIO_OType_t enumeration
 * @param  GPIO_PuPd: Select GPIO pull resistor. This parameter can be a value of @ref LibGPIO_PuPd_t enumeration
 * @param  GPIO_Speed: Select GPIO speed. This parameter can be a value of @ref LibGPIO_Speed_t enumeration
 * @retval None
 */
void LibGPIO_Init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, LibGPIO_Mode_t GPIO_Mode, LibGPIO_OType_t GPIO_OType, LibGPIO_PuPd_t GPIO_PuPd, LibGPIO_Speed_t GPIO_Speed);

/**
 * @brief  Initializes GPIO pins(s) as alternate function
 * @note   This function also enables clock for GPIO port
 * @param  GPIOx: Pointer to GPIOx port you will use for initialization
 * @param  GPIO_Pin: GPIO pin(s) you will use for initialization
 * @param  GPIO_OType: Select GPIO Output type. This parameter can be a value of @ref LibGPIO_OType_t enumeration
 * @param  GPIO_PuPd: Select GPIO pull resistor. This parameter can be a value of @ref LibGPIO_PuPd_t enumeration
 * @param  GPIO_Speed: Select GPIO speed. This parameter can be a value of @ref LibGPIO_Speed_t enumeration
 * @param  Alternate: Alternate function you will use
 * @retval None
 */
void LibGPIO_InitAlternate(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, LibGPIO_OType_t GPIO_OType, LibGPIO_PuPd_t GPIO_PuPd, LibGPIO_Speed_t GPIO_Speed, uint8_t Alternate);

/**
 * @brief  Deinitializes pin(s)
 * @note   Pins(s) will be set as analog mode to get low power consumption
 * @param  GPIOx: GPIOx PORT where you want to set pin as input
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to set them as input
 * @retval None
 */
void LibGPIO_DeInit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
 * @brief  Sets pin(s) as input
 * @note   Pins HAVE to be initialized first using @ref LibGPIO_Init() or @ref LibGPIO_InitAlternate() function
 * @note   This is just an option for fast input mode
 * @param  GPIOx: GPIOx PORT where you want to set pin as input
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to set them as input
 * @retval None
 */
void LibGPIO_SetPinAsInput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
 * @brief  Sets pin(s) as output
 * @note   Pins HAVE to be initialized first using @ref LibGPIO_Init() or @ref LibGPIO_InitAlternate() function
 * @note   This is just an option for fast output mode
 * @param  GPIOx: GPIOx PORT where you want to set pin as output
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to set them as output
 * @retval None
 */
void LibGPIO_SetPinAsOutput(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
 * @brief  Sets pin(s) as analog
 * @note   Pins HAVE to be initialized first using @ref LibGPIO_Init() or @ref LibGPIO_InitAlternate() function
 * @note   This is just an option for fast analog mode
 * @param  GPIOx: GPIOx PORT where you want to set pin as analog
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to set them as analog
 * @retval None
 */
void LibGPIO_SetPinAsAnalog(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
 * @brief  Sets pin(s) as alternate function
 * @note   For proper alternate function, you should first init pin using @ref LibGPIO_InitAlternate() function.
 *            This functions is only used for changing GPIO mode
 * @param  GPIOx: GPIOx PORT where you want to set pin as alternate
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to set them as alternate
 * @retval None
 */
void LibGPIO_SetPinAsAlternate(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
 * @brief  Sets pull resistor settings to GPIO pin(s)
 * @note   Pins HAVE to be initialized first using @ref LibGPIO_Init() or @ref LibGPIO_InitAlternate() function
 * @param  *GPIOx: GPIOx PORT where you want to select pull resistor
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to set them as output
 * @param  GPIO_PuPd: Pull resistor option. This parameter can be a value of @ref LibGPIO_PuPd_t enumeration
 * @retval None
 */
void LibGPIO_SetPullResistor(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, LibGPIO_PuPd_t GPIO_PuPd);

/**
 * @brief  Sets pin(s) low
 * @note   Defined as macro to get maximum speed using register access
 * @param  GPIOx: GPIOx PORT where you want to set pin low
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to set them low
 * @retval None
 */
#define LibGPIO_SetPinLow(GPIOx, GPIO_Pin)			((GPIOx)->BSRRH = (GPIO_Pin))

/**
 * @brief  Sets pin(s) high
 * @note   Defined as macro to get maximum speed using register access
 * @param  GPIOx: GPIOx PORT where you want to set pin high
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to set them high
 * @retval None
 */
#define LibGPIO_SetPinHigh(GPIOx, GPIO_Pin) 		((GPIOx)->BSRRL = (GPIO_Pin))

/**
 * @brief  Sets pin(s) value
 * @note   Defined as macro to get maximum speed using register access
 * @param  GPIOx: GPIOx PORT where you want to set pin value
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to set them value
 * @param  val: If parameter is 0 then pin will be low, otherwise high
 * @retval None
 */
#define LibGPIO_SetPinValue(GPIOx, GPIO_Pin, val)	((val) ? LibGPIO_SetPinHigh(GPIOx, GPIO_Pin) : LibGPIO_SetPinLow(GPIOx, GPIO_Pin))

/**
 * @brief  Toggles pin(s)
 * @note   Defined as macro to get maximum speed using register access
 * @param  GPIOx: GPIOx PORT where you want to toggle pin value
 * @param  GPIO_Pin: Select GPIO pin(s). You can select more pins with | (OR) operator to toggle them all at a time
 * @retval None
 */
#define LibGPIO_TogglePinValue(GPIOx, GPIO_Pin)		((GPIOx)->ODR ^= (GPIO_Pin))

/**
 * @brief  Sets value to entire GPIO PORT
 * @note   Defined as macro to get maximum speed using register access
 * @param  GPIOx: GPIOx PORT where you want to set value
 * @param  value: Value for GPIO OUTPUT data
 * @retval None
 */
#define LibGPIO_SetPortValue(GPIOx, value)			((GPIOx)->ODR = (value))

/**
 * @brief  Gets input data bit
 * @note   Defined as macro to get maximum speed using register access
 * @param  GPIOx: GPIOx PORT where you want to read input bit value
 * @param  GPIO_Pin: GPIO pin where you want to read value
 * @retval 1 in case pin is high, or 0 if low
 */
#define LibGPIO_GetInputPinValue(GPIOx, GPIO_Pin)	(((GPIOx)->IDR & (GPIO_Pin)) == 0 ? 0 : 1)

/**
 * @brief  Gets output data bit
 * @note   Defined as macro to get maximum speed using register access
 * @param  GPIOx: GPIOx PORT where you want to read output bit value
 * @param  GPIO_Pin: GPIO pin where you want to read value
 * @retval 1 in case pin is high, or 0 if low
 */
#define LibGPIO_GetOutputPinValue(GPIOx, GPIO_Pin)	(((GPIOx)->ODR & (GPIO_Pin)) == 0 ? 0 : 1)

/**
 * @brief  Gets input value from entire GPIO PORT
 * @note   Defined as macro to get maximum speed using register access
 * @param  GPIOx: GPIOx PORT where you want to read input data value
 * @retval Entire PORT INPUT register
 */
#define LibGPIO_GetPortInputValue(GPIOx)			((GPIOx)->IDR)

/**
 * @brief  Gets output value from entire GPIO PORT
 * @note   Defined as macro to get maximum speed using register access
 * @param  GPIOx: GPIOx PORT where you want to read output data value
 * @retval Entire PORT OUTPUT register
 */
#define LibGPIO_GetPortOutputValue(GPIOx)			((GPIOx)->ODR)

/**
 * @brief  Gets port source from desired GPIOx PORT
 * @note   Meant for private use, unless you know what are you doing
 * @param  GPIOx: GPIO PORT for calculating port source
 * @retval Calculated port source for GPIO
 */
uint16_t LibGPIO_GetPortSource(GPIO_TypeDef* GPIOx);

/**
 * @brief  Gets pin source from desired GPIO pin
 * @note   Meant for private use, unless you know what are you doing
 * @param  GPIO_Pin: GPIO pin for calculating port source
 * @retval Calculated pin source for GPIO pin
 */
uint16_t LibGPIO_GetPinSource(uint16_t GPIO_Pin);

/**
 * @brief  Locks GPIOx register for future changes
 * @note   You are not able to config GPIO registers until new MCU reset occurs
 * @param  *GPIOx: GPIOx PORT where you want to lock config registers
 * @param  GPIO_Pin: GPIO pin(s) where you want to lock config registers
 * @retval None
 */
void LibGPIO_Lock(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
 * @brief  Gets bit separated pins which were used at least once in library and were not deinitialized
 * @param  *GPIOx: Pointer to GPIOx peripheral where to check used GPIO pins
 * @retval Bit values for used pins
 */
uint16_t LibGPIO_GetUsedPins(GPIO_TypeDef* GPIOx);

/**
 * @brief  Gets bit separated pins which were not used at in library or were deinitialized
 * @param  *GPIOx: Pointer to GPIOx peripheral where to check used GPIO pins
 * @retval Bit values for free pins
 */
uint16_t LibGPIO_GetFreePins(GPIO_TypeDef* GPIOx);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
