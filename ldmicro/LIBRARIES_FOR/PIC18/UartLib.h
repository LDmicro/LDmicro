#ifndef __UARTLIB_H__
#define __UARTLIB_H__

#include <htc.h>

unsigned UART_GetPrescaler(long fcpu, long bds);
void UART_Init(long bauds);
void UART_Transmit(unsigned char data);
unsigned char UART_Receive(void);
unsigned char UART_Transmit_Ready(void);
unsigned char UART_Transmit_Busy(void);
unsigned char UART_Receive_Avail(void);
void          UART_Write(char *string);

#endif