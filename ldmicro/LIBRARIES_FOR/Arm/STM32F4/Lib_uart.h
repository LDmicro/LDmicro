/**
 * UART Library for STM32F4 without interrupts
 */

#ifndef LIBUART_H
#define LIBUART_H 250

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

/*
                PINSPACK 1      PINSPACK 2      PINSPACK 3
    U(S)ARTX    TX     RX       TX     RX       TX     RX

    USART1      PA9    PA10     PB6    PB7      -      -
    USART2      PA2    PA3      PD5    PD6      -      -
    USART3      PB10   PB11     PC10   PC11     PD8    PD9
    UART4       PA0    PA1      PC10   PC11     -      -
    UART5       PC12   PD2      -      -        -      -
    USART6       PC6    PC7     PG14   PG9      -      -
*/

#include "stm32f4xx.h"

#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"

#include "lib_gpio.h"


typedef enum
    {
	LibUart_PinsPack_1,
	LibUart_PinsPack_2,
	LibUart_PinsPack_3,
    } LibUart_PinsPack_t;

typedef enum
    {
	LibUart_HardwareFlowControl_None = 0x0000,     // No flow control
	LibUart_HardwareFlowControl_RTS = 0x0100,      // RTS flow control
	LibUart_HardwareFlowControl_CTS = 0x0200,      // CTS flow control
	LibUart_HardwareFlowControl_RTS_CTS = 0x0300   // RTS and CTS flow controls
    } LibUart_HardwareFlowControl_t;


void LibUart_Init(USART_TypeDef* USARTx, uint32_t baudrate, uint16_t datalen, uint16_t parity, uint16_t stops);

uint8_t LibUart_Getc(USART_TypeDef* USARTx);
void LibUart_Putc(USART_TypeDef* USARTx, volatile char c);

uint8_t LibUart_Transmit_Ready(USART_TypeDef* USARTx);
uint8_t LibUart_Received_Data(USART_TypeDef* USARTx);


/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
