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
#ifndef __MCUTABLE_H__
#define __MCUTABLE_H__

#include "stdafx.h"

#include "ldconfig.h"
#include "mcutable.hpp"

// clang-format off

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
//   port bit pin  pinName          ArduinoPin  ArduinoName
//                                              Mega/Mega2560
    { 'E',  0,  2, "PE0 (RXD0/PCINT8)",      0, "0; //RX" },
    { 'E',  1,  3, "PE1 (TXD0)",             0, "1; //TX" },
    { 'E',  2,  4, "PE2 (AIN0/XCK0)",        0, "" },
    { 'E',  3,  5, "PE3 (AIN1/OC3A)",        0, "5; //PWM~" },
    { 'E',  4,  6, "PE4 (INT4/OC3B)",        0, "2; //PWM~" },
    { 'E',  5,  7, "PE5 (INT5/OC3C)",        0, "3; //PWM~" },
    { 'E',  6,  8, "PE6 (INT6/T3)",          0, "" },
    { 'E',  7,  9, "PE7 (INT7/CLKO/ICP3)",   0, "" },

    { 'H',  0, 12, "PH0 (RXD2)"            , 0, "17; //RX2" },
    { 'H',  1, 13, "PH1 (TXD2)"            , 0, "16; //TX2" },
    { 'H',  2, 14, "PH2 (XCK2)"            , 0, "" },
    { 'H',  3, 15, "PH3 (OC4A)"            , 0, "6; //PWM~" },
    { 'H',  4, 16, "PH4 (OC4B)"            , 0, "7; //PWM~" },
    { 'H',  5, 17, "PH5 (OC4C)"            , 0, "8; //PWM~" },
    { 'H',  6, 18, "PH6 (OC2B)"            , 0, "9; //PWM~" },
    { 'H',  7, 27, "PH7 (T4)"              , 0, "" },

    { 'B',  0, 19, "PB0 (PCINT0/_SS)"      , 0, "53" },
    { 'B',  1, 20, "PB1 (PCINT1/SCK)"      , 0, "52" },
    { 'B',  2, 21, "PB2 (PCINT2/MOSI)"     , 0, "51" },
    { 'B',  3, 22, "PB3 (PCINT3/MISO)"     , 0, "50" },
    { 'B',  4, 23, "PB4 (PCINT4/OC2A)"     , 0, "10; //PWM~" },
    { 'B',  5, 24, "PB5 (PCINT5/OC1A)"     , 0, "11; //PWM~" },
    { 'B',  6, 25, "PB6 (PCINT6/OC1B)"     , 0, "12; //PWM~" },
    { 'B',  7, 26, "PB7 (PCINT7/OC1C/OC0A)", 0, "13; //PWM~" },

    { 'L',  0, 35, "PL0 (ICP4)"            , 0, "49" },
    { 'L',  1, 36, "PL1 (ICP5)"            , 0, "48" },
    { 'L',  2, 37, "PL2 (T5)"              , 0, "47" },
    { 'L',  3, 38, "PL3 (OC5A)"            , 0, "46" },
    { 'L',  4, 39, "PL4 (OC5B)"            , 0, "45" },
    { 'L',  5, 40, "PL5 (OC5C)"            , 0, "44" },
    { 'L',  6, 41, "PL6"                   , 0, "43" },
    { 'L',  7, 42, "PL7"                   , 0, "42" },

    { 'D',  0, 43, "PD0 (INT0/SCL)",         0, "21; //SCL" },
    { 'D',  1, 44, "PD1 (INT1/SDA)",         0, "20; //SDA" },
    { 'D',  2, 45, "PD2 (INT2/RXD1)",        0, "19; //RX1" },
    { 'D',  3, 46, "PD3 (INT3/TXD1)",        0, "18; //TX1" },
    { 'D',  4, 47, "PD4 (ICP1)",             0, "" },
    { 'D',  5, 48, "PD5 (XCK1)",             0, "" },
    { 'D',  6, 49, "PD6 (T1)",               0, "" },
    { 'D',  7, 50, "PD7 (T0)",               0, "38" },

    { 'G',  0, 51, "PG0 (_WR)"             , 0, "41" },
    { 'G',  1, 52, "PG1 (_RD)"             , 0, "40" },
    { 'G',  2, 70, "PG2 (ALE)"             , 0, "39" },
    { 'G',  3, 28, "PG3 (TOSC2)"           , 0, "" },
    { 'G',  4, 29, "PG4 (TOSC1)"           , 0, "" },
    { 'G',  5,  1, "PG5 (OC0B)"            , 0, "4; //PWM~" },

    { 'C',  0, 53, "PC0 (A8)",               0, "37" },
    { 'C',  1, 54, "PC1 (A9)",               0, "36" },
    { 'C',  2, 55, "PC2 (A10)",              0, "35" },
    { 'C',  3, 56, "PC3 (A11)",              0, "34" },
    { 'C',  4, 57, "PC4 (A12)",              0, "33" },
    { 'C',  5, 58, "PC5 (A13)",              0, "32" },
    { 'C',  6, 59, "PC6 (A14)",              0, "31" },
    { 'C',  7, 60, "PC7 (A15)",              0, "30" },

    { 'J',  0, 63, "PJ0 (PCINT9/RXD3)"     , 0, "15; //RX3" },
    { 'J',  1, 64, "PJ1 (PCINT10/TXD3)"    , 0, "14; //TX3" },
    { 'J',  2, 65, "PJ2 (PCINT11/XCK3)"    , 0, "" },
    { 'J',  3, 66, "PJ3 (PCINT12)"         , 0, "" },
    { 'J',  4, 67, "PJ4 (PCINT13)"         , 0, "" },
    { 'J',  5, 68, "PJ5 (PCINT14)"         , 0, "" },
    { 'J',  6, 69, "PJ6 (PCINT15)"         , 0, "" },
    { 'J',  7, 79, "PJ7"                   , 0, "" },

    { 'A',  0, 78, "PA0 (AD8)",              0, "22" },
    { 'A',  1, 77, "PA1 (AD9)",              0, "23" },
    { 'A',  2, 76, "PA2 (AD10)",             0, "24" },
    { 'A',  3, 75, "PA3 (AD11)",             0, "25" },
    { 'A',  4, 74, "PA4 (AD12)",             0, "26" },
    { 'A',  5, 73, "PA5 (AD13)",             0, "27" },
    { 'A',  6, 72, "PA6 (AD14)",             0, "28" },
    { 'A',  7, 71, "PA7 (AD15)",             0, "29" },

    { 'K',  0, 89, "PK0 (ADC8/PCINT16)",     0, "A8"  }, // ADC or I/O
    { 'K',  1, 88, "PK1 (ADC9/PCINT17)",     0, "A9"  },
    { 'K',  2, 87, "PK2 (ADC10/PCINT18)",    0, "A10" },
    { 'K',  3, 86, "PK3 (ADC11/PCINT19)",    0, "A11" },
    { 'K',  4, 85, "PK4 (ADC12/PCINT20)",    0, "A12" },
    { 'K',  5, 84, "PK5 (ADC13/PCINT21)",    0, "A13" },
    { 'K',  6, 83, "PK6 (ADC14/PCINT22)",    0, "A14" },
    { 'K',  7, 82, "PK7 (ADC15/PCINT23)",    0, "A15" },

    { 'F',  0, 97, "PF0 (ADC0)",             0, "A0"  }, // ADC or I/O
    { 'F',  1, 96, "PF1 (ADC1)",             0, "A1"  },
    { 'F',  2, 95, "PF2 (ADC2)",             0, "A2"  },
    { 'F',  3, 94, "PF3 (ADC3)",             0, "A3"  },
    { 'F',  4, 93, "PF4 (ADC4/TCK)",         0, "A4"  },
    { 'F',  5, 92, "PF5 (ADC5/TMS)",         0, "A5"  },
    { 'F',  6, 91, "PF6 (ADC6/TDO)",         0, "A6"  },
    { 'F',  7, 90, "PF7 (ADC7/TDI)",         0, "A7"  },

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
    { 'B',  0,  1, "PB0 (T0/XCK)" },
    { 'B',  1,  2, "PB1 (T1)" },
    { 'B',  2,  3, "PB2 (INT2/AIN0)" },
    { 'B',  3,  4, "PB3 (OC0/AIN1)" },
    { 'B',  4,  5, "PB4 (_SS)" },
    { 'B',  5,  6, "PB5 (MOSI)" },
    { 'B',  6,  7, "PB6 (MISO)" },
    { 'B',  7,  8, "PB7 (SCK)" },
    { 'D',  0, 14, "PD0 (RXD)" },
    { 'D',  1, 15, "PD1 (TXD)" },
    { 'D',  2, 16, "PD2 (INT0)" },
    { 'D',  3, 17, "PD3 (INT1)" },
    { 'D',  4, 18, "PD4 (OC1B)" },
    { 'D',  5, 19, "PD5 (OC1A)" },
    { 'D',  6, 20, "PD6 (ICP1)" },
    { 'D',  7, 21, "PD7 (OC2)" },
    { 'C',  0, 22, "PC0 (SCL)" },
    { 'C',  1, 23, "PC1 (SDA)" },
    { 'C',  2, 24, "PC2 (TCK)" },
    { 'C',  3, 25, "PC3 (TMS)" },
    { 'C',  4, 26, "PC4 (TDO)" },
    { 'C',  5, 27, "PC5 (TDI)" },
    { 'C',  6, 28, "PC6 (TOSC1)" },
    { 'C',  7, 29, "PC7 (TOSC2)" },
    { 'A',  0, 40, "PA0 (ADC0)"  },
    { 'A',  1, 39, "PA1 (ADC1)"  },
    { 'A',  2, 38, "PA2 (ADC2)"  },
    { 'A',  3, 37, "PA3 (ADC3)"  },
    { 'A',  4, 36, "PA4 (ADC4)"  },
    { 'A',  5, 35, "PA5 (ADC5)"  },
    { 'A',  6, 34, "PA6 (ADC6)"  },
    { 'A',  7, 33, "PA7 (ADC7)"  },
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
// ATmega16 or ATmega32 in 44-Pin packages
McuIoPinInfo AvrAtmega16or32IoPinInfo44[] = {
    { 'B',  0, 40, "PB0 (T0/XCK)" },
    { 'B',  1, 41, "PB1 (T1)" },
    { 'B',  2, 42, "PB2 (INT2/AIN0)" },
    { 'B',  3, 43, "PB3 (OC0/AIN1)" },
    { 'B',  4, 44, "PB4 (_SS)" },
    { 'B',  5,  1, "PB5 (MOSI)" },
    { 'B',  6,  2, "PB6 (MISO)" },
    { 'B',  7,  3, "PB7 (SCK)" },
    { 'D',  0,  9, "PD0 (RXD)" },
    { 'D',  1, 10, "PD1 (TXD)" },
    { 'D',  2, 11, "PD2 (INT0)" },
    { 'D',  3, 12, "PD3 (INT1)" },
    { 'D',  4, 13, "PD4 (OC1B)" },
    { 'D',  5, 14, "PD5 (OC1A)" },
    { 'D',  6, 15, "PD6 (ICP1)" },
    { 'D',  7, 16, "PD7 (OC2)" },
    { 'C',  0, 19, "PC0 (SCL)" },
    { 'C',  1, 20, "PC1 (SDA)" },
    { 'C',  2, 21, "PC2 (TCK)" },
    { 'C',  3, 22, "PC3 (TMS)" },
    { 'C',  4, 23, "PC4 (TDO)" },
    { 'C',  5, 24, "PC5 (TDI)" },
    { 'C',  6, 25, "PC6 (TOSC1)" },
    { 'C',  7, 26, "PC7 (TOSC2)" },
    { 'A',  0, 37, "PA0 (ADC0)" },
    { 'A',  1, 36, "PA1 (ADC1)" },
    { 'A',  2, 35, "PA2 (ADC2)" },
    { 'A',  3, 34, "PA3 (ADC3)" },
    { 'A',  4, 33, "PA4 (ADC4)" },
    { 'A',  5, 32, "PA5 (ADC5)" },
    { 'A',  6, 31, "PA6 (ADC6)" },
    { 'A',  7, 30, "PA7 (ADC7)" },
};

