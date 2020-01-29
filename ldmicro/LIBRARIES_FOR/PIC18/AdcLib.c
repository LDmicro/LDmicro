#include <htc.h>

#include "ladder.h"
#include "AdcLib.h"

static char adcs = 0;
static int  chsPos = 0, adcsPos = 0;

// Initialize ADC
void ADC_Init()
{
#if defined(LDTARGET_pic18f4XX0)
    adcs = 0; // Fosc / 2
    if(_XTAL_FREQ > 2000000)
        adcs = 4; // Fosc / 4
    if(_XTAL_FREQ > 5000000)
        adcs = 1; // Fosc / 8
    if(_XTAL_FREQ > 10000000)
        adcs = 5; // Fosc / 16
    if(_XTAL_FREQ > 20000000)
        adcs = 2; // Fosc / 32
    if(_XTAL_FREQ > 30000000)
        adcs = 6; // Fosc / 64

    chsPos = 2;
    adcsPos = 0;
#endif
}

// Read ADC Channel
int ADC_Read(int canal, int refs)
{
    canal &= 0x0F; // 4 bits only
    refs &= 0x0F;  // 4 bits REFS field defines pin config in ladder

    char adcon1 = ADCON1; // save port config
    char adcon2 = ADCON2; // save port config

#if defined(LDTARGET_pic18f4XX0)
    ADCON0 = (canal << chsPos); // set channel
    ADCON1 = (refs << 0);       // set analog pins
    ADCON2 = (adcs << adcsPos); // set speed
    ADFM = 1;                   // set right justification
#endif

    ADON = 1; // start ADC

    delay_us(20); // delay before ADC is ready

    GO = 1; // Start ADC conversion
    while(GO_DONE == 1)
        ; // GO_DONE bit will be cleared once conversion is complete

    ADON = 0; // stop ADC

    // restore port config (analog / digital)
    ADCON1 = adcon1;
    ADCON2 = adcon2;

    return ((ADRESH << 8) + ADRESL); // return right justified 10-bit result
}
