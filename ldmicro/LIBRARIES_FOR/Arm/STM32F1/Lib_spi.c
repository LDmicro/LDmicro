/**
 * Copyright (C) JG 2019
 */

#include "lib_spi.h"
#include "lib_gpio.h"

/**
  * @brief  Basic SPI init in mode 0
  * @param  SPIx = SPI1 or SPI2
  * @param  SPI_DataSize = LibSPI_DataSize_8b or LibSPI_DataSize_16b
  * @param  SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2 to SPI_BaudRatePrescaler_256
  * @retval None
  */
void LibSPI_Init(SPI_TypeDef *SPIx, LibSPI_DataSize_t SPI_DataSize, uint16_t SPI_BaudRatePrescaler)
{
    // Init with extra default settings
    if(SPIx == SPI1) {
        LibSPI_InitFull(SPI1, LibSPI1_MODE, SPI_BaudRatePrescaler, SPI_DataSize, LibSPI1_MASTERSLAVE, LibSPI1_FIRSTBIT);
    }
    if(SPIx == SPI2) {
        LibSPI_InitFull(SPI2, LibSPI2_MODE, SPI_BaudRatePrescaler, SPI_DataSize, LibSPI2_MASTERSLAVE, LibSPI2_FIRSTBIT);
    }
}

/**
  * @brief  Full SPI init
  * @param  SPIx = SPI1 or SPI2
  * @param  SPI_Mode = LibSPI_Mode_0 to LibSPI_Mode_3 for polarity and phase
  * @param  SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2 to SPI_BaudRatePrescaler_256
  * @param  SPI_DataSize = LibSPI_DataSize_8b or LibSPI_DataSize_16b
  * @param  SPI_MasterSlave = SPI_Mode_Master or SPI_Mode_Slave
  * @param  SPI_FirstBit = SPI_FirstBit_MSB or SPI_FirstBit_LSB
  * @retval None
  */
void LibSPI_InitFull(SPI_TypeDef *SPIx, LibSPI_Mode_t SPI_Mode, uint16_t SPI_BaudRatePrescaler, LibSPI_DataSize_t SPI_DataSize, uint16_t SPI_MasterSlave, uint16_t SPI_FirstBit)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    SPI_InitTypeDef  SPI_InitStruct;

    // Set default settings
    SPI_StructInit(&SPI_InitStruct);

    if(SPIx == SPI1) {
        // Configure clocks
        RCC_PCLK2Config(RCC_HCLK_Div2);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

        // Configure SCK and MOSI pins as Alternate Function Push Pull
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;

        GPIO_Init(GPIOA, &GPIO_InitStruct);

        // Configure MISO pin as Input Floating
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;

        GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    if(SPIx == SPI2) {
        // Configure clocks
        RCC_PCLK2Config(RCC_HCLK_Div2);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

        // Configure SCK and MOSI pins as Alternate Function Push Pull
        // SPI2 can go slower than SPI1 because APB1 clock is half of APB2
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;

        GPIO_Init(GPIOB, &GPIO_InitStruct);

        // Confugure MISO pin as Input Floating
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_14;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;

        GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    // Fill SPI settings
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStruct.SPI_Mode = SPI_MasterSlave;
    SPI_InitStruct.SPI_DataSize = SPI_DataSize;
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler;
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit;
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;

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
    SPI_Cmd(SPIx, DISABLE);
    // Init SPI
    SPI_Init(SPIx, &SPI_InitStruct);
    // Enable SPI
    SPI_Cmd(SPIx, ENABLE);
}

/**
  * @brief  Compute prescaler according to SPI required frequency
  * @param  SPIx = SPI1 or SPI2
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
    if(SPIx == SPI1) {
        APB_Frequency = RCC_Clocks.PCLK2_Frequency;
    } else {
        APB_Frequency = RCC_Clocks.PCLK1_Frequency;
    }

    // Calculate prescaler value
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
  * @brief  Send (and receive) single data via SPI
  * @param  SPIx = SPI1 or SPI2
  * @param  data = data to send
  * @retval data received (if any)
  */
uint16_t LibSPI_Send(SPI_TypeDef *SPIx, uint16_t data)
{
    // Check if SPI is enabled
    if(!((SPIx)->CR1 & SPI_CR1_SPE))
        return 0xFFFF;

    // Wait for previous transmissions to complete
    while(!SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE))
        ;
    SPI_I2S_SendData(SPIx, data);

    // Wait for reception to complete
    while(!SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE))
        ;
    return SPI_I2S_ReceiveData(SPIx); // read recieived
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
    if(!((SPIx)->CR1 & SPI_CR1_SPE))
        return;

    // Wait for previous transmissions to complete
    while(!SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE))
        ;

    for(i = 0; i < count; i++) {
        // Fill output buffer with data
        SPIx->DR = dataOut[i];

        // Wait for SPI to end everything
        while(((SPIx)->SR & (SPI_SR_TXE | SPI_SR_RXNE)) == 0 || ((SPIx)->SR & SPI_SR_BSY))
            ;

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
    if(!((SPIx)->CR1 & SPI_CR1_SPE))
        return;

    // Wait for previous transmissions to complete
    while(!SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE))
        ;

    for(i = 0; i < count; i++) {
        // Fill output buffer with data
        SPIx->DR = dummy;

        // Wait for SPI to end everything
        while(((SPIx)->SR & (SPI_SR_TXE | SPI_SR_RXNE)) == 0 || ((SPIx)->SR & SPI_SR_BSY))
            ;

        // Save data to buffer
        dataIn[i] = SPIx->DR;
    }
}

/**
  * @brief  Stop SPI
  * @param  SPIx = SPI1 or SPI2
  * @retval None
  */
void LibSPI_Stop(SPI_TypeDef *SPIx)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    // Disable SPI
    SPI_Cmd(SPIx, DISABLE);

    if(SPIx == SPI1) {
        // Stop clocks
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, DISABLE);

        // Configure SCK and MOSI pins as floating inputs
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;

        GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    if(SPIx == SPI2) {
        // Stop clocks
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, DISABLE);

        // Configure SCK and MOSI pins as floating inputs
        GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;

        GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}
