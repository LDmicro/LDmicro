#ifndef __USRLIB_H__
#define __USRLIB_H__

#include <htc.h>

#if defined(LDTARGET_pic16f873) || \
    defined(LDTARGET_pic16f874) || \
    defined(LDTARGET_pic16f876) || \
    defined(LDTARGET_pic16f877)
    #define LDTARGET_pic16f87X
#endif

#if defined(LDTARGET_pic16f882) || \
    defined(LDTARGET_pic16f883) || \
    defined(LDTARGET_pic16f884) || \
    defined(LDTARGET_pic16f886) || \
    defined(LDTARGET_pic16f887)
    #define LDTARGET_pic16f88X
#endif

// standard functions to implement in C

unsigned swap(unsigned var);
int opposite(int var);


unsigned char bcd2bin(unsigned char var);
unsigned char bin2bcd(unsigned char var);

void setPortDigitalIO();

#define delay_us(us)         __delay_us(us) // us must be constant
#define delay_ms(ms)         __delay_ms(ms) // ms must be constant
#define delay_cycles(cycles) _delay(cycles)

#ifndef T0IF
    #define T0IF TMR0IF
#endif

#define PORTB_PULL_UPS_ON()  do { nRBPU = 0; } while(0)
#define PORTB_PULL_UPS_OFF() do { nRBPU = 1; } while(0)

//#define PORT_BIT_SET(port, pin) PORT##port##bits.R##port##pin = 1
// or
#define PORT_BIT_SET(port, pin) R##port##pin = 1

//#define PORT_BIT_CLEAR(port, pin) PORT##port##bits.R##port##pin = 0
// or
#define PORT_BIT_CLEAR(port, pin) R##port##pin = 0

//#define PORT_BIT_STATE(port, pin) PORT##port##bits.R##port##pin
// or
#define PORT_BIT_STATE(port, pin) R##port##pin

#define PIN_BIT_STATE(port, pin) R##port##pin


#define WDT_Init()    WDTCON = 1
#define WDT_Restart() CLRWDT()

#endif
