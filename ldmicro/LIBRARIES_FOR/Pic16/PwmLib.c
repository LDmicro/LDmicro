#include <htc.h>

#include "ladder.h"
#include "PwmLib.h"

int PWM_Prediv(long fmcu, long fpwm)
{
    long q = (fmcu / 4) / fpwm; // theoriquement egal a (PR2+1) * prediv

    if(q <= 256)
        return (1); // predivision = 1
    if(q <= 1024)
        return (4); // predivision = 4
    if(q <= 4096)
        return (16); // predivision = 16
    return (0);      // predivision hors limites
}

void PWM_Init(int canal, long fmcu, long fpwm, int resol)
{
    long q = (fmcu / 4) / fpwm;
    int  cs = PWM_Prediv(fmcu, fpwm);
    char prescal = 0;
    char pr2 = 0;

    if(cs == 0) // fpwm trop petite
    {
        prescal = 0x02;
        pr2 = 255;
    }
    if(cs == 1) {
        prescal = 0x01;
        if(q > 2)
            pr2 = q - 1;
        else
            pr2 = 1;
    }
    if(cs == 4) {
        prescal = 0x01;
        pr2 = (q / 4) - 1;
    }
    if(cs == 16) {
        prescal = 0x02;
        pr2 = (q / 16) - 1;
    }

    if(canal == 1) {
#if defined(LDTARGET_pic16f88)
        if(CFG_WORD & (1 << 12))  // Bit CCPMX
            TRISBbits.TRISB0 = 0; // PWM output on RB0
        else
            TRISBbits.TRISB3 = 0; // PWM output on RB3
#endif

#if defined(LDTARGET_pic16f628)
        TRISBbits.TRISB3 = 0; // PWM output on RB3
#endif

#if defined(LDTARGET_pic16f819)
        if(CFG_WORD & (1 << 12))  // Bit CCPMX
            TRISBbits.TRISB2 = 0; // PWM output on RB2
        else
            TRISBbits.TRISB3 = 0; // PWM output on RB3
#endif

#if defined(LDTARGET_pic16f876) || defined(LDTARGET_pic16f877) || defined(LDTARGET_pic16f886) \
    || defined(LDTARGET_pic16f887)
        TRISCbits.TRISC2 = 0; // configure RC2 as output(RC2= PWM1 output)
#endif

        CCP1CON = 0x0C; // select PWM mode.
        CCPR1L = 0;     // set duty to 0

        PR2 = pr2; // set PWM period
        T2CON &= ~0x03;
        T2CON |= prescal; // set precaler
        TMR2ON = 1;       // start Timer 2 for PWM generation
    }

#if defined(LDTARGET_pic16f876) || defined(LDTARGET_pic16f877) || defined(LDTARGET_pic16f886) \
    || defined(LDTARGET_pic16f887)
    if(canal == 2) {
        TRISCbits.TRISC1 = 0; // configure RC1 as output(RC1= PWM2 output)

        CCP2CON = 0x0C; // select PWM mode.
        CCPR2L = 0;     // set duty to 0

        PR2 = pr2; // set PWM period
        T2CON &= ~0x03;
        T2CON |= prescal; // set precaler
        TMR2ON = 1;       // start Timer 2 for PWM generation
    }
#endif
}

void PWM_Set(int canal, unsigned percent, int resol)
{
    unsigned value = ((PR2 + 1) * percent) / 25; // 10 bit value

    if(canal == 1) {
        TMR2ON = 0;
        CCPR1L = value >> 2;
        CCP1CON = ((value & 0x0003) << 4) + 0x0C;
        TMR2ON = 1;
    }
#if defined(LDTARGET_pic16f876) || defined(LDTARGET_pic16f877) || defined(LDTARGET_pic16f886) \
    || defined(LDTARGET_pic16f887)
    if(canal == 2) {
        TMR2ON = 0;
        CCPR2L = value >> 2;
        CCP2CON = ((value & 0x0003) << 4) + 0x0C;
        TMR2ON = 1;
    }
#endif
}

void PWM_Stop(int canal)
{
    if(canal == 1) {
        CCP1CON = 0;
        CCPR1L = 0;

        PR2 = 0;
        TMR2ON = 0; // stop Timer 2 and PWM generation
    }
#if defined(LDTARGET_pic16f876) || defined(LDTARGET_pic16f877) || defined(LDTARGET_pic16f886) \
    || defined(LDTARGET_pic16f887)
    if(canal == 2) {
        CCP2CON = 0;
        CCPR2L = 0;

        PR2 = 0;
        TMR2ON = 0; // stop Timer 2 and PWM generation
    }
#endif
}
