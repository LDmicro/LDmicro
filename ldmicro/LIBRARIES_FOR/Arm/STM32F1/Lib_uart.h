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
    U(S)ARTx    CLK     TX      RX      CTS     RTS

    USART1      PA8     PA9     PA10    PA11    PA12
    USART2      PA4     PA2     PA3     PA0     PA1
    USART3      PB12    PB10    PB11    PB13    PB14
*/

#include "stm32f10x.h"

#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"

#include "lib_gpio.h"


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
