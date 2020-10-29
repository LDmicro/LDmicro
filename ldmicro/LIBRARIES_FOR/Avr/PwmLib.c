#include <avr/io.h>

// Librairie AtMega pour PWM

#include "ladder.h"
#include "PwmLib.h"

// compute prediv coefficient
int PWM_Prediv(long fmcu, long fpwm, int csmax)
{
    long p = (fmcu / 256) / fpwm; // predivision theorique

    if(csmax == 0)
        return (0); // desactivera la pwm

    if(csmax == 5) { // pwm a 5 possibilites de predivision
        if(p >= 1024)
            return (5); // cs (tres) approximatif
        if(p >= 256)
            return (4);
        if(p >= 64)
            return (3);
        if(p >= 8)
            return (2);
        if(p >= 1)
            return (1);
        return (0);
    } else { // csmax = 7
        if(p >= 1024)
            return (7); // cs (tres) approximatif
        if(p >= 256)
            return (6);
        if(p >= 128)
            return (5);
        if(p >= 64)
            return (4);
        if(p >= 32)
            return (3);
        if(p >= 8)
            return (2);
        if(p >= 1)
            return (1);
        return (0);
    }
}

#if defined(LDTARGET_atmega8)
// set fast PWM mode with non-inverted output
void PWM_Init(int canal, long fmcu, long fpwm, int resol, int csmax)
{
    int cs = PWM_Prediv(fmcu, fpwm, csmax);

    if(canal == 0x1A) {
        TCCR1A |= (1 << WGM10) | (1 << WGM11) | (1 << COM1A1);
        TCCR1B = (1 << WGM12) | cs; // 10 bits
        // set OC pin as output
        DDRB |= (1 << 1);

        OCR1A = 0;
    }
    if(canal == 0x1B) {
        TCCR1A |= (1 << WGM10) | (1 << WGM11) | (1 << COM1B1);
        TCCR1B = (1 << WGM12) | cs; // 10 bits
        // set OC pin as output
        DDRB |= (1 << 2);

        OCR1B = 0;
    }
    if(canal == 0x02) {
        TCCR2 = (1 << WGM20) | (1 << WGM21) | (1 << COM21) | cs; // 8 bits
        // set OC pin as output
        DDRB |= (1 << 3);

        OCR2 = 0;
    }
}

// modify duty
void PWM_Set(int canal, unsigned percent, int resol)
{
    unsigned value = 0;
    if(resol == 0) {
        PWM_Stop(canal);
        return;
    }
    if(resol == 8) {
        value = (255 * percent) / 100; // PWM 8 bit
    } else {
        value = (1023 * percent) / 100; // PWM 10 bit
    }

    if(canal == 0x1A)
        OCR1A = value;
    if(canal == 0x1B)
        OCR1B = value;
    if(canal == 0x02)
        OCR2 = value;
}

void PWM_Stop(int canal)
{
    if(canal == 0x1A)
        TCCR1A &= ~(1 << COM1A1);
    if(canal == 0x1B)
        TCCR1A &= ~(1 << COM1B1);
    if(canal == 0x02)
        TCCR2 &= ~(1 << COM21);
}
#endif

#if defined(LDTARGET_atmega16) || defined(LDTARGET_atmega32)
// set fast PWM mode with non-inverted output
void PWM_Init(int canal, long fmcu, long fpwm, int resol, int csmax)
{
    int cs = PWM_Prediv(fmcu, fpwm, csmax);

    if(canal == 0x00) {
        TCCR0 = (1 << WGM00) | (1 << WGM01) | (1 << COM01) | cs; // 8 bits
        // set OC pin as output
        DDRB |= (1 << 3);

        OCR0 = 0;
    }
    if(canal == 0x1A) {
        TCCR1A |= (1 << WGM10) | (1 << WGM11) | (1 << COM1A1);
        TCCR1B = (1 << WGM12) | cs; // 10 bits
        // set OC pin as output
        DDRD |= (1 << 5);

        OCR1A = 0;
    }
    if(canal == 0x1B) {
        TCCR1A |= (1 << WGM10) | (1 << WGM11) | (1 << COM1B1);
        TCCR1B = (1 << WGM12) | cs; // 10 bits
        // set OC pin as output
        DDRD |= (1 << 4);

        OCR1B = 0;
    }
    if(canal == 0x02) {
        TCCR2 = (1 << WGM20) | (1 << WGM21) | (1 << COM21) | cs; // 8 bits
        // set OC pin as output
        DDRD |= (1 << 7);

        OCR2 = 0;
    }
}

