#ifndef __USRLIB_H__
#define __USRLIB_H__

#ifdef ADC_OFF
   #define setPortDigitalIO() setup_adc_ports(ADC_OFF)
#else
   #define setPortDigitalIO()
#endif


#define PORTB_PULL_UPS_ON()  port_b_pullups(true)
#define PORTB_PULL_UPS_OFF() port_b_pullups(false)


#define PORT_BIT_SET(port, pin) output_high(PIN_##port##pin)
#define PORT_BIT_CLEAR(port, pin) output_low(PIN_##port##pin)


#define WDT_Init() setup_wdt(WDT_2304MS)
#define WDT_Restart() wdt_reset()

#endif
