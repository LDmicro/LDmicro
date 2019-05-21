/**
 * Copyright (C) Tilen Majerle 2014, and JG 2019
 */

#include "lib_uart.h"

void LibUsart1_InitPins();
void LibUsart2_InitPins();
void LibUsart3_InitPins();

/**
 * @brief  Initialize USART peripheral and corresponding pins
 * @param  USARTx = USART1 to USART3
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
        // Enable clocks for USART1
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        // Init pins
        LibGPIO_Conf(GPIOA, GPIO_PIN_9, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);        // TX
        LibGPIO_Conf(GPIOA, GPIO_PIN_10, GPIO_Mode_IN_FLOATING, GPIO_Speed_50MHz); // RX
    }

    if(USARTx == USART2) {
        // Enable clock for USART2
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        // Init pins
        LibGPIO_Conf(GPIOA, GPIO_PIN_2, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);       // TX
        LibGPIO_Conf(GPIOA, GPIO_PIN_3, GPIO_Mode_IN_FLOATING, GPIO_Speed_50MHz); // RX
    }

    if(USARTx == USART3) {
        // Enable clock for USART3
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        // Init pins
        LibGPIO_Conf(GPIOB, GPIO_PIN_10, GPIO_Mode_AF_PP, GPIO_Speed_50MHz);       // TX
        LibGPIO_Conf(GPIOB, GPIO_PIN_11, GPIO_Mode_IN_FLOATING, GPIO_Speed_50MHz); // RX
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
 * @param  USARTx = USART1 to USART3
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
 * @param  USARTx = USART1 to USART3
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
 * @param  USARTx = USART1 to USART3
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
 * @param  USARTx = USART1 to USART3
 * @retval True if any, false if none
 */
uint8_t LibUart_Received_Data(USART_TypeDef *USARTx)
{
    if((USARTx->SR & USART_SR_RXNE) == 0)
        return 0;
    else
        return 1;
}
