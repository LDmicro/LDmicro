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
// Description of the intermediate code that we generate. The routines in
// intcode.cpp generate this intermediate code from the program source. Then
// either we simulate the intermediate code with the routines in simulate.cpp
// or we convert it to PIC or AVR instructions so that it can run on a
// real device.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------

#ifndef __INTCODE_H
#define __INTCODE_H

#include "compilercommon.hpp"

#define NEW_CMP        // (C) LDmicro.GitHub@gmail.com
#define TABLE_IN_FLASH // (C) LDmicro.GitHub@gmail.com
#ifdef TABLE_IN_FLASH
//#define TABLE_IN_FLASH_LINEAR // (C) LDmicro.GitHub@gmail.com
#endif
#define NEW_INT 1

// clang-format off

#define INT_SET_BIT                              1
#define INT_CLEAR_BIT                            2
#define INT_COPY_BIT_TO_BIT                      3
#define INT_COPY_NOT_BIT_TO_BIT                  301
#define INT_SET_BIT_AND_BIT                      331
#define INT_SET_BIT_OR_BIT                       332
#define INT_SET_BIT_XOR_BIT                      333
#if NEW_INT > 0
#define INT_VARIABLE_SET_BIT                     3001
#define INT_VARIABLE_CLEAR_BIT                   3002
#define INT_VARIABLE_NOT_BIT                     3003
//#define INT_VARIABLE_SET_BITS                    3006
//#define INT_VARIABLE_CLEAR_BITS                  3007
#endif
#define INT_SET_VARIABLE_TO_LITERAL              4
//#define INT_SET_VARIABLE_INDIRECT                4xxx
#define INT_SET_VARIABLE_TO_VARIABLE             5
#define INT_STRING_INIT                          5013 // store literal string in flash
#define INT_SET_VARIABLE_INDEXED                 5014 // read literal string[index] from flash
#if NEW_INT > 0
#define INT_SET_BIN2BCD                          5001
#define INT_SET_BCD2BIN                          5002
#define INT_SET_OPPOSITE                         5003
#endif
#define INT_SET_SWAP                             5004

#define INT_INC_GROUP_BEGIN                      6000
#define INT_INCREMENT_VARIABLE                   6001
#define INT_DECREMENT_VARIABLE                   6002
#define INT_INC_GROUP_END                        6010
#define INT_INC_GROUP(x) (((x) >= INT_INC_GROUP_BEGIN) && ((x) <= INT_INC_GROUP_END))

#define INT_ADD_GROUP_BEGIN                      4000
#define INT_SET_VARIABLE_ADD                     4001
#define INT_SET_VARIABLE_SUBTRACT                4002
#define INT_SET_VARIABLE_MULTIPLY                4003
#define INT_SET_VARIABLE_DIVIDE                  4004
#define INT_SET_VARIABLE_MOD                     4005
#define INT_ADD_GROUP_END                        4010
#define INT_ADD_GROUP(x) (((x) >= INT_ADD_GROUP_BEGIN) && ((x) <= INT_ADD_GROUP_END))

#if NEW_INT > 0
#define INT_COPY_VAR_BIT_TO_VAR_BIT             1021
#define INT_NOT_VAR_BIT_TO_VAR_BIT              1022
#define INT_XOR_VAR_BIT_TO_VAR_BIT              1023
#endif
#ifdef TABLE_IN_FLASH
#define INT_FLASH_INIT                          1003 // store table in flash
#define INT_FLASH_READ                          1004 // read table[index] from flash
#define INT_RAM_READ                            1005
#endif

#define INT_SLEEP                               1099
#define INT_READ_ADC                            11
#define INT_SET_PWM                             12
#define INT_UART_SEND1                          1301
#define INT_UART_SEND_READY                     1303
#define INT_UART_SEND_BUSY                      1304
#define INT_UART_WR                             1310
#define INT_UART_RECV1                          1400
#define INT_UART_RECV_AVAIL                     1403

