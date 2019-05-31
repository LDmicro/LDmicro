
#include <avr/io.h>
#include <avr/common.h>

#if !defined(EEWE) && defined(EEPE)
  #define EEWE EEPE
#endif

#if !defined(EEMWE) && defined(EEMPE)
  #define EEMWE EEMPE
#endif

unsigned char EEPROM_read(int address);

void EEPROM_write(int address, unsigned char data);
