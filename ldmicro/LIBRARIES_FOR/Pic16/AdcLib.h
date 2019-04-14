#include <htc.h>

/*
PIC16F87X have a built in 10 bit Successive Approximation ADC which is multiplexed among 8 input pins.
The A/D module has high and low-voltage reference input that is software selectable to some combination 
of VDD, VSS, RA2 or RA3. With 5v as the Vref the resolution of Pic16f87X ADC can be determined as below: 
resolution of ADC = Vref/(2^{10}-1) = 5/1023 =0.004887 = 4.887mv

ADC input pins are multiplexed with other GPIO pins on RA0-7.
The ADC pins can be enabled by configuring the corresponding ADCON1 register.

ADCON0 	Used to Turn ON the ADC, Select the Sampling Freq and also Start the conversion.
ADCON1 	Used to configure the gpio pins for ADC

ADRESH 	Holds the higher byte of ADC result
ADRESL 	Holds the lower byte of ADC result

ADCON0
7 		6 		5 		4 		3 		2 			1 	0
ADCS1 	ADCS0 	CHS2 	CHS1 	CHS0 	GO/DONE 	— 	ADON

ADCS2-ADCS0:A/D Conversion Clock Select bits

CHS2-CHS0:Analog Channel Select bits
000 = Channel 0 (AN0)
001 = Channel 1 (AN1)
010 = Channel 2 (AN2)
011 = Channel 3 (AN3)
100 = Channel 4 (AN4)
101 = Channel 5 (AN5)
110 = Channel 6 (AN6)
111 = Channel 7 (AN7)

GO/DONE: A/D Conversion Status bit
When ADON = 1:
1 = A/D conversion in progress (setting this bit starts the A/D conversion which is automatically cleared by hardware when the A/D conversion is complete)
0 = A/D conversion not in progress

ADON: A/D On bit
1 = A/D converter module is powered up
0 = A/D converter module is shut-off and consumes no operating current

ADCON1
7 		6 		5 	4 	3 		2 		1 		0
ADFM 	ADCS2 	— 	— 	PCFG3 	PCFG2 	PCFG1 	PCFG0

ADFM: A/D Result Format Select bit
1 = Right justified. Six (6) Most Significant bits of ADRESH are read as ‘0’.
0 = Left justified. Six (6) Least Significant bits of ADRESL are read as ‘0’.

ADCS2: A/D Conversion Clock Select bit
Check ADCS1:ADCS0 of ADCON0 register. 

PCFG3:0: Analog pin configuration and voltage references (see datasheet)
0000 = all pins of RA are analog pins
0010 = RA0-RA4 are analog pins
0100 = RA0, RA1 and RA3 are analog pins
1001 = RA0-RA5 are analog pins
1110 = RA0 pin is analog pin
*/

#if (defined LDTARGET_pic16f873) 
	#define LDTARGET_pic16f87X
#endif
#if (defined LDTARGET_pic16f874) 
	#define LDTARGET_pic16f87X
#endif
#if (defined LDTARGET_pic16f876) 
	#define LDTARGET_pic16f87X
#endif
#if (defined LDTARGET_pic16f877) 
	#define LDTARGET_pic16f87X
#endif

#if (defined LDTARGET_pic16f882) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f883) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f884) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f886) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f887) 
	#define LDTARGET_pic16f88X
#endif


void ADC_Init();
int ADC_Read(int canal, int refs);


