#include <htc.h>


#if (defined LDTARGET_pic16f873) 
	#define LDTARGET_pic16f87X
#endif
#if (defined LDTARGET_pic16f874) 
	#define LDTARGET_pic16f87X
#endif
#if (defined LDTARGET_pic16f876) 
	#define LDTARGET_pic16f87X
#endif
#if (defined LDTARGET_pic16f877) 
	#define LDTARGET_pic16f87X
#endif

#if (defined LDTARGET_pic16f882) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f883) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f884) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f886) 
	#define LDTARGET_pic16f88X
#endif
#if (defined LDTARGET_pic16f887) 
	#define LDTARGET_pic16f88X
#endif


int SPI_GetPrescaler(long fcpu, long fspi);
void SPI_Init(char division);
char SPI_Send(char tx);