// modify duty
void PWM_Set(int canal, unsigned percent, int resol)
{
    unsigned value = 0;
    if(resol == 0) {
        PWM_Stop(canal);
        return;
    }
    if(resol == 8)
        value = (255 * percent) / 100; // PWM 8 bit
    else
        value = (1023 * percent) / 100; // PWM 10 bit

    if(canal == 0x00)
        OCR0 = value;
    if(canal == 0x1A)
        OCR1A = value;
    if(canal == 0x1B)
        OCR1B = value;
    if(canal == 0x02)
        OCR2 = value;
}

void PWM_Stop(int canal)
{
    if(canal == 0x00)
        TCCR0 &= ~(1 << COM01);
    if(canal == 0x1A)
        TCCR1A &= ~(1 << COM1A1);
    if(canal == 0x1B)
        TCCR1A &= ~(1 << COM1B1);
    if(canal == 0x02)
        TCCR2 &= ~(1 << COM21);
}
#endif

#if defined(LDTARGET_atmega64) || defined(LDTARGET_atmega128)
// set fast PWM mode with non-inverted output
void PWM_Init(int canal, long fmcu, long fpwm, int resol, int csmax)
{
    int cs = PWM_Prediv(fmcu, fpwm, csmax);

    if(canal == 0x02) {
        TCCR2 = (1 << WGM20) | (1 << WGM21) | (1 << COM21) | cs; // 8 bits
        // set OC pin as output
        DDRB |= (1 << 7);

        OCR2 = 0;
    }
}

// modify duty
void PWM_Set(int canal, unsigned percent, int resol)
{
    long value = 0;
    if(resol == 0) {
        PWM_Stop(canal);
        return;
    }
    if(resol == 8)
        value = (255 * percent) / 100; // PWM 8 bit
    else
        value = (1023 * percent) / 100; // PWM 10 bit

    if(canal == 0x02)
        OCR2 = value;
}

void PWM_Stop(int canal)
{
    if(canal == 0x02)
        TCCR2 &= ~(1 << COM21);
}

#endif

#if defined(LDTARGET_atmega162)
// set fast PWM mode with non-inverted output
void PWM_Init(int canal, long fmcu, long fpwm, int resol, int csmax)
{
    int cs = PWM_Prediv(fmcu, fpwm, csmax);

    if(canal == 0x02) {
        TCCR2 = (1 << WGM20) | (1 << WGM21) | (1 << COM21) | cs; // 8 bits
        // set OC pin as output
        DDRB |= (1 << 1);

        OCR2 = 0;
    }
}

// modify duty
void PWM_Set(int canal, unsigned percent, int resol)
{
    unsigned value = 0;
    if(resol == 0) {
        PWM_Stop(canal);
        return;
    }
    if(resol == 8)
        value = (255 * percent) / 100; // PWM 8 bit
    else
        value = (1023 * percent) / 100; // PWM 10 bit

    if(canal == 0x02)
        OCR2 = value;
}

void PWM_Stop(int canal)
{
    if(canal == 0x02)
        TCCR2 &= ~(1 << COM21);
}
#endif

#if defined(LDTARGET_atmega48) || defined(LDTARGET_atmega88) || defined(LDTARGET_atmega168) || defined(LDTARGET_atmega328)
// set fast PWM mode with non-inverted output
void PWM_Init(int canal, long fmcu, long fpwm, int resol, int csmax)
{
    int cs = PWM_Prediv(fmcu, fpwm, csmax);

    if(canal == 0x0A) {
        // set fast PWM mode with non-inverted output
        TCCR0A |= (1 << WGM00) | (1 << WGM01) | (1 << COM0A1);
        TCCR0B = cs;
        // set OC pin as output
        DDRD |= (1 << 6);

        OCR0A = 0;
    }
    if(canal == 0x0B) {
        // set fast PWM mode with non-inverted output
        TCCR0A |= (1 << WGM00) | (1 << WGM01) | (1 << COM0B1);
        TCCR0B = cs;
        // set OC pin as output
        DDRD |= (1 << 5);

        OCR0B = 0;
    }
    if(canal == 0x2A) {
        // set fast PWM mode with non-inverted output
        TCCR2A |= (1 << WGM20) | (1 << WGM21) | (1 << COM2A1);
        TCCR2B = cs;
        // set OC pin as output
        DDRB |= (1 << 3);

        OCR2A = 0;
    }
    if(canal == 0x2B) {
        // set fast PWM mode with non-inverted output
        TCCR2A |= (1 << WGM20) | (1 << WGM21) | (1 << COM2B1);
        TCCR2B = cs;
        // set OC pin as output
        DDRD |= (1 << 3);

        OCR2A = 0;
    }
}

