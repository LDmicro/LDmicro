#include <avr/io.h>

// Librairie AtMega pour SPI en mode maitre

#include "ladder.h"
#include "SpiLib.h"

// au cas ou SPI non utilise mais librairie compilee
#ifndef SPIPORT
#define SPIPORT PORTB
#define SPIDDR DDRB
#define MOSI 0
#define MISO 0
#define SCK 0
#define SS 0
#endif

// Calcule la predivision a appliquer selon la frequence voulue
int SPI_GetPrescaler(long fcpu, long fspi)
{
    int  i = 1;
    long q = fcpu / fspi;

    for(i = 1; i <= 7; i++) {
        if(q > (1 << i))
            continue;
        break;
    }
    if(i >= 8)
        i = 7;

    return (1 << i);
}

// Initialise le SPI
void SPI_Init(char division)
{
    // MOSI, SCK et SS configures en sorties au niveau bas
    SPIDDR |= (1 << SS) + (1 << MOSI) + (1 << SCK);
    SPIPORT &= ~((1 << SS) + (1 << MOSI) + (1 << SCK));
    switch(division) {
        case 2:
            SPCR = 0;
            SPSR = (1 << SPI2X);
            break;
        case 4:
            SPCR = 0;
            break;
        case 8:
            SPCR = 1;
            SPSR = (1 << SPI2X);
            break;
        case 16:
            SPCR = 1;
            break;
        case 32:
            SPCR = 2;
            SPSR = (1 << SPI2X);
            break;
        case 64:
            SPCR = 2;
            break;
        case 128:
            SPCR = 3;
            break;
    }
    SPCR |= (1 << SPE) | (1 << MSTR); // mode maître
    SPDR = 0;                         // Emission d'initialisation du SPI
}

// Emission (et reception) d'un octet
// Transmission (and reception) of a byte
char SPI_Send(char tx)
{
    // The SPIF bit is cleared by first reading
    // the SPI Status Register with SPIF set,
    // then accessing the SPI Data Register(SPDR).
    char tmp = SPSR; // Ignore the "warning: unused variable 'tmp'" !!!

    // SS n'est pas gere automatiquement en mode maitre
    // SS is not managed automatically in master mode
    SPIPORT &= ~(1 << SS);

    // Ecriture dans SPDR provoque emission sur MOSI
    // Writing in SPDR causes emission on MOSI
    SPDR = tx;

    while(!(SPSR & (1 << SPIF)))
        ; // Attente fin emission / reception
          // Wait for the end of transmission / reception

    SPIPORT |= (1 << SS);

    // Octet recu parallement sur MISO
    // Byte received in parallel on MISO
    return SPDR;
}
