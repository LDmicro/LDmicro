
#include <htc.h>

// standard functions to implement in C

unsigned swap(unsigned var);
int opposite(int var);


unsigned char bcd2bin(unsigned char var);
unsigned char bin2bcd(unsigned char var);

#if 0
void delay_us(const unsigned ud);     	// ud must be constant
void delay_ms(const unsigned md);		// md must be constant
#endif

#define delay_us(ud) 	\
	{	\
    __delay_us(ud);		\
    }


#define delay_ms(md)	\
	{	\
    __delay_ms(md);		\
    }

