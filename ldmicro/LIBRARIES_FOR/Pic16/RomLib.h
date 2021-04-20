#include <htc.h>
#include <stdint.h>

unsigned char EEPROM_read(uint16_t address);

void EEPROM_write(uint16_t address, unsigned char data);

unsigned char EEPROM_busy();

unsigned char EEPROM_error();
