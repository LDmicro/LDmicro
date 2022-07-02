#ifndef __USRLIB_H__
#define __USRLIB_H__

#define PORT_BIT_SET(port, pin)   GPIO_SetBits(GPIO##port, GPIO_PIN_##pin)

#define PORT_BIT_CLEAR(port, pin) GPIO_ResetBits(GPIO##port, GPIO_PIN_##pin)

#define PORT_BIT_STATE(port, pin) GPIO_ReadOutputDataBit(GPIO##port, GPIO_PIN_##pin)

#define PIN_BIT_STATE(port, pin)  GPIO_ReadInputDataBit(GPIO##port, GPIO_PIN_##pin)


#define WDT_Init()    //wdt_enable(WDTO_2S)wdt_enable(WDTO_2S)

#define WDT_Restart() //wdt_reset()

#endif
