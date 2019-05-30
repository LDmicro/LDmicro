
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

#if 0
void delay_us(const unsigned ud);       // ud must be constant
void delay_ms(const unsigned md);       // md must be constant
#endif

#define delay_us(ud)    \
    {   \
    __delay_us(ud);     \
    }


#define delay_ms(md)    \
    {   \
    __delay_ms(md);     \
    }
