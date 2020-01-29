
#include <htc.h>

unsigned UART_GetPrescaler(long fcpu, long bds);
void UART_Init(long bauds);
	
int UART_Recv();
void UART_Send(char ch);

	 
	 
	 