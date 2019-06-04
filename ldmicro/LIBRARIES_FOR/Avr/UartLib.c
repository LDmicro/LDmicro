#include "ladder.h"

#include "UartLib.h"

void UART_Init(unsigned int divisor) {
   UBRRH = (divisor >> 8) & 0xFF;
   UBRRL = divisor & 0xFF;
   UCSRB = (1 << RXEN) | (1 << TXEN);
}

void UART_Transmit(unsigned char data) {
  // Wait for empty transmit buffer
  while( !( UCSRA & (1<<UDRE)) );
  // Put data into buffer, sends the data
  UDR = data;
}

unsigned char UART_Receive(void) {
  // Wait for data to be received
  while( !(UCSRA & (1<<RXC)) );
  // Get and return received data from buffer
  return UDR;
}

ldBOOL UART_Transmit_Ready(void) {
  return UCSRA & (1<<UDRE);
}

ldBOOL UART_Transmit_Busy(void) {
  return (UCSRA & (1<<UDRE)) == 0;
}

ldBOOL UART_Receive_Avail(void) {
  return UCSRA & (1<<RXC);
}