McuAdcPinInfo AvrAtmega16or32AdcPinInfo44[] = {
    { 37, 0x00 },
    { 36, 0x01 },
    { 35, 0x02 },
    { 34, 0x03 },
    { 33, 0x04 },
    { 32, 0x05 },
    { 31, 0x06 },
    { 30, 0x07 },
};

//-----------------------------------------------------------------------------
// ATmega16U4 or ATmega32U4 in 44-Pin packages

//-----------------------------------------------------------------------------
// ATmega8 PDIP-28

McuIoPinInfo AvrAtmega8IoPinInfo28[] = {
//   port bit pin  pinName        ArduinoPin  ArduinoName
//                                         X
    { 'D',  0,  2, "PD0 (RXD)"          ,  2, "0" },
    { 'D',  1,  3, "PD1 (TXD)"          ,  3, "1" },
    { 'D',  2,  4, "PD2 (INT0)"         ,  4, "2" },
    { 'D',  3,  5, "PD3 (INT1)"         ,  5, "3" },
    { 'D',  4,  6, "PD4 (XCK / T0)"     ,  6, "4" },
    { 'D',  5, 11, "PD5 (T1)"           , 11, "5" },
    { 'D',  6, 12, "PD6 (AIN0)"         , 12, "6" },
    { 'D',  7, 13, "PD7 (AIN1)"         , 13, "7" },
    { 'B',  0, 14, "PB0 (ICP1)"         , 14, "8" },
    { 'B',  1, 15, "PB1 (OC1A)"         , 15, "9" },
    { 'B',  2, 16, "PB2 (SS / OC1B)"    , 16, "10" },
    { 'B',  3, 17, "PB3 (MOSI / OC2)"   , 17, "11" },
    { 'B',  4, 18, "PB4 (MISO)"         , 18, "12" },
    { 'B',  5, 19, "PB5 (SCK)"          , 19, "13" },
//  { 'B',  6,  9, "PB6 (XTAL1 / TOSC1)",  9, "" },
//  { 'B',  7, 10, "PB7 (XTAL2 / TOSC2)", 10, "" },
    { 'C',  0, 23, "PC0 (ADC0)"         , 23, "A0" },
    { 'C',  1, 24, "PC1 (ADC1)"         , 24, "A1" },
    { 'C',  2, 25, "PC2 (ADC2)"         , 25, "A2" },
    { 'C',  3, 26, "PC3 (ADC3)"         , 26, "A3" },
    { 'C',  4, 27, "PC4 (ADC4 / SDA)"   , 27, "A4" },
    { 'C',  5, 28, "PC5 (ADC5 / SCL)"   , 28, "A5" },
//  { 'C',  6,  1, "PC6 (_RESET)"       ,  1, "" },
};

//-----------------------------------------------------------------------------
McuIoPinInfo AvrAtmega8IoPinInfo[] = {
    { 'D',  0,  2, "PD0 (RXD)" },
    { 'D',  1,  3, "PD1 (TXD)" },
    { 'D',  2,  4, "PD2 (INT0)" },
    { 'D',  3,  5, "PD3 (INT1)" },
    { 'D',  4,  6, "PD4 (XCK / T0)" },
    { 'D',  5, 11, "PD5 (T1)" },
    { 'D',  6, 12, "PD6 (AIN0)" },
    { 'D',  7, 13, "PD7 (AIN1)" },
    { 'B',  0, 14, "PB0 (ICP1)" },
    { 'B',  1, 15, "PB1 (OC1A)" },
    { 'B',  2, 16, "PB2 (SS / OC1B)" },
    { 'B',  3, 17, "PB3 (MOSI / OC2)" },
    { 'B',  4, 18, "PB4 (MISO)" },
    { 'B',  5, 19, "PB5 (SCK)" },
//  { 'B',  6,  9, "PB6 (XTAL1 / TOSC1)" },
//  { 'B',  7, 10, "PB7 (XTAL2 / TOSC2)" },
    { 'C',  0, 23, "PC0 (ADC0)" },
    { 'C',  1, 24, "PC1 (ADC1)" },
    { 'C',  2, 25, "PC2 (ADC2)" },
    { 'C',  3, 26, "PC3 (ADC3)" },
    { 'C',  4, 27, "PC4 (ADC4 / SDA)" },
    { 'C',  5, 28, "PC5 (ADC5 / SCL)" },
//  { 'C',  6,  1, "PC6 (_RESET)" },
};

//-----------------------------------------------------------------------------
// ATmega328 PDIP-28

