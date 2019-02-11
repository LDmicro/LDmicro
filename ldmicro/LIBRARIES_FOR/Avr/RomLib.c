#include <avr/io.h>

// Librairie AtMega pour EEPROM

#include "../ladder.h"
#include "RomLib.h"			 // Fichier header librairie EEPROM


// Fonction de lecture d'un octet
unsigned char EEPROM_read(int address)
     {
     while (EECR & (1<<EEWE));     // attendre fin d'écriture précédente
     EEAR = address;
     EECR |= (1<<EERE);            // Data transférée de l'EEPROM dans EEDR

     return EEDR;
     }

// Fonction d'écriture d'un octet
void EEPROM_write(int address, unsigned char data)
     {
     while (EECR & (1<<EEWE));     // attendre fin d'écriture précédente
     EEAR = address;
     EEDR = data;
     EECR |= (1<<EEMWE);           // Pas d'IT autorisée entre les 2
     EECR |= (1<<EEWE) ;           // instructions
     }



