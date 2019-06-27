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

#if 1
volatile uint16_t usSysTick = 0;

void SysTick_Handler() {
    usSysTick++;
}

// delays in +/- us
void delay_us(uint16_t us)
{
    //NVIC_DisableIRQ(SysTick_IRQn);
    //__disable_irq();
    us += usSysTick;
    //NVIC_EnableIRQ(SysTick_IRQn);
    //__enable_irq();
    //while(us > usSysTick)
    while(us != usSysTick)
        ;
}

// delays in +/- ms
void delay_ms(uint16_t ms)
{
    for(; ms > 0; ms--)
        delay_us(1000);
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
