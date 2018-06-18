#ifndef __BITS_H
#define __BITS_H

// clang-format off

#define BIT0   0
#define BIT1   1
#define BIT2   2
#define BIT3   3
#define BIT4   4
#define BIT5   5
#define BIT6   6
#define BIT7   7
#define BIT8   8
#define BIT9   9
#define BIT10 10
#define BIT11 11
#define BIT12 12
#define BIT13 13
#define BIT14 14
#define BIT15 15
#define BIT16 16
#define BIT17 17
#define BIT18 18
#define BIT19 19
#define BIT20 20
#define BIT21 21
#define BIT22 22
#define BIT23 23
#define BIT24 24
#define BIT25 25
#define BIT26 26
#define BIT27 27
#define BIT28 28
#define BIT29 29
#define BIT30 30
#define BIT31 31
#define BIT32 32

// clang-format on

//bitNumber to bitMask
#define _BV(bit) (1 << (bit))
#define bit(b) (1UL << (b))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01) // Returns the value of the bit (0 or 1).
#define bitReadMask(value, bit) ((value) & bit(bit)) // Returns the mask value of the bit (0 or 0xXXXX).
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define byte0(w) (lowByte(w))
#define byte1(w) ((uint8_t) ((w) >> 8))
#define byte2(w) ((uint8_t) ((w) >> 16))
#define byte3(w) ((uint8_t) ((w) >> 24))

#endif
