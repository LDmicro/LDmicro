#include "ladder.h"

#include "UartLib.h"

void UART_Init(unsigned int divisor)
{
    UBRRH = (divisor >> 8) & 0xFF;
    UBRRL = divisor & 0xFF;
    UCSRB = (1 << RXEN) | (1 << TXEN);
}

void UART_Transmit(unsigned char data)
{
    // Wait for empty transmit buffer
    while(!(UCSRA & (1 << UDRE)))
        ; // Put data into buffer, sends the data
    UDR = data;
}

unsigned char UART_Receive(void)
{
    // Wait for data to be received
    while(!(UCSRA & (1 << RXC)))
        ; // Get and return received data from buffer
    return UDR;
}

unsigned char UART_Transmit_Ready(void)
{
    return UCSRA & (1 << UDRE);
}

unsigned char UART_Transmit_Busy(void)
{
    return (UCSRA & (1 << UDRE)) == 0;
}

unsigned char UART_Receive_Avail(void)
{
    return UCSRA & (1 << RXC);
}

void UART_Write(char *string)
{
    /*
    int i= 0;
    while(string[i] != 0) {
        SPI_Send(string[i]);
        i++;
    }
*/
    while(*string) {
        UART_Transmit(*string);
        string++;
    }
}
