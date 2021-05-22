#ifndef __USRLIB_H__
#define __USRLIB_H__

#include <avr\\sleep.h>
#include <avr\\wdt.h>

#define PIN_BIT_STATE(port, pin)  digitalRead(pin_##pin)
#define PORT_BIT_STATE(port, pin) digitalRead(pin_##pin)
#define PORT_BIT_SET(port, pin)   digitalWrite(pin_##pin, LOW)
#define PORT_BIT_CLEAR(port, pin) digitalWrite(pin_##pin, HIGH)

#define delay_us(us)  delayMicroseconds(us)
#define delay_ms(ms)  delay(ms)

#define WDT_Init()    wdt_enable(WDTO_2S)
#define WDT_Restart() wdt_reset()
#define WDT_Disable() wdt_disable()

#endif
