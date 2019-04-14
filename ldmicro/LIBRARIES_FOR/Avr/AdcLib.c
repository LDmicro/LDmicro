#include <avr/io.h>

// Librairie AtMega pour ADC

#include "ladder.h"
#include "PwmLib.h"

// NB:  pas d'ADC sur ATMega 164

#if defined(LDTARGET_atmega8) || defined(LDTARGET_atmega48) || defined(LDTARGET_atmega88)        \
    || defined(LDTARGET_atmega168) || defined(LDTARGET_atmega328) || defined(LDTARGET_atmega16)  \
    || defined(LDTARGET_atmega32) || defined(LDTARGET_atmega64) || defined(LDTARGET_atmega128)   \
    || defined(LDTARGET_atmega164) || defined(LDTARGET_atmega324) || defined(LDTARGET_atmega644) \
    || defined(LDTARGET_atmega1284) || defined(LDTARGET_atmega2560)
void ADC_Init(int div, int resol)
{
    if(resol == 8) // conversion 8 ou 10 bits
        ADMUX = (1 << REFS0) | (1 << ADLAR);
    else
        ADMUX = (1 << REFS0);

    // fréquence d'échantillonage selon fréquence XTAL
    ADCSRA |= (div & 0x07); // division par 2^div
    ADCSRA = (1 << ADEN);   // demarrage ADC

    ADMUX &= 0xE0; // RAZ canaux acquisition 0-7

#if defined(LDTARGET_atmega2560)
    ADCSRB &= ~(1 << 3); // RAZ canaux acquisition 8-15
#endif
}

void ADC_Stop()
{
    ADCSRA = 0;
}

unsigned ADC_Read(int canal)
{
    ADMUX &= 0xE0;           // RAZ canaux acquisition
    ADMUX |= (canal & 0x07); // canal acquisition 0 à 7

#if defined(LDTARGET_atmega2560)
    ADCSRB &= ~(1 << 3);
    ADCSRB |= (canal & (1 << 3)); // canal acquisition 8 a 15
#endif

    ADCSRA |= (1 << ADSC); // acquisition analogique
    while(ADCSRA & (1 << ADSC))
        ; // attente fin conversion

    if(ADMUX & (1 << ADLAR))
        return ADCH; // 8 bits seulement
    else
        return ADC; // 10 bits ADC= ADCL + ADCH
}
#endif