/* EEPROM Write Cycle Time is 4ms(typical time) up to 5-8-10ms(max time) for PIC Microchip MCU.
// EEPROM Write Typ Programming Time is 8.5ms for Atmel MCU.
// LDmicro writes EEPROM according to the postpone algorithm:
// one byte per one PLC cycle without waiting inside the PLC cycle.
// The low byte begins to be written in the current PLC loop,
// the high bytes are written in the next PLC cycles
// when the EEPROM completes the previous byte writing procedure.
// The postpone algorithm needs two additional internal MCU variables. */
#define EEPROM_POSTPONE_BYTES_COUNTER "$EepromPostponeBytesCounter" // size is 1 byte
// This internal MCU variable indicates how many bytes have not yet written to the EEPROM.
#define EEPROM_POSTPONE_BYTES    "$EepromPostponeBytes"    // size is 4 byte
// This internal MCU variable stores copy of persistent variable that will be written to the EEPROM in the next PLC cycle.
//#define INT_EEPROM_BUSY_CHECK                   1501 // obsolete
#define INT_EEPROM_BUSY                         15
#ifdef NEW_FEATURE
#define INT_EEPROM_INIT                         16
#endif
#define INT_EEPROM_READ                         17
//#define INT_EEPROM_WRITE                        17 // obsolete
#define INT_EEPROM_WRITE_BYTE                   18

#define INT_SPI                                 19
#define INT_SPI_INIT                            1901 // one BYTE(8-bit) or WORD(16-bit) per PLC cycle
//#define INT_SPI_COMPLETE                        1902
//#define INT_SPI_BUSY                            1903
#define INT_SPI_WRITE                           1904 // full string per PLC cycle
//#define INT_SPI_READ                            1905

#define INT_I2C_READ                            1951  ///// Added by JG
#define INT_I2C_WRITE                           1952

#define INT_WRITE_STRING                        21 // netzer
#define INT_STRING                              22 // sprintf(), strcpy(), or "literal string"

#define INT_AND_GROUP_BEGIN                     30
#define INT_SET_VARIABLE_AND                    30
#define INT_SET_VARIABLE_OR                     31
#define INT_SET_VARIABLE_XOR                    32
#define INT_SET_VARIABLE_SHL                    33
#define INT_SET_VARIABLE_SHR                    34
#define INT_SET_VARIABLE_SR0                    35
#define INT_SET_VARIABLE_ROL                    36
#define INT_SET_VARIABLE_ROR                    37
#define INT_AND_GROUP_END                       39
#define INT_AND_GROUP(x) (((x) >= INT_AND_GROUP_BEGIN) && ((x) <= INT_AND_GROUP_END))

#define INT_SET_VARIABLE_NOT                    380
#define INT_SET_VARIABLE_NEG                    381 // a = -a

#define INT_SET_VARIABLE_RANDOM                 341
#define INT_SET_SEED_RANDOM                     342
#define INT_SET_VARIABLE_ENTROPY                343

#define INT_IF_GROUP_BEGIN                      40
#define INT_IF_BIT_SET_IN_VAR                   46
#define INT_IF_BIT_CLEAR_IN_VAR                 47
#define INT_IF_BITS_SET_IN_VAR                  48
#define INT_IF_BITS_CLEAR_IN_VAR                49
/*
#define INT_IF_GROUP(x) (((x) >= 50) && ((x) < 60))
*/
#define INT_IF_BIT_SET                          50
#define INT_IF_BIT_CLEAR                        51
#define INT_IF_BIT_EQU_BIT                      58
#define INT_IF_BIT_NEQ_BIT                      59
#ifndef NEW_CMP
#define INT_IF_VARIABLE_LES_LITERAL             52
#define INT_IF_VARIABLE_EQUALS_VARIABLE         53
#define INT_IF_VARIABLE_GRT_VARIABLE            54
#endif
#ifdef NEW_CMP
#define INT_IF_EQU                              60
#define INT_IF_NEQ                              61
#define INT_IF_LES                              62
#define INT_IF_GRT                              63
#define INT_IF_LEQ                              64
#define INT_IF_GEQ                              65
#define INT_IF_VARIABLE_LES_LITERAL             INT_IF_LES
#define INT_IF_VARIABLE_EQUALS_VARIABLE         INT_IF_EQU
#define INT_IF_VARIABLE_GRT_VARIABLE            INT_IF_GRT
#endif
#define INT_IF_GROUP_END                        70
#define INT_IF_GROUP(x) (((x) >= INT_IF_GROUP_BEGIN) && ((x) <= INT_IF_GROUP_END))

