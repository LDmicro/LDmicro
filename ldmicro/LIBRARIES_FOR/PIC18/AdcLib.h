#include <htc.h>

/*
PIC18F4520 has a built in 10 bit Successive Approximation ADC which is multiplexed among 13 input pins.
The ADC pins can be enabled by configuring the corresponding ADCON1 register.

ADCON0 	Used to Turn ON the ADC, Select the Sampling Freq and also Start the conversion.
ADCON1 	Used to configure the gpio pins for ADC

ADRESH 	Holds the higher byte of ADC result
ADRESL 	Holds the lower byte of ADC result

ADCON0
7 	6 	5 		4 		3 		2 		1 			0
-	-	CHS3	CHS2 	CHS1 	CHS0 	GO/DONE 	ADON

CHS3-CHS0:Analog Channel Select bits
0000 = Channel 0 (AN0)
0001 = Channel 1 (AN1)
0010 = Channel 2 (AN2)
0011 = Channel 3 (AN3)
0100 = Channel 4 (AN4)
0101 = Channel 5 (AN5)
0110 = Channel 6 (AN6)
0111 = Channel 7 (AN7)
1000 = Channel 8 (AN8)
1001 = Channel 9 (AN9)
1010 = Channel 10 (AN10)
1011 = Channel 11 (AN11)
1100 = Channel 12 (AN12)
1101 = -
1110 = -
1111 = -

GO/DONE: A/D Conversion Status bit
When ADON = 1:
1 = A/D conversion in progress (setting this bit starts the A/D conversion which is automatically cleared by hardware when the A/D conversion is complete)
0 = A/D conversion not in progress

ADON: A/D On bit
1 = A/D converter module is powered up
0 = A/D converter module is shut-off and consumes no operating current

ADCON1
7 	6 	5 		4 		3 		2 		1 		0
-	-	VCFG1FM VCFG0 	PCFG3 	PCFG2 	PCFG1 	PCFG0

VCFG1:VCFG0 = voltage reference 
00 = VRef+ is Vcc and VRef- is Gnd

PCFG3:0: Analog pin configuration (see datasheet)
0000 = AN0:AN12 are analog pins
...
1111 = no analog pin

ADCON2
7 		6 	5 		4 		3 		2 		1 		0
ADFM	-	ACQT2	ACQT1	ACQT0	ADCS2	ADCS1	ADCS0

ADFM: A/D Result Format Select bit
1 = Right justified. Six (6) Most Significant bits of ADRESH are read as ‘0’.
0 = Left justified. Six (6) Least Significant bits of ADRESL are read as ‘0’.

ACQT2:ACQT0: A/D Acquisition Time Select bits

ADCS2:ADCS0: A/D Conversion Clock Select bits
111 = FRC (clock derived from A/D RC oscillator)(1)
110 = FOSC/64
101 = FOSC/16
100 = FOSC/4
011 = FRC (clock derived from A/D RC oscillator)(1)
010 = FOSC/32
001 = FOSC/8
000 = FOSC/2
*/


#if (defined LDTARGET_pic18f4520) 
	#define LDTARGET_pic18f4XX0			// For PIC18F4520 or PIC18F4550
#endif

void ADC_Init();
int ADC_Read(int canal, int refs);