// modify duty
void PWM_Set(int canal, unsigned percent, int resol)
{
    unsigned value = 0;
    if(resol == 0) {
        PWM_Stop(canal);
        return;
    }
    if(resol == 8)
        value = (255 * percent) / 100; // PWM 8 bit
    else
        value = (1023 * percent) / 100; // PWM 10 bit

    if(canal == 0x0A)
        OCR0A = value;
    if(canal == 0x0B)
        OCR0B = value;
    if(canal == 0x2A)
        OCR2A = value;
    if(canal == 0x2B)
        OCR2B = value;
}

void PWM_Stop(int canal)
{
    if(canal == 0x0A)
        TCCR0A &= ~(1 << COM0A1);
    if(canal == 0x0B)
        TCCR0A &= ~(1 << COM0B1);
    if(canal == 0x2A)
        TCCR2A &= ~(1 << COM2A1);
    if(canal == 0x2B)
        TCCR2A &= ~(1 << COM2B1);
}
#endif

#if defined(LDTARGET_atmega164) || defined(LDTARGET_atmega324) || defined(LDTARGET_atmega644) || defined(LDTARGET_atmega1284p)
// set fast PWM mode with non-inverted output
void PWM_Init(int canal, long fmcu, long fpwm, int resol, int csmax)
{
    int cs = PWM_Prediv(fmcu, fpwm, csmax);

    if(canal == 0x2A) {
        // set fast PWM mode with non-inverted output
        TCCR2A |= (1 << WGM20) | (1 << WGM21) | (1 << COM2A1);
        TCCR2B = cs;
        // set OC pin as output
        DDRD |= (1 << 7);

        OCR2A = 0;
    }
}

// modify duty
void PWM_Set(int canal, unsigned percent, int resol)
{
    unsigned value = 0;
    if(resol == 0) {
        PWM_Stop(canal);
        return;
    }
    if(resol == 8)
        value = (255 * percent) / 100; // PWM 8 bit
    else
        value = (1023 * percent) / 100; // PWM 10 bit

    if(canal == 0x2A)
        OCR2A = value;
}

void PWM_Stop(int canal)
{
    if(canal == 0x2A)
        TCCR2A &= ~(1 << COM2A1);
}
#endif

