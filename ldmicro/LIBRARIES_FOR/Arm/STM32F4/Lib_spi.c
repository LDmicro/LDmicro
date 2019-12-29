/**
 * Copyright (C) Tilen Majerle 2014, and JG 2016
 */

#include "lib_spi.h"

/**
  * @brief  Basic SPI init in mode 0 pinspack 1
  * @param  SPIx = SPI1 to SPI6
  * @param  SPI_DataSize = LibSPI_DataSize_8b or LibSPI_DataSize_16b
  * @param  SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2 to SPI_BaudRatePrescaler_256
  * @retval None
  */
void LibSPI_Init(SPI_TypeDef *SPIx, LibSPI_DataSize_t SPI_DataSize, uint16_t SPI_BaudRatePrescaler)
{
    // Init with default settings
    if(SPIx == SPI1) {
        LibSPIx_Init(SPI1, LibSPI_PinsPack_1, LibSPI1_MODE, SPI_BaudRatePrescaler, LibSPI1_MASTERSLAVE, LibSPI1_FIRSTBIT);
    }
    if(SPIx == SPI2) {
        LibSPIx_Init(SPI2, LibSPI_PinsPack_1, LibSPI2_MODE, SPI_BaudRatePrescaler, LibSPI2_MASTERSLAVE, LibSPI2_FIRSTBIT);
    }
    if(SPIx == SPI3) {
        LibSPIx_Init(SPI3, LibSPI_PinsPack_1, LibSPI3_MODE, SPI_BaudRatePrescaler, LibSPI3_MASTERSLAVE, LibSPI3_FIRSTBIT);
    }
    if(SPIx == SPI4) {
        LibSPIx_Init(SPI4, LibSPI_PinsPack_1, LibSPI4_MODE, SPI_BaudRatePrescaler, LibSPI4_MASTERSLAVE, LibSPI4_FIRSTBIT);
    }
    if(SPIx == SPI5) {
        LibSPIx_Init(SPI5, LibSPI_PinsPack_1, LibSPI5_MODE, SPI_BaudRatePrescaler, LibSPI5_MASTERSLAVE, LibSPI5_FIRSTBIT);
    }
    if(SPIx == SPI6) {
        LibSPIx_Init(SPI6, LibSPI_PinsPack_1, LibSPI6_MODE, SPI_BaudRatePrescaler, LibSPI6_MASTERSLAVE, LibSPI6_FIRSTBIT);
    }

    LibSPI_SetDataSize(SPIx, SPI_DataSize);
}

/**
  * @brief  Full SPI init
  * @param  SPIx = SPI1 to SPI6
  * @param
  * @param  SPI_DataSize = LibSPI_DataSize_8b or LibSPI_DataSize_16b
  * @param  SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2 to SPI_BaudRatePrescaler_256
  * @retval None
  */

void LibSPI_InitFull(SPI_TypeDef *SPIx, LibSPI_PinsPack_t pinspack, uint16_t SPI_BaudRatePrescaler, LibSPI_Mode_t SPI_Mode_t, uint16_t SPI_Mode, uint16_t SPI_FirstBit)
{
    // Init FULL SPI settings by user
    if(SPIx == SPI1) {
        LibSPIx_Init(SPI1, pinspack, SPI_Mode_t, SPI_BaudRatePrescaler, SPI_Mode, SPI_FirstBit);
    }
    if(SPIx == SPI2) {
        LibSPIx_Init(SPI2, pinspack, SPI_Mode_t, SPI_BaudRatePrescaler, SPI_Mode, SPI_FirstBit);
    }
    if(SPIx == SPI3) {
        LibSPIx_Init(SPI3, pinspack, SPI_Mode_t, SPI_BaudRatePrescaler, SPI_Mode, SPI_FirstBit);
    }
    if(SPIx == SPI4) {
        LibSPIx_Init(SPI4, pinspack, SPI_Mode_t, SPI_BaudRatePrescaler, SPI_Mode, SPI_FirstBit);
    }
    if(SPIx == SPI5) {
        LibSPIx_Init(SPI5, pinspack, SPI_Mode_t, SPI_BaudRatePrescaler, SPI_Mode, SPI_FirstBit);
    }
    if(SPIx == SPI6) {
        LibSPIx_Init(SPI6, pinspack, SPI_Mode_t, SPI_BaudRatePrescaler, SPI_Mode, SPI_FirstBit);
    }
}

/**
  * @brief  Compute prescaler according to SPI required frequency
  * @param  SPIx = SPI1 to SPI6
  * @param  MAX_SPI_Frequency = required frequency
  * @retval SPI_BaudRatePrescaler_2 to SPI_BaudRatePrescaler_256
  */

