/**
 * SPI library for STM32F4xx
 */

#ifndef LibSPI_H
#define LibSPI_H 200

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

/**
 * It supports all 6 SPIs in master mode with 3 Lines Full Duplex mode
 *
 * All six SPIs work the same principle by default:
 *	- Master mode (SPI_Mode_Master)
 *	- 8 or 16 data bits (SPI_DataSize_8b or SPI_DataSize_8b)
 *	- Clock polarity low, data sampled at first edge, SPI mode 0 (LibSPI_Mode_0)
 *	- Prescaler from 2 to 256 (SPI_BaudRatePrescaler_2 to SPI_BaudRatePrescaler_256)
 *	- Firstbit MSB (SPI_FirstBit_MSB)
 *	- Software SS pin configure
 *	- Direction is full duplex 3 wires
 *

            PINS PACK 1            PINS PACK 2            PINS PACK 3
    SPIX    MOSI   MISO    SCK     MOSI   MISO    SCK     MOSI   MISO    SCK

    SPI1    PA7    PA6     PA5     PB5    PB4     PB3
    SPI2    PC3    PC2     PB10    PB15   PB14    PB13    PI3    PI2     PI0
    SPI3    PB5    PB4     PB3     PC12   PC11    PC10
    SPI4    PE6    PE5     PE2     PE14   PE13    PE12
    SPI5    PF9    PF8     PF7     PF11   PH7     PH6
    SPI6    PG14   PG12    PG13

 *	In case these pins are not good for you, you can use
 *	LibSPI_PinsPack_Custom in function and callback function will be called,
 *	where you can initialize your custom pinout for your SPI peripheral
 */

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"
#include "lib_gpio.h"

/**
 * @brief  SPI modes selection
 */
typedef enum
    {
	LibSPI_Mode_0, /*!< Clock polarity low, clock phase 1st edge */
	LibSPI_Mode_1, /*!< Clock polarity low, clock phase 2nd edge */
	LibSPI_Mode_2, /*!< Clock polarity high, clock phase 1st edge */
	LibSPI_Mode_3  /*!< Clock polarity high, clock phase 2nd edge */
    } LibSPI_Mode_t;

/**
 * @brief  USART PinsPack enumeration to select pins combination for USART
 */
typedef enum
    {
	LibSPI_PinsPack_1,       /*!< Select PinsPack1 from Pinout table for specific SPI */
	LibSPI_PinsPack_2,       /*!< Select PinsPack2 from Pinout table for specific SPI */
	LibSPI_PinsPack_3,       /*!< Select PinsPack3 from Pinout table for specific SPI */
	LibSPI_PinsPack_Custom   /*!< Select custom pins for specific SPI, callback will be called, look @ref LibSPI_InitCustomPinsCallback */
    } LibSPI_PinsPack_t;

/**
 * @brief  Daza size enumeration
 */
typedef enum
    {
	LibSPI_DataSize_8b, /*!< SPI in 8-bits mode */
	LibSPI_DataSize_16b /*!< SPI in 16-bits mode */
    } LibSPI_DataSize_t;

#define USE_SPI1
#define USE_SPI2
#define USE_SPI3
#ifdef SPI4
#define USE_SPI4
#else
#warning "SPI4 undefined. Please update library with STD drivers from ST.com"
#endif
#ifdef SPI5
#define USE_SPI5
#else
#warning "SPI5 undefined. Please update library with STD drivers from ST.com"
#endif
#ifdef SPI6
#define USE_SPI6
#else
#warning "SPI6 undefined. Please update library with STD drivers from ST.com"
#endif

//----- SPI1 options start -------
//Options can be overwriten in defines.h file
#ifndef LibSPI1_PRESCALER
#define LibSPI1_PRESCALER	SPI_BaudRatePrescaler_32
#endif
//Specify datasize
#ifndef LibSPI1_DATASIZE
#define LibSPI1_DATASIZE 	SPI_DataSize_8b
#endif
//Specify which bit is first
#ifndef LibSPI1_FIRSTBIT
#define LibSPI1_FIRSTBIT 	SPI_FirstBit_MSB
#endif
//Mode, master or slave
#ifndef LibSPI1_MASTERSLAVE
#define LibSPI1_MASTERSLAVE	SPI_Mode_Master
#endif
//Specify mode of operation, clock polarity and clock phase
#ifndef LibSPI1_MODE
#define LibSPI1_MODE		LibSPI_Mode_0
#endif
//----- SPI1 options end -------