#if defined(LDTARGET_atmega2560)
// set fast PWM mode with non-inverted output
void PWM_Init(int canal, long fmcu, long fpwm, int resol, int csmax)
{
    int cs = PWM_Prediv(fmcu, fpwm, csmax);

    if(canal == 0x0A) {
        // set fast PWM mode with non-inverted output
        TCCR0A |= (1 << WGM00) | (1 << WGM01) | (1 << COM0A1);
        TCCR0B = cs;
        // set OC pin as output
        DDRB |= (1 << 7);

        OCR0A = 0;
    }
    if(canal == 0x0B) {
        // set fast PWM mode with non-inverted output
        TCCR0A |= (1 << WGM00) | (1 << WGM01) | (1 << COM0B1);
        TCCR0B = cs;
        // set OC pin as output
        DDRG |= (1 << 5);

        OCR0B = 0;
    }
    if(canal == 0x1A) {
        TCCR1A |= (1 << WGM10) | (1 << WGM11) | (1 << COM1A1);
        TCCR1B = (1 << WGM12) | cs; // 10 bits
        // set OC pin as output
        DDRB |= (1 << 5);

        OCR1A = 0;
    }
    if(canal == 0x1B) {
        TCCR1A |= (1 << WGM10) | (1 << WGM11) | (1 << COM1B1);
        TCCR1B = (1 << WGM12) | cs; // 10 bits
        // set OC pin as output
        DDRB |= (1 << 6);

        OCR1B = 0;
    }
    if(canal == 0x1C) {
        TCCR1A |= (1 << WGM10) | (1 << WGM11) | (1 << COM1C1);
        TCCR1B = (1 << WGM12) | cs; // 10 bits
        // set OC pin as output
        DDRB |= (1 << 7);

        OCR1C = 0;
    }
    if(canal == 0x2A) {
        // set fast PWM mode with non-inverted output
        TCCR2A |= (1 << WGM20) | (1 << WGM21) | (1 << COM2A1);
        TCCR2B = cs;
        // set OC pin as output
        DDRB |= (1 << 4);

        OCR2A = 0;
    }
    if(canal == 0x2B) {
        // set fast PWM mode with non-inverted output
        TCCR2A |= (1 << WGM20) | (1 << WGM21) | (1 << COM2B1);
        TCCR2B = cs;
        // set OC pin as output
        DDRH |= (1 << 6);

        OCR2B = 0;
    }
    if(canal == 0x3A) {
        TCCR3A |= (1 << WGM30) | (1 << WGM31) | (1 << COM3A1);
        TCCR3B = (1 << WGM32) | cs; // 10 bits
        // set OC pin as output
        DDRE |= (1 << 3);

        OCR3A = 0;
    }
    if(canal == 0x3B) {
        TCCR3A |= (1 << WGM30) | (1 << WGM31) | (1 << COM3B1);
        TCCR3B = (1 << WGM32) | cs; // 10 bits
        // set OC pin as output
        DDRE |= (1 << 4);

        OCR3B = 0;
    }
    if(canal == 0x3C) {
        TCCR3A |= (1 << WGM30) | (1 << WGM31) | (1 << COM3C1);
        TCCR3B = (1 << WGM32) | cs; // 10 bits
        // set OC pin as output
        DDRE |= (1 << 5);

        OCR3C = 0;
    }
    if(canal == 0x4A) {
        TCCR4A |= (1 << WGM40) | (1 << WGM41) | (1 << COM4A1);
        TCCR4B = (1 << WGM42) | cs; // 10 bits
        // set OC pin as output
        DDRH |= (1 << 3);

        OCR4A = 0;
    }
    if(canal == 0x4B) {
        TCCR4A |= (1 << WGM40) | (1 << WGM41) | (1 << COM4B1);
        TCCR4B = (1 << WGM42) | cs; // 10 bits
        // set OC pin as output
        DDRH |= (1 << 4);

        OCR4B = 0;
    }
    if(canal == 0x4C) {
        TCCR4A |= (1 << WGM10) | (1 << WGM11) | (1 << COM4C1);
        TCCR4B = (1 << WGM42) | cs; // 10 bits
        // set OC pin as output
        DDRH |= (1 << 5);

        OCR4C = 0;
    }
}

// modify duty
void PWM_Set(int canal, unsigned percent, int resol)
{
    unsigned value = 0;
    if(resol == 0) {
        PWM_Stop(canal);
        return;
    }
    if(resol == 8)
        value = (255 * percent) / 100; // PWM 8 bit
    else
        value = (1023 * percent) / 100; // PWM 10 bit

    if(canal == 0x0A)
        OCR0A = value;
    if(canal == 0x0B)
        OCR0B = value;
    if(canal == 0x1A)
        OCR1A = value;
    if(canal == 0x1B)
        OCR1B = value;
    if(canal == 0x1C)
        OCR1C = value;
    if(canal == 0x2A)
        OCR2A = value;
    if(canal == 0x2B)
        OCR2B = value;
    if(canal == 0x3A)
        OCR3A = value;
    if(canal == 0x3B)
        OCR3B = value;
    if(canal == 0x3C)
        OCR3C = value;
    if(canal == 0x4A)
        OCR4A = value;
    if(canal == 0x4B)
        OCR4B = value;
    if(canal == 0x4C)
        OCR4C = value;
}

void PWM_Stop(int canal)
{
    if(canal == 0x0A)
        TCCR0A &= ~(1 << COM0A1);
    if(canal == 0x0B)
        TCCR0A &= ~(1 << COM0B1);
    if(canal == 0x1A)
        TCCR1A &= ~(1 << COM1A1);
    if(canal == 0x1B)
        TCCR1A &= ~(1 << COM1B1);
    if(canal == 0x1C)
        TCCR1A &= ~(1 << COM1C1);
    if(canal == 0x2A)
        TCCR2A &= ~(1 << COM2A1);
    if(canal == 0x2B)
        TCCR2A &= ~(1 << COM2B1);
    if(canal == 0x3A)
        TCCR3A &= ~(1 << COM3A1);
    if(canal == 0x3B)
        TCCR3A &= ~(1 << COM3B1);
    if(canal == 0x3C)
        TCCR3A &= ~(1 << COM3C1);
    if(canal == 0x4A)
        TCCR4A &= ~(1 << COM4A1);
    if(canal == 0x4B)
        TCCR4A &= ~(1 << COM4B1);
    if(canal == 0x4C)
        TCCR4A &= ~(1 << COM4C1);
}
#endif