uint16_t LibSPI_GetPrescaler(SPI_TypeDef *SPIx, uint32_t MAX_SPI_Frequency)
{
    RCC_ClocksTypeDef RCC_Clocks;
    uint32_t          APB_Frequency;
    uint8_t           i;

    // Prevent wrong input
    if(MAX_SPI_Frequency == 0)
        return SPI_BaudRatePrescaler_256;

    // Get clock values from RCC
    RCC_GetClocksFreq(&RCC_Clocks);

    // Calculate max SPI clock
    if((SPIx == SPI1) || (SPIx == SPI4) || (SPIx == SPI5) || (SPIx == SPI6)) {
        APB_Frequency = RCC_Clocks.PCLK2_Frequency;
    } else {
        APB_Frequency = RCC_Clocks.PCLK1_Frequency;
    }

    // Calculate prescaler value
    // Bits 5:3 in CR1 SPI registers are prescalers
    // 000 = 2, 001 = 4, 002 = 8, ..., 111 = 256
    for(i = 0; i < 8; i++) {
        if(APB_Frequency / (1 << (i + 1)) <= MAX_SPI_Frequency) {
            // Bits for BP are 5:3 in CR1 register
            return (i << 3);
        }
    }

    // Use max prescaler possible
    return SPI_BaudRatePrescaler_256;
}

/**
  * @brief  Modifie data size to 8b or 16b
  * @param  SPIx = SPI1 to SPI6
  * @param  DataSize = LibSPI_DataSize_8b or LibSPI_DataSize_16b
  * @retval LibSPI_DataSize_8b or LibSPI_DataSize_16b
  */
LibSPI_DataSize_t LibSPI_SetDataSize(SPI_TypeDef *SPIx, LibSPI_DataSize_t DataSize)
{
    LibSPI_DataSize_t status = (SPIx->CR1 & SPI_CR1_DFF) ? LibSPI_DataSize_16b : LibSPI_DataSize_8b;

    // Disable SPI first
    SPIx->CR1 &= ~SPI_CR1_SPE;

    // Set proper value
    if(DataSize == LibSPI_DataSize_16b) {
        // Set bit for frame
        SPIx->CR1 |= SPI_CR1_DFF;
    } else {
        // Clear bit for frame
        SPIx->CR1 &= ~SPI_CR1_DFF;
    }

    // Enable SPI back
    SPIx->CR1 |= SPI_CR1_SPE;

    // Return status
    return status;
}

/**
  * @brief  Send (and receive) single data via SPI
  * @param  SPIx = SPI1 to SPI6
  * @param  data = data to send
  * @retval data received (if any)
  */
uint16_t LibSPI_Send(SPI_TypeDef *SPIx, uint16_t data)
{
    // Check if SPI is enabled
    SPI_CHECK_ENABLED_RESP(SPIx, 0);

    // Wait for previous transmissions to complete
    SPI_WAIT(SPIx);
    // Fill output buffer with data
    SPIx->DR = data;
    // Wait for transmission to complete
    SPI_WAIT(SPIx);

    // Return received data from buffer if any
    return SPIx->DR;
}

/**
  * @brief  Send (and receive) multiple 8 bits data via SPI
  * @param  SPIx = SPI1 to SPI6
  * @param  dataOut = buffer containing data to send
  * @param  dataIn = buffer to contain received data (if any)
  * @param  count = number of data to send
  * @retval None
  */
void LibSPI_SendMulti(SPI_TypeDef *SPIx, uint8_t *dataOut, uint8_t *dataIn, uint32_t count)
{
    uint32_t i;

    // Check if SPI is enabled
    SPI_CHECK_ENABLED(SPIx);

    // Wait for previous transmissions to complete if DMA TX enabled for SPI
    SPI_WAIT(SPIx);

    for(i = 0; i < count; i++) {
        // Fill output buffer with data
        SPIx->DR = dataOut[i];

        // Wait for SPI to end everything
        SPI_WAIT(SPIx);

        // Read data register
        dataIn[i] = SPIx->DR;
    }
}

/**
  * @brief  Send multiple 8 bits data via SPI
  * @param  SPIx = SPI1 to SPI6
  * @param  dataOut = buffer containing data to send
  * @param  count = number of data to send
  * @retval None
  */
void LibSPI_WriteMulti(SPI_TypeDef *SPIx, uint8_t *dataOut, uint32_t count)
{
    uint32_t i;

    // Check if SPI is enabled
    SPI_CHECK_ENABLED(SPIx);

    // Wait for previous transmissions to complete if DMA TX enabled for SPI
    SPI_WAIT(SPIx);

    for(i = 0; i < count; i++) {
        // Fill output buffer with data
        SPIx->DR = dataOut[i];

        // Wait for SPI to end everything
        SPI_WAIT(SPIx);

        // Read data register
        (void)SPIx->DR;
    }
}

