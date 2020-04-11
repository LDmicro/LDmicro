#include "romLib.h"

//DATA_EEPROM - Returns the number of bytes of data EEPROM
//EEPROM_ADDRESS - Returns the address of the start of EEPROM. 0 if not supported by the device.

unsigned char EEPROM_read(int addr) {
    return read_eeprom(addr);
}

static unsigned char EEPROM_busy_flag = 0;

void EEPROM_write(int addr, unsigned char data) {
    EEPROM_busy_flag = 1;
    write_eeprom(addr, data);
}

// Write complete
#INT_EEPROM
void EEPROM_isr_handler() {
   EEPROM_busy_flag = 0;
}

unsigned char EEPROM_busy() {
    return EEPROM_busy_flag; 
}
    
