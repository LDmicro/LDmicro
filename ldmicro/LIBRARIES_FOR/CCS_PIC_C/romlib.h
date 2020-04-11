#ifndef __ROMLIB_H__
#define __ROMLIB_H__

#include "ladder.h"

void EEPROM_write(unsigned char addr, unsigned char data);

unsigned char EEPROM_read(unsigned char addr);

unsigned char EEPROM_busy();

#endif

