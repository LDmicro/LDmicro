#ifndef __USRLIB_H__
#define __USRLIB_H__

//void setPortDigitalIO();

#ifdef ADC_OFF
   #define setPortDigitalIO() setup_adc_ports(ALL_DIGITAL)
#else
   #define setPortDigitalIO() 
#endif

#endif
