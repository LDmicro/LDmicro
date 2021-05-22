#ifndef __USRLIB_H__
#define __USRLIB_H__

#include <htc.h>

#if defined(LDTARGET_pic18f4520)
    #define LDTARGET_pic18f4XX0
#endif

// standard functions to implement in C

unsigned swap(unsigned var);
int opposite(int var);


unsigned char bcd2bin(unsigned char var);
unsigned char bin2bcd(unsigned char var);

void setPortDigitalIO();

#if 0
void delay_us(const unsigned ud);       // ud must be constant
void delay_ms(const unsigned md);       // md must be constant
#endif

#define delay_us(ud) __delay_us(ud)
#define delay_ms(md) __delay_ms(md)


#define PORTB_PULL_UPS_ON()  do { nRBPU = 0; } while(0)
#define PORTB_PULL_UPS_OFF() do { nRBPU = 1; } while(0)

#define PORT_BIT_SET(port, pin) PORT##port##bits.R##port##pin = 1
// or
//#define PORT_BIT_SET(port, pin) R##port##pin = 1 // deprecated warning

#define PORT_BIT_CLEAR(port, pin) PORT##port##bits.R##port##pin = 0
// or
//#define PORT_BIT_CLEAR(port, pin) R##port##pin = 0 // deprecated warning

#define PORT_BIT_STATE(port, pin) PORT##port##bits.R##port##pin
// or
//#define PORT_BIT_STATE(port, pin) R##port##pin // deprecated warning

#define PIN_BIT_STATE(port, pin) PORT##port##bits.R##port##pin


#define WDT_Init()    WDTCON = 1
#define WDT_Restart() CLRWDT()

#endif
