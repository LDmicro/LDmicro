#ifndef __UARTLIB_H__
#define __UARTLIB_H__

#define USART4 UART4
#define USART5 UART5

#define UARTn_TO_UARTx(n) (USART##n)
//TODO: assert(n in [2,3,4,5])

#define UART_Init(UARTn, baudRate) LibUart_Init(UARTn_TO_UARTx(UARTn), baudRate, USART_WordLength_8b, USART_Parity_No, USART_StopBits_1)
/*
#define UART_Transmit(UARTx, data) LibUart_Putc(UARTx, data)

#define UART_Receive(UARTx)

#define UART_Transmit_Ready(UARTx) LibUart_Transmit_Ready(UARTx)

#define UART_Transmit_Busy(void)UARTx) (!LibUart_Transmit_Ready(UARTx))

#define UART_Receive_Avail(UARTx) LibUart_Received_Data(UARTx)

#define UART_Write(UARTx, string) \
    while(*string) { \
        UART_Transmit(UARTx, *string); \
        string++; \
    };
*/
#endif
