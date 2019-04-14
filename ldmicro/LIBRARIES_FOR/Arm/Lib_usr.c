/**
 * Copyright (C) JG 2018
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

// delays in +/- µs
void delay_us(uint16_t us)
{
    uint32_t d = 10 * us / 2;

    for(; d > 0; d--)
        ;
}

// delays in +/- ms
void delay_ms(uint16_t ms)
{
    uint32_t d = 6000 * ms;

    for(; d > 0; d--)
        ;
}
