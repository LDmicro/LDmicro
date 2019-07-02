/**
 * SPI library for STM32F10x
 */

#ifndef LibSPI_H
#define LibSPI_H 200

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

/**
 * It supports all 2 SPIs in master mode with 3 Lines Full Duplex mode
 *
 * All 2 SPIs work the same principle by default:
 *  - Master mode (SPI_Mode_Master)
 *  - 8 or 16 data bits (SPI_DataSize_8b or SPI_DataSize_8b)
 *  - Clock polarity low, data sampled at first edge, SPI mode 0 (LibSPI_Mode_0)
 *  - Prescaler from 2 to 256 (SPI_BaudRatePrescaler_2 to SPI_BaudRatePrescaler_256)
 *  - Firstbit MSB (SPI_FirstBit_MSB)
 *  - Software SS pin configure
 *  - Direction is full duplex 3 wires
 *

    SPIX    MOSI    MISO    SCK     SS

    SPI1    PA7     PA6     PA5     PA4
    SPI2    PB15    PB14    PB13    PB12

 */

#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_spi.h"
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
 * @brief  Daza size enumeration
 */
typedef enum
    {
    LibSPI_DataSize_8b, /*!< SPI in 8-bits mode */
    LibSPI_DataSize_16b /*!< SPI in 16-bits mode */
    } LibSPI_DataSize_t;

#define USE_SPI1
#define USE_SPI2

//----- SPI1 options start -------
//Options can be overwriten in defines.h file
#ifndef LibSPI1_PRESCALER
#define LibSPI1_PRESCALER   SPI_BaudRatePrescaler_32
#endif
//Specify datasize
#ifndef LibSPI1_DATASIZE
#define LibSPI1_DATASIZE    SPI_DataSize_8b
#endif
//Specify which bit is first
#ifndef LibSPI1_FIRSTBIT
#define LibSPI1_FIRSTBIT    SPI_FirstBit_MSB
#endif
//Mode, master or slave
#ifndef LibSPI1_MASTERSLAVE
#define LibSPI1_MASTERSLAVE SPI_Mode_Master
#endif
//Specify mode of operation, clock polarity and clock phase
#ifndef LibSPI1_MODE
#define LibSPI1_MODE        LibSPI_Mode_0
#endif
//----- SPI1 options end -------

//----- SPI2 options start -------
//Options can be overwriten in defines.h file
#ifndef LibSPI2_PRESCALER
#define LibSPI2_PRESCALER   SPI_BaudRatePrescaler_32
#endif
//Specify datasize
#ifndef LibSPI2_DATASIZE
#define LibSPI2_DATASIZE    SPI_DataSize_8b
#endif
//Specify which bit is first
#ifndef LibSPI2_FIRSTBIT
#define LibSPI2_FIRSTBIT    SPI_FirstBit_MSB
#endif
//Mode, master or slave
#ifndef LibSPI2_MASTERSLAVE
#define LibSPI2_MASTERSLAVE SPI_Mode_Master
#endif
//Specify mode of operation, clock polarity and clock phase
#ifndef LibSPI2_MODE
#define LibSPI2_MODE        LibSPI_Mode_0
#endif
//----- SPI2 options end -------

void LibSPI_Init(SPI_TypeDef* SPIx, LibSPI_DataSize_t SPI_DataSize, uint16_t SPI_BaudRatePrescaler);

void LibSPI_InitFull(SPI_TypeDef* SPIx, LibSPI_Mode_t SPI_Mode, uint16_t SPI_BaudRatePrescaler,
    LibSPI_DataSize_t SPI_DataSize, uint16_t SPI_MasterSlave, uint16_t SPI_FirstBit);

uint16_t LibSPI_GetPrescaler(SPI_TypeDef* SPIx, uint32_t MAX_SPI_Frequency);

uint16_t LibSPI_Send(SPI_TypeDef* SPIx, uint16_t data);

void LibSPI_WriteMulti(SPI_TypeDef* SPIx, uint8_t* dataOut, uint32_t count);

void LibSPI_ReadMulti(SPI_TypeDef* SPIx, uint8_t *dataIn, uint8_t dummy, uint32_t count);

void LibSPI_Stop(SPI_TypeDef* SPIx);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
