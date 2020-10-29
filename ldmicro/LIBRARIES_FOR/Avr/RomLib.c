// Librairie AtMega pour EEPROM

#include "ladder.h"
#include "RomLib.h" // Fichier header librairie EEPROM

void Set_EEPROM_Address_Register(uint16_t address)
{
    EEARL = address & 0xFF;
    #ifdef EEARH
    EEARH = (address >> 8) & 0xFF;
    #endif
}

// Fonction de lecture d'un octet
unsigned char EEPROM_read(uint16_t address)
{
    while(EECR & (1 << EEWE))
        ; // attendre fin d'écriture précédente
    Set_EEPROM_Address_Register(address);

    EECR |= (1 << EERE); // Data transférée de l'EEPROM dans EEDR

    return EEDR;
}

// Fonction d'écriture d'un octet
void /*__attribute__ ((optimize("Os")))*/ EEPROM_write(uint16_t address, unsigned char data)
{
    while(EECR & (1 << EEWE))
        ; // attendre fin d'écriture précédente
    Set_EEPROM_Address_Register(address);
    EEDR = data;
    char cSREG;
    cSREG = SREG; /* store SREG value */
    cli(); /* disable interrupts during timed sequence */
    /*
    Write a logical one to the EEMWE bit while writing a zero to EEWE in EECR.
    Within four clock cycles after setting EEMWE, write a logical one to EEWE.
    */
#if 0
    EECR |= (1 << EEMWE); /* start EEPROM write */ //  Pas d'IT autorisée entre les 2 instructions
    EECR |= (1 << EEWE); // No interrupts allowed between the 2 instructions
    #warning More than four clock cycles! Data not written to the EEPROM!
#elif 1
//  asm("sbi EECR, EEMWE"); // start EEPROM write
//  asm("sbi EECR, EEWE"); // No interrupts allowed between the 2 instructions.
    // Is anybody know how to write asm() command? Mad syntax.
    __asm__ __volatile__ ("sbi %0, %1": /* no outputs */: "I"(_SFR_IO_ADDR(EECR)), "I"(EEMWE)); // start EEPROM write
    __asm__ __volatile__ ("sbi %0, %1": /* no outputs */: "I"(_SFR_IO_ADDR(EECR)), "I"(EEWE)); // No interrupts allowed between the 2 instructions.
#else
    char cEECR1 = EECR | (1 << EEMWE);
    char cEECR2 = cEECR1 | (1 << EEWE);
    EECR = cEECR1; /* start EEPROM write */ //  Pas d'IT autorisée entre les 2 instructions
    EECR = cEECR2; // No interrupts allowed between the 2 instructions.
    // Success, see disassembled code.
#endif
    SREG = cSREG; /* restore SREG value (I-bit) */
}

unsigned char EEPROM_busy()
{
    return EECR & (1 << EEWE);
}
