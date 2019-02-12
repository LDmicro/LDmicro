#include <avr/io.h>


void ADC_Init(int div, int resol);

void ADC_Stop();

unsigned ADC_Read(int canal);
