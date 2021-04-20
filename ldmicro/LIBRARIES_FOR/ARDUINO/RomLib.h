/*
#include <stdint.h>
#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>
*/

#include <EEPROM.h>

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

unsigned char EEPROM_busy();

//#define eeprom_busy_wait() do {} while(!eeprom_is_ready())

#ifdef USE_MACRO
  #define EEPROM_write( addr, x ) EEPROM.write(addr, x)
  #define EEPROM_read( addr ) EEPROM.read(addr)
#else
  void EEPROM_write(int addr, unsigned char data);
  unsigned char EEPROM_read(int addr);
#endif
