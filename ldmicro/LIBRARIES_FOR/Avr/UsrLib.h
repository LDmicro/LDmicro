#ifndef __USRLIB_H__
#define __USRLIB_H__

#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

// standard functions to implement in C

#define F_QUO   ((25 * F_CPU) / 100000)     // 1 MHz -> 250

// At 1MHz, 1 cycle instruction = 1µs

extern unsigned char u__delay1;
extern unsigned char u__delay2;
extern unsigned u__delay;

// user functions

#if 0
void delay_us1(unsigned char ud);   // 0 < ud < 50          higher values may not work properly !
void delay_us2(unsigned ud);        // 50 < ud < 1000       higher values may not work properly !
#endif
void delay_ms(unsigned md);         // 0 < md < 20000       higher values may not work properly !


// internal functions
void us_delay1();
void us_delay2();

// u__delay= (ud * (F_CPU / 100000) / 40) + 1
// 25 * F_CPU * shift division by 1024 ~= 25 * F_CPU * division by 1000 but much faster
// for very short delays (0 < ud < 50)
// ud is an unsigned char
#define delay_us1(ud) { \
    u__delay1= ((ud * F_QUO) >> 10) + 1; \
    us_delay1(); \
}

// u__delay= (ud * (F_CPU / 100000) / 40) + 1
// 25 * F_CPU * shift division by 1024 ~= 25 * F_CPU * division by 1000 but much faster
// for short delays (50 < ud < 1000)
// ud is an unsigned
#define delay_us2(ud) { \
    u__delay= (ud * F_QUO) >> 10; \
    u__delay2= u__delay >> 5; \
    u__delay1= (u__delay & 0x001F) + 1; \
    us_delay2(); \
    us_delay1(); \
}

#define delay_cycles(ticks) __builtin_avr_delay_cycles(ticks)

// for AtMega328
#ifndef EEWE
    #define EEWE    EEPE
#endif
#ifndef EEMWE
    #define EEMWE   EEMPE
#endif

/*
unsigned char EEPROM_Read(int address);
void EEPROM_Write(int address, unsigned char byte);
*/

uint16_t swap(uint16_t var);
int16_t opposite(int16_t var);

uint8_t bcd2bin(uint8_t var);
uint8_t bin2bcd(uint8_t var);

#ifndef TIFR1
    #define TIFR1 TIFR
#endif


//#define PIN_BIT_STATE(port, pin) ((PIN##port & (1 << P##port##pin)) != 0)
#define PIN_BIT_STATE(port, pin) (PIN##port & (1 << P##port##pin))

//#define PORT_BIT_STATE(port, pin) ((PORT##port & (1 << P##port##pin)) != 0)
#define PORT_BIT_STATE(port, pin) (PORT##port & (1 << P##port##pin))

#define PORT_BIT_SET(port, pin) PORT##port |= (1 << P##port##pin)
//#define PORT_BIT_SET(port, pin) do {PORT##port |= (1 << P##port##pin); } while(0)

#define PORT_BIT_CLEAR(port, pin) PORT##port &= ~(1 << P##port##pin)
//#define PORT_BIT_CLEAR(port, pin) do {PORT##port &= ~(1 << P##port##pin); } while(0)


#define WDT_Init() wdt_enable(WDTO_2S)

#define WDT_Restart() wdt_reset()

#endif