#define INT_ELSE                                60 + 100
#define INT_END_IF                              61 + 100
//
#define USE_SFR
#ifdef USE_SFR
// Special function
#define INT_READ_SFR_LITERAL                    1061
#define INT_WRITE_SFR_LITERAL                   1062
#define INT_SET_SFR_LITERAL                     1063
#define INT_CLEAR_SFR_LITERAL                   1064
#define INT_TEST_SFR_LITERAL                    1065

#define INT_READ_SFR_VARIABLE                   1066
#define INT_WRITE_SFR_VARIABLE                  1067
#define INT_SET_SFR_VARIABLE                    1068
#define INT_CLEAR_SFR_VARIABLE                  1069
#define INT_TEST_SFR_VARIABLE                   1070

#define INT_WRITE_SFR_LITERAL_L                 1071
#define INT_WRITE_SFR_VARIABLE_L                1072

#define INT_SET_SFR_LITERAL_L                   1073
#define INT_SET_SFR_VARIABLE_L                  1074

#define INT_CLEAR_SFR_LITERAL_L                 1075
#define INT_CLEAR_SFR_VARIABLE_L                1076

#define INT_TEST_SFR_LITERAL_L                  1077
#define INT_TEST_SFR_VARIABLE_L                 1078

#define INT_TEST_C_SFR_LITERAL                  1079
#define INT_TEST_C_SFR_VARIABLE                 1080
#define INT_TEST_C_SFR_LITERAL_L                1081
#define INT_TEST_C_SFR_VARIABLE_L               1082
// Special function
#endif

#define INT_PWM_OFF                             2001
#define INT_SET_NPULSE                          2002
#define INT_OFF_NPULSE                          2003
#define INT_QUAD_ENCOD                          2004

#define INT_CLRWDT                              2010
#define INT_LOCK                                2011
#define INT_DELAY                               2012

#define INT_AllocKnownAddr                      2020
#define INT_AllocFwdAddr                        2021
#define INT_FwdAddrIsNow                        2022
#define INT_LABEL                               2023
#define INT_GOTO                                2024
#define INT_GOSUB                               2025
#define INT_RETURN                              2026

#define INT_SIMULATE_NODE_STATE                  180

#define INT_COMMENT                              100

// Only used for the interpretable code.
#define INT_END_OF_PROGRAM                       255

// clang-format on

#if !defined(INTCODE_H_CONSTANTS_ONLY)
struct SeriesNode;
struct IntOp {
    int           op;
    NameArray     name1;
    NameArray     name2;
    NameArray     name3;
    NameArray     name4;
    NameArray     name5;
    NameArray     name6;
    int32_t       literal1;
    int32_t       literal2;
    int32_t       literal3; // side effect: internaly used in simulation of INT_FLASH_READ
    const int32_t * data;   // for INT_FLASH_INIT
    bool *        poweredAfter;
    bool *        workingNow;
    int           rung;     //= rungNow  //this IntOp located in rung,
    SeriesNode *  node;     //= nodeNow  //
    FileNameArray fileName; //in .c source file name
    int           fileLine; //and line in file
    bool          simulated;

    IntOp();
};

#define MAX_INT_OPS (1024 * 24)
extern std::vector<IntOp> IntCode;
extern int                ProgWriteP;
#endif // !defined(INTCODE_H_CONSTANTS_ONLY)

#endif //__INTCODE_H
/*                               exclusive  Shift   Shift
Language    NOT     AND     OR      OR      left    right
C/C++,
Java,
C#, Ruby    ~       &       |       ^       <<      >>

Pascal      not     and     or      xor     shl     shr

PL/I        INOT    IAND    IOR     IEOR

The bitwise complement is equal to the two's complement
of the value minus one. If two's complement arithmetic is used, then
    NOT x = -x - 1.

http://en.wikipedia.org/wiki/Bitwise_operation
http://en.wikipedia.org/wiki/Two%27s_complement
*/
