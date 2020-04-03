#include <stdint.h>
#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>

#ifndef EEARL
  #ifdef EEAR
    #define EEARL EEAR
  #else
    #error Check the name of the EEPROM address register!!!
  #endif
#endif

#if !defined(EEWE)
  #if defined(EEPE)
    #define EEWE EEPE
  #else
    #error Check the name of the EEWE/EEPE bit!!!
  #endif
#endif

#if !defined(EEMWE)
  #if defined(EEMPE)
    #define EEMWE EEMPE
  #else
    #error Check the name of the EEMWE/EEMPE bit!!!
  #endif
#endif

unsigned char EEPROM_read(uint16_t address);

void EEPROM_write(uint16_t address, unsigned char data);

unsigned char EEPROM_busy();
