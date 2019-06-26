/**
 * Copyright (C) JG 2019
 */

#include "Lib_usr.h"

// swap bits from higher to lower
uint16_t swap(uint16_t var)
{
    int      i = 0;
    uint16_t res = 0;

    for(i = 0; i < 16; i++)
        if(var & (1 << i))
            res |= (1 << (15 - i));

    return res;
}

// take the opposite
int16_t opposite(int16_t var)
{
    return (-var);
}

// convert BCD to DEC
uint8_t bcd2bin(uint8_t var)
{
    uint8_t res = 10 * (var / 16) + (var % 16);

    return res;
}

// convert DEC to BCD
uint8_t bin2bcd(uint8_t var)
{
    uint8_t res = 16 * (var / 10) + (var % 10);

    return res;
}

//static volatile uint32_t usSysTick = 0;
volatile uint32_t usSysTick = 0;

void SysTick_Handler() {
    usSysTick++;
}

#if 1
// delays in +/- us
void delay_us(uint32_t us)
{
    us += usSysTick;
    while(us != usSysTick)
        ;
}

// delays in +/- ms
void delay_ms(uint32_t ms)
{
    ms = ms * 1000 + usSysTick;
    while(ms != usSysTick)
        ;
}
#else
// delays in +/- us
void delay_us(uint16_t us)
{
    uint32_t d = 7 * us / 2;

    for(; d > 0; d--)
        ;
}

// delays in +/- ms
void delay_ms(uint16_t ms)
{
    uint32_t d = 4000 * ms;

    for(; d > 0; d--)
        ;
}
#endif

void _sbrk()
{
}
