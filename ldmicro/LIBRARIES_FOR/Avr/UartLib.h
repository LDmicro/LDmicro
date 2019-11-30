#ifndef __UARTLIB_H__
#define __UARTLIB_H__

// clang-format off

#ifndef UCSRA
  #define UCSRA UCSR0A
#endif
#ifndef UDRE
  #define UDRE UDRE0
#endif
#ifndef UDR
  #define UDR UDR0
#endif
#ifndef RXC
  #define RXC RXC0
#endif
#ifndef UBRRH
  #define UBRRH UBRR0H
#endif
#ifndef UBRRL
  #define UBRRL UBRR0L
#endif
#ifndef UCSRB
  #define UCSRB UCSR0B
#endif

#ifndef RXEN
  #define RXEN RXEN0
#endif
#ifndef TXEN
  #define TXEN TXEN0
#endif

// clang-format on

void          UART_Init(unsigned int divisor);
void          UART_Transmit(unsigned char data);
unsigned char UART_Receive(void);
unsigned char UART_Transmit_Ready(void);
unsigned char UART_Transmit_Busy(void);
unsigned char UART_Receive_Avail(void);
void          UART_Write(char *string);

#endif