/**
  * @brief  receive multiple 8 bits data via SPI
  * @param  SPIx = SPI1 to SPI6
  * @param  dataIn = buffer to contain received data
  * @param  dummy = dummy data to send (to generate clock)
  * @param  count = number of data to receive
  * @retval None
  */
void LibSPI_ReadMulti(SPI_TypeDef *SPIx, uint8_t *dataIn, uint8_t dummy, uint32_t count)
{
    uint32_t i;

    // Check if SPI is enabled
    SPI_CHECK_ENABLED(SPIx);

    // Wait for previous transmissions to complete if DMA TX enabled for SPI
    SPI_WAIT(SPIx);

    for(i = 0; i < count; i++) {
        // Fill output buffer with data
        SPIx->DR = dummy;

        // Wait for SPI to end everything
        SPI_WAIT(SPIx);

        // Save data to buffer
        dataIn[i] = SPIx->DR;
    }
}

/**
  * @brief  Send (and receive) multiple 16 bits data via SPI
  * @param  SPIx = SPI1 to SPI6
  * @param  dataOut = buffer containing data to send
  * @param  dataIn = buffer to contain received data (if any)
  * @param  count = number of data to send
  * @retval None
  */
void LibSPI_SendMulti16(SPI_TypeDef *SPIx, uint16_t *dataOut, uint16_t *dataIn, uint32_t count)
{
    uint32_t i;

    // Check if SPI is enabled
    SPI_CHECK_ENABLED(SPIx);

    // Wait for previous transmissions to complete if DMA TX enabled for SPI
    SPI_WAIT(SPIx);

    for(i = 0; i < count; i++) {
        // Fill output buffer with data
        SPIx->DR = dataOut[i];

        // Wait for SPI to end everything
        SPI_WAIT(SPIx);

        // Read data register
        dataIn[i] = SPIx->DR;
    }
}

/**
  * @brief  Send multiple 16 bits data via SPI
  * @param  SPIx = SPI1 to SPI6
  * @param  dataOut = buffer containing data to send
  * @param  count = number of data to send
  * @retval None
  */
void LibSPI_WriteMulti16(SPI_TypeDef *SPIx, uint16_t *dataOut, uint32_t count)
{
    uint32_t i;

    // Check if SPI is enabled
    SPI_CHECK_ENABLED(SPIx);

    // Wait for previous transmissions to complete if DMA TX enabled for SPI
    SPI_WAIT(SPIx);

    for(i = 0; i < count; i++) {
        // Fill output buffer with data
        SPIx->DR = dataOut[i];

        // Wait for SPI to end everything
        SPI_WAIT(SPIx);

        // Read data register
        (void)SPIx->DR;
    }
}

/**
  * @brief  receive multiple 16 bits data via SPI
  * @param  SPIx = SPI1 to SPI6
  * @param  dataIn = buffer to contain received data
  * @param  dummy = dummy data to send (to generate clock)
  * @param  count = number of data to receive
  * @retval None
  */
void LibSPI_ReadMulti16(SPI_TypeDef *SPIx, uint16_t *dataIn, uint16_t dummy, uint32_t count)
{
    uint32_t i;

    // Check if SPI is enabled
    SPI_CHECK_ENABLED(SPIx);

    // Wait for previous transmissions to complete if DMA TX enabled for SPI
    SPI_WAIT(SPIx);

    for(i = 0; i < count; i++) {
        // Fill output buffer with data
        SPIx->DR = dummy;

        // Wait for SPI to end everything
        SPI_WAIT(SPIx);

        // Save data to buffer
        dataIn[i] = SPIx->DR;
    }
}

/**
  * @brief  private functions
  */
static void LibSPIx_Init(SPI_TypeDef *SPIx, LibSPI_PinsPack_t pinspack, LibSPI_Mode_t SPI_Mode, uint16_t SPI_BaudRatePrescaler, uint16_t SPI_MasterSlave, uint16_t SPI_FirstBit)
{
    SPI_InitTypeDef SPI_InitStruct;

    // Set default settings
    SPI_StructInit(&SPI_InitStruct);

    if(SPIx == SPI1) {
        // Enable SPI clock
        RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

        // Init pins
        LibSPI1_INT_InitPins(pinspack);

        // Set options
        SPI_InitStruct.SPI_DataSize = LibSPI1_DATASIZE;
    }
    if(SPIx == SPI2) {
        // Enable SPI clock
        RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

        // Init pins
        LibSPI2_INT_InitPins(pinspack);

        // Set options
        SPI_InitStruct.SPI_DataSize = LibSPI2_DATASIZE;
    }
    if(SPIx == SPI3) {
        // Enable SPI clock
        RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;

        // Init pins
        LibSPI3_INT_InitPins(pinspack);

        // Set options
        SPI_InitStruct.SPI_DataSize = LibSPI3_DATASIZE;
    }
    if(SPIx == SPI4) {
        // Enable SPI clock
        RCC->APB2ENR |= RCC_APB2ENR_SPI4EN;

        // Init pins
        LibSPI4_INT_InitPins(pinspack);

        // Set options
        SPI_InitStruct.SPI_DataSize = LibSPI4_DATASIZE;
    }
    if(SPIx == SPI5) {
        // Enable SPI clock
        RCC->APB2ENR |= RCC_APB2ENR_SPI5EN;

        // Init pins
        LibSPI5_INT_InitPins(pinspack);

        // Set options
        SPI_InitStruct.SPI_DataSize = LibSPI5_DATASIZE;
    }
    if(SPIx == SPI6) {
        // Enable SPI clock
        RCC->APB2ENR |= RCC_APB2ENR_SPI6EN;

        // Init pins
        LibSPI6_INT_InitPins(pinspack);

        // Set options
        SPI_InitStruct.SPI_DataSize = LibSPI6_DATASIZE;
    }

    // Fill SPI settings
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler;
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit;
    SPI_InitStruct.SPI_Mode = SPI_MasterSlave;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
    //SPI_InitStruct.SPI_DataSize = SPI_DataSize_16b;

    // SPI mode
    if(SPI_Mode == LibSPI_Mode_0) {
        SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
        SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    }
    if(SPI_Mode == LibSPI_Mode_1) {
        SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
        SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
    }
    if(SPI_Mode == LibSPI_Mode_2) {
        SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;
        SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
    }
    if(SPI_Mode == LibSPI_Mode_3) {
        SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;
        SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
    }

    // Disable first
    SPIx->CR1 &= ~SPI_CR1_SPE;

    // Init SPI
    SPI_Init(SPIx, &SPI_InitStruct);

    // Enable SPI
    SPIx->CR1 |= SPI_CR1_SPE;
}

void LibSPI1_INT_InitPins(LibSPI_PinsPack_t pinspack)
{
    // Init SPI pins
    if(pinspack == LibSPI_PinsPack_1) {
        LibGPIO_InitAlternate(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI1);
    }
    if(pinspack == LibSPI_PinsPack_2) {
        LibGPIO_InitAlternate(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI1);
    }
}

void LibSPI2_INT_InitPins(LibSPI_PinsPack_t pinspack)
{
    // Init SPI pins
    if(pinspack == LibSPI_PinsPack_1) {
        LibGPIO_InitAlternate(GPIOB, GPIO_PIN_10, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI2);
        LibGPIO_InitAlternate(GPIOC, GPIO_PIN_2 | GPIO_PIN_3, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI2);
    }
    if(pinspack == LibSPI_PinsPack_2) {
        LibGPIO_InitAlternate(GPIOB, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI2);
    }
    if(pinspack == LibSPI_PinsPack_3) {
        LibGPIO_InitAlternate(GPIOI, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI2);
    }
}

void LibSPI3_INT_InitPins(LibSPI_PinsPack_t pinspack)
{
    // Enable SPI pins
    if(pinspack == LibSPI_PinsPack_1) {
        LibGPIO_InitAlternate(GPIOB, GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI3);
    }
    if(pinspack == LibSPI_PinsPack_2) {
        LibGPIO_InitAlternate(GPIOC, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI3);
    }
}

void LibSPI4_INT_InitPins(LibSPI_PinsPack_t pinspack)
{
    // Init SPI pins
    if(pinspack == LibSPI_PinsPack_1) {
        LibGPIO_InitAlternate(GPIOE, GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_6, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI4);
    }
    if(pinspack == LibSPI_PinsPack_2) {
        LibGPIO_InitAlternate(GPIOE, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI4);
    }
}

void LibSPI5_INT_InitPins(LibSPI_PinsPack_t pinspack)
{
    // Init SPI pins
    if(pinspack == LibSPI_PinsPack_1) {
        LibGPIO_InitAlternate(GPIOF, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI5);
    }
    if(pinspack == LibSPI_PinsPack_2) {
        LibGPIO_InitAlternate(GPIOF, GPIO_PIN_11, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI5);
        LibGPIO_InitAlternate(GPIOH, GPIO_PIN_6 | GPIO_PIN_7, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI5);
    }
}

void LibSPI6_INT_InitPins(LibSPI_PinsPack_t pinspack)
{
    if(pinspack == LibSPI_PinsPack_1) {
        // Init SPI pins
        LibGPIO_InitAlternate(GPIOG, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14, LibGPIO_OType_PP, LibGPIO_PuPd_NOPULL, LibGPIO_Speed_High, GPIO_AF_SPI6);
    }
}
