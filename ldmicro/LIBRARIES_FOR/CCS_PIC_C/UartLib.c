#include "uartLib.h"

void UART_Init(void) {
  // UART baud rate setup
  // setup_uart(2400);
}

void UART_Transmit(unsigned char data) {
  // Wait for empty transmit buffer
  //while( !TRMT );
  // Put data into buffer, sends the data
  putchar(data);
}

unsigned char UART_Receive(void) {
  // Wait for data to be received
  while( !kbhit() );
  // Get and return received data from buffer
  return getch();
}

BOOLEAN_t UART_Transmit_Ready(void) {
  return 1; // ???
}

BOOLEAN_t UART_Transmit_Busy(void) {
  return 0; // ???
}

BOOLEAN_t UART_Receive_Avail(void) {
  // Check if a character has been received
  return kbhit();
}
