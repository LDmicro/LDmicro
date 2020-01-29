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
#if defined(LDTARGET_pic18f4520)
    // The GPIOs that can also be A/D inputs default to being A/D
    // inputs, so turn that around
    ADCON1 = (1 << 7) |   // right-justify A/D result
             (0x0F << 0); // all digital inputs
#endif
}
