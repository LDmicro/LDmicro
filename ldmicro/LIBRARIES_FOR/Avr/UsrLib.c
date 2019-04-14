#include <avr/io.h>

// Librairie AtMega

#include "ladder.h"
#include "UsrLib.h"

// Read one byte in Eeprom
unsigned char EEPROM_Read(int address)
{
    while(EECR & (1 << EEWE))
        ; // attente fin operation precedente

    EEAR = address;
    EECR |= (1 << EERE); // octet transfere de l'EEPROM dans EEDR

    return EEDR;
}

// Write one byte in Eeprom
void EEPROM_Write(int address, unsigned char byte)
{
    while(EECR & (1 << EEWE))
        ; // attente fin operation precedente

    EEAR = address;
    EEDR = byte;
    EECR |= (1 << EEMWE);
    EECR |= (1 << EEWE);
}

// swap bits from higher to lower
uint16_t swap(uint16_t var)
{
    int      i = 0;
    uint16_t res = 0;

    for(i = 0; i < 16; i++)
        if(var & (1 << i))
            res |= (1 << (15 - i));

    return res;
}

// take the opposite
int16_t opposite(int16_t var)
{
    return (-var);
}

// convert BCD to DEC
uint8_t bcd2bin(uint8_t var)
{
    uint8_t res = 10 * (var / 16) + (var % 16);

    return res;
}

// convert DEC to BCD
uint8_t bin2bcd(uint8_t var)
{
    uint8_t res = 16 * (var / 10) + (var % 10);

    return res;
}

unsigned char u__delay1;
unsigned char u__delay2;
unsigned      u__delay;
unsigned      m__delay;

//
// this function lasts 4 * (u__delay1-1) + 5 cycles ~= 4 * u__delay1 cycles
// u__delay1 must be > 0
//
void us_delay1() // no parameter because takes time to manage from stack => use global variable
{
    __asm__(
        "cli\n" // 1 cycle      // disable interrupts

        "lds r0, u__delay1\n" // 2 cycles     // r0 register is free to use in gccavr
        "loop1:"
        "nop\n"        // 1 cycle
        "dec r0\n"     // 1 cycle
        "brne loop1\n" // 2 cycles or 1 cycle at end

        "sei\n" // 1 cycle      // enable interrupts again
    );
}

//
// this function lasts about 32 * 4 * u__delay2 cycles
// u_delay2 must be > 0
//
void us_delay2() // no parameter because takes time to manage from stack => use global variable
{
    __asm__(
        "nop\n"      // 1 cycle
        "push r16\n" // 1 cycle

        "lds r0, u__delay2\n" // 2 cycles
        "ldi r16, 31\n"       // 1 cycle

        "loop2:"
        "nop\n"        // 1 cycle
        "dec r16\n"    // 1 cycle
        "brne loop2\n" // 2 cycles or 1 cycle at end   }
        "nop\n"        // 0 or 1 cycle                 } = 2 cycles

        "ldi r16, 31\n" // 1 cycle
        "dec r0\n"      // 1 cycle
        "brne loop2\n"  // 2 cycles or 1 cycle at end

        "pop r16\n" // 1 cycle
        "nop\n"     // 1 cycle
    );
}

void delay_ms(unsigned md)
{
    m__delay = md;
    /*
    while (m__delay--)          // works fine !
        {
        delay_us1(100);
        delay_us1(100);
        delay_us1(100);
        delay_us1(100);
        delay_us1(100);
        delay_us1(100);
        delay_us1(100);
        delay_us1(100);
        delay_us1(100);
        delay_us1(100);
        }
*/

    while(m__delay--) // works fine too !
    {
        delay_us2(1000);
    }
}
