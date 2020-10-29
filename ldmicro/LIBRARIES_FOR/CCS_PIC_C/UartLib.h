#ifndef __UARTLIB_H__
#define __UARTLIB_H__

#include "ladder.h"

void UART_Init(void);
void UART_Transmit(unsigned char data);
unsigned char UART_Receive(void);
BOOLEAN_t UART_Transmit_Ready(void);
BOOLEAN_t UART_Transmit_Busy(void);
BOOLEAN_t UART_Receive_Avail(void);

#endif

/*
void          UART_Init(unsigned int divisor);
void          UART_Transmit(unsigned char data);
unsigned char UART_Receive(void);
unsigned char UART_Transmit_Ready(void);
unsigned char UART_Transmit_Busy(void);
int UART_Receive_Avail(void);
void          UART_Write(char *string);

#ifndef UCSRA
  #define UCSRA UCSR0A
#endif
#ifndef UDRE
  #define UDRE UDRE0
#endif
#ifndef RXC
  #define RXC RXC0
#endif
*/