//----- SPI2 options start -------
//Options can be overwriten in defines.h file
#ifndef LibSPI2_PRESCALER
#define LibSPI2_PRESCALER	SPI_BaudRatePrescaler_32
#endif
//Specify datasize
#ifndef LibSPI2_DATASIZE
#define LibSPI2_DATASIZE 	SPI_DataSize_8b
#endif
//Specify which bit is first
#ifndef LibSPI2_FIRSTBIT
#define LibSPI2_FIRSTBIT 	SPI_FirstBit_MSB
#endif
//Mode, master or slave
#ifndef LibSPI2_MASTERSLAVE
#define LibSPI2_MASTERSLAVE SPI_Mode_Master
#endif
//Specify mode of operation, clock polarity and clock phase
#ifndef LibSPI2_MODE
#define LibSPI2_MODE		LibSPI_Mode_0
#endif
//----- SPI2 options end -------

//----- SPI3 options start -------
//Options can be overwriten in defines.h file
#ifndef LibSPI3_PRESCALER
#define LibSPI3_PRESCALER	SPI_BaudRatePrescaler_32
#endif
//Specify datasize
#ifndef LibSPI3_DATASIZE
#define LibSPI3_DATASIZE 	SPI_DataSize_8b
#endif
//Specify which bit is first
#ifndef LibSPI3_FIRSTBIT
#define LibSPI3_FIRSTBIT 	SPI_FirstBit_MSB
#endif
//Mode, master or slave
#ifndef LibSPI3_MASTERSLAVE
#define LibSPI3_MASTERSLAVE SPI_Mode_Master
#endif
//Specify mode of operation, clock polarity and clock phase
#ifndef LibSPI3_MODE
#define LibSPI3_MODE		LibSPI_Mode_0
#endif
//----- SPI3 options end -------

//----- SPI4 options start -------
//Options can be overwriten in defines.h file
#ifndef LibSPI4_PRESCALER
#define LibSPI4_PRESCALER	SPI_BaudRatePrescaler_32
#endif
//Specify datasize
#ifndef LibSPI4_DATASIZE
#define LibSPI4_DATASIZE 	SPI_DataSize_8b
#endif
//Specify which bit is first
#ifndef LibSPI4_FIRSTBIT
#define LibSPI4_FIRSTBIT 	SPI_FirstBit_MSB
#endif
//Mode, master or slave
#ifndef LibSPI4_MASTERSLAVE
#define LibSPI4_MASTERSLAVE	SPI_Mode_Master
#endif
//Specify mode of operation, clock polarity and clock phase
#ifndef LibSPI4_MODE
#define LibSPI4_MODE		LibSPI_Mode_0
#endif
//----- SPI4 options end -------

//----- SPI5 options start -------
//Options can be overwriten in defines.h file
#ifndef LibSPI5_PRESCALER
#define LibSPI5_PRESCALER	SPI_BaudRatePrescaler_32
#endif
//Specify datasize
#ifndef LibSPI5_DATASIZE
#define LibSPI5_DATASIZE 	SPI_DataSize_8b
#endif
//Specify which bit is first
#ifndef LibSPI5_FIRSTBIT
#define LibSPI5_FIRSTBIT 	SPI_FirstBit_MSB
#endif
//Mode, master or slave
#ifndef LibSPI5_MASTERSLAVE
#define LibSPI5_MASTERSLAVE SPI_Mode_Master
#endif
//Specify mode of operation, clock polarity and clock phase
#ifndef LibSPI5_MODE
#define LibSPI5_MODE		LibSPI_Mode_0
#endif
//----- SPI5 options end -------

//----- SPI6 options start -------
//Options can be overwriten in defines.h file
#ifndef LibSPI6_PRESCALER
#define LibSPI6_PRESCALER	SPI_BaudRatePrescaler_32
#endif
//Specify datasize
#ifndef LibSPI6_DATASIZE
#define LibSPI6_DATASIZE 	SPI_DataSize_8b
#endif
//Specify which bit is first
#ifndef LibSPI6_FIRSTBIT
#define LibSPI6_FIRSTBIT 	SPI_FirstBit_MSB
#endif
//Mode, master or slave
#ifndef LibSPI6_MASTERSLAVE
#define LibSPI6_MASTERSLAVE SPI_Mode_Master
#endif
//Specify mode of operation, clock polarity and clock phase
#ifndef LibSPI6_MODE
#define LibSPI6_MODE		LibSPI_Mode_0
#endif
//----- SPI6 options end -------

/**
 * @brief  Check SPI busy status
 */
#define SPI_IS_BUSY(SPIx) (((SPIx)->SR & (SPI_SR_TXE | SPI_SR_RXNE)) == 0 || ((SPIx)->SR & SPI_SR_BSY))

/**
 * @brief  SPI wait till end
 */
#define SPI_WAIT(SPIx)            while (SPI_IS_BUSY(SPIx))

/**
 * @brief  Checks if SPI is enabled
 */
#define SPI_CHECK_ENABLED(SPIx)   if (!((SPIx)->CR1 & SPI_CR1_SPE)) {return;}

/**
 * @brief  Checks if SPI is enabled and returns value from function if not
 */
#define SPI_CHECK_ENABLED_RESP(SPIx, val)   if (!((SPIx)->CR1 & SPI_CR1_SPE)) {return (val);}

/**
 * @}
 */

/**
 * @defgroup LibSPI_Functions
 * @brief    Library Functions
 * @{
 */

/**
 * @brief  Initializes SPIx peripheral with custom pinspack and default other settings
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  SPI_DataSize: SPI data size LibSPI_DataSize_8b or LibSPI_DataSize_16b
 * @param  SPI_BaudRatePrescaler: SPI prescaler SPI_BaudRatePrescaler_2 to SPI_BaudRatePrescaler_256
 * @retval None
 */
void LibSPI_Init(SPI_TypeDef* SPIx, LibSPI_DataSize_t SPI_DataSize, uint16_t SPI_BaudRatePrescaler);

/**
 * @brief  Initializes SPIx peripheral with custom pinspack and SPI mode and default other settings
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  pinspack: Pinspack you will use from default SPI table. This parameter can be a value of @ref LibSPI_PinsPack_t enumeration
 * @param  SPI_Mode: SPI mode you will use. This parameter can be a value of @ref LibSPI_Mode_t enumeration
 * @retval None
 */
void LibSPI_InitWithMode(SPI_TypeDef* SPIx, LibSPI_PinsPack_t pinspack, LibSPI_Mode_t SPI_Mode);

/**
 * @brief  Initializes SPIx peripheral with custom settings
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  pinspack: Pinspack you will use from default SPI table. This parameter can be a value of @ref LibSPI_PinsPack_t enumeration
 * @param  SPI_BaudRatePrescaler: SPI baudrate prescaler. This parameter can be a value of @ref SPI_BaudRatePrescaler
 * @param  SPI_Mode_t: SPI mode you will use. This parameter can be a value of @ref LibSPI_Mode_t enumeration
 * @param  SPI_Mode: SPI mode you will use:
 *            - SPI_Mode_Master: SPI in master mode (default)
 *            - SPI_Mode_Slave: SPI in slave mode
 * @param  SPI_FirstBit: select first bit for SPI
 *            - SPI_FirstBit_MSB: MSB is first bit (default)
 *            - SPI_FirstBit_LSB: LSB is first bit
 * @retval None
 */
void LibSPI_InitFull(SPI_TypeDef* SPIx, LibSPI_PinsPack_t pinspack, uint16_t SPI_BaudRatePrescaler, LibSPI_Mode_t SPI_Mode_t, uint16_t SPI_Mode, uint16_t SPI_FirstBit);

/**
 * @brief  Calculates bits for SPI prescaler register to get minimal prescaler value for SPI peripheral
 * @note   SPI has 8 prescalers available, 2,4,6,...,128,256
 * @note   This function will return you a bits you must set in your CR1 register.
 *
 * @note   Imagine, you can use 20MHz max clock in your system, your system is running on 168MHz, and you use SPI on APB2 bus.
 *         On 168 and 180MHz devices, APB2 works on Fclk/2, so 84 and 90MHz.
 *         So, if you calculate this, prescaler will need to be 84MHz / 20MHz = 4.xx, but if you use 4 prescaler, then you will be over 20MHz.
 *         You need 8 prescaler then. This function will calculate this.
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6.
 *            Different SPIx works on different clock and is important to know for which SPI you need prescaler.
 * @param  MAX_SPI_Frequency: Max SPI frequency you can use. Function will calculate the minimum prescaler you need for that.
 *
 * @retval Bits combination for SPI_CR1 register, with aligned location already, prepared to set parameter for @ref LibSPI_InitFull() function.
 */
uint16_t LibSPI_GetPrescaler(SPI_TypeDef* SPIx, uint32_t MAX_SPI_Frequency);

/**
 * @brief  Sets data size for SPI at runtime
 * @note   You can select either 8 or 16 bits data array.
 * @param  *SPIx: Pointer to SPIx peripheral where data size will be set
 * @param  DataSize: Datasize which will be used. This parameter can be a value of @ref LibSPI_DataSize_t enumeration
 * @retval Status of data size before changes happen
 */
LibSPI_DataSize_t LibSPI_SetDataSize(SPI_TypeDef* SPIx, LibSPI_DataSize_t DataSize);

/**
 * @brief  Sends and receives single byte over SPI
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  data: 8-bit data size to send over SPI
 * @retval Received byte from slave device
 */
uint16_t LibSPI_Send(SPI_TypeDef* SPIx, uint16_t data);

/**
 * @brief  Sends and receives multiple bytes over SPIx
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  *dataOut: Pointer to array with data to send over SPI
 * @param  *dataIn: Pointer to array to to save incoming data
 * @param  count: Number of bytes to send/receive over SPI
 * @retval None
 */
void LibSPI_SendMulti(SPI_TypeDef* SPIx, uint8_t* dataOut, uint8_t* dataIn, uint32_t count);

/**
 * @brief  Writes multiple bytes over SPI
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  *dataOut: Pointer to array with data to send over SPI
 * @param  count: Number of elements to send over SPI
 * @retval None
 */
void LibSPI_WriteMulti(SPI_TypeDef* SPIx, uint8_t* dataOut, uint32_t count);

/**
 * @brief  Receives multiple data bytes over SPI
 * @note   Selected SPI must be set in 16-bit mode
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  *dataIn: Pointer to 8-bit array to save data into
 * @param  dummy: Dummy byte  to be sent over SPI, to receive data back. In most cases 0x00 or 0xFF
 * @param  count: Number of bytes you want read from device
 * @retval None
 */
void LibSPI_ReadMulti(SPI_TypeDef* SPIx, uint8_t *dataIn, uint8_t dummy, uint32_t count);

/**
 * @brief  Sends single byte over SPI
 * @note   Selected SPI must be set in 16-bit mode
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  data: 16-bit data size to send over SPI
 * @retval Received 16-bit value from slave device
 */
static __INLINE uint16_t LibSPI_Send16(SPI_TypeDef* SPIx, uint8_t data) {
	/* Check if SPI is enabled */
	SPI_CHECK_ENABLED_RESP(SPIx, 0);

	/* Wait for previous transmissions to complete if DMA TX enabled for SPI */
	SPI_WAIT(SPIx);

	/* Fill output buffer with data */
	SPIx->DR = data;

	/* Wait for transmission to complete */
	SPI_WAIT(SPIx);

	/* Return data from buffer */
	return SPIx->DR;
}

/**
 * @brief  Sends and receives multiple bytes over SPIx in 16-bit SPI mode
 * @note   Selected SPI must be set in 16-bit mode
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  *dataOut: Pointer to array with data to send over SPI
 * @param  *dataIn: Pointer to array to to save incoming data
 * @param  count: Number of 16-bit values to send/receive over SPI
 * @retval None
 */
void LibSPI_SendMulti16(SPI_TypeDef* SPIx, uint16_t* dataOut, uint16_t* dataIn, uint32_t count);

/**
 * @brief  Writes multiple data via SPI in 16-bit SPI mode
 * @note   Selected SPI must be set in 16-bit mode
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  *dataOut: Pointer to 16-bit array with data to send over SPI
 * @param  count: Number of elements to send over SPI
 * @retval None
 */
void LibSPI_WriteMulti16(SPI_TypeDef* SPIx, uint16_t* dataOut, uint32_t count);

/**
 * @brief  Receives multiple data bytes over SPI in 16-bit SPI mode
 * @note   Selected SPI must be set in 16-bit mode
 * @param  *SPIx: Pointer to SPIx peripheral you will use, where x is between 1 to 6
 * @param  *dataIn: Pointer to 16-bit array to save data into
 * @param  dummy: Dummy 16-bit value to be sent over SPI, to receive data back. In most cases 0x00 or 0xFF
 * @param  count: Number of 16-bit values you want read from device
 * @retval None
 */
void LibSPI_ReadMulti16(SPI_TypeDef* SPIx, uint16_t* dataIn, uint16_t dummy, uint32_t count);


/* Private functions */
static void LibSPIx_Init(SPI_TypeDef* SPIx, LibSPI_PinsPack_t pinspack, LibSPI_Mode_t SPI_Mode, uint16_t SPI_BaudRatePrescaler, uint16_t SPI_MasterSlave, uint16_t SPI_FirstBit);
void LibSPI1_INT_InitPins(LibSPI_PinsPack_t pinspack);
void LibSPI2_INT_InitPins(LibSPI_PinsPack_t pinspack);
void LibSPI3_INT_InitPins(LibSPI_PinsPack_t pinspack);
void LibSPI4_INT_InitPins(LibSPI_PinsPack_t pinspack);
void LibSPI5_INT_InitPins(LibSPI_PinsPack_t pinspack);
void LibSPI6_INT_InitPins(LibSPI_PinsPack_t pinspack);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif

