/**
 * Copyright (C) Tilen Majerle 2014, and JG 2016
 */

#include "lib_uart.h"

void LibUsart1_InitPins(LibUart_PinsPack_t pinspack);
void LibUsart2_InitPins(LibUart_PinsPack_t pinspack);
void LibUsart3_InitPins(LibUart_PinsPack_t pinspack);
void LibUart4_InitPins(LibUart_PinsPack_t pinspack);
void LibUart5_InitPins(LibUart_PinsPack_t pinspack);
void LibUsart6_InitPins(LibUart_PinsPack_t pinspack);

/**
 * @brief  Initialize USART peripheral and corresponding pins
 * @param  USARTx = USART1, USART2, USART3, UART4, UART5 or USART6
 * @param  baudrate = standard baudrate value from 1200 to 115200
 * @param  datalen = USART_WordLength_8b or USART_WordLength_9b
 * @param  parity = USART_Parity_No or USART_Parity_Even or USART_Parity_Odd
 * @param  stops = USART_StopBits_1 or USART_StopBits_2
 * @retval None
 */
void LibUart_Init(USART_TypeDef *USARTx, uint32_t baudrate, uint16_t datalen, uint16_t parity, uint16_t stops)
{
    USART_InitTypeDef USART_InitStruct;

    USART_InitStruct.USART_BaudRate = baudrate;
    USART_InitStruct.USART_HardwareFlowControl = LibUart_HardwareFlowControl_None; // no flow control
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                   // full-duplex TX + RX
    USART_InitStruct.USART_Parity = parity;
    USART_InitStruct.USART_StopBits = stops;
    USART_InitStruct.USART_WordLength = datalen;

    if(USARTx == USART1) {
        LibUsart1_InitPins(LibUart_PinsPack_1);
    }
    if(USARTx == USART2) {
        LibUsart2_InitPins(LibUart_PinsPack_1);
    }
    if(USARTx == USART3) {
        LibUsart3_InitPins(LibUart_PinsPack_1);
    }
    if(USARTx == UART4) {
        LibUart4_InitPins(LibUart_PinsPack_1);
    }
    if(USARTx == UART5) {
        LibUart5_InitPins(LibUart_PinsPack_1);
    }
    if(USARTx == USART6) {
        LibUsart6_InitPins(LibUart_PinsPack_1);
    }

    // Disable and Deinit
    USART_Cmd(USARTx, DISABLE);
    USART_DeInit(USARTx);

    // ReInit and Enable
    USART_Init(USARTx, &USART_InitStruct);
    USART_Cmd(USARTx, ENABLE);
}

/**
 * @brief  Get character from UART (blocking function)
 * @param  USARTx = USART1, USART2, USART3, UART4, UART5 or USART6
 * @retval Received character
 */
uint8_t LibUart_Getc(USART_TypeDef *USARTx)
{
    while((USARTx->SR & USART_SR_RXNE) == 0)
        ;

    return (unsigned char)(USARTx->DR & (uint16_t)0x01FF);
}

/**
 * @brief  Send character via UART
 * @param  USARTx = USART1, USART2, USART3, UART4, UART5 or USART6
 * @param  c = character to be send
 * @retval None
 */
void LibUart_Putc(USART_TypeDef *USARTx, volatile char c)
{
    // Wait to be ready to send
    while(!(USARTx->SR & USART_FLAG_TXE))
        ;
    // Send data
    USARTx->DR = (uint16_t)(c & 0x01FF);
}

/**
 * @brief  Test if Ready to Send character via UART
 * @param  USARTx = USART1, USART2, USART3, UART4, UART5 or USART6
 * @retval True if Ok, false if not
 */
uint8_t LibUart_Transmit_Ready(USART_TypeDef *USARTx)
{
    if(USARTx->SR & USART_FLAG_TXE)
        return 1;
    else
        return 0;
}

/**
 * @brief  Test if character was received via UART
 * @param  USARTx = USART1, USART2, USART3, UART4, UART5 or USART6
 * @retval True if any, false if none
 */
uint8_t LibUart_Received_Data(USART_TypeDef *USARTx)
{
    if((USARTx->SR & USART_SR_RXNE) == 0)
        return 0;
    else
        return 1;
}

/**
 * @brief  Internal functions
 */

void LibUsart1_InitPins(LibUart_PinsPack_t pinspack)
{
    // Enable clock for USART1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // Init pins
    if(pinspack == LibUart_PinsPack_1) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOA, GPIO_Pin_9 | GPIO_Pin_10, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_USART1);
    }
    if(pinspack == LibUart_PinsPack_2) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOB, GPIO_Pin_6 | GPIO_Pin_7, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_USART1);
    }
}

void LibUsart2_InitPins(LibUart_PinsPack_t pinspack)
{
    // Enable clock for USART2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // Init pins
    if(pinspack == LibUart_PinsPack_1) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOA, GPIO_Pin_2 | GPIO_Pin_3, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_USART2);
    }
    if(pinspack == LibUart_PinsPack_2) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOD, GPIO_Pin_5 | GPIO_Pin_6, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_USART2);
    }
}

void LibUsart3_InitPins(LibUart_PinsPack_t pinspack)
{
    // Enable clock for USART3
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    // Init pins
    if(pinspack == LibUart_PinsPack_1) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOB, GPIO_Pin_10 | GPIO_Pin_11, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_USART3);
    }
    if(pinspack == LibUart_PinsPack_2) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOC, GPIO_Pin_10 | GPIO_Pin_11, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_USART3);
    }
    if(pinspack == LibUart_PinsPack_3) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOD, GPIO_Pin_8 | GPIO_Pin_9, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_USART3);
    }
}

void LibUart4_InitPins(LibUart_PinsPack_t pinspack)
{
    // Enable clock for UART4
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

    // Init pins
    if(pinspack == LibUart_PinsPack_1) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOA, GPIO_Pin_0 | GPIO_Pin_1, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_UART4);
    }
    if(pinspack == LibUart_PinsPack_2) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOC, GPIO_Pin_10 | GPIO_Pin_11, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_UART4);
    }
}

void LibUart5_InitPins(LibUart_PinsPack_t pinspack)
{
    // Enable clock for UART5
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART5, ENABLE);

    // Init pins
    if(pinspack == LibUart_PinsPack_1) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); // by JG
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); // by JG
        LibGPIO_InitAlternate(GPIOC, GPIO_Pin_12, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_UART5);
        LibGPIO_InitAlternate(GPIOD, GPIO_Pin_2, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_UART5);
    }
}

void LibUsart6_InitPins(LibUart_PinsPack_t pinspack)
{
    // Enable clocks for USART6
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

    // Init pins
    if(pinspack == LibUart_PinsPack_1) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOC, GPIO_Pin_6 | GPIO_Pin_7, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_USART6);
    }
    if(pinspack == LibUart_PinsPack_2) {
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE); // by JG
        LibGPIO_InitAlternate(
            GPIOG, GPIO_Pin_14 | GPIO_Pin_9, GPIO_OType_PP, GPIO_PuPd_UP, LibGPIO_Speed_High, GPIO_AF_USART6);
    }
}
