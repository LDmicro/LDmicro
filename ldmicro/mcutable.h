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

#include "ldconfig.h"
#include <cstdint>

//-----------------------------------------------
//See Atmel AVR Instruction set inheritance table
//https://en.wikipedia.org/wiki/Atmel_AVR_instruction_set#Instruction_set_inheritance
//and
//PIC instruction listings
//https://en.wikipedia.org/wiki/PIC_instruction_listings
typedef enum CoreTag {
    NOTHING,

    AVRcores,
    ReducedCore,
    MinimalCore,
    ClassicCore8K,
    ClassicCore128K,
    EnhancedCore8K,
    EnhancedCore128K,
    EnhancedCore4M,
    XMEGAcore,

    PICcores,
    BaselineCore12bit, // baseline PIC10F, PIC12F5xx, PIC16F5xx.
    ELANclones13bit,
    MidrangeCore14bit,         // midrange PIC12F6xx, PIC16Fxx. The mid-range core is available in the majority of devices labeled PIC12 and PIC16.
    EnhancedMidrangeCore14bit, // PIC microcontrollers with the Enhanced Mid-Range core are denoted as PIC12F1XXX and PIC16F1XXX
    PIC18HighEndCore16bit,
    PIC24_dsPICcore16bit,

    ESPcores,
    ESP8266Core,

    PCcores,
    PC_LPT_COM,
} Core;

//-----------------------------------------------
// Processor definitions. These tables tell us where to find the I/Os on
// a processor, what bit in what register goes with what pin, etc. There
// is one master SupportedMcus table, which contains entries for each
// supported microcontroller.

typedef struct McuIoPinInfoTag {
    char    port;
    int     bit;
    int     pin;
    char    pinName[MAX_NAME_LEN];
    int     ArduinoPin;
    char    ArduinoName[MAX_NAME_LEN];
    int     portN;   // 1=LPT1, 2=LPT2,... 1=COM1,...
    int     dbPin;   // in DB1..25 of PC ports
    int     ioType;  // IN=IO_TYPE_DIG_INPUT, OUT=IO_TYPE_DIG_OUTPUT of PC ports
    int     addr;    // addr of PC ports
} McuIoPinInfo;

typedef struct McuAdcPinInfoTag
{
    int     pin;
    uint8_t muxRegValue;
} McuAdcPinInfo;

typedef struct McuSpiInfoTag
{
    char     name[MAX_NAME_LEN];
    uint32_t REG_CTRL;
    uint32_t REG_STAT;
    uint32_t REG_DATA;
    int      MISO;
    int      MOSI;
    int      SCK;
    int      _SS;
} McuSpiInfo;

typedef struct McuPwmPinInfoTag
{
    int pin;
    int timer;
    //for AVR's
    int     resolution; // bits
    uint8_t maxCS;      // can be only 5 or 7 for AVR
    ////////////// n = 0...5
    /////////////// x = A or B
    uint32_t REG_OCRnxL; // or REG_OCRn          // Output Compare Register Low byte
    uint32_t REG_OCRnxH; // or 0, if not exist   // Output Compare Register High byte
    uint32_t REG_TCCRnA; // or REG_TCCRn         // Timer/Counter Control Register/s
    uint8_t  COMnx1;     // bit COMnx1 or COMn1 for REG_TCCRnA
    uint8_t  COMnx0;     // bit COMnx0 or COMn0 for REG_TCCRnA
    uint8_t  WGMa;       //                      // mask WGM3:0 for REG_TCCRnA if need
    uint32_t REG_TCCRnB; // or 0, if not exist   // Timer/Counter Control Registers
    uint8_t  WGMb;       //                      // mask WGM3:0 for REG_TCCRnB if need
    char     name[MAX_NAME_LEN];
} McuPwmPinInfo, *PMcuPwmPinInfo;

typedef struct McuExtIntPinInfoTag
{
    int pin;
} McuExtIntPinInfo;

