#include <htc.h>

/*
PIC 16F876 has 2PWM module with a resolution of 10-bits.
The 8-Msb bits are stored in CCPRxL and remaining 2-bits in CCPxCON register.
Below tables shows the PWM module of PIC.

PWM Channel 	Port Pin 	Control Register 	Duty Cycle Register 	Period Register

PWM1 			RC2 		CCP1CON 			CCPR1L 					PR2
PWM2 			RC1 		CCP2CON 			CCPR2L 					PR2

PIC uses TIMER2 for generating the PWM signals. 
Lets relate the above concepts with the PIC registers.
Period:PR2 register is used to specify the PWM Period. The PWM period can be calculated
using the following formula. PWM_Period = [(PR2) + 1] * 4 * TOSC * (TMR2_Prescale_Value)

DutyCycle:CCPR1L and CCP1CON<5:4> are used to specify the DutyCycle.
The CCPRxL contains the eight MSbs and the CCPxCON<5:4> contains the two LSbs.
CCPx1L::CCPxCON<5:4> musn't be > 4*(PR2+1). This max value gives a 100 % duty cycle.
Thus, if PR2 is very small, resolution is ridiculous ! But frequencies are precise.

PWM Signal Generation: Once the PWM is configured and Timer2 is enabled, TMR2 starts
incrementing depending on the prescaler. Once the TMR2 value is equal to dutyCycle(CCPR1L+CCP1CON<5:4>)
the PWM pin will be pulled LOW. The timer still continues to increment till it matches with the period PR2.
After which the PWM pin will be pulled HIGH and TMR2 is reset for next cycle.

CCPxCON 	This register is used to configure the CCP module for Capture/Compare/PWM opertaion.
CCPRxL 		This register holds the 8-Msb bits of PWM, lower 2-bits are part of CCPxCON register.
TMR2 		Free running counter which will be compared with CCPR1L and PR2 for generating the PWM output.

CCPxCON
bit 7 	6 	5 		4 		3 		2 		1 		0
	— 	— 	CCPx1 	CCPx0 	CCPxM3 	CCPxM2 	CCPxM1 	CCPxM0

CCPxM3:CCPxM0: 11xx = PWM mode
*/

void PWM_Init(int canal, long fmcu, long fpwm, int resol);

int PWM_Prediv(long fmcu, long fpwm);

void PWM_Set(int canal, unsigned percent, int resol);

void PWM_Stop(int canal);

