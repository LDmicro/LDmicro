
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

// Receive on UART
// Return (int) -1 if no available char
int UART_Recv()
{
    if(RCIF)
        return RCREG; // A char has been received

    return (-1); // No available char
}

// Send a char on UART
void UART_Send(char ch)
{
    TXREG = ch; // Load char to be transmitted and reset TXIF

    while(TXIF == 0)
        ; // Wait till transmitter register empty
}
