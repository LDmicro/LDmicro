#include <htc.h>

// Librairie AtMega

#include "ladder.h"
#include "UsrLib.h"

// swap bits from higher to lower
unsigned swap(unsigned var)
{
    int      i = 0;
    unsigned res = 0;

    for(i = 0; i < 16; i++)
        if(var & (1 << i))
            res |= (1 << (15 - i));

    return res;
}

// take the opposite
int opposite(int var)
{
    return (-var);
}

// convert BCD to DEC
unsigned char bcd2bin(unsigned char var)
{
    unsigned char res = 10 * (var / 16) + (var % 16);

    return res;
}

// convert DEC to BCD
unsigned char bin2bcd(unsigned char var)
{
    unsigned char res = 16 * (var / 10) + (var % 10);

    return res;
}

// set PORTA as digital IO instead of analogical by default
void setPortDigitalIO()
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
    ADCON1 = (1 << 7) | // right-justify A/D result
             (7 << 0);  // all digital inputs
#endif

#if defined(LDTARGET_pic16f72)
    ADCON1 = 0x7; // all digital inputs
#endif

//  Comment("Set up the ANSELx registers. 1-analog input, 0-digital I/O.");
#if defined(LDTARGET_pic16f88) || defined(LDTARGET_pic12f683) || defined(LDTARGET_pic12f675)
    ANSEL = 0x00; // all digital inputs
#endif

#if defined(LDTARGET_pic16f887) || defined(LDTARGET_pic16f88)
    ANSEL = 0x00;  // all digital inputs
    ANSELH = 0x00; // all digital inputs
#endif
}
