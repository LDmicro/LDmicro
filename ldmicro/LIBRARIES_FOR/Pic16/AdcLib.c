#include <htc.h>

#include "ladder.h"
#include "AdcLib.h"

static char adcs = 0;
static int  chsPos = 0, adcsPos = 0;

#ifndef LDTARGET_pic16f628

// Initialize ADC
void ADC_Init()
{
    adcs = 0; // 2*Tosc
    if(_XTAL_FREQ > 1250000)
        adcs = 1; // 8*Tosc
    if(_XTAL_FREQ > 5000000)
        adcs = 2; // 32*Tosc

#if defined(LDTARGET_pic16f88X)
    chsPos = 2;
    adcsPos = 6;
#endif

#if defined(LDTARGET_pic16f88) || defined(LDTARGET_pic16f819) || defined(LDTARGET_pic16f87X)
    chsPos = 3;
    adcsPos = 6;
#endif
}

// Read ADC Channel
int ADC_Read(int canal, int refs)
{
    canal &= 0x07; // 3 bits only
    refs &= 0x0F;  // 4 bits REFS field defines pin config in ladder

    char adcon1 = ADCON1; // save port config

#if defined(LDTARGET_pic16f819) || defined(LDTARGET_pic16f87X)
    ADCON0 = (adcs << adcsPos) | (canal << chsPos); // set channel + speed
    ADCON1 = (refs << 0);                           // set analog pins
    ADFM = 1;                                       // set right justification
#endif

#if defined(LDTARGET_pic16f88) || defined(LDTARGET_pic16f88X)
    ADCON0 = (adcs << adcsPos) | (canal << chsPos); // set channel + speed
    ADCON1 = ((refs & 0x03) << 4);                  // set analog pins
    ADFM = 1;                                       // set right justification
#endif

#if defined(LDTARGET_pic16f88)
    ANSEL = 0x7F;
#endif

#if defined(LDTARGET_pic16f88X)
    ANSEL = 0xFF;
    ANSELH = 0x3F;
#endif

    ADON = 1; // start ADC

    delay_us(20); // delay before ADC is ready

    GO = 1; // Start ADC conversion
    while(GO_DONE == 1)
        ; // GO_DONE bit will be cleared once conversion is complete

    ADON = 0; // stop ADC

#if defined(LDTARGET_pic16f88)
    ANSEL = 0;
#endif

#if defined(LDTARGET_pic16f88X)
    ANSEL = 0;
    ANSELH = 0;
#endif

    // restore port config (analog / digital)
    ADCON1 = adcon1;

    return ((ADRESH << 8) + ADRESL); // return right justified 10-bit result
}

#endif

void ADC_SetAsDigitalIO()
{
#if defined(LDTARGET_pic16f628) || defined(LDTARGET_pic12f683) || defined(LDTARGET_pic12f675)
    // This is also a nasty special case; the comparators on the
    // PIC16F628 are enabled by default and need to be disabled, or
    // else the PORTA GPIOs don't work.

    // Comment("Comparator Off. Normal port I/O.");
    CMCON = 0x07;
#endif

#if defined(LDTARGET_pic16f87X) || defined(LDTARGET_pic16f819)
    // The GPIOs that can also be A/D inputs default to being A/D
    // inputs, so turn that around
    ADCON1 = (1 << 7) |   // right-justify A/D result
             (7 << 0);    // all digital inputs
#endif

#if defined(LDTARGET_pic16f72)
    ADCON1 = 0x7; // all digital inputs
#endif

//  Comment("Set up the ANSELx registers. 1-analog input, 0-digital I/O.");
#if defined(LDTARGET_pic16f88) || defined(LDTARGET_pic12f683) || defined(LDTARGET_pic12f675)
    ANSEL = 0x00;       // all digital inputs
#endif

#if defined(LDTARGET_pic16f887) || defined(LDTARGET_pic16f88)
    ANSEL = 0x00;  // all digital inputs
    ANSELH = 0x00; // all digital inputs
#endif
}


