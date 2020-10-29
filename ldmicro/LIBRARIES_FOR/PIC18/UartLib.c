
#include <htc.h>

#include "ladder.h"
#include "UartLib.h"

// Compute baud rate prescaler
unsigned UART_GetPrescaler(long fcpu, long bds)
{
    long p = (fcpu / 4) / bds;

    return (p - 1);
}

// Init UART : 8 bits, 1 stop, no parity
void UART_Init(long bauds)
{
    unsigned prescal = UART_GetPrescaler(_XTAL_FREQ, bauds);

    // #if defined(LDTARGET_pic18f4520)
    TRISC6 = 1; // Enable TX ; will be configured as I/ O by Uart
    TRISC7 = 1; // Enable RX ; will be configured as I/ O by Uart

    BAUDCON = 0;            // Reset BAUDCON register
    BRG16 = 1;              // High rate mode
    SPBRGH = prescal >> 8;  // Set baud rate :
    SPBRG = prescal & 0xFF; // Bds= Fosc / ( 4 * (SPBRGH:SPBRG+1)) in high rate mode

    RCSTA = 0; // Reset RSTA register
    CREN = 1;  // Enable receiver
    SPEN = 1;  // Enable Uart

    TXSTA = 0; // Reset TXTA register
    BRGH = 1;  // High rate mode
    TXEN = 1;  // Enable transmitter
               // #endif
}

// Send a char on UART
void UART_Transmit(unsigned char data)
{
    // Wait for empty transmit buffer

    while(TXIF == 0) // 0 = The USART transmit buffer is full
        ; // Wait till transmitter register empty
    TXREG = data; // Put data into buffer, sends the data
    // Do not wait until the transmitter register empty
}

// Receive on UART
unsigned char UART_Receive(void)
{
    // Wait for data to be received
    while(RCIF == 0)
        ; // Get and return received data from buffer
    return RCREG; // A char has been received
}

unsigned char UART_Transmit_Ready(void)
{
    //return TRMT == 1; // 1 = Transmit Shift Register empty
    return TXIF == 1; // 1 = The USART transmit buffer is empty
}

unsigned char UART_Transmit_Busy(void)
{
    //return TRMT == 0; // 0 = Transmit Shift Register full
    return TXIF == 0; // 0 = The USART transmit buffer is full
}

unsigned char UART_Receive_Avail(void)
{
    return RCIF == 1;
}

void UART_Write(char *string)
{
    while(*string) {
        UART_Transmit(*string);
        string++;
    }
}
