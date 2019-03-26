#include <htc.h>

#include "../../ladder.h"
#include "SpiLib.h"


#ifndef LDTARGET_pic16f628

// Compute division factor
int SPI_GetPrescaler(long fcpu, long fspi)
	{
    int i= 1;
	long q=  fcpu / fspi;

    for (i= 2 ; i <= 6 ; i+=2)
    	{
        if (q > (1 << i)) continue;
        break;
        }
    if (i >= 6) i= 6;

    return (1 << i);
    }

// Master mode SPI Init
void SPI_Init(char division)
	{
// no spi on pic16f628

#if defined(LDTARGET_pic16f87X) || defined (LDTARGET_pic16f88X)
	TRISC3= 0;		// SCK= Ouput
	TRISC4= 1;		// SDI= Input (MISO)
	TRISC5= 0; 		// SDO= Output (MOSI)
#endif

#if defined(LDTARGET_pic16f88) || defined (LDTARGET_pic16f819)
	TRISB4= 0;		// SCK= Ouput
	TRISB1= 1;		// SDI= Input (MISO)
	TRISB2= 0; 		// SDO= Output (MOSI)
#endif

	SSPSTAT= 0;		// reset status register
	CKE= 1;			// transmit on clock rising edges

	// clock division factor 4, 16 or 64
	if (division == 4)
		SSPCON= 0x0;
	else if (division == 16)
		SSPCON= 0x1;
	else
		SSPCON= 0x2;

	SSPEN= 1;					// activate SPI

	}

// Send (and receive) on SPI
char SPI_Send(char tx)
	{
	SSPBUF = tx;

	while (!BF);  	// wait for transmission to complete

	return SSPBUF;
	}

#endif
