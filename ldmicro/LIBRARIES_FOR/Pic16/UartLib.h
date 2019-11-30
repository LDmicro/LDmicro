#ifndef __UARTLIB_H__
#define __UARTLIB_H__

void          UART_Init(unsigned char divisor, unsigned char brgh);
void          UART_Transmit(unsigned char data);
unsigned char UART_Receive(void);
unsigned char UART_Transmit_Ready(void);
unsigned char UART_Transmit_Busy(void);
unsigned char UART_Receive_Avail(void);
void          UART_Write(char *string);

#endif