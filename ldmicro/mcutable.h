//-----------------------------------------------------------------------------
// Copyright 2007 Jonathan Westhues
//
// This file is part of LDmicro.
//
// LDmicro is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// LDmicro is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with LDmicro.  If not, see <http://www.gnu.org/licenses/>.
//------
//
// The table of supported MCUs, used to determine where the IOs are, what
// instruction set, what init code, etc.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// AT90USB647( AT90USB646 should also work but I do not have hardware to test ).
McuIoPinInfo AvrAT90USB647_64TQFPIoPinInfo[] = {
    { 'E',  6,  1  },
    { 'E',  7,  2  },

    { 'E',  3,  9 },

    { 'B',  0, 10 },
    { 'B',  1, 11 },
    { 'B',  2, 12 },
    { 'B',  3, 13 },
    { 'B',  4, 14 },
    { 'B',  5, 15 },
    { 'B',  6, 16 },
    { 'B',  7, 17 },

    { 'E',  4, 18 },
    { 'E',  5, 19 },

    { 'D',  0, 25 },
    { 'D',  1, 26 },
    { 'D',  2, 27 },
    { 'D',  3, 28 },
    { 'D',  4, 29 },
    { 'D',  5, 30 },
    { 'D',  6, 31 },
    { 'D',  7, 32 },

    { 'E',  0, 33 },
    { 'E',  1, 34 },

    { 'C',  0, 35 },
    { 'C',  1, 36 },
    { 'C',  2, 37 },
    { 'C',  3, 38 },
    { 'C',  4, 39 },
    { 'C',  5, 40 },
    { 'C',  6, 41 },
    { 'C',  7, 42 },

    { 'E',  2, 43 },

    { 'A',  7, 44 },
    { 'A',  6, 45 },
    { 'A',  5, 46 },
    { 'A',  4, 47 },
    { 'A',  3, 48 },
    { 'A',  2, 49 },
    { 'A',  1, 50 },
    { 'A',  0, 51 },

    { 'F',  7, 54 },
    { 'F',  6, 55 },
    { 'F',  5, 56 },
    { 'F',  4, 57 },
    { 'F',  3, 58 },
    { 'F',  2, 59 },
    { 'F',  1, 60 },
    { 'F',  0, 61 },
};

McuAdcPinInfo AvrAT90USB647_64TQFPAdcPinInfo[] = {
    { 61, 0x00 },
    { 60, 0x01 },
    { 59, 0x02 },
    { 58, 0x03 },
    { 57, 0x04 },
    { 56, 0x05 },
    { 55, 0x06 },
    { 54, 0x07 },
};

//-----------------------------------------------------------------------------
// ATmega2560
McuIoPinInfo AvrAtmega2560_100TQFPIoPinInfo[] = {
    { 'G',  5,  1 },

    { 'E',  0,  2 },
    { 'E',  1,  3 },
    { 'E',  2,  4 },
    { 'E',  3,  5 },
    { 'E',  4,  6 },
    { 'E',  5,  7 },
    { 'E',  6,  8 },
    { 'E',  7,  9 },

    { 'H',  0, 12 },
    { 'H',  1, 13 },
    { 'H',  2, 14 },
    { 'H',  3, 15 },
    { 'H',  4, 16 },
    { 'H',  5, 17 },
    { 'H',  6, 18 },

    { 'B',  0, 19 },
    { 'B',  1, 20 },
    { 'B',  2, 21 },
    { 'B',  3, 22 },
    { 'B',  4, 23 },
    { 'B',  5, 24 },
    { 'B',  6, 25 },
    { 'B',  7, 26 },

    { 'H',  7, 27 },

    { 'G',  3, 28 },
    { 'G',  4, 29 },

    { 'L',  0, 35 },
    { 'L',  1, 36 },
    { 'L',  2, 37 },
    { 'L',  3, 38 },
    { 'L',  4, 39 },
    { 'L',  5, 40 },
    { 'L',  6, 41 },
    { 'L',  7, 42 },

    { 'D',  0, 43 },
    { 'D',  1, 44 },
    { 'D',  2, 45 },
    { 'D',  3, 46 },
    { 'D',  4, 47 },
    { 'D',  5, 48 },
    { 'D',  6, 49 },
    { 'D',  7, 50 },

    { 'G',  0, 51 },
    { 'G',  1, 52 },

    { 'C',  0, 53 },
    { 'C',  1, 54 },
    { 'C',  2, 55 },
    { 'C',  3, 56 },
    { 'C',  4, 57 },
    { 'C',  5, 58 },
    { 'C',  6, 59 },
    { 'C',  7, 60 },

    { 'J',  0, 63 },
    { 'J',  1, 64 },
    { 'J',  2, 65 },
    { 'J',  3, 66 },
    { 'J',  4, 67 },
    { 'J',  5, 68 },
    { 'J',  6, 69 },

    { 'G',  2, 70 },

    { 'A',  7, 71 },
    { 'A',  6, 72 },
    { 'A',  5, 73 },
    { 'A',  4, 74 },
    { 'A',  3, 75 },
    { 'A',  2, 76 },
    { 'A',  1, 77 },
    { 'A',  0, 78 },

    { 'J',  7, 79 },

    { 'K',  7, 82 },
    { 'K',  6, 83 },
    { 'K',  5, 84 },
    { 'K',  4, 85 },
    { 'K',  3, 86 },
    { 'K',  2, 87 },
    { 'K',  1, 88 },
    { 'K',  0, 89 },

    { 'F',  7, 90 },
    { 'F',  6, 91 },
    { 'F',  5, 92 },
    { 'F',  4, 93 },
    { 'F',  3, 94 },
    { 'F',  2, 95 },
    { 'F',  1, 96 },
    { 'F',  0, 97 },

};

McuAdcPinInfo AvrAtmega2560_100TQFPAdcPinInfo[] = {
    { 97, 0x00 },
    { 96, 0x01 },
    { 95, 0x02 },
    { 94, 0x03 },
    { 93, 0x04 },
    { 92, 0x05 },
    { 91, 0x06 },
    { 90, 0x07 },
    { 89, 0x08 },
    { 88, 0x09 },
    { 87, 0x0A },
    { 86, 0x0B },
    { 85, 0x0C },
    { 84, 0x0D },
    { 83, 0x0E },
    { 82, 0x0F },
};

//-----------------------------------------------------------------------------
// ATmega128 or ATmega64

McuIoPinInfo AvrAtmega128_64TQFPIoPinInfo[] = {
    { 'E',  0,  2 },
    { 'E',  1,  3 },
    { 'E',  2,  4 },
    { 'E',  3,  5 },
    { 'E',  4,  6 },
    { 'E',  5,  7 },
    { 'E',  6,  8 },
    { 'E',  7,  9 },

    { 'B',  0, 10 },
    { 'B',  1, 11 },
    { 'B',  2, 12 },
    { 'B',  3, 13 },
    { 'B',  4, 14 },
    { 'B',  5, 15 },
    { 'B',  6, 16 },
    { 'B',  7, 17 },

    { 'G',  3, 18 },
    { 'G',  4, 19 },

    { 'D',  0, 25 },
    { 'D',  1, 26 },
    { 'D',  2, 27 },
    { 'D',  3, 28 },
    { 'D',  4, 29 },
    { 'D',  5, 30 },
    { 'D',  6, 31 },
    { 'D',  7, 32 },

    { 'G',  0, 33 },
    { 'G',  1, 34 },

    { 'C',  0, 35 },
    { 'C',  1, 36 },
    { 'C',  2, 37 },
    { 'C',  3, 38 },
    { 'C',  4, 39 },
    { 'C',  5, 40 },
    { 'C',  6, 41 },
    { 'C',  7, 42 },

    { 'G',  2, 43 },

    { 'A',  7, 44 },
    { 'A',  6, 45 },
    { 'A',  5, 46 },
    { 'A',  4, 47 },
    { 'A',  3, 48 },
    { 'A',  2, 49 },
    { 'A',  1, 50 },
    { 'A',  0, 51 },

    { 'F',  7, 54 },
    { 'F',  6, 55 },
    { 'F',  5, 56 },
    { 'F',  4, 57 },
    { 'F',  3, 58 },
    { 'F',  2, 59 },
    { 'F',  1, 60 },
    { 'F',  0, 61 },
};

McuAdcPinInfo AvrAtmega128_64TQFPAdcPinInfo[] = {
    { 61, 0x00 },
    { 60, 0x01 },
    { 59, 0x02 },
    { 58, 0x03 },
    { 57, 0x04 },
    { 56, 0x05 },
    { 55, 0x06 },
    { 54, 0x07 },
};


//-----------------------------------------------------------------------------
// ATmega162

McuIoPinInfo AvrAtmega162IoPinInfo[] = {
    { 'B',  0,  1 },
    { 'B',  1,  2 },
    { 'B',  2,  3 },
    { 'B',  3,  4 },
    { 'B',  4,  5 },
    { 'B',  5,  6 },
    { 'B',  6,  7 },
    { 'B',  7,  8 },
    { 'D',  0, 10 },
    { 'D',  1, 11 },
    { 'D',  2, 12 },
    { 'D',  3, 13 },
    { 'D',  4, 14 },
    { 'D',  5, 15 },
    { 'D',  6, 16 },
    { 'D',  7, 17 },
    { 'C',  0, 21 },
    { 'C',  1, 22 },
    { 'C',  2, 23 },
    { 'C',  3, 24 },
    { 'C',  4, 25 },
    { 'C',  5, 26 },
    { 'C',  6, 27 },
    { 'C',  7, 28 },
    { 'E',  2, 29 },
    { 'E',  1, 30 },
    { 'E',  0, 31 },
    { 'A',  7, 32 },
    { 'A',  6, 33 },
    { 'A',  5, 34 },
    { 'A',  4, 35 },
    { 'A',  3, 36 },
    { 'A',  2, 37 },
    { 'A',  1, 38 },
    { 'A',  0, 39 },
};


//-----------------------------------------------------------------------------
// ATmega16 or ATmega32

McuIoPinInfo AvrAtmega16or32IoPinInfo[] = {
    { 'B',  0,  1 },
    { 'B',  1,  2 },
    { 'B',  2,  3 },
    { 'B',  3,  4 },
    { 'B',  4,  5 },
    { 'B',  5,  6 },
    { 'B',  6,  7 },
    { 'B',  7,  8 },
    { 'D',  0, 14 },
    { 'D',  1, 15 },
    { 'D',  2, 16 },
    { 'D',  3, 17 },
    { 'D',  4, 18 },
    { 'D',  5, 19 },
    { 'D',  6, 20 },
    { 'D',  7, 21 },
    { 'C',  0, 22 },
    { 'C',  1, 23 },
    { 'C',  2, 24 },
    { 'C',  3, 25 },
    { 'C',  4, 26 },
    { 'C',  5, 27 },
    { 'C',  6, 28 },
    { 'C',  7, 29 },
    { 'A',  7, 33 },
    { 'A',  6, 34 },
    { 'A',  5, 35 },
    { 'A',  4, 36 },
    { 'A',  3, 37 },
    { 'A',  2, 38 },
    { 'A',  1, 39 },
    { 'A',  0, 40 },
};

McuAdcPinInfo AvrAtmega16or32AdcPinInfo[] = {
    { 40, 0x00 },
    { 39, 0x01 },
    { 38, 0x02 },
    { 37, 0x03 },
    { 36, 0x04 },
    { 35, 0x05 },
    { 34, 0x06 },
    { 33, 0x07 },
};


//-----------------------------------------------------------------------------
// ATmega8 PDIP-28

McuIoPinInfo AvrAtmega8IoPinInfo[] = {
    { 'D',  0,  2 },
    { 'D',  1,  3 },
    { 'D',  2,  4 },
    { 'D',  3,  5 },
    { 'D',  4,  6 },
    { 'D',  5, 11 },
    { 'D',  6, 12 },
    { 'D',  7, 13 },
    { 'B',  0, 14 },
    { 'B',  1, 15 },
    { 'B',  2, 16 },
    { 'B',  3, 17 },
    { 'B',  4, 18 },
    { 'B',  5, 19 },
    { 'C',  0, 23 },
    { 'C',  1, 24 },
    { 'C',  2, 25 },
    { 'C',  3, 26 },
    { 'C',  4, 27 },
    { 'C',  5, 28 },
};

McuAdcPinInfo AvrAtmega8AdcPinInfo[] = {
    { 23, 0x00 }, // ADC0
    { 24, 0x01 },
    { 25, 0x02 },
    { 26, 0x03 },
    { 27, 0x04 },
    { 28, 0x05 }, // ADC5
};


//-----------------------------------------------------------------------------
// ATmega8 32 pin packages TQFP/QFN/MLF

McuIoPinInfo AvrAtmega8IoPinInfo32[] = {
    { 'D',  0, 30, "PD0 (RXD)" }, // {char port;  int bit;  int pin;}
    { 'D',  1, 31, "PD1 (TXD)" },
    { 'D',  2, 32, "PD2 (INT0)" },
    { 'D',  3,  1, "PD3 (INT1)" },
    { 'D',  4,  2, "PD4 (XCK / T0)" },
    { 'D',  5,  9, "PD5 (T1)" },
    { 'D',  6, 10, "PD6 (AIN0)" },
    { 'D',  7, 11, "PD7 (AIN1)" },
    { 'B',  0, 12, "PB0 (ICP1)" },
    { 'B',  1, 13, "PB1 (OC1A)" },
    { 'B',  2, 14, "PB2 (OC1B / SS)" },
    { 'B',  3, 15, "PB3 (MOSI / OC2)" },
    { 'B',  4, 16, "PB4 (MISO)" },
    { 'B',  5, 17, "PB5 (SCK)" },
    { 'B',  6,  7, "PB6 (XTAL1 / TOSC1)" },
    { 'B',  7,  8, "PB7 (XTAL2 / TOSC2)" },
    { 'C',  0, 23, "PC0 (ADC0)" },
    { 'C',  1, 24, "PC1 (ADC1)" },
    { 'C',  2, 25, "PC2 (ADC2)" },
    { 'C',  3, 26, "PC3 (ADC3)" },
    { 'C',  4, 27, "PC4 (ADC4 / SDA)" },
    { 'C',  5, 28, "PC5 (ADC5 / SCL)" },
    { 'C',  6, 29, "PC6 (RESET)" },
};

McuAdcPinInfo AvrAtmega8AdcPinInfo32[] = {
    { 23, 0x00 }, // ADC0 {int pin;   BYTE muxRegValue;}
    { 24, 0x01 },
    { 25, 0x02 },
    { 26, 0x03 },
    { 27, 0x04 },
    { 28, 0x05 }, // ADC5
    { 19, 0x06 },
    { 22, 0x07 }, // ADC7
};

//McuExtIntPinInfo
int AvrAtmega8ExtIntPinInfo32[] = {
    32, //INT0
    1,  //INT1
};

//-----------------------------------------------------------------------------
// ATmega164 or ATmega324 or ATmega644 or ATmega1264

McuIoPinInfo AvrAtmega164IoPinInfo[] = {
    { 'B',  0,  1 },
    { 'B',  1,  2 },
    { 'B',  2,  3 },
    { 'B',  3,  4 },
    { 'B',  4,  5 },
    { 'B',  5,  6 },
    { 'B',  6,  7 },
    { 'B',  7,  8 },
    { 'D',  0, 14 },
    { 'D',  1, 15 },
    { 'D',  2, 16 },
    { 'D',  3, 17 },
    { 'D',  4, 18 },
    { 'D',  5, 19 },
    { 'D',  6, 20 },
    { 'D',  7, 21 },
    { 'C',  0, 22 },
    { 'C',  1, 23 },
    { 'C',  2, 24 },
    { 'C',  3, 25 },
    { 'C',  4, 26 },
    { 'C',  5, 27 },
    { 'C',  6, 28 },
    { 'C',  7, 29 },
    { 'A',  7, 33 },
    { 'A',  6, 34 },
    { 'A',  5, 35 },
    { 'A',  4, 36 },
    { 'A',  3, 37 },
    { 'A',  2, 38 },
    { 'A',  1, 39 },
    { 'A',  0, 40 },
};

McuAdcPinInfo AvrAtmega164AdcPinInfo[] = {
    { 40, 0x00 },
    { 39, 0x01 },
    { 38, 0x02 },
    { 37, 0x03 },
    { 36, 0x04 },
    { 35, 0x05 },
    { 34, 0x06 },
    { 33, 0x07 },
};
//-----------------------------------------------------------------------------
// A variety of 18-pin PICs that share the same digital IO assignment.

McuIoPinInfo Pic18PinIoInfo[] = {
    { 'A',  2,  1 },
    { 'A',  3,  2 },
    { 'A',  4,  3 },
    { 'A',  5,  4 },
    { 'B',  0,  6 },
    { 'B',  1,  7 },
    { 'B',  2,  8 },
    { 'B',  3,  9 },
    { 'B',  4, 10 },
    { 'B',  5, 11 },
    { 'B',  6, 12 },
    { 'B',  7, 13 },
    { 'A',  6, 15 },
    { 'A',  7, 16 },
    { 'A',  0, 17 },
    { 'A',  1, 18 },
};

McuAdcPinInfo Pic16f819AdcPinInfo[] = {
    {  1, 0x02 },
    {  2, 0x03 },
    {  3, 0x04 },
    { 17, 0x00 },
    { 18, 0x01 },
};

McuAdcPinInfo Pic16f88AdcPinInfo[] = {
    {  1, 0x02 },
    {  2, 0x03 },
    {  3, 0x04 },
    { 12, 0x05 },
    { 13, 0x06 },
    { 17, 0x00 },
    { 18, 0x01 },
};


//-----------------------------------------------------------------------------
// PIC16F877

McuIoPinInfo Pic16f877IoPinInfo[] = {
    { 'A',  0,  2 },
    { 'A',  1,  3 },
    { 'A',  2,  4 },
    { 'A',  3,  5 },
    { 'A',  4,  6 },
    { 'A',  5,  7 },
    { 'E',  0,  8 },
    { 'E',  1,  9 },
    { 'E',  2, 10 },
    { 'C',  0, 15 },
    { 'C',  1, 16 },
    { 'C',  2, 17 },
    { 'C',  3, 18 },
    { 'D',  0, 19 },
    { 'D',  1, 20 },
    { 'D',  2, 21 },
    { 'D',  3, 22 },
    { 'C',  4, 23 },
    { 'C',  5, 24 },
    { 'C',  6, 25 },
    { 'C',  7, 26 },
    { 'D',  4, 27 },
    { 'D',  5, 28 },
    { 'D',  6, 29 },
    { 'D',  7, 30 },
    { 'B',  0, 33 },
    { 'B',  1, 34 },
    { 'B',  2, 35 },
    { 'B',  3, 36 },
    { 'B',  4, 37 },
    { 'B',  5, 38 },
    { 'B',  6, 39 },
    { 'B',  7, 40 },
};

McuAdcPinInfo Pic16f877AdcPinInfo[] = {
    {  2,   0x00 },
    {  3,   0x01 },
    {  4,   0x02 },
    {  5,   0x03 },
    {  7,   0x04 },
    {  8,   0x05 },
    {  9,   0x06 },
    { 10,   0x07 },
};


//-----------------------------------------------------------------------------
// PIC16F876

McuIoPinInfo Pic16f876IoPinInfo[] = {
    { 'A', 0,  2 },
    { 'A', 1,  3 },
    { 'A', 2,  4 },
    { 'A', 3,  5 },
    { 'A', 4,  6 },
    { 'A', 5,  7 },
    { 'C', 0, 11 },
    { 'C', 1, 12 },
    { 'C', 2, 13 },
    { 'C', 3, 14 },
    { 'C', 4, 15 },
    { 'C', 5, 16 },
    { 'C', 6, 17 },
    { 'C', 7, 18 },
    { 'B', 0, 21 },
    { 'B', 1, 22 },
    { 'B', 2, 23 },
    { 'B', 3, 24 },
    { 'B', 4, 25 },
    { 'B', 5, 26 },
    { 'B', 6, 27 },
    { 'B', 7, 28 },
};

McuAdcPinInfo Pic16f876AdcPinInfo[] = {
    {  2, 0x00 },
    {  3, 0x01 },
    {  4, 0x02 },
    {  5, 0x03 },
    {  7, 0x04 }
};


//-----------------------------------------------------------------------------
// PIC16F887

McuIoPinInfo Pic16f887IoPinInfo[] = {
    { 'A',  0,  2 },
    { 'A',  1,  3 },
    { 'A',  2,  4 },
    { 'A',  3,  5 },
    { 'A',  4,  6 },
    { 'A',  5,  7 },
    { 'E',  0,  8 },
    { 'E',  1,  9 },
    { 'E',  2, 10 },
    { 'C',  0, 15 },
    { 'C',  1, 16 },
    { 'C',  2, 17 },
    { 'C',  3, 18 },
    { 'D',  0, 19 },
    { 'D',  1, 20 },
    { 'D',  2, 21 },
    { 'D',  3, 22 },
    { 'C',  4, 23 },
    { 'C',  5, 24 },
    { 'C',  6, 25 },
    { 'C',  7, 26 },
    { 'D',  4, 27 },
    { 'D',  5, 28 },
    { 'D',  6, 29 },
    { 'D',  7, 30 },
    { 'B',  0, 33 },
    { 'B',  1, 34 },
    { 'B',  2, 35 },
    { 'B',  3, 36 },
    { 'B',  4, 37 },
    { 'B',  5, 38 },
    { 'B',  6, 39 },
    { 'B',  7, 40 },
};

McuAdcPinInfo Pic16f887AdcPinInfo[] = {
    {  2,   0x00 },
    {  3,   0x01 },
    {  4,   0x02 },
    {  5,   0x03 },
    {  7,   0x04 },
    {  8,   0x05 },
    {  9,   0x06 },
    { 10,   0x07 },
    { 33,   0x0c },
    { 34,   0x0a },
    { 35,   0x08 },
    { 36,   0x09 },
    { 37,   0x0b },
    { 38,   0x0d },
};


//-----------------------------------------------------------------------------
// PIC16F886

McuIoPinInfo Pic16f886IoPinInfo[] = {
    { 'A', 0,  2 },
    { 'A', 1,  3 },
    { 'A', 2,  4 },
    { 'A', 3,  5 },
    { 'A', 4,  6 },
    { 'A', 5,  7 },
    { 'C', 0, 11 },
    { 'C', 1, 12 },
    { 'C', 2, 13 },
    { 'C', 3, 14 },
    { 'C', 4, 15 },
    { 'C', 5, 16 },
    { 'C', 6, 17 },
    { 'C', 7, 18 },
    { 'B', 0, 21 },
    { 'B', 1, 22 },
    { 'B', 2, 23 },
    { 'B', 3, 24 },
    { 'B', 4, 25 },
    { 'B', 5, 26 },
    { 'B', 6, 27 },
    { 'B', 7, 28 },
};

McuAdcPinInfo Pic16f886AdcPinInfo[] = {
    {  2, 0x00 },
    {  3, 0x01 },
    {  4, 0x02 },
    {  5, 0x03 },
    {  7, 0x04 },
    { 21, 0x0c },
    { 22, 0x0a },
    { 23, 0x08 },
    { 24, 0x09 },
    { 25, 0x0b },
    { 26, 0x0d },
};

//-----------------------------------------------------------------------------
// Controllino Maxi
McuIoPinInfo ControllinoMaxiIoPinInfo[] = {
	{ 'E',  4,  6, "D0" },
	{ 'E',  5,  7, "D1" },
	{ 'G',  5,  1, "D2" },
	{ 'E',  3,  5, "D3" },
	{ 'H',  3, 15, "D4" },
	{ 'H',  4, 16, "D5" },
	{ 'H',  5, 17, "D6" },
	{ 'H',  6, 18, "D7" },
	{ 'B',  4, 23, "D8" },
	{ 'B',  5, 24, "D9" },
	{ 'B',  6, 25, "D10" },
	{ 'B',  7, 26, "D11" },

	{ 'C',  6, 59, "R9" },
	{ 'C',  7, 60, "R8" },
	{ 'A',  7, 71, "R7" },
	{ 'A',  6, 72, "R6" },
	{ 'A',  5, 73, "R5" },
	{ 'A',  4, 74, "R4" },
	{ 'A',  3, 75, "R3" },
	{ 'A',  2, 76, "R2" },
	{ 'A',  1, 77, "R1" },
	{ 'A',  0, 78, "R0" },

	{ 'K',  1, 88, "A9" },
	{ 'K',  0, 89, "A8" },

	{ 'F',  7, 90, "A7" },
	{ 'F',  6, 91, "A6" },
	{ 'F',  5, 92, "A5" },
	{ 'F',  4, 93, "A4" },
	{ 'F',  3, 94, "A3" },
	{ 'F',  2, 95, "A2" },
	{ 'F',  1, 96, "A1" },
	{ 'F',  0, 97, "A0" },
};

McuAdcPinInfo ControllinoMaxiAdcPinInfo[] = {
	{ 97, 0x00 },
	{ 96, 0x01 },
	{ 95, 0x02 },
	{ 94, 0x03 },
	{ 93, 0x04 },
	{ 92, 0x05 },
	{ 91, 0x06 },
	{ 90, 0x07 },
	{ 89, 0x08 },
	{ 88, 0x09 }
};

#define arraylen(x) (sizeof(x)/sizeof((x)[0]))

McuIoInfo SupportedMcus[] = { // NUM_SUPPORTED_MCUS
    {
        "Atmel AVR ATmega2560 100-TQFP",
        'P',
        { 0x20, 0x23, 0x26, 0x29, 0x2C, 0x2F, 0x32, 0x100, 0x103, 0x106, 0x109 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B, 0x2E, 0x32, 0x34, 0x102, 0x105, 0x108, 0x10B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A, 0x2D, 0x30, 0x33, 0x101, 0x104, 0x107, 0x10A }, // DDRx
        128*1024,
        { { 0x200, 8192 } },
        AvrAtmega2560_100TQFPIoPinInfo,
        arraylen(AvrAtmega2560_100TQFPIoPinInfo),
        AvrAtmega2560_100TQFPAdcPinInfo,
        arraylen(AvrAtmega2560_100TQFPAdcPinInfo),
        1023,
        { 2 , 3 },
        23,
        ISA_AVR1,
        EnhancedCore4M,
        0
    },
    {
        "Atmel AVR AT90USB647 64-TQFP",
        'P',
        { 0x20, 0x23, 0x26, 0x29, 0x2C, 0x2F }, // PINx
        { 0x22, 0x25, 0x28, 0x2B, 0x2E, 0x31 }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A, 0x2D, 0x30 }, // DDRx
        64*1024,
        { { 0x100, 4096 } },
        AvrAT90USB647_64TQFPIoPinInfo,
        arraylen(AvrAT90USB647_64TQFPIoPinInfo),
        AvrAT90USB647_64TQFPAdcPinInfo,
        arraylen(AvrAT90USB647_64TQFPAdcPinInfo),
        1023,
        { 27, 28 },
        17,
        ISA_AVR,
        EnhancedCore128K, //???
        0
    },
    {
        "Atmel AVR ATmega128 64-TQFP",
        'P',
        { 0x39, 0x36, 0x33, 0x30, 0x21, 0x20, 0x63 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32, 0x23, 0x62, 0x65 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31, 0x22, 0x61, 0x64 }, // DDRx
        64*1024,
        { { 0x100, 4096 } },
        AvrAtmega128_64TQFPIoPinInfo,
        arraylen(AvrAtmega128_64TQFPIoPinInfo),
        AvrAtmega128_64TQFPAdcPinInfo,
        arraylen(AvrAtmega128_64TQFPAdcPinInfo),
        1023,
        { 27, 28 },
        17,
        ISA_AVR,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega64 64-TQFP",
        'P',
        { 0x39, 0x36, 0x33, 0x30, 0x21, 0x20, 0x63 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32, 0x23, 0x62, 0x65 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31, 0x22, 0x61, 0x64 }, // DDRx
        32*1024,
        { { 0x100, 4096 } },
        AvrAtmega128_64TQFPIoPinInfo,
        arraylen(AvrAtmega128_64TQFPIoPinInfo),
        AvrAtmega128_64TQFPAdcPinInfo,
        arraylen(AvrAtmega128_64TQFPAdcPinInfo),
        1023,
        { 27, 28 },
        17,
        ISA_AVR,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega162 40-PDIP",
        'P',
        { 0x39, 0x36, 0x33, 0x30, 0x25 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32, 0x27 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31, 0x26 }, // DDRx
        8*1024,
        { { 0x100, 1024 } },
        AvrAtmega162IoPinInfo,
        arraylen(AvrAtmega162IoPinInfo),
        NULL,
        0,
        0,
        { 0, 0 },
        2, // OC2
        ISA_AVR,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega32 40-PDIP",
        'P',
        { 0x39, 0x36, 0x33, 0x30 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31 }, // DDRx
        16*1024,
        { { 0x60, 2048 } },
        AvrAtmega16or32IoPinInfo,
        arraylen(AvrAtmega16or32IoPinInfo),
        AvrAtmega16or32AdcPinInfo,
        arraylen(AvrAtmega16or32AdcPinInfo),
        1023,
        { 14, 15 },
        21, // OC2
        ISA_AVR,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega16 40-PDIP",
        'P',
        { 0x39, 0x36, 0x33, 0x30 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31 }, // DDRx
        8*1024,
        { { 0x60, 1024 } },
        AvrAtmega16or32IoPinInfo,
        arraylen(AvrAtmega16or32IoPinInfo),
        AvrAtmega16or32AdcPinInfo,
        arraylen(AvrAtmega16or32AdcPinInfo),
        1023,
        { 14, 15 },
        21, // OC2
        ISA_AVR,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega48 28-PDIP",
        'P',
        { 0xff, 0x23, 0x26, 0x29 }, // PINx
        { 0xff, 0x25, 0x28, 0x2B }, // PORTx
        { 0xff, 0x24, 0x27, 0x2A }, // DDRx
        2*1024,
        { { 0x100, 512 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR1,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega88 28-PDIP",
        'P',
        { 0xff, 0x23, 0x26, 0x29 }, // PINx
        { 0xff, 0x25, 0x28, 0x2B }, // PORTx
        { 0xff, 0x24, 0x27, 0x2A }, // DDRx
        4*1024,
        { { 0x100, 1024 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR1,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega168 28-PDIP",
        'P',
        { 0xff, 0x23, 0x26, 0x29 }, // PINx
        { 0xff, 0x25, 0x28, 0x2B }, // PORTx
        { 0xff, 0x24, 0x27, 0x2A }, // DDRx
        8*1024,
        { { 0x100, 1024 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR1,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega328 28-PDIP",
        'P',
        { 0xff, 0x23, 0x26, 0x29 }, // PINx
        { 0xff, 0x25, 0x28, 0x2B }, // PORTx
        { 0xff, 0x24, 0x27, 0x2A }, // DDRx
        16*1024,
        { { 0x100, 2048 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR1,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega164 40-PDIP",
        'P',
        { 0x20, 0x23, 0x26, 0x29 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A }, // DDRx
        8*1024,
        { { 0x100, 1024 } },
        AvrAtmega164IoPinInfo,
        arraylen(AvrAtmega164IoPinInfo),
        AvrAtmega164AdcPinInfo,
        arraylen(AvrAtmega164AdcPinInfo),
        1023,
        { 14, 15 },
        21,
        ISA_AVR1,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega324 40-PDIP",
        'P',
        { 0x20, 0x23, 0x26, 0x29 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A }, // DDRx
        16*1024,
        { { 0x100, 2048 } },
        AvrAtmega164IoPinInfo,
        arraylen(AvrAtmega164IoPinInfo),
        AvrAtmega164AdcPinInfo,
        arraylen(AvrAtmega164AdcPinInfo),
        1023,
        { 14, 15 },
        21,
        ISA_AVR1,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega644 40-PDIP",
        'P',
        { 0x20, 0x23, 0x26, 0x29 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A }, // DDRx
        32*1024,
        { { 0x100, 4096 } },
        AvrAtmega164IoPinInfo,
        arraylen(AvrAtmega164IoPinInfo),
        AvrAtmega164AdcPinInfo,
        arraylen(AvrAtmega164AdcPinInfo),
        1023,
        { 14, 15 },
        21,
        ISA_AVR1,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega1284 40-PDIP",
        'P',
        { 0x20, 0x23, 0x26, 0x29 }, // PINx
        { 0x22, 0x25, 0x28, 0x2B }, // PORTx
        { 0x21, 0x24, 0x27, 0x2A }, // DDRx
        64*1024,
        { { 0x100, 16384 } },
        AvrAtmega164IoPinInfo,
        arraylen(AvrAtmega164IoPinInfo),
        AvrAtmega164AdcPinInfo,
        arraylen(AvrAtmega164AdcPinInfo),
        1023,
        { 14, 15 },
        21,
        ISA_AVR1,
        EnhancedCore128K,
        0
    },
    {
        "Atmel AVR ATmega8 32 pin packages", //char            *mcuName;
        'P',                                 //char             portPrefix;
        { 0xff, 0x36, 0x33, 0x30 }, // PINx  //DWORD            inputRegs[MAX_IO_PORTS]; // A is 0, J is 9
        { 0xff, 0x38, 0x35, 0x32 }, // PORTx //DWORD            outputRegs[MAX_IO_PORTS];
        { 0xff, 0x37, 0x34, 0x31 }, // DDRx  //DWORD            dirRegs[MAX_IO_PORTS];
        4*1024,                              //DWORD            flashWords;
        { { 0x60, 1024 } },                  //{DWORD start; int len;} ram[MAX_RAM_SECTIONS];
        AvrAtmega8IoPinInfo32,               //McuIoPinInfo    *pinInfo;
        arraylen(AvrAtmega8IoPinInfo32),     //int              pinCount;
        AvrAtmega8AdcPinInfo32,              //McuAdcPinInfo   *adcInfo;
        arraylen(AvrAtmega8AdcPinInfo32),    //int              adcCount;
        1023,                                //int              adcMax;
        { 30, 31 },                          //{int rxPin; int txPin;} uartNeeds;
        15,// OC2                            //int              pwmNeedsPin;
        ISA_AVR,                             //int              whichIsa;
        EnhancedCore8K,                      //AvrFamily        Family;
        0,                                   //DWORD            configurationWord;
//      { 32, 1 }, // INT0, INT1             //int int0PinA; int int1PinA;} QuadEncodNeeds;
//      AvrAtmega8ExtIntPinInfo32,
//      arraylen(AvrAtmega8ExtIntPinInfo32),
    },
    {
        "Atmel AVR ATmega8 28-PDIP",
        'P',
        { 0xff, 0x36, 0x33, 0x30 }, // PINx     (but there is no xxxA)
        { 0xff, 0x38, 0x35, 0x32 }, // PORTx
        { 0xff, 0x37, 0x34, 0x31 }, // DDRx
        4*1024,
        { { 0x60, 1024 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR,
        EnhancedCore8K,
        0
    },
    {
        "Microchip PIC16F628 18-PDIP or 18-SOIC",
        'R',
        { 0x05, 0x06 }, // PORTx
        { 0x05, 0x06 }, // PORTx
        { 0x85, 0x86 }, // TRISx
        2048,
        { { 0x20, 96 }, { 0xa0, 80 }, { 0x120, 48 } },
        Pic18PinIoInfo,
        arraylen(Pic18PinIoInfo),
        NULL,
        0,
        0,
        { 7, 8 },
        0,
        ISA_PIC16,
        NOT_AVR,
        // code protection off, data code protection off, LVP disabled,
        // BOD reset enabled, RA5/nMCLR is RA5, PWRT enabled, WDT disabled,
        // HS oscillator
        0x3f62
    },
    {
        "Microchip PIC16F88 18-PDIP or 18-SOIC",
        'R',
        { 0x05, 0x06 }, // PORTx
        { 0x05, 0x06 }, // PORTx
        { 0x85, 0x86 }, // TRISx
        4096,
        { { 0x20, 96 }, { 0xa0, 80 }, { 0x120, 48 } },
        Pic18PinIoInfo,
        arraylen(Pic18PinIoInfo),
        Pic16f88AdcPinInfo,
        arraylen(Pic16f88AdcPinInfo),
        1023,
        { 8, 11 },
        0,
        ISA_PIC16,
        NOT_AVR,
            (1 << 13) |         // CP off
            (1 << 12) |         // CCP on RB2 (doesn't matter)
            (1 << 11) |         // ICD disabled
            (3 <<  9) |         // flash write protection off
            (1 <<  8) |         // code protection off
            (0 <<  7) |         // LVP disabled
            (1 <<  6) |         // BOR enabled
            (0 <<  5) |         // RA5/nMCLR is RA5
            (0 <<  4) |         // for osc sel, later
            (0 <<  3) |         // PWRT enabled
            (0 <<  2) |         // WDT disabled
            (2 <<  0),          // HS oscillator
    },
    {
        "Microchip PIC16F819 18-PDIP or 18-SOIC",
        'R',
        { 0x05, 0x06 }, // PORTx
        { 0x05, 0x06 }, // PORTx
        { 0x85, 0x86 }, // TRISx
        2048,
        { { 0x20, 96 } },
        Pic18PinIoInfo,
        arraylen(Pic18PinIoInfo),
        Pic16f819AdcPinInfo,
        arraylen(Pic16f819AdcPinInfo),
        1023,
        { 0, 0 },
        0,
        ISA_PIC16,
        NOT_AVR,
            (1 << 13) | // code protect off
            (1 << 12) | // CCP1 on RB2 (doesn't matter, can't use)
            (1 << 11) | // disable debugger
            (3 <<  9) | // flash protection off
            (1 <<  8) | // data protect off
            (0 <<  7) | // LVP disabled
            (1 <<  6) | // BOR enabled
            (0 <<  5) | // nMCLR/RA5 is RA5
            (0 <<  3) | // PWRTE enabled
            (0 <<  2) | // WDT disabled
            (2 <<  0),  // HS oscillator
    },
    {
        "Microchip PIC16F877 40-PDIP",
        'R',
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0x88, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xa0, 80 }, { 0x110, 96 }, { 0x190, 96 } },
        Pic16f877IoPinInfo,
        arraylen(Pic16f877IoPinInfo),
        Pic16f877AdcPinInfo,
        arraylen(Pic16f877AdcPinInfo),
        1023,
        { 26, 25 },
        16,
        ISA_PIC16,
        NOT_AVR,
        // code protection off, debug off, flash write off, EE code protection
        // off, LVP disabled, BOD enabled, CP off, PWRT enabled, WDT disabled,
        // HS oscillator
        0x3f72
    },
    {
        "Microchip PIC16F876 28-PDIP or 28-SOIC",
        'R',
        { 0x05, 0x06, 0x07 }, // PORTx
        { 0x05, 0x06, 0x07 }, // PORTx
        { 0x85, 0x86, 0x87 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xa0, 80 }, { 0x110, 96 }, { 0x190, 96 } },
        Pic16f876IoPinInfo,
        arraylen(Pic16f876IoPinInfo),
        Pic16f876AdcPinInfo,
        arraylen(Pic16f876AdcPinInfo),
        1023,
        { 18, 17 },
        12,
        ISA_PIC16,
        NOT_AVR,
        // code protection off, debug off, flash write off, EE code protection
        // off, LVP disabled, BOD enabled, CP off, PWRT enabled, WDT disabled,
        // HS oscillator
        0x3f72
    },
    {
        "Microchip PIC16F887 40-PDIP",
        'R',
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0x88, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xa0, 80 }, { 0x120, 80 }, { 0x1a0, 80 } },
        Pic16f887IoPinInfo,
        arraylen(Pic16f887IoPinInfo),
        Pic16f887AdcPinInfo,
        arraylen(Pic16f887AdcPinInfo),
        1023,
        { 26, 25 },
        16,
        ISA_PIC16,
        NOT_AVR,
            (3 << (9+16)) | // flash write protection off
            (0 << (8+16)) | // BOR at 2.1 V
            (1 << 13) |     // ICD disabled
            (0 << 12) |     // LVP disabled
            (0 << 11) |     // fail-safe clock monitor disabled
            (0 << 10) |     // internal/external switchover disabled
            (3 <<  8) |     // brown-out detect enabled
            (1 <<  7) |     // data code protection disabled
            (1 <<  6) |     // code protection disabled
            (1 <<  5) |     // nMCLR enabled
            (0 <<  4) |     // PWRTE enabled
            (0 <<  3) |     // WDTE disabled
            (2 <<  0)       // HS oscillator

    },
    {
        "Microchip PIC16F886 28-PDIP or 28-SOIC",
        'R',
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0x88, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xa0, 80 }, { 0x120, 80 }, { 0x1a0, 80 } },
        Pic16f886IoPinInfo,
        arraylen(Pic16f886IoPinInfo),
        Pic16f886AdcPinInfo,
        arraylen(Pic16f886AdcPinInfo),
        1023,
        { 18, 17 },
        12,
        ISA_PIC16,
        NOT_AVR,
            (3 << (9+16)) | // flash write protection off
            (0 << (8+16)) | // BOR at 2.1 V
            (1 << 13) |     // ICD disabled
            (0 << 12) |     // LVP disabled
            (0 << 11) |     // fail-safe clock monitor disabled
            (0 << 10) |     // internal/external switchover disabled
            (3 <<  8) |     // brown-out detect enabled
            (1 <<  7) |     // data code protection disabled
            (1 <<  6) |     // code protection disabled
            (1 <<  5) |     // nMCLR enabled
            (0 <<  4) |     // PWRTE enabled
            (0 <<  3) |     // WDTE disabled
            (2 <<  0)       // HS oscillator
    },	
	{
		"Controllino Maxi",
		'P',
		{ 0x20, 0x23, 0x26, 0x29, 0x2C, 0x2F, 0x32, 0x100, 0x103, 0x106, 0x109 }, // PINx
		{ 0x22, 0x25, 0x28, 0x2B, 0x2E, 0x32, 0x34, 0x102, 0x105, 0x108, 0x10B }, // PORTx
		{ 0x21, 0x24, 0x27, 0x2A, 0x2D, 0x30, 0x33, 0x101, 0x104, 0x107, 0x10A }, // DDRx
			128 * 1024,
			{ { 0x200, 8192 } },
			ControllinoMaxiIoPinInfo,
			arraylen(ControllinoMaxiIoPinInfo),
			ControllinoMaxiAdcPinInfo,
			arraylen(ControllinoMaxiAdcPinInfo),
			1023,
			{ 2 , 3 },
			23,
			ISA_AVR1,
			EnhancedCore4M,
			0
	},
	{
        "ANSI C Code",
        'x',
        { 0x00 },
        { 0x00 },
        { 0x00 },
        0,
        { { 0x00, 0 } },
        NULL,
        0,
        NULL,
        0,
        0,
        { 0, 0 },
        0,
        ISA_ANSIC,
        NOT_AVR,
        0x00
    },
    {
        "Interpretable Byte Code",
        'x',
        { 0x00 },
        { 0x00 },
        { 0x00 },
        0,
        { { 0x00, 0 } },
        NULL,
        0,
        NULL,
        0,
        0,
        { 0, 0 },
        0,
        ISA_INTERPRETED,
        NOT_AVR,
        0x00
    },
	{
		"Extended Byte Code",
		'x',
		{ 0x00 },
		{ 0x00 },
		{ 0x00 },
			0,
			{ { 0x00, 0 } },
			NULL,
			0,
			NULL,
			0,
			0,
			{ 0, 0 },
			0,
			ISA_XINTERPRETED,
			NOT_AVR,
			0x00
	},
	{
        "Netzer Byte Code",
        'R',
        { 0x00 },
        { 0x00 },
        { 0x00 },
        0,
        { { 0x00, 0 } },
        NULL,
        0,
        NULL,
        0,
        0,
        { 0, 0 },
        0,
        ISA_NETZER,
        NOT_AVR,
        0x00
    }
};

