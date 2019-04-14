#include <avr/io.h>


void PWM_Init(int canal, long fmcu, long fpwm, int resol, int csmax);

int PWM_Prediv(long fmcu, long fpwm, int csmax);

void PWM_Set(int canal, unsigned percent, int resol);

void PWM_Stop(int canal);