McuIoPinInfo AvrAtmega328IoPinInfo[] = {
//   port bit pin  pinName                 ArduinoPin  ArduinoName
//                                                  X
    { 'D',  0,  2, "PD0 (PCINT16 / RXD)"         ,  2, "0; //RX<-" },
    { 'D',  1,  3, "PD1 (PCINT17 / TXD)"         ,  3, "1; //TX->" },
    { 'D',  2,  4, "PD2 (PCINT18 / INT0)"        ,  4, "2" },
    { 'D',  3,  5, "PD3 (PCINT19 / OC2B / INT1)" ,  5, "3; //PWM~" },
    { 'D',  4,  6, "PD4 (PCINT20 / XCK / T0)"    ,  6, "4" },
    { 'D',  5, 11, "PD5 (PCINT21 / OC0B / T1)"   , 11, "5; //PWM~" },
    { 'D',  6, 12, "PD6 (PCINT22 / OC0A / AIN0)" , 12, "6; //PWM~" },
    { 'D',  7, 13, "PD7 (PCINT23 / AIN1)"        , 13, "7" },
    { 'B',  0, 14, "PB0 (PCINT0 / CLKO / ICP1)"  , 14, "8" },
    { 'B',  1, 15, "PB1 (OC1A / PCINT1)"         , 15, "9; //PWM~" },
    { 'B',  2, 16, "PB2 (SS / OC1B / PCINT2)"    , 16, "10; //PWM~" },
    { 'B',  3, 17, "PB3 (MOSI / OC2A / PCINT3)"  , 17, "11; //PWM~" },
    { 'B',  4, 18, "PB4 (MISO / PCINT4)"         , 18, "12" },
    { 'B',  5, 19, "PB5 (SCK / PCINT5)"          , 19, "13" },
//              7                                      VCC
//              8                                      GND
//  { 'B',  6,  9, "PB6 (PCINT6 / XTAL1 / TOSC1)",  9, "crystal" },
//  { 'B',  7, 10, "PB7 (PCINT7 / XTAL2 / TOSC2)", 10, "crystal" },
//             20                                     AVCC
//             21                                     AREF
//             22                                      GND
    { 'C',  0, 23, "PC0 (ADC0 / PCINT8)"         , 23, "A0" },
    { 'C',  1, 24, "PC1 (ADC1 / PCINT9)"         , 24, "A1" },
    { 'C',  2, 25, "PC2 (ADC2 / PCINT10)"        , 25, "A2" },
    { 'C',  3, 26, "PC3 (ADC3 / PCINT11)"        , 26, "A3" },
    { 'C',  4, 27, "PC4 (ADC4 / SDA / PCINT12)"  , 27, "A4" },
    { 'C',  5, 28, "PC5 (ADC5 / SCL / PCINT13)"  , 28, "A5" },
//  { 'C',  6,  1, "PC6 (PCINT14 / _RESET)"      ,  1, "reset" }
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
// ATtiny10 6-Pin packages

//-----------------------------------------------------------------------------
// ATtiny85 8-Pin packages
//-----------------------------------------------------------------------------
// ATmega8 32-Pin packages TQFP/QFN/MLF

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

McuExtIntPinInfo AvrExtIntPinInfo32[] = {
    { 32 }, // PD2/INT0
    {  1 }, // PD3/INT1
};

//-----------------------------------------------------------------------------
// PIC's
McuExtIntPinInfo PicExtIntPinInfo6[] = {
    {    }, //
};

McuExtIntPinInfo PicExtIntPinInfo14[] = {
    { 11 }, // INT
};

McuExtIntPinInfo PicExtIntPinInfo18[] = {
    {  6 }, // RB0/INT
};

McuExtIntPinInfo PicExtIntPinInfo28[] = {
    { 21 }, // RB0/INT
};

McuExtIntPinInfo PicExtIntPinInfo40[] = {
    { 33 }, // RB0/INT
};

McuExtIntPinInfo PicExtIntPinInfo64[] = {
    { 48 }, // RB0/INT
};

//-----------------------------------------------------------------------------
// ATmega328 32-Pin packages TQFP/QFN/MLF
//-----------------------------------------------------------------------------
// AVR's SPI Info Tables

McuSpiInfo McuSpiInfoATmega2560[] = {
//     name     REG_CTRL REG_STAT REG_DATA MISO  MOSI SCK  _SS
//                                         PB3   PB2  PB1  PB0
    { "SPI",       0x4C,    0x4D,    0x4E,  22,   21,  20,  19 },
};

//-----------------------------------------------------------------------------
// AVR's PWM Info Tables

McuPwmPinInfo AvrPwmPinInfo28_[] = { // ATmega8, etc.
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    {  0, 0, 8,    5, 0   ,  0x52,  0x53,  0,     0,        0, 0   ,  0    }, // Timer0

    { 15, 1, 8,    5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x08 }, // OC1A // Fast PWM  8-bit
    { 15, 1, 9,    5, 0x4A,  0x4B,  0x4F,  7,     6,        2, 0x4E,  0x08 }, // OC1A // Fast PWM  9-bit
    { 15, 1,10,    5, 0x4A,  0x4B,  0x4F,  7,     6,        3, 0x4E,  0x08 }, // OC1A // Fast PWM 10-bit

    { 16, 1, 8,    5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x08 }, // OC1B // Fast PWM  8-bit
    { 16, 1, 9,    5, 0x48,  0x49,  0x4F,  5,     4,        2, 0x4E,  0x08 }, // OC1B // Fast PWM  9-bit
    { 16, 1,10,    5, 0x48,  0x49,  0x4F,  5,     4,        3, 0x4E,  0x08 }, // OC1B // Fast PWM 10-bit

    { 17, 2, 8,    7, 0x43,  0   ,  0x45,  5,     4,     0x48, 0   ,  0    }, // OC2  // Fast PWM
};

McuPwmPinInfo AvrPwmPinInfo28[] = { // ATmega328, etc.
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    { 12, 0, 8,    5, 0x47,  0   ,  0x44,  7,     6,        3, 0x45,  0    ,"OC0A" }, // Fast PWM  8-bit
    { 11, 0, 8,    5, 0x48,  0   ,  0x44,  5,     4,        3, 0x45,  0    ,"OC0B" }, // Fast PWM  8-bit

    { 15, 1, 8,    5, 0x88,  0x89,  0x80,  7,     6,        1, 0x81,  0x08 ,"OC1A" }, // Fast PWM  8-bit
    { 15, 1, 9,    5, 0x88,  0x89,  0x80,  7,     6,        2, 0x81,  0x08 ,"OC1A" }, // Fast PWM  9-bit
    { 15, 1,10,    5, 0x88,  0x89,  0x80,  7,     6,        3, 0x81,  0x08 ,"OC1A" }, // Fast PWM 10-bit

    { 16, 1, 8,    5, 0x8A,  0x8B,  0x80,  5,     4,        1, 0x81,  0x08 ,"OC1B" }, // Fast PWM  8-bit
    { 16, 1, 9,    5, 0x8A,  0x8B,  0x80,  5,     4,        2, 0x81,  0x08 ,"OC1B" }, // Fast PWM  9-bit
    { 16, 1,10,    5, 0x8A,  0x8B,  0x80,  5,     4,        3, 0x81,  0x08 ,"OC1B" }, // Fast PWM 10-bit

    { 17, 2, 8,    7, 0xB3,  0   ,  0xB0,  7,     6,        3, 0xB1,  0    ,"OC2A" }, // Fast PWM  8-bit
    {  5, 2, 8,    7, 0xB4,  0   ,  0xB0,  5,     4,        3, 0xB1,  0    ,"OC2B" }, // Fast PWM  8-bit
};

McuPwmPinInfo AvrPwmPinInfo32_[] = {
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
//  { 13, 1, 8,    5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x00 }, // OC1A // Phase Correct PWM
//  { 14, 1, 8,    5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x00 }, // OC1B // Phase Correct PWM
//  { 15, 2, 8,    7, 0x43,  0   ,  0x45,  5,     4,     0x40, 0   ,  0    }, // OC2  // Phase Correct PWM
    {  0, 0, 8,    5, 0   ,  0x52,  0x53,  0,     0,        0, 0   ,  0    }, // Timer0
    { 13, 1, 8,    5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x08 }, // OC1A // Fast PWM 8-bit
    { 14, 1, 8,    5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x08 }, // OC1B // Fast PWM 8-bit
    { 15, 2, 8,    7, 0x43,  0   ,  0x45,  5,     4,     0x48, 0   ,  0    }, // OC2  // Fast PWM
};

McuPwmPinInfo AvrAtmega2560PwmPinInfo[] = {
////     ti  reso max REG_    REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL  OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
//  { 26, 0, 8,     5, 0x47,  0,     0x44,  7,     6,        1, 0x45,  0x08 }, // OC0A
    {  1, 0, 8,     5, 0x48,  0,     0x44,  5,     4,        2, 0x45,  0x08 }, // OC0B

    { 24, 1, 8,     5, 0x88,  0x89,  0x80,  7,     6,        1, 0x81,  0x08 }, // OC1A
    { 24, 1, 9,     5, 0x88,  0x89,  0x80,  7,     6,        2, 0x81,  0x08 }, // OC1A
    { 24, 1,10,     5, 0x88,  0x89,  0x80,  7,     6,        3, 0x81,  0x08 }, // OC1A

    { 25, 1, 8,     5, 0x8A,  0x8B,  0x80,  5,     4,        1, 0x81,  0x08 }, // OC1B
    { 25, 1, 9,     5, 0x8A,  0x8B,  0x80,  5,     4,        2, 0x81,  0x08 }, // OC1B
    { 25, 1,10,     5, 0x8A,  0x8B,  0x80,  5,     4,        3, 0x81,  0x08 }, // OC1B

    { 26, 1, 8,     5, 0x8C,  0x8D,  0x80,  3,     2,        1, 0x81,  0x08 }, // OC1C
    { 26, 1, 9,     5, 0x8C,  0x8D,  0x80,  3,     2,        2, 0x81,  0x08 }, // OC1C
    { 26, 1,10,     5, 0x8C,  0x8D,  0x80,  3,     2,        3, 0x81,  0x08 }, // OC1C

    { 23, 2, 8,     5, 0xB3,  0,     0xB0,  7,     6,        1, 0xB1,  0x08 }, // OC2A
    { 18, 2, 8,     5, 0xB4,  0,     0xB0,  5,     4,        2, 0xB1,  0x08 }, // OC2B

    {  5, 3, 8,     5, 0x98,  0x99,  0x90,  7,     6,        1, 0x91,  0x08 }, // OC3A
    {  5, 3, 9,     5, 0x98,  0x99,  0x90,  7,     6,        2, 0x91,  0x08 }, // OC3A
    {  5, 3,10,     5, 0x98,  0x99,  0x90,  7,     6,        3, 0x91,  0x08 }, // OC3A

    {  6, 3, 8,     5, 0x9A,  0x9B,  0x90,  5,     4,        1, 0x91,  0x08 }, // OC3B
    {  6, 3, 9,     5, 0x9A,  0x9B,  0x90,  5,     4,        2, 0x91,  0x08 }, // OC3B
    {  6, 3,10,     5, 0x9A,  0x9B,  0x90,  5,     4,        3, 0x91,  0x08 }, // OC3B

    {  7, 3, 8,     5, 0x9C,  0x9D,  0x90,  3,     2,        1, 0x91,  0x08 }, // OC3C
    {  7, 3, 9,     5, 0x9C,  0x9D,  0x90,  3,     2,        2, 0x91,  0x08 }, // OC3C
    {  7, 3,10,     5, 0x9C,  0x9D,  0x90,  3,     2,        3, 0x91,  0x08 }, // OC3C

    { 15, 4, 8,     5, 0xA8,  0xA9,  0xA0,  7,     6,        1, 0xA1,  0x08 }, // OC4A
    { 15, 4, 9,     5, 0xA8,  0xA9,  0xA0,  7,     6,        2, 0xA1,  0x08 }, // OC4A
    { 15, 4,10,     5, 0xA8,  0xA9,  0xA0,  7,     6,        3, 0xA1,  0x08 }, // OC4A

    { 16, 4, 8,     5, 0xAA,  0xAB,  0xA0,  5,     4,        1, 0xA1,  0x08 }, // OC4B
    { 16, 4, 9,     5, 0xAA,  0xAB,  0xA0,  5,     4,        2, 0xA1,  0x08 }, // OC4B
    { 16, 4,10,     5, 0xAA,  0xAB,  0xA0,  5,     4,        3, 0xA1,  0x08 }, // OC4B

    { 17, 4, 8,     5, 0xAC,  0xAD,  0xA0,  3,     2,        1, 0xA1,  0x08 }, // OC4C
    { 17, 4, 9,     5, 0xAC,  0xAD,  0xA0,  3,     2,        2, 0xA1,  0x08 }, // OC4C
    { 17, 4,10,     5, 0xAC,  0xAD,  0xA0,  3,     2,        3, 0xA1,  0x08 }, // OC4C

//  { 38, 5, 8,     5, 0x128, 0x129, 0x120, 7,     6,        1, 0x121, 0x08 }, // OC5A ???
//  { 39, 5, 8,     5, 0x12A, 0x12B, 0x120, 5,     4,        1, 0x121, 0x08 }, // OC5B
//  { 40, 5, 8,     5, 0x12C, 0x12D, 0x120, 3,     2,        1, 0x121, 0x08 }, // OC5C
};

McuPwmPinInfo AvrAtmega16_32PwmPinInfo40[] = {
////     ti  reso max REG_    REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL  OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    {  4, 0, 8,     5, 0x5C,  0,     0x53,  5,     4,     0x48, 0   ,  0    }, // OC0

    { 19, 1, 8,     5, 0x4A,  0x4B,  0x4F,  7,     6,        1, 0x4E,  0x08 }, // OC1A // Fast PWM  8-bit
    { 19, 1, 9,     5, 0x4A,  0x4B,  0x4F,  7,     6,        2, 0x4E,  0x08 }, // OC1A // Fast PWM  9-bit
    { 19, 1,10,     5, 0x4A,  0x4B,  0x4F,  7,     6,        3, 0x4E,  0x08 }, // OC1A // Fast PWM 10-bit

    { 18, 1, 8,     5, 0x48,  0x49,  0x4F,  5,     4,        1, 0x4E,  0x08 }, // OC1B // Fast PWM  8-bit
    { 18, 1, 9,     5, 0x48,  0x49,  0x4F,  5,     4,        2, 0x4E,  0x08 }, // OC1B // Fast PWM  9-bit
    { 18, 1,10,     5, 0x48,  0x49,  0x4F,  5,     4,        3, 0x4E,  0x08 }, // OC1B // Fast PWM 10-bit

    { 21, 2, 8,     7, 0x43,  0   ,  0x45,  5,     4,     0x48, 0   ,  0    }, // OC2  // Fast PWM
};

McuPwmPinInfo AvrPwmPinInfo40_[] = {
    { 21, 2 },
};

McuPwmPinInfo AvrAtmega162PwmPinInfo40_[] = {
    {  2, 2 },
};

McuPwmPinInfo AvrPwmPinInfo44_[] = {
    { 16, 2 },
};

McuPwmPinInfo AvrPwmPinInfo64_[] = {
    { 17, 2 },
};

McuPwmPinInfo AvrPwmPinInfo100_[] = {
    { 23, 2 },
};

McuPwmPinInfo AvrPwmPinInfo32[] = {
////     ti  reso max REG_   REG_   REG_   bit    bit    mask  REG_   mask
//// pin mer lutn  CS OCRnxL OCRnxH TCCRnA COMnx1 COMnx0 WGMa  TCCRnB WGMb
    { 10, 0, 8,    5, 0x47,  0,     0x44,  7,     6,     3,    0x45,  0    }, // OC0A // Fast PWM
    {  9, 0, 8,    5, 0x48,  0,     0x44,  5,     4,     3,    0x45,  0    }, // OC0B

    { 13, 1, 8,    5, 0x88,  0x89,  0x80,  7,     6,     1,    0x81,  0x08 }, // OC1A
    { 13, 1, 9,    5, 0x88,  0x89,  0x80,  7,     6,     2,    0x81,  0x08 }, // OC1A
    { 13, 1,10,    5, 0x88,  0x89,  0x80,  7,     6,     3,    0x81,  0x08 }, // OC1A

    { 14, 1, 8,    5, 0x8A,  0x8B,  0x80,  5,     4,     1,    0x81,  0x08 }, // OC1B
    { 14, 1, 9,    5, 0x8A,  0x8B,  0x80,  5,     4,     2,    0x81,  0x08 }, // OC1B
    { 14, 1,10,    5, 0x8A,  0x8B,  0x80,  5,     4,     3,    0x81,  0x08 }, // OC1B

    { 15, 2, 8,    7, 0xB3,  0,     0xB0,  7,     6,     3,    0xB1,  0    }, // OC2A
    {  1, 2, 8,    7, 0xB4,  0,     0xB0,  5,     4,     3,    0xB1,  0    }, // OC2B
};

//-----------------------------------------------------------------------------
// PIC's PWM Info Tables
////     ti
//// pin mer
McuPwmPinInfo PicPwmPinInfo8[] = {
    {  5, 1 },
};

McuPwmPinInfo PicPwmPinInfo18[] = {
    {  9, 1 },
};

McuPwmPinInfo PicPwmPinInfo28_1[] = {
    { 13, 2 },
};

McuPwmPinInfo PicPwmPinInfo28_2[] = {
    { 12, 2 },
    { 13, 1 },
};

McuPwmPinInfo PicPwmPinInfo40[] = {
    { 16, 2 },
    { 17, 1 },
};

McuPwmPinInfo PicPwmPinInfo64[] = {
    { 29, 2 },
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
// A variety of 14-Pin PICs that share the same digital IO assignment.

//-----------------------------------------------------------------------------
// A variety of 18-Pin PICs that share the same digital IO assignment.

McuIoPinInfo Pic18PinIoInfo[] = {
    { 'A',  2,  1, "RA2/AN2/VREF" },
    { 'A',  3,  2, "RA3/AN3/CMP1" },
    { 'A',  4,  3, "RA4/T0CKI/CMP2" },
    { 'A',  5,  4, "RA5/_MCLR/Vpp (Input Only)" },
//  { ' ', -1,  5, "Vss" },
    { 'B',  0,  6, "RB0/INT" },
    { 'B',  1,  7, "RB1/RX/DT" },
    { 'B',  2,  8, "RB2/TX/CK" },
    { 'B',  3,  9, "RB3/CCP1" },
    { 'B',  4, 10, "RB4" },
    { 'B',  5, 11, "RB5" },
    { 'B',  6, 12, "RB6" },
    { 'B',  7, 13, "RB7" },
//  { ' ', -1, 14, "Vdd" },
    { 'A',  6, 15, "RA6/OSC2/CLKOUT" },
    { 'A',  7, 16, "RA7/OSC1/CLKIN" },
    { 'A',  0, 17, "RA0/AN0" },
    { 'A',  1, 18, "RA1/AN1" },
};

McuAdcPinInfo Pic16F819AdcPinInfo[] = {
    {  1, 0x02 },
    {  2, 0x03 },
    {  3, 0x04 },
    { 17, 0x00 },
    { 18, 0x01 },
};


//-----------------------------------------------------------------------------
// PIC16F88

McuIoPinInfo Pic16F88PinIoInfo[] = {
    { 'A',  2,  1, "RA2/AN2" },
    { 'A',  3,  2, "RA3/AN3/CMP1" },
    { 'A',  4,  3, "RA4/AN4/T0CKI/CMP2" },
    { 'A',  5,  4, "RA5/_MCLR/Vpp (Input Only)" }, // without pull-up resistor
//  { ' ', -1,  5, "Vss" },
    { 'B',  0,  6, "RB0/INT" },
    { 'B',  1,  7, "RB1/SDI/SDA" },
    { 'B',  2,  8, "RB2/SDO/RX/DT" },
    { 'B',  3,  9, "RB3/CCP1" },
    { 'B',  4, 10, "RB4/SCK/SCL" },
    { 'B',  5, 11, "RB5/_SS/TX/CK" },
    { 'B',  6, 12, "RB6/AN5" },
    { 'B',  7, 13, "RB7/AN6" },
//  { ' ', -1, 14, "Vdd" },
    { 'A',  6, 15, "RA6/OSC2/CLKOUT" },
    { 'A',  7, 16, "RA7/OSC1/CLKIN" },
    { 'A',  0, 17, "RA0/AN0" },
    { 'A',  1, 18, "RA1/AN1" },
};

McuAdcPinInfo Pic16F88AdcPinInfo[] = {
    { 17, 0x00 },
    { 18, 0x01 },
    {  1, 0x02 },
    {  2, 0x03 },
    {  3, 0x04 },
    { 12, 0x05 },
    { 13, 0x06 },
};

//-----------------------------------------------------------------------------
// PIC16F1826, PIC16F1827
//-----------------------------------------------------------------------------
// PIC16F877, PIC16F874

McuIoPinInfo Pic16F877IoPinInfo[] = {
    { 'A',  0,  2, "RA0/AN0" },
    { 'A',  1,  3, "RA1/AN1" },
    { 'A',  2,  4, "RA2/AN2/VREF-" },
    { 'A',  3,  5, "RA3/AN3/VREF+" },
    { 'A',  4,  6, "RA4/T0CKI" },
    { 'A',  5,  7, "RA5/AN4/_SS" },
    { 'E',  0,  8, "RE0/_RD/AN5" },
    { 'E',  1,  9, "RE1/_WR/AN6" },
    { 'E',  2, 10, "RE2/_CS/AN7" },
//  {           1, "_MCLR/Vpp" },
    { 'C',  0, 15, "RC0/T1OSO/T1CKI" },
    { 'C',  1, 16, "RC1/T1OSI/CCP2" },
    { 'C',  2, 17, "RC2/CCP1" },
    { 'C',  3, 18, "RC3/SCK/SCL" },
    { 'D',  0, 19, "RD0/PSP0" },
    { 'D',  1, 20, "RD1/PSP1" },
    { 'D',  2, 21, "RD2/PSP2" },
    { 'D',  3, 22, "RD3/PSP3" },
    { 'C',  4, 23, "RC4/SDI/SDA" },
    { 'C',  5, 24, "RC5/SDO" },
    { 'C',  6, 25, "RC6/TX/CK" },
    { 'C',  7, 26, "RC7/RX/DT" },
    { 'D',  4, 27, "RD4/PSP4" },
    { 'D',  5, 28, "RD5/PSP5" },
    { 'D',  6, 29, "RD6/PSP6" },
    { 'D',  7, 30, "RD7/PSP7" },
    { 'B',  0, 33, "RB0/INT" },
    { 'B',  1, 34, "RB1" },
    { 'B',  2, 35, "RB2" },
    { 'B',  3, 36, "RB3" },
    { 'B',  4, 37, "RB4" },
    { 'B',  5, 38, "RB5" },
    { 'B',  6, 39, "RB6" },
    { 'B',  7, 40, "RB7" }
};

McuAdcPinInfo Pic16F877AdcPinInfo[] = {
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
// PIC16F72 28-Pin PDIP, SOIC, SSOP

//-----------------------------------------------------------------------------
// PIC16F876, PIC16F873 28-Pin PDIP, SOIC
McuIoPinInfo Pic16F876IoPinInfo[] = {
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

McuAdcPinInfo Pic16F876AdcPinInfo[] = {
    {  2, 0x00 },
    {  3, 0x01 },
    {  4, 0x02 },
    {  5, 0x03 },
    {  7, 0x04 }
};

//-----------------------------------------------------------------------------
// PIC16F887, PIC16F884

McuIoPinInfo Pic16F887IoPinInfo[] = {
    { 'A',  0,  2, "RA0/AN0/ULPWU/C12IN0-" },
    { 'A',  1,  3, "RA1/AN1/C12IN1-" },
    { 'A',  2,  4, "RA2/AN2/VREF-/CVREF/C2IN+" },
    { 'A',  3,  5, "RA3/AN3/VREF+/C1IN+" },
    { 'A',  4,  6, "RA4/T0CKI/C1OUT" },
    { 'A',  5,  7, "RA5/AN4/_SS/C2OUT" },
    { 'E',  0,  8, "RE0/AN5" },
    { 'E',  1,  9, "RE1/AN6" },
    { 'E',  2, 10, "RE2/AN7" },
    { 'E',  3,  1, "RE3/_MCLR/Vpp (Input Only)" },
    { 'A',  7, 13, "RA7/OSC1/CLKIN" },
    { 'A',  6, 14, "RA6/OSC2/CLKOUT" },
    { 'C',  0, 15, "RC0/T1OSO/T1CKI" },
    { 'C',  1, 16, "RC1/T1OSI/CCP2" },
    { 'C',  2, 17, "RC2/P1A/CCP1" },
    { 'C',  3, 18, "RC3/SCK/SCL" },
    { 'D',  0, 19, "RD0" },
    { 'D',  1, 20, "RD1" },
    { 'D',  2, 21, "RD2" },
    { 'D',  3, 22, "RD3" },
    { 'C',  4, 23, "RC4/SDI/SDA" },
    { 'C',  5, 24, "RC5/SDO" },
    { 'C',  6, 25, "RC6/TX/CK" },
    { 'C',  7, 26, "RC7/RX/DT" },
    { 'D',  4, 27, "RD4" },
    { 'D',  5, 28, "RD5/P1B" },
    { 'D',  6, 29, "RD6/P1C" },
    { 'D',  7, 30, "RD7/P1D" },
    { 'B',  0, 33, "RB0/AN12/INT" },
    { 'B',  1, 34, "RB1/AN10/C12IN3-" },
    { 'B',  2, 35, "RB2/AN8" },
    { 'B',  3, 36, "RB3/AN9/C12IN2-" },
    { 'B',  4, 37, "RB4/AN11" },
    { 'B',  5, 38, "RB5/AN13/_T1G" },
    { 'B',  6, 39, "RB6/ICSPCLK" },
    { 'B',  7, 40, "RB7/ICSPDAT" }
};

McuAdcPinInfo Pic16F887AdcPinInfo[] = {
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
// 28-Pin PDIP, SOIC, SSOP
// PIC16F886,  PIC16F883, PIC16F882
// PIC16F1512, PIC16F1513
// PIC16F1516, PIC16F1518

McuIoPinInfo Pic28Pin_SPDIP_SOIC_SSOP[] = {
    { 'E', 3,  1, "RE3/_MCLR/Vpp (Input Only)" },

    { 'A', 0,  2, "RA0/AN0/ULPWU/C12IN0-" },
    { 'A', 1,  3, "RA1/AN1/C12IN1-" },
    { 'A', 2,  4, "RA2/AN2/VREF-/CVREF/C2IN+" },
    { 'A', 3,  5, "RA3/AN3/VREF+/C1IN+" },
    { 'A', 4,  6, "RA4/T0CKI/C1OUT" },
    { 'A', 5,  7, "RA5/AN4/_SS/C2OUT" },
//  {          8, "Vss" },
    { 'A', 7,  9, "RA7/OSC2/CLKOUT" },
    { 'A', 6, 10, "RA6/OSC1/CLKIN" },
    { 'C', 0, 11, "RC0/T1OSO/T1CKI" },
    { 'C', 1, 12, "RC1/T1OSI/CCP2" },
    { 'C', 2, 13, "RC2/P1A/CCP1" },
    { 'C', 3, 14, "RC3/SCK/SCL" },
    { 'C', 4, 15, "RC4/SDI/SDA" },
    { 'C', 5, 16, "RC5/SDO" },
    { 'C', 6, 17, "RC6/TX/CK" },
    { 'C', 7, 18, "RC7/RX/DT" },
//  {         19, "Vss" },
//  {         20, "Vdd" },
    { 'B', 0, 21, "RB0/AN12/INT" },
    { 'B', 1, 22, "RB1/AN10/P1C/C12IN3-" },
    { 'B', 2, 23, "RB2/AN8/P1B" },
    { 'B', 3, 24, "RB3/AN9/C12IN2-" },
    { 'B', 4, 25, "RB4/AN11/P1D" },
    { 'B', 5, 26, "RB5/AN13/_T1G" },
    { 'B', 6, 27, "RB6/ICSPCLK" },
    { 'B', 7, 28, "RB7/ICSPDAT" }
};

McuAdcPinInfo Pic16F886AdcPinInfo[] = {
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
// PIC16F1512, PIC16F1513, PIC16F1516, PIC16F1518
//-----------------------------------------------------------------------------
// PIC16F1526, PIC16F1527

//-----------------------------------------------------------------------------
// 6-Pin SOT-23
// PIC10F200, PIC10F202, PIC10F204, PIC10F206
// PIC10F220, PIC10F222
McuIoPinInfo Pic6Pin_SOT23[] = {
    { 'P', 0, 1, "GP0/ICSPDAT" },
    { 'P', 1, 3, "GP1/ICSPCLK" },
    { 'P', 2, 4, "GP2/T0CKI/FOSC4" },
    { 'P', 3, 6, "GP3/_MCLR/Vpp (Input Only)" },
};

//-----------------------------------------------------------------------------
// 8-Pin PDIP, SOIC, DFN-S, DFN
// PIC12F629
// PIC12F675
// PIC12F683
// PIC12F752
//-----------------------------------------------------------------------------
// ESP8266
//-----------------------------------------------------------------------------
// Controllino Maxi
McuIoPinInfo ControllinoMaxiIoPinInfo[] = {
//   port bit pin pinName ArduinoPin
    { 'E',  4,  6, "D0",  2 },
    { 'E',  5,  7, "D1",  3 },
    { 'G',  5,  1, "D2",  4 },
    { 'E',  3,  5, "D3",  5 },
    { 'H',  3, 15, "D4",  6 },
    { 'H',  4, 16, "D5",  7 },
    { 'H',  5, 17, "D6",  8 },
    { 'H',  6, 18, "D7",  9 },
    { 'B',  4, 23, "D8",  10 },
    { 'B',  5, 24, "D9",  11 },
    { 'B',  6, 25, "D10", 12 },
    { 'B',  7, 26, "D11", 13 },

    { 'A',  0, 78, "R0",  22 },
    { 'A',  1, 77, "R1",  23 },
    { 'A',  2, 76, "R2",  24 },
    { 'A',  3, 75, "R3",  25 },
    { 'A',  4, 74, "R4",  26 },
    { 'A',  5, 73, "R5",  27 },
    { 'A',  6, 72, "R6",  28 },
    { 'C',  6, 59, "R9",  29 },
    { 'A',  7, 71, "R7",  30 },
    { 'C',  7, 60, "R8",  31 },

    { 'F',  0, 97, "A0",  54 },
    { 'F',  1, 96, "A1",  55 },
    { 'F',  2, 95, "A2",  56 },
    { 'F',  3, 94, "A3",  57 },
    { 'F',  4, 93, "A4",  58 },
    { 'F',  5, 92, "A5",  59 },
    { 'F',  6, 91, "A6",  60 },
    { 'F',  7, 90, "A7",  61 },
    { 'K',  0, 89, "A8",  62 },
    { 'K',  1, 88, "A9",  63 },
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

McuPwmPinInfo ControllinoMaxiPwmPinInfo[] = {
////ATmega2560
////     ti
//// pin mer
    {  1, 0 },
    { 24, 1 },
    { 25, 1 },
    { 26, 1 },
    { 18, 2 },
    { 23, 2 },
    {  5, 3 },
    {  6, 3 },
    {  7, 3 },
    { 15, 4 },
    { 16, 4 },
    { 17, 4 },
    { 38, 5 },
    { 39, 5 },
    { 40, 5 },
};

//-----------------------------------------------------------------------------
// PC LPT & COM
McuIoPinInfo PcCfg[] = {
// Dynamically loaded by LoadPcPorts() in psports.cpp
//   port; bit; pin; pinName; ArduinoPin; ArduinoName; pinFunctions; pinUsed;  portN; dbPin; ioType
    { 'L',   0,   1,      "",          0,          "",/*          0,       0,*/    1,     2,      2},
};

//===========================================================================
McuIoInfo SupportedMcus_[] = {
    {
        "Atmel AVR ATmega2560 100-TQFP",
        "ATmega2560",
        "m2560def",
        "mega2560",
        "",
        'P',
//        A     B     C     D     E     F     G     H      I   J      K      L
        { 0x20, 0x23, 0x26, 0x29, 0x2C, 0x2F, 0x32, 0x100, 0,  0x103, 0x106, 0x109 }, // PINx  input
        { 0x22, 0x25, 0x28, 0x2B, 0x2E, 0x32, 0x34, 0x102, 0,  0x105, 0x108, 0x10B }, // PORTx output
        { 0x21, 0x24, 0x27, 0x2A, 0x2D, 0x30, 0x33, 0x101, 0,  0x104, 0x107, 0x10A }, // DDRx  dir
        128*1024,
        { { 0x200, 8192 } },
        AvrAtmega2560_100TQFPIoPinInfo,
        arraylen(AvrAtmega2560_100TQFPIoPinInfo),
        AvrAtmega2560_100TQFPAdcPinInfo,
        arraylen(AvrAtmega2560_100TQFPAdcPinInfo),
        1023,
        { 2 , 3 },
        23,
        ISA_AVR,
        EnhancedCore4M,
        100,
        0,
        AvrAtmega2560PwmPinInfo,
        arraylen(AvrAtmega2560PwmPinInfo),
        nullptr,
        0,
        McuSpiInfoATmega2560,
        arraylen(McuSpiInfoATmega2560),
    },
    {
        "Atmel AVR AT90USB647 64-TQFP",
        "",
        "",
        "",
        "",
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
        64,
        0,
        AvrPwmPinInfo64_,
        arraylen(AvrPwmPinInfo64_)
    },
    {
        "Atmel AVR ATmega128 64-TQFP",
        "",
        "",
        "",
        "",
        'P',
//        A     B     C     D     E     F     G     H      I   J      K      L
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
        64,
        0,
        AvrPwmPinInfo64_,
        arraylen(AvrPwmPinInfo64_)
    },
    {
        "Atmel AVR ATmega64 64-TQFP",
        "",
        "",
        "",
        "",
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
        64,
        0,
        AvrPwmPinInfo64_,
        arraylen(AvrPwmPinInfo64_)
    },
    {
        "Atmel AVR ATmega162 40-PDIP",
        "",
        "",
        "",
        "",
        'P',
        { 0x39, 0x36, 0x33, 0x30, 0x25 }, // PINx
        { 0x3b, 0x38, 0x35, 0x32, 0x27 }, // PORTx
        { 0x3a, 0x37, 0x34, 0x31, 0x26 }, // DDRx
        8*1024,
        { { 0x100, 1024 } },
        AvrAtmega162IoPinInfo,
        arraylen(AvrAtmega162IoPinInfo),
        nullptr,
        0,
        0,
        { 0, 0 },
        2, // OC2
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrAtmega162PwmPinInfo40_,
        arraylen(AvrAtmega162PwmPinInfo40_)
    },
    {
        "Atmel AVR ATmega32 40-PDIP",
        "ATmega32",
        "m32def",
        "mega32",
        "",
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
        40,
        0,
        AvrAtmega16_32PwmPinInfo40,
        arraylen(AvrAtmega16_32PwmPinInfo40)
    },
    {
        "Atmel AVR ATmega16 40-PDIP",
        "",
        "",
        "",
        "",
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
        40,
        0,
        AvrAtmega16_32PwmPinInfo40,
        arraylen(AvrAtmega16_32PwmPinInfo40)
    },
    {
        "Atmel AVR ATmega48 28-PDIP",
        "",
        "",
        "",
        "",
        'P',
        { 0, 0x23, 0x26, 0x29 }, // PINx
        { 0, 0x25, 0x28, 0x2B }, // PORTx
        { 0, 0x24, 0x27, 0x2A }, // DDRx
        2*1024,
        { { 0x100, 512 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR,
        EnhancedCore128K,
        28,
        0,
        AvrPwmPinInfo28,
        arraylen(AvrPwmPinInfo28)
    },
    {
        "Atmel AVR ATmega88 28-PDIP",
        "",
        "",
        "",
        "",
        'P',
        { 0, 0x23, 0x26, 0x29 }, // PINx
        { 0, 0x25, 0x28, 0x2B }, // PORTx
        { 0, 0x24, 0x27, 0x2A }, // DDRx
        4*1024,
        { { 0x100, 1024 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR,
        EnhancedCore128K,
        28,
        0,
        AvrPwmPinInfo28,
        arraylen(AvrPwmPinInfo28)
    },
    {
        "Atmel AVR ATmega168 28-PDIP",
        "",
        "",
        "",
        "",
        'P',
        { 0, 0x23, 0x26, 0x29 }, // PINx
        { 0, 0x25, 0x28, 0x2B }, // PORTx
        { 0, 0x24, 0x27, 0x2A }, // DDRx
        8*1024,
        { { 0x100, 1024 } },
        AvrAtmega8IoPinInfo,
        arraylen(AvrAtmega8IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR,
        EnhancedCore128K,
        28,
        0,
        AvrPwmPinInfo28,
        arraylen(AvrPwmPinInfo28)
    },
    {
        "Atmel AVR ATmega328 28-PDIP",
        "ATmega328",
        "m328def",
        "mega328",
        "",
        'P',
        { 0, 0x23, 0x26, 0x29 }, // PINx
        { 0, 0x25, 0x28, 0x2B }, // PORTx
        { 0, 0x24, 0x27, 0x2A }, // DDRx
        16*1024,
        { { 0x100, 2048 } },
        AvrAtmega328IoPinInfo,
        arraylen(AvrAtmega328IoPinInfo),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR,
        EnhancedCore128K,
        28,
        0,
        AvrPwmPinInfo28,
        arraylen(AvrPwmPinInfo28)
    },
    {
        "Atmel AVR ATmega164 40-PDIP",
        "",
        "",
        "",
        "",
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
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrPwmPinInfo40_,
        arraylen(AvrPwmPinInfo40_)
    },
    {
        "Atmel AVR ATmega324 40-PDIP",
        "",
        "",
        "",
        "",
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
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrPwmPinInfo40_,
        arraylen(AvrPwmPinInfo40_)
    },
    {
        "Atmel AVR ATmega644 40-PDIP",
        "",
        "",
        "",
        "",
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
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrPwmPinInfo40_,
        arraylen(AvrPwmPinInfo40_)
    },
    {
        "Atmel AVR ATmega1284 40-PDIP",
        "",
        "",
        "",
        "",
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
        ISA_AVR,
        EnhancedCore128K,
        40,
        0,
        AvrPwmPinInfo40_,
        arraylen(AvrPwmPinInfo40_)
    },
    {
        "Atmel AVR ATmega8 32-Pin packages", //char            *mcuName;
        "ATmega8",
        "m8def", // "iom8"
        "mega8",
        "",
        'P',                                 //char             portPrefix;
        { 0, 0x36, 0x33, 0x30 }, // PINx  //DWORD            inputRegs[MAX_IO_PORTS]; // A is 0, J is 9
        { 0, 0x38, 0x35, 0x32 }, // PORTx //DWORD            outputRegs[MAX_IO_PORTS];
        { 0, 0x37, 0x34, 0x31 }, // DDRx  //DWORD            dirRegs[MAX_IO_PORTS];
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
        32,
//      { 32, 1 }, // INT0, INT1             //int int0PinA; int int1PinA;} QuadEncodNeeds;
//      AvrAtmega8ExtIntPinInfo32,
//      arraylen(AvrAtmega8ExtIntPinInfo32),
        AvrPwmPinInfo32_,
        arraylen(AvrPwmPinInfo32_),
        AvrExtIntPinInfo32,
        arraylen(AvrExtIntPinInfo32)
    },
    {
        "Atmel AVR ATmega8 28-PDIP",
        "ATmega8",
        "m8def",
        "mega8",
        "",
        'P',
        { 0, 0x36, 0x33, 0x30 }, // PINx     (but there is no xxxA)
        { 0, 0x38, 0x35, 0x32 }, // PORTx
        { 0, 0x37, 0x34, 0x31 }, // DDRx
        4*1024,
        { { 0x60, 1024 } },
        AvrAtmega8IoPinInfo28,
        arraylen(AvrAtmega8IoPinInfo28),
        AvrAtmega8AdcPinInfo,
        arraylen(AvrAtmega8AdcPinInfo),
        1023,
        { 2, 3 },
        17,
        ISA_AVR,
        EnhancedCore8K,
        28,
        0,
        AvrPwmPinInfo28_,
        arraylen(AvrPwmPinInfo28_)
    },
//===========================================================================
    {
        "Microchip PIC16F628 18-PDIP or 18-SOIC",
        "PIC16F628",
        "P16F628",
        "P16F628",
        "PIC16F628",
        'R',
        { 0x05, 0x06 }, // PORTx
        { 0x05, 0x06 }, // PORTx
        { 0x85, 0x86 }, // TRISx
        2048,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 48 } },
        Pic18PinIoInfo,
        arraylen(Pic18PinIoInfo),
        nullptr,
        0,
        0,
        { 7, 8 },
        0,
        ISA_PIC16,
        MidrangeCore14bit,
        18,
        // code protection off, data code protection off, LVP disabled,
        // BOD reset enabled, RA5/nMCLR is _MCLR, PWRT enabled, WDT disabled,
        0x3f62, // HS oscillator, _MCLR
//      0x3f42, // HS oscillator, RA5 pin function is digital Input
//      0x3F70, // INTOSC oscillator, _MCLR
//      0x3F50, // INTOSC oscillator, RA5 pin function is digital Input
            /*
            (1 << 13) | // Code protection off,
            (1 <<  8) | // Data memory code protection off,
            (0 <<  7) | // LVP disabled, RB4/PGM is digital I/O,
            (1 <<  6) | // BOR reset enabled,
//          (0 <<  5) | // RA5/_MCLR is RA5, RA5 pin function is digital Input,
            (1 <<  5) | // RA5/_MCLR/Vpp pin function is _MCLR
            (0 <<  3) | // PWRT enabled,
            (0 <<  2) | // WDT disabled,
//          (1 <<  2) | // WDT enabled,
            (2 <<  0),  // HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN
//       (0x10 <<  0),  // INTOSC oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN
            */
        PicPwmPinInfo18,
        arraylen(PicPwmPinInfo18),
        PicExtIntPinInfo18,
        arraylen(PicExtIntPinInfo18)
    },
    {
        "Microchip PIC16F88 18-PDIP or 18-SOIC",
        "PIC16F88",
        "P16F88",
        "P16F88",
        "PIC16F88",
        'R',
        { 0x05, 0x06 }, // PORTx
        { 0x05, 0x06 }, // PORTx
        { 0x85, 0x86 }, // TRISx
        4096,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 48 } },
        Pic16F88PinIoInfo,
        arraylen(Pic16F88PinIoInfo),
        Pic16F88AdcPinInfo,
        arraylen(Pic16F88AdcPinInfo),
        1023,
        { 8, 11 },
        0,
        ISA_PIC16,
        MidrangeCore14bit,
        18,
//          (1 << 17) | // IESO: Internal External Switchover mode enabled
//          (1 << 16) | // FCMEN: Fail-Safe Clock Monitor enabled
            (1 << 13) | // Flash Program Memory Code Protection off
            (0 << 12) | // CCP1 function on RB3 (doesn't matter)
            (1 << 11) | // DEBUG: ICD disabled
            (3 <<  9) | // Flash Program Memory write protection off
            (1 <<  8) | // Data EE Memory Code protection off
            (0 <<  7) | // LVP disabled
            (1 <<  6) | // BOR enabled
            (0 <<  5) | // RA5/nMCLR is RA5
            (0 <<  4) | // FOSC2=0 for osc sel, HS oscillator
            (0 <<  3) | // PWRT enabled
            (0 <<  2) | // WDT disabled
            (2 <<  0),  // HS oscillator
        PicPwmPinInfo18,
        arraylen(PicPwmPinInfo18),
        PicExtIntPinInfo18,
        arraylen(PicExtIntPinInfo18)
    },
    {
        "Microchip PIC16F819 18-PDIP or 18-SOIC",
        "PIC16F819",
        "P16F819",
        "P16F819",
        "PIC16F819",
        'R',
        { 0x05, 0x06 }, // PORTx
        { 0x05, 0x06 }, // PORTx
        { 0x85, 0x86 }, // TRISx
        2048,
        { { 0x20, 96 } },
        Pic18PinIoInfo,
        arraylen(Pic18PinIoInfo),
        Pic16F819AdcPinInfo,
        arraylen(Pic16F819AdcPinInfo),
        1023,
        { 0, 0 },
        0,
        ISA_PIC16,
        MidrangeCore14bit,
        18,
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
//          (1 <<  4) | // 100 = INTRC oscillator; port I/O function on both RA6/OSC2/CLKO pin and RA7/OSC1/CLKI pin
            (2 <<  0),  // 010 = HS oscillator
        PicPwmPinInfo18,
        arraylen(PicPwmPinInfo18),
        PicExtIntPinInfo18,
        arraylen(PicExtIntPinInfo18)
    },
    {
        "Microchip PIC16F877 40-PDIP",
        "PIC16F877",
        "P16F877",
        "16F877",
        "pic16F877",
        'R',
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0x88, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x110, 96 }, { 0x190, 96 } },
        Pic16F877IoPinInfo,
        arraylen(Pic16F877IoPinInfo),
        Pic16F877AdcPinInfo,
        arraylen(Pic16F877AdcPinInfo),
        1023,
        { 26, 25 },
        16,
        ISA_PIC16,
        MidrangeCore14bit,
        40,
        // code protection off, debug off, flash write off, EE code protection
        // off, LVP disabled, BOD enabled, CP off, PWRT enabled, WDT disabled,
        // HS oscillator
        0x3f72,
        PicPwmPinInfo40,
        arraylen(PicPwmPinInfo40),
        PicExtIntPinInfo40,
        arraylen(PicExtIntPinInfo40)
    },
    {
        "Microchip PIC16F876 28-PDIP or 28-SOIC",
        "PIC16F876",
        "P16F876",
        "P16F876",
        "PIC16F876",
        'R',
        { 0x05, 0x06, 0x07 }, // PORTx
        { 0x05, 0x06, 0x07 }, // PORTx
        { 0x85, 0x86, 0x87 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x110, 96 }, { 0x190, 96 } },
        Pic16F876IoPinInfo,
        arraylen(Pic16F876IoPinInfo),
        Pic16F876AdcPinInfo,
        arraylen(Pic16F876AdcPinInfo),
        1023,
        { 18, 17 },
        12,
        ISA_PIC16,
        MidrangeCore14bit,
        28,
        // code protection off, debug off, flash write off, EE code protection
        // off, LVP disabled, BOD enabled, CP off, PWRT enabled, WDT disabled,
        // HS oscillator
        0x3f72,
        PicPwmPinInfo28_2,
        arraylen(PicPwmPinInfo28_2),
        PicExtIntPinInfo28,
        arraylen(PicExtIntPinInfo28)
    },
    {
        "Microchip PIC16F887 40-PDIP",
        "PIC16F887",
        "P16F887",
        "P16F887",
        "PIC16F887",
        'R',
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x05, 0x06, 0x07, 0x08, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0x88, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 80 }, { 0x1a0, 80 } },
        Pic16F887IoPinInfo,
        arraylen(Pic16F887IoPinInfo),
        Pic16F887AdcPinInfo,
        arraylen(Pic16F887AdcPinInfo),
        1023,
        { 26, 25 },
        16,
        ISA_PIC16,
        MidrangeCore14bit,
        40,
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
            (2 <<  0),      // HS oscillator
        PicPwmPinInfo40,
        arraylen(PicPwmPinInfo40),
        PicExtIntPinInfo40,
        arraylen(PicExtIntPinInfo40)
    },
    {
        "Microchip PIC16F886 28-PDIP or 28-SOIC",
        "PIC16F886",
        "P16F886",
        "P16F886",
        "PIC16F886",
        'R',
        { 0x05, 0x06, 0x07, 0, 0x09 }, // PORTx = A B C E
        { 0x05, 0x06, 0x07, 0, 0x09 }, // PORTx
        { 0x85, 0x86, 0x87, 0, 0x89 }, // TRISx
        8*1024,
        { { 0x20, 96 }, { 0xA0, 80 }, { 0x120, 80 }, { 0x1a0, 80 } },
        Pic28Pin_SPDIP_SOIC_SSOP,
        arraylen(Pic28Pin_SPDIP_SOIC_SSOP),
        Pic16F886AdcPinInfo,
        arraylen(Pic16F886AdcPinInfo),
        1023,
        { 18, 17 },
        12,
        ISA_PIC16,
        MidrangeCore14bit,
        28,
         (0x38 << (8+16)) | // Unimplemented: Read as 1
            (3 << (9+16)) | // flash write protection off
            (0 << (8+16)) | // BOR at 2.1 V
         (0xff << 16) |     // Unimplemented: Read as 1
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
            (2 <<  0),      // HS oscillator
        PicPwmPinInfo28_2,
        arraylen(PicPwmPinInfo28_2),
        PicExtIntPinInfo28,
        arraylen(PicExtIntPinInfo28)
    },
    {
        "Microchip PIC10F200 6-SOT",
        "PIC10F200",
        "P10F200",
        "P10F200",
        "PIC10F200",
        'G',
//        A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06 }, // PORTx = GPIO
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06 }, // PORTx
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06 }, // TRISx
        256-1, //Location 00FFh contains the internal clock oscillator
               //calibration value. This value should never be overwritten.
        { { 0x10, 16 } },
        Pic6Pin_SOT23,
        arraylen(Pic6Pin_SOT23),
        nullptr,
        0,
        0,
        { },
        0,
        ISA_PIC16,
        BaselineCore12bit,
        6,
            (0 <<  4) |     // nMCLR disabled
            (1 <<  3) |     // Code protection disabled
            (0 <<  2) |     // WDTE disabled
            (0 <<  1) |     //
            (0 <<  0),      //
        nullptr,
        0
    },
//===========================================================================
    {
        "Controllino Maxi / Ext bytecode",
        "",
        "",
        "",
        "",
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
        ISA_XINTERPRETED,
        EnhancedCore4M,
        100,
        0,
        ControllinoMaxiPwmPinInfo,
        arraylen(ControllinoMaxiPwmPinInfo),
    },
    {
        "PC LPT COM",
        "",
        "",
        "",
        "",
        'L', // PC LPT & COM support
        { 0x00 },
        { 0x00 },
        { 0x00 },
        0,
        { { 0x00, int(0xffffff) } },
        PcCfg,
        arraylen(PcCfg),
        nullptr,
        0,
        0,
        { 0, 0 },
        0,
        ISA_PC,
        PC_LPT_COM,
    },
};

// clang-format on

std::vector<McuIoInfo> SupportedMcus(SupportedMcus_, SupportedMcus_ + arraylen(SupportedMcus_));

#endif //__MCUTABLE_H__