typedef struct McuIoInfoTag
{
    const char* mcuName;
    const char* mcuList;
    const char* mcuInc; // ASM*.INC // D:\WinAVR\avr\include\avr
    const char* mcuH;   // C*.H     // D:\cvavr2\inc   // C:\Program Files\PICC\Devices
    const char* mcuH2;  // C*.H     //                 // C:\Program Files\HI-TECH Software\PICC\9.83\include
    char        portPrefix;
    uint32_t    inputRegs[MAX_IO_PORTS]; // A is 0, J is 9
    uint32_t    outputRegs[MAX_IO_PORTS];
    uint32_t    dirRegs[MAX_IO_PORTS];
    uint32_t    flashWords;
    struct
    {
        uint32_t start;
        int      len;
    } ram[MAX_RAM_SECTIONS];
    McuIoPinInfo*  pinInfo;
    int            pinCount;
    McuAdcPinInfo* adcInfo;
    int            adcCount;
    int            adcMax;
    struct
    {
        int rxPin;
        int txPin;
    } uartNeeds;
    int      pwmNeedsPin;
    int      whichIsa;
    Core     core;
    int      pins;
    uint32_t configurationWord; // only PIC

    McuPwmPinInfo* pwmInfo;
    int            pwmCount;

    McuExtIntPinInfo* ExtIntInfo;
    int               ExtIntCount;

    McuSpiInfo* spiInfo;
    int         spiCount;

    struct
    {
        uint32_t start;
        int      len;
    } rom[MAX_ROM_SECTIONS]; //EEPROM or HEI?
} McuIoInfo;

//-----------------------------------------------------------------------------
// AT90USB647( AT90USB646 should also work but I do not have hardware to test ).
extern McuIoPinInfo AvrAT90USB647_64TQFPIoPinInfo[];
extern McuAdcPinInfo AvrAT90USB647_64TQFPAdcPinInfo[];

//-----------------------------------------------------------------------------
// ATmega2560
extern McuIoPinInfo AvrAtmega2560_100TQFPIoPinInfo[];
extern McuAdcPinInfo AvrAtmega2560_100TQFPAdcPinInfo[];
//-----------------------------------------------------------------------------
// ATmega128 or ATmega64

extern McuIoPinInfo AvrAtmega128_64TQFPIoPinInfo[];
extern McuAdcPinInfo AvrAtmega128_64TQFPAdcPinInfo[];

//-----------------------------------------------------------------------------
// ATmega162

extern McuIoPinInfo AvrAtmega162IoPinInfo[];


//-----------------------------------------------------------------------------
// ATmega16 or ATmega32

extern McuIoPinInfo AvrAtmega16or32IoPinInfo[];
extern McuAdcPinInfo AvrAtmega16or32AdcPinInfo[];

//-----------------------------------------------------------------------------
// ATmega16 or ATmega32 in 44-Pin packages
extern McuIoPinInfo AvrAtmega16or32IoPinInfo44[];
extern McuAdcPinInfo AvrAtmega16or32AdcPinInfo44[];

//-----------------------------------------------------------------------------
// ATmega16U4 or ATmega32U4 in 44-Pin packages

//-----------------------------------------------------------------------------
// ATmega8 PDIP-28

extern McuIoPinInfo AvrAtmega8IoPinInfo28[];
//-----------------------------------------------------------------------------
extern McuIoPinInfo AvrAtmega8IoPinInfo[];

//-----------------------------------------------------------------------------
// ATmega328 PDIP-28

extern McuIoPinInfo AvrAtmega328IoPinInfo[];
extern McuAdcPinInfo AvrAtmega8AdcPinInfo[];


//-----------------------------------------------------------------------------
// ATtiny10 6-Pin packages

//-----------------------------------------------------------------------------
// ATtiny85 8-Pin packages
//-----------------------------------------------------------------------------
// ATmega8 32-Pin packages TQFP/QFN/MLF

extern McuIoPinInfo AvrAtmega8IoPinInfo32[];
extern McuAdcPinInfo AvrAtmega8AdcPinInfo32[];

extern McuExtIntPinInfo AvrExtIntPinInfo32[] ;

//-----------------------------------------------------------------------------
// PIC's
extern McuExtIntPinInfo PicExtIntPinInfo6[];
extern McuExtIntPinInfo PicExtIntPinInfo14[];
extern McuExtIntPinInfo PicExtIntPinInfo18[];
extern McuExtIntPinInfo PicExtIntPinInfo28[];
extern McuExtIntPinInfo PicExtIntPinInfo40[];
extern McuExtIntPinInfo PicExtIntPinInfo64[];
//-----------------------------------------------------------------------------
// ATmega328 32-Pin packages TQFP/QFN/MLF
//-----------------------------------------------------------------------------
// AVR's SPI Info Tables

extern McuSpiInfo McuSpiInfoATmega2560[];

//-----------------------------------------------------------------------------
// AVR's PWM Info Tables

