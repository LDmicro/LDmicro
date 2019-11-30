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
#ifndef __MCUTABLE_HPP__
#define __MCUTABLE_HPP__

#include "stdafx.h"

#include "ldconfig.h"

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
    AVRcores_, // end of AVRcores

    PICcores,
    BaselineCore12bit, // baseline PIC10F, PIC12F5xx, PIC16F5xx.
    ELANclones13bit,
    MidrangeCore14bit, // midrange PIC12F6xx, PIC16Fxx. The mid-range core is available in the majority of devices labeled PIC12 and PIC16.
    EnhancedMidrangeCore14bit, // PIC microcontrollers with the Enhanced Mid-Range core are denoted as PIC12F1XXX and PIC16F1XXX
    PIC18HighEndCore16bit,
    PIC24_dsPICcore16bit,
    PICcores_, // end of PICcores

    ESPcores,
    ESP8266Core,
    ESPcores_, // end of ESPcores

    ARMcores,
    CortexF1,
    CortexF2,
    CortexF3,
    CortexF4,
    ARMcores_, // end of ARMcores

    PCcores,
    PC_LPT_COM,
} Core;

//-----------------------------------------------
// Processor definitions. These tables tell us where to find the I/Os on
// a processor, what bit in what register goes with what pin, etc. There
// is one master SupportedMcus table, which contains entries for each
// supported microcontroller.

typedef struct McuIoPinInfoTag {
    char port;
    int  bit;
    int  pin;
    char pinName[MAX_NAME_LEN];
    int  ArduinoPin;
    char ArduinoName[MAX_NAME_LEN];
    int  portN;  // 1=LPT1, 2=LPT2,... 1=COM1,...
    int  dbPin;  // in DB1..25 of PC ports
    int  ioType; // IN=IO_TYPE_DIG_INPUT, OUT=IO_TYPE_DIG_OUTPUT of PC ports
    int  addr;   // addr of PC ports
} McuIoPinInfo;

typedef struct McuAdcPinInfoTag {
    int     pin;
    uint8_t muxRegValue;
} McuAdcPinInfo;

typedef struct McuSpiInfoTag {
    char     name[MAX_NAME_LEN];
    uint32_t REG_CTRL;
    uint32_t REG_STAT;
    uint32_t REG_DATA;
    int      MISO;
    int      MOSI;
    int      SCK;
    int      _SS;
    //  bool    isUsed;
} McuSpiInfo;

typedef struct McuI2cInfoTag {
    char     name[MAX_NAME_LEN];
    uint32_t REG_CTRL;
    uint32_t REG_STAT;
    uint32_t REG_DATA;
    uint32_t REG_RATE;
    int      SCL;
    int      SDA;
    //  bool    isUsed;
} McuI2cInfo;

typedef struct McuPwmPinInfoTag {
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

typedef struct McuExtIntPinInfoTag {
    int pin;
} McuExtIntPinInfo;

typedef struct McuIoInfoTag {
    const char *mcuName;
    const char *deviceName; // DEVICE or PART_NAME or -chip or Alias list
    const char *mcuInc;     // ASM*.INC // D:\WinAVR\avr\include\avr
    const char *mcuH;       // C*.H     // D:\cvavr2\inc   // C:\Program Files\PICC\Devices
    const char *mcuH2;      // C*.H     //                 // C:\Program Files\HI-TECH Software\PICC\9.83\include
    char        portPrefix;
    uint32_t    inputRegs[MAX_IO_PORTS]; // A is 0, J is 9
    uint32_t    outputRegs[MAX_IO_PORTS];
    uint32_t    dirRegs[MAX_IO_PORTS];
    uint32_t    flashWords;
    struct {
        uint32_t start;
        int      len;
    } ram[MAX_RAM_SECTIONS];
    McuIoPinInfo * pinInfo;
    uint32_t       pinCount;
    McuAdcPinInfo *adcInfo;
    uint32_t       adcCount;
    uint32_t       adcMax;
    struct {
        int rxPin;
        int txPin;
    } uartNeeds;
//    int      pwmNeedsPin; // obsolete
    int      whichIsa;
    Core     core;
    int      pins;
    uint32_t configurationWord; // only PIC

    McuPwmPinInfo *pwmInfo;
    uint32_t       pwmCount;

    McuExtIntPinInfo *ExtIntInfo;
    uint32_t          ExtIntCount;

    McuSpiInfo *spiInfo;
    uint32_t    spiCount;

    McuI2cInfo       *i2cInfo;              
    uint32_t          i2cCount;           

    struct {
        uint32_t start;
        int      len;
    } rom[MAX_ROM_SECTIONS]; //EEPROM or HEI?
} McuIoInfo;

#ifndef arraylen

#if LD_WITH_CONSTEXPR
namespace {
    template <class T, uint32_t N> constexpr uint32_t arraylen(T (&)[N])
    {
        return N;
    }
} // namespace
#else
#define arraylen(x) (sizeof(x) / sizeof((x)[0]))
#endif

#endif

std::vector<McuIoInfo> &supportedMcus();
bool                    fillPcPinInfos();

#endif //__MCUTABLE_HPP__
