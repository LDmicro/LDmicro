#include <stdint.h>
#include <avr/io.h>
#include <avr/common.h>
#include <avr/interrupt.h>

#if !defined(EEWE) && defined(EEPE)
  #define EEWE EEPE
#endif

#if !defined(EEMWE) && defined(EEMPE)
  #define EEMWE EEMPE
#endif

unsigned char EEPROM_read(uint16_t address);

void EEPROM_write(uint16_t address, unsigned char data);

unsigned char EEPROM_busy();