extern McuPwmPinInfo AvrPwmPinInfo28_[];
extern McuPwmPinInfo AvrPwmPinInfo28[];
extern McuPwmPinInfo AvrPwmPinInfo32_[];
extern McuPwmPinInfo AvrAtmega2560PwmPinInfo[];
extern McuPwmPinInfo AvrAtmega16_32PwmPinInfo40[];
extern McuPwmPinInfo AvrPwmPinInfo40_[];
extern McuPwmPinInfo AvrAtmega162PwmPinInfo40_[];
extern McuPwmPinInfo AvrPwmPinInfo44_[];
extern McuPwmPinInfo AvrPwmPinInfo64_[];
extern McuPwmPinInfo AvrPwmPinInfo100_[];
extern McuPwmPinInfo AvrPwmPinInfo32[];
//-----------------------------------------------------------------------------
// PIC's PWM Info Tables
////     ti
//// pin mer
extern McuPwmPinInfo PicPwmPinInfo18[];
extern McuPwmPinInfo PicPwmPinInfo28_1[];
extern McuPwmPinInfo PicPwmPinInfo28_2[] ;
extern McuPwmPinInfo PicPwmPinInfo40[];
extern McuPwmPinInfo PicPwmPinInfo64[];
//-----------------------------------------------------------------------------
// ATmega164 or ATmega324 or ATmega644 or ATmega1264

extern McuIoPinInfo AvrAtmega164IoPinInfo[];
extern McuAdcPinInfo AvrAtmega164AdcPinInfo[];
//-----------------------------------------------------------------------------
// A variety of 14-Pin PICs that share the same digital IO assignment.

//-----------------------------------------------------------------------------
// A variety of 18-Pin PICs that share the same digital IO assignment.

extern McuIoPinInfo Pic18PinIoInfo[];
extern McuAdcPinInfo Pic16F819AdcPinInfo[];
//-----------------------------------------------------------------------------
// PIC16F88

extern McuIoPinInfo Pic16F88PinIoInfo[];
extern McuAdcPinInfo Pic16F88AdcPinInfo[];

//-----------------------------------------------------------------------------
// PIC16F1826, PIC16F1827
//-----------------------------------------------------------------------------
// PIC16F877, PIC16F874

extern McuIoPinInfo Pic16F877IoPinInfo[];
extern McuAdcPinInfo Pic16F877AdcPinInfo[];

//-----------------------------------------------------------------------------
// PIC16F72 28-Pin PDIP, SOIC, SSOP

//-----------------------------------------------------------------------------
// PIC16F876, PIC16F873 28-Pin PDIP, SOIC
extern McuIoPinInfo Pic16F876IoPinInfo[];
extern McuAdcPinInfo Pic16F876AdcPinInfo[];
//-----------------------------------------------------------------------------
// PIC16F887, PIC16F884

extern McuIoPinInfo Pic16F887IoPinInfo[];
extern McuAdcPinInfo Pic16F887AdcPinInfo[];

//-----------------------------------------------------------------------------
// 28-Pin PDIP, SOIC, SSOP
// PIC16F886,  PIC16F883, PIC16F882
// PIC16F1512, PIC16F1513
// PIC16F1516, PIC16F1518

extern McuIoPinInfo Pic28Pin_SPDIP_SOIC_SSOP[];
extern McuAdcPinInfo Pic16F886AdcPinInfo[];

//-----------------------------------------------------------------------------
// PIC16F1512, PIC16F1513, PIC16F1516, PIC16F1518
//-----------------------------------------------------------------------------
// PIC16F1526, PIC16F1527

//-----------------------------------------------------------------------------
// 6-Pin SOT-23
// PIC10F200, PIC10F202, PIC10F204, PIC10F206
// PIC10F220, PIC10F222
extern McuIoPinInfo Pic6Pin_SOT23[];

//-----------------------------------------------------------------------------
// 8-Pin PDIP, SOIC, DFN-S, DFN
// PIC12F675, PIC12F629
//-----------------------------------------------------------------------------
// ESP8266
//-----------------------------------------------------------------------------
// Controllino Maxi
extern McuIoPinInfo ControllinoMaxiIoPinInfo[];
extern McuAdcPinInfo ControllinoMaxiAdcPinInfo[];
extern McuPwmPinInfo ControllinoMaxiPwmPinInfo[];
//-----------------------------------------------------------------------------
// PC LPT & COM
extern McuIoPinInfo PcCfg[];

//===========================================================================
extern McuIoInfo SupportedMcus[];

uint32_t supportedMcuLen();
uint32_t pcCfgLen();

#endif //__MCUTABLE_H__
