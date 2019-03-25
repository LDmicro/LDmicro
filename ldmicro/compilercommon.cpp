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
// Routines common to the code generators for all processor architectures.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "intcode.h"
#include "compilercommon.hpp"

// Assignment of the internal relays to memory, efficient, one bit per
// relay.
static struct {
    char  name[MAX_NAME_LEN];
    DWORD addr;
    int   bit;
    bool  assignedTo;
} InternalRelays[MAX_IO];
static int InternalRelayCount;

// Assignment of the `variables,' used for timers, counters, arithmetic, and
// other more general things. Allocate 2 octets (16 bits) per.
// Allocate 1 octets for  8-bits variables.
// Allocate 3 octets for  24-bits variables.
struct VariablesList {
    // vvv from compilercommon.cpp
    char    name[MAX_NAME_LEN];
    DWORD   addrl;
    int     Allocated;  // the number of bytes allocated in the MCU SRAM for variable
    int     SizeOfVar;  // SizeOfVar can be less than Allocated
    // ^^^ from compilercommon.cpp
    int     type;       // see PlcProgramSingleIo
    // vvv from simulate.cpp
    //  SDWORD  val;        // value in simulation mode.
    //  char    valstr[MAX_COMMENT_LEN]; // value in simulation mode for STRING types.
    //  DWORD   usedFlags;  // in simulation mode.
    //  int     initedRung; // Variable inited in rung.
    //  DWORD   initedOp;   // Variable inited in Op number.
    //  char    rungs[MAX_COMMENT_LEN]; // Rungs, where variable is used.
    // ^^^ from simulate.cpp
};

static std::array<VariablesList, MAX_IO> Variables;
static int    VariableCount = 0;

#define NO_MEMORY 0xffffffff
static DWORD NextBitwiseAllocAddr;
static int   NextBitwiseAllocBit;
static int   MemOffset;
DWORD        RamSection;
DWORD        RomSection;

int CompileFailure= 0;      ///// added by JG

//-----------------------------------------------------------------------------
static LabelAddr LabelAddrArr[MAX_RUNGS];
static int LabelAddrCount = 0;

LabelAddr * GetLabelAddr(const NameArray& name)
{
    if(name.length() == 0)
        oops();
    int i;
    for(i = 0; i < LabelAddrCount; i++) {
        if(strcmp(name.c_str(), LabelAddrArr[i].name) == 0)
            break;
    }
    if(i >= MAX_RUNGS) {
        Error(_("Labels limit '%d' exceeded!"), MAX_RUNGS);
    }
    if(i == LabelAddrCount) {
        LabelAddrCount++;
        memset(&LabelAddrArr[i], 0, sizeof(LabelAddrArr[i]));
        strcpy(LabelAddrArr[i].name, name.c_str());
    }
    return &(LabelAddrArr[i]);
}

//-----------------------------------------------------------------------------

int UsedROM()
{
    if(!Prog.mcu())
        return 0;

    int n = 0;
    for(uint32_t i = 0; i < RomSection; i++) {
        n += Prog.mcu()->rom[i].len;
    }
    return n + EepromAddrFree;
}

int UsedRAM()
{
    if(!Prog.mcu())
        return 0;

    int n = 0;
    for(uint32_t i = 0; i < RamSection; i++) {
        n += Prog.mcu()->ram[i].len;
    }
    return n + MemOffset;
}

//-----------------------------------------------------------------------------
void PrintVariables(FileTracker& f)
{
    fprintf(f, "\n");
    fprintf(f,
            ";|  # | Name                                                    | Size      | Address      | Bit # |\n");

    fprintf(f, ";|Variables: %d\n", VariableCount);
    for(int i = 0; i < VariableCount; i++) {
        if(Variables[i].addrl) {
            fprintf(f,
                    ";|%3d | %-50s\t| %3d byte  | 0x%04X       |\n",
                    i,
                    Variables[i].name,
                    Variables[i].SizeOfVar,
                    Variables[i].addrl);
        }
        /*
        else {
            DWORD addr;
            int   bit;
            bool forRead;
            forRead = false;
            switch(Variables[i].name[0]) {
                case 'I':
                case 'X':
                    forRead = true;
                    break;
                default:
                    break;
            }
            MemForSingleBit(Variables[i].name, forRead, &addr, &bit);
            fprintf(f, ";|%3d %-50s\t| %3d bit   | 0x%04X = %3d | %d     |\n", i, Variables[i].name, 1, addr, addr, bit);
        }
        */
    }
    fprintf(f, "\n");

    fprintf(f, ";|Internal Relays: %d\n", InternalRelayCount);
    for(int i = 0; i < InternalRelayCount; i++) {
        fprintf(f,
                ";|%3d | %-50s\t| %3d bit   | 0x%04X       | %d     |\n",
                i,
                InternalRelays[i].name,
                1,
                InternalRelays[i].addr,
                InternalRelays[i].bit);
    }
    fprintf(f, "\n");
}
//-----------------------------------------------------------------------------
static void ClrInternalData()
{
    MemOffset = 0;
    RamSection = 0;
    RomSection = 0;
    EepromAddrFree = 0;
    LabelAddrCount = 0;
    //  VariableCount = 0;
    for(int i = 0; i < VariableCount; i++) {
        Variables[i].Allocated = 0;
        Variables[i].addrl = 0;
    }
}
//-----------------------------------------------------------------------------
// Forget what memory has been allocated on the target, so we start from
// everything free.
//-----------------------------------------------------------------------------
void AllocStart()
{
    NextBitwiseAllocAddr = NO_MEMORY;
    InternalRelayCount = 0;
    ClrInternalData();
    ClrSimulationData();
}

//-----------------------------------------------------------------------------
// Return the address of a previously unused octet of RAM on the target, or
// signal an error if there is no more available.
//-----------------------------------------------------------------------------
DWORD AllocOctetRam(int bytes) // The desired number of bytes.
{
    if(!Prog.mcu())
        return 0;

    if(Prog.mcu()->whichIsa > ISA_HARDWARE)
        return 0;

    if((MemOffset + bytes) >= Prog.mcu()->ram[RamSection].len) {
        RamSection++;
        MemOffset = 0;
    }

    if((RamSection >= MAX_RAM_SECTIONS) || ((MemOffset + bytes) >= Prog.mcu()->ram[RamSection].len)) {
        THROW_COMPILER_EXCEPTION_FMT("%s %s",
                                     _("RAM:"),
                                     _("Out of memory; simplify program or choose microcontroller with more memory."));
    }

    MemOffset += bytes;
    return Prog.mcu()->ram[RamSection].start + MemOffset - bytes;
}

DWORD AllocOctetRam()
{
    return AllocOctetRam(1);
}

//-----------------------------------------------------------------------------
int InputRegIndex(DWORD addr)
{
    if((addr == std::numeric_limits<DWORD>::max()) || (addr == 0))
        oops();
    for(int i = 0; i < MAX_IO_PORTS; i++)
        if(Prog.mcu()->inputRegs[i] == addr)
            return i;
    oops();
    return -1;
}

//-----------------------------------------------------------------------------
int OutputRegIndex(DWORD addr)
{
    if((addr == std::numeric_limits<DWORD>::max()) || (addr == 0))
        oops();
    for(int i = 0; i < MAX_IO_PORTS; i++)
        if(Prog.mcu()->outputRegs[i] == addr)
            return i;
    oops();
    return -1;
}

//-----------------------------------------------------------------------------
// Return the address (octet address) and bit of a previously unused bit of
// RAM on the target.
//-----------------------------------------------------------------------------
void AllocBitRam(DWORD *addr, int *bit)
{
    if(NextBitwiseAllocAddr != NO_MEMORY) {
        *addr = NextBitwiseAllocAddr;
        *bit = NextBitwiseAllocBit;
        NextBitwiseAllocBit++;
        if(NextBitwiseAllocBit > 7) {
            NextBitwiseAllocAddr = NO_MEMORY;
        }
        return;
    }

    *addr = AllocOctetRam();
    *bit = 0;
    NextBitwiseAllocAddr = *addr;
    NextBitwiseAllocBit = 1;
}

//-----------------------------------------------------------------------------
// Return the address (octet) and bit of the bit in memory that represents the
// given input or output pin. Raises an internal error if the specified name
// is not present in the I/O list or a user error if a pin has not been
// assigned to that I/O name. Will allocate if it no memory allocated for it
// yet, else will return the previously allocated bit.
//-----------------------------------------------------------------------------
static void MemForPin(const NameArray& name, DWORD *addr, int *bit, bool asInput)
{
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, name.c_str()) == 0)
            break;
    }
    if(i >= Prog.io.count)
        oops();

    if(asInput && Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT)
        oops();
    if(!asInput && Prog.io.assignment[i].type != IO_TYPE_DIG_OUTPUT && Prog.io.assignment[i].type != IO_TYPE_PWM_OUTPUT)
        oops();

    *addr = -1;
    *bit = -1;
    if(Prog.mcu()) {
        McuIoPinInfo *iop = PinInfo(Prog.io.assignment[i].pin);
        if(iop) {
            if(Prog.mcu()->core == PC_LPT_COM) {
                *addr = iop->addr;
                *bit = iop->bit;
            } else {
                if(asInput) {
                    *addr = Prog.mcu()->inputRegs[iop->port - 'A'];
                } else {
                    *addr = Prog.mcu()->outputRegs[iop->port - 'A'];
                }
                *bit = iop->bit;
            }
        } else {
            THROW_COMPILER_EXCEPTION_FMT(_("Must assign pins for all I/O.\r\n\r\n'%s' is not assigned."), name.c_str());
        }
    }
}

void AddrBitForPin(int pin, DWORD *addr, int *bit, bool asInput)
{
    *addr = -1;
    *bit = -1;
    if(Prog.mcu()) {
        McuIoPinInfo *iop = PinInfo(pin);
        if(iop) {
            if(asInput) {
                *addr = Prog.mcu()->inputRegs[iop->port - 'A'];
            } else {
                *addr = Prog.mcu()->outputRegs[iop->port - 'A'];
            }
            *bit = iop->bit;
        } else {
            if(pin)
                THROW_COMPILER_EXCEPTION_FMT(_("Not described pin %d "), pin);
        }
    }
}

//-----------------------------------------------------------------------------
int SingleBitAssigned(const NameArray& name)
{
    int pin = 0;
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, name.c_str()) == 0)
            break;
    }
    if(i >= Prog.io.count)
        oops();

    if(Prog.mcu()) {
        pin = Prog.io.assignment[i].pin;
        auto pp = std::find_if(Prog.mcu()->pinInfo,
                               Prog.mcu()->pinInfo + Prog.mcu()->pinCount,
                               [pin](const McuIoPinInfo &info) { return pin == info.pin; });
        if(pp == (Prog.mcu()->pinInfo + Prog.mcu()->pinCount))
            pin = 0;
    }
    return pin;
}

//-----------------------------------------------------------------------------
int GetAssignedType(const NameArray& name, const NameArray& fullName)
{
    int type = NO_PIN_ASSIGNED;
    if(fullName.length())
        if(fullName[0] == 'I') {
            if(fullName[1] == 'b')
                return IO_TYPE_INTERNAL_RELAY;
            else if(fullName[1] == 'i')
                return IO_TYPE_GENERAL;
            else
                oops();
        }
    for(int i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, name.c_str()) == 0) {
            type = Prog.io.assignment[i].type;
            break;
        }
    }
    return type;
}

//-----------------------------------------------------------------------------
// Determine the mux register settings to read a particular ADC channel.
//-----------------------------------------------------------------------------
uint8_t MuxForAdcVariable(const NameArray& name)
{
    int res = 0;
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if((strcmp(Prog.io.assignment[i].name, name.str()) == 0) &&
            (Prog.io.assignment[i].type == IO_TYPE_READ_ADC))
            break;
    }
    if(i >= Prog.io.count)
        oops();

    if(Prog.mcu()) {
        uint32_t j;
        for(j = 0; j < Prog.mcu()->adcCount; j++) {
            if(Prog.mcu()->adcInfo[j].pin == Prog.io.assignment[i].pin) {
                break;
            }
        }
        if(j == Prog.mcu()->adcCount) {
            /////   Error("i=%d pin=%d", i, Prog.io.assignment[i].pin);         ///// Comment by JG
            THROW_COMPILER_EXCEPTION_FMT(_("Must assign pins for all ADC inputs (name '%s')."), name.c_str());
            return 0;
        }
        res = Prog.mcu()->adcInfo[j].muxRegValue;
    }

    return res;
}

//-----------------------------------------------------------------------------
// Added by JG to force SPI pins assignment
//-----------------------------------------------------------------------------
int PinsForSpiVariable(const NameArray& name, int n, char *spipins)
{
    int res = 0, port= 0;
    int i;

    if(!Prog.mcu()) return 0;
    if(!spipins) return 0;

    for(i = 0; i < Prog.io.count; i++)
    {
        if(strncmp(Prog.io.assignment[i].name, name.c_str(), n) == 0)
        {
            if (Prog.io.assignment[i].type == IO_TYPE_SPI_MOSI)
            {
                for(uint32_t j = 0; j < Prog.mcu()->spiCount; j++)
                    if((name == Prog.mcu()->spiInfo[j].name) &&
                        (Prog.mcu()->spiInfo[j].MOSI == Prog.io.assignment[i].pin))
                    {
                        McuIoPinInfo *iop = PinInfo(Prog.io.assignment[i].pin);
                        port= iop->port;                        // all SPI pins supposed on same port
                        spipins[0]= iop->bit;
                        res++; break;
                    }
            }
            if (Prog.io.assignment[i].type == IO_TYPE_SPI_MISO)
            {
                for(uint32_t j = 0; j < Prog.mcu()->spiCount; j++)
                    if((name == Prog.mcu()->spiInfo[j].name) &&
                        (Prog.mcu()->spiInfo[j].MISO == Prog.io.assignment[i].pin))
                    {
                        McuIoPinInfo *iop = PinInfo(Prog.io.assignment[i].pin);
                        spipins[1]= iop->bit;
                        res++; break;
                    }
            }
            if (Prog.io.assignment[i].type == IO_TYPE_SPI_SCK)
            {
                for(uint32_t j = 0; j < Prog.mcu()->spiCount; j++)
                    if((name == Prog.mcu()->spiInfo[j].name) &&
                        (Prog.mcu()->spiInfo[j].SCK == Prog.io.assignment[i].pin))
                    {
                        McuIoPinInfo *iop = PinInfo(Prog.io.assignment[i].pin);
                        spipins[2]= iop->bit;
                        res++; break;
                    }
            }
            if (Prog.io.assignment[i].type == IO_TYPE_SPI__SS)
            {
                for(uint32_t j = 0; j < Prog.mcu()->spiCount; j++)
                    if((name == Prog.mcu()->spiInfo[j].name) &&
                        (Prog.mcu()->spiInfo[j]._SS == Prog.io.assignment[i].pin))
                    {
                        McuIoPinInfo *iop = PinInfo(Prog.io.assignment[i].pin);
                        spipins[3]= iop->bit;
                        res++; break;
                    }
            }
        }
    }

    if(res != 4)
    {
        THROW_COMPILER_EXCEPTION_FMT(_("Must assign pins for SPI device (name '%s')."), name.c_str());
    }
    return port;        // spi port
}


//-----------------------------------------------------------------------------
// Added by JG to force I2C pins assignment
//-----------------------------------------------------------------------------
int PinsForI2cVariable(const NameArray& name, int n, char *i2cpins)
{
    int res = 0, port= 0;
    int i;

    if(!Prog.mcu()) return 0;
    if(!i2cpins) return 0;

    for(i = 0; i < Prog.io.count; i++)
    {
        if(strncmp(Prog.io.assignment[i].name, name.c_str(), n) == 0)
        {
            if (Prog.io.assignment[i].type == IO_TYPE_I2C_SCL)
            {
                for(uint32_t j = 0; j < Prog.mcu()->i2cCount; j++)
                    if((name == Prog.mcu()->i2cInfo[j].name) &&
                        (Prog.mcu()->i2cInfo[j].SCL == Prog.io.assignment[i].pin))
                    {
                        McuIoPinInfo *iop = PinInfo(Prog.io.assignment[i].pin);
                        port= iop->port;                        // all I2C pins supposed on same port
                        i2cpins[0]= iop->bit;
                        res++; break;
                    }
            }
            if (Prog.io.assignment[i].type == IO_TYPE_I2C_SDA)
            {
                for(uint32_t j = 0; j < Prog.mcu()->i2cCount; j++)
                    if((name == Prog.mcu()->i2cInfo[j].name) &&
                        (Prog.mcu()->i2cInfo[j].SDA == Prog.io.assignment[i].pin))
                    {
                        McuIoPinInfo *iop = PinInfo(Prog.io.assignment[i].pin);
                        i2cpins[1]= iop->bit;
                        res++; break;
                    }
            }
        }
    }

    if(res != 2)
    {
        THROW_COMPILER_EXCEPTION_FMT(_("Must assign pins for I2C device (name '%s')."), name.c_str());
    }
    return port;        // i2c port
}


//-----------------------------------------------------------------------------
int byteNeeded(long long int i)
{
    if((-128 <= i) && (i <= 127))
        return 1;
    else if((-32768 <= i) && (i <= 32767))
        return 2;
    else if((-8388608 <= i) && (i <= 8388607))
        return 3;
    else if((-2147483648LL <= i) && (i <= 2147483647LL))
        return 4; // not FULLY implemented for LDmicro
    oops();
    return 0;
}

//-----------------------------------------------------------------------------
int TestByteNeeded(int count, SDWORD *vals)
{
    int res = -1;
    int r;
    for(int i = 0; i < count; i++) {
        r = byteNeeded(vals[i]);
        if(res < r)
            res = r;
    }
    return res;
}

//-----------------------------------------------------------------------------
// Allocate 1,2,3 or 4 byte for a variable, used for a variety of purposes.
//-----------------------------------------------------------------------------
int MemForVariable(const NameArray& name, DWORD *addrl, int sizeOfVar)
{
    if(strlenalnum(name.c_str()) == 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Empty variable name '%s'.\nrungNow=%d"), name.c_str(), rungNow + 1);
    }

    int i;
    for(i = 0; i < VariableCount; i++) {
        if((Variables[i].type == IO_TYPE_TABLE_IN_FLASH) || (Variables[i].type == IO_TYPE_VAL_IN_FLASH))
            Variables[i].Allocated = Variables[i].SizeOfVar;
    }
    for(i = 0; i < VariableCount; i++) {
        if(name == Variables[i].name)
            break;
    }
    if(i >= MAX_IO) {
        THROW_COMPILER_EXCEPTION(_("Internal limit exceeded (number of vars)"));
    }
    if(i == VariableCount) {
        VariableCount++;
        memset(&Variables[i], 0, sizeof(Variables[i]));
        strcpy(Variables[i].name, name.c_str());
        if(name[0] == '#') {
            Variables[i].SizeOfVar = 1;
        }
    }
    if(sizeOfVar < 0) { // get addr, get size
        if(addrl)
            *addrl = Variables[i].addrl;

    } else if(sizeOfVar > 0) { // set size, set addr
        if(Variables[i].SizeOfVar == sizeOfVar) {
            // not need
        } else if(Variables[i].Allocated == 0) {
            //dbp("Size %d set to %d for var '%s'", Variables[i].SizeOfVar, sizeOfVar, name);
            Variables[i].SizeOfVar = sizeOfVar;
        } else {
            Variables[i].SizeOfVar = sizeOfVar;
            if(Variables[i].Allocated >= sizeOfVar) {
            } else {
                Variables[i].Allocated = 0; // Request to reallocate memory of var
            }
        }
        if(addrl) {
            Variables[i].addrl = *addrl;
        }
    } else { // if(sizeOfVar == 0) // if(addrl) { Allocate SRAM }
        if(name[0] == '#') {
            DWORD addr = 0xff;
            if(IsNumber(&name[1])) {
                addr = hobatoi(&name[1]);

                if((addr == 0xff) || (addr == 0)) {
                    dbps("Not a FSR");
                    //Error("Not a FSR");
                } else {
                    if(Variables[i].Allocated == 0) {
                        Variables[i].addrl = addr;
                    }
                    Variables[i].Allocated = 1;
                }
            } else {
                int j = name[name.length() - 1] - 'A';
                if((j >= 0) && (j < MAX_IO_PORTS)) {
                    if((strstr(name.c_str(), "#PORT")) && (name.length() == 6)) { // #PORTx
                        if(IS_MCU_REG(j)) {
                            addr = Prog.mcu()->outputRegs[j];
                        }
                    }
                    if((strstr(name.c_str(), "#PIN")) && (name.length() == 5)) { // #PINx
                        if(IS_MCU_REG(j)) {
                            addr = Prog.mcu()->inputRegs[j];
                        }
                    }
                    if((strstr(name.c_str(), "#TRIS")) && (name.length() == 6)) { // #TRISx
                        if(IS_MCU_REG(j)) {
                            addr = Prog.mcu()->dirRegs[j];
                        }
                    }
                }
                /*
                if((addr == 0xff) || (addr == 0)) {
                    return MemForVariable(&name[1], addrl);
                }
                */
                if((addr == 0xff) || (addr == 0)) {
                    //dbps("Not a PORT/PIN");
                    //Error(_("Not a #PORT/#PIN/#TRIS/%s "), name);
                } else {
                    if(Variables[i].Allocated == 0) {
                        Variables[i].addrl = addr;
                    }
                    Variables[i].Allocated = 1;
                }
            }
        } else {
            if((sizeOfVar < 1) || (sizeOfVar > 4)) {
                sizeOfVar = Variables[i].SizeOfVar;
            }
            if((sizeOfVar < 1) || (sizeOfVar > 4)) {
                sizeOfVar = 2;
            }
            if(sizeOfVar < 1) {
                THROW_COMPILER_EXCEPTION_FMT(_("Size of var '%s'(%d) reset as signed 8 bit variable."), name.c_str(), sizeOfVar);
                sizeOfVar = 1;
            } else if(sizeOfVar > 4) {
                THROW_COMPILER_EXCEPTION_FMT(_("Size of var '%s'(%d) reset as signed 32 bit variable."), name.c_str(), sizeOfVar);
                sizeOfVar = 4;
            }
            if(Variables[i].SizeOfVar != sizeOfVar) {
                if(!Variables[i].SizeOfVar)
                    Variables[i].SizeOfVar = sizeOfVar;
                else {
                    // Error("no Resize %s %d %d", name, Variables[i].SizeOfVar, sizeOfVar);
                }
            }
            if(addrl) {
                if(Variables[i].Allocated == 0) {
                    if(sizeOfVar == 1) {
                        Variables[i].addrl = AllocOctetRam();
                    } else if(sizeOfVar == 2) {
                        Variables[i].addrl = AllocOctetRam(2);
                    } else if(sizeOfVar == 3) {
                        Variables[i].addrl = AllocOctetRam(3);
                    } else if(sizeOfVar == 4) {
                        Variables[i].addrl = AllocOctetRam(4);
                    } else {
                        THROW_COMPILER_EXCEPTION_FMT(_("Var '%s' not allocated %d."), name.c_str(), sizeOfVar);
                    }
                    Variables[i].Allocated = sizeOfVar;

                    if(Variables[i].SizeOfVar < sizeOfVar) {
                        //dbp("Err: Allocated '%s', upsize %d set to %d", name, Variables[i].SizeOfVar, sizeOfVar);
                    }

                } else if(Variables[i].Allocated != sizeOfVar) {
                    //Warning(" Variable '%s' already allocated as signed %d bit", Variables[i].name, Variables[i].Allocated*8);
                    //CompileError();
                }
            }
        }
        if(addrl)
            *addrl = Variables[i].addrl;
    }
    return Variables[i].SizeOfVar;
}

int MemForVariable(const NameArray& name, DWORD *addr)
{
    return MemForVariable(name, addr, 0);
}

//-----------------------------------------------------------------------------
static int MemOfVar(const char *name, DWORD *addr)
{
    MemForVariable(name, addr, -1); //get WORD memory for pointer to LPM
    return SizeOfVar(name);         //and return size of element of table in flash memory
}

int MemOfVar(const NameArray &name, DWORD *addr)
{
    return MemOfVar(name.c_str(), addr);
}

int SetMemForVariable(const NameArray &name, DWORD addr, int sizeOfVar)
{
    MemForVariable(name, &addr, sizeOfVar); //allocate WORD memory for pointer to LPM

    return MemForVariable(name, nullptr, sizeOfVar); //and set size of element of table in flash memory
}

//-----------------------------------------------------------------------------
int SetSizeOfVar(const NameArray &name, int sizeOfVar, bool showError)
{
    if(showError)
        if((sizeOfVar < 1)/* || (4 < sizeOfVar)*/) {
            Warning(_("Invalid size (%d) of variable '%s' set to 2!"), sizeOfVar, name.c_str());
            sizeOfVar = 2;
        }
#ifndef NEW_CMP
    sizeOfVar = 2;
#endif
    return MemForVariable(name, nullptr, sizeOfVar);
}

int SetSizeOfVar(const NameArray &name, int sizeOfVar)
{
    return SetSizeOfVar(name, sizeOfVar, true);
}

int SizeOfVar(const NameArray &name)
{
    if(IsNumber(name))
        return byteNeeded(hobatoi(name.c_str()));
    else
        return MemForVariable(name, nullptr, 0);
}

//-----------------------------------------------------------------------------
int GetVariableType(const NameArray &name)
{
    if(strlenalnum(name.c_str()) == 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Empty variable name '%s'.\nrungNow=%d"), name.c_str(), rungNow + 1);
    }

    int i;
    for(i = 0; i < VariableCount; i++) {
        if(name == Variables[i].name)
            break;
    }
    if(i >= MAX_IO) {
        THROW_COMPILER_EXCEPTION(_("Internal limit exceeded (number of vars)"));
    }
    if(i < VariableCount) {
        return Variables[i].type;
    }
    return IO_TYPE_PENDING;
}

int SetVariableType(const NameArray &name, int type)
{
    if(strlenalnum(name.c_str()) == 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Empty variable name '%s'.\nrungNow=%d"), name.c_str(), rungNow + 1);
    }
    int i;
    for(i = 0; i < VariableCount; i++) {
        if(name == Variables[i].name)
            break;
    }
    if(i >= MAX_IO) {
        THROW_COMPILER_EXCEPTION(_("Internal limit exceeded (number of vars)"));
    }
    if(i == VariableCount) {
        VariableCount++;
        memset(&Variables[i], 0, sizeof(Variables[i]));
        strcpy(Variables[i].name, name.c_str());
        if(name[0] == '#') {
            Variables[i].SizeOfVar = 1;
            Variables[i].Allocated = 0;
        } else {
            Variables[i].SizeOfVar = 0;
            Variables[i].Allocated = 0;
        }
        Variables[i].type = type;
    } else {
        if(Variables[i].type == IO_TYPE_PENDING) {
            Variables[i].type = type;
        } else {
            if((Variables[i].type == IO_TYPE_COUNTER) && (type == IO_TYPE_GENERAL)) {
                /*skip*/;
            } else if((Variables[i].type == IO_TYPE_GENERAL) && (type == IO_TYPE_COUNTER)) {
                Variables[i].type = type; // replace // iolist.cpp
            } else {
            }
        }
    }
    return Variables[i].type;
}
//-----------------------------------------------------------------------------

int AllocOfVar(const NameArray &name)
{
    if(strlenalnum(name.c_str()) == 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Empty variable name '%s'.\nrungNow=%d"), name.c_str(), rungNow + 1);
    }

    int i;
    for(i = 0; i < VariableCount; i++) {
        if(name == Variables[i].name)
            break;
    }
    if(i >= MAX_IO) {
        THROW_COMPILER_EXCEPTION(_("Internal limit exceeded (number of vars)"));
    }
    if(i < VariableCount) {
        return Variables[i].Allocated;
    }
    return 0;
}

//-----------------------------------------------------------------------------
void SaveVarListToFile(FileTracker& f)
{
    std::sort(std::begin(Variables), std::end(Variables),
              [](const VariablesList& a, const VariablesList& b) {return (strcmp(a.name, b.name) < 0);});

    for(int i = 0; i < VariableCount; i++) {
        if(!IsIoType(Variables[i].type) && (Variables[i].type != IO_TYPE_INTERNAL_RELAY)
           && (Variables[i].name[0] != '$')) {
            fprintf(f,
                    "  %3d bytes %s%s\n",
                    SizeOfVar(Variables[i].name),
                    Variables[i].name,
                    Variables[i].Allocated ? "" : _(" \tNow not used !!!"));
        }
    }
}

//-----------------------------------------------------------------------------
bool LoadVarListFromFile(FileTracker& f)
{
    //ClrInternalData(); // VariableCount = 0;
    //ClrSimulationData();
    AllocStart();
    char line[MAX_NAME_LEN];
    char name[MAX_NAME_LEN];
    int  sizeOfVar;
    bool Ok;

    while(fgets(line, sizeof(line), f)) {
        if(!strlen(strspace(line)))
            continue;
        if(strcmp(line, "END") == 0) {
            return true;
        }
        Ok = false;
        // Don't internationalize this! It's the file format, not UI.
        if(sscanf(line, " %s signed %d bit variable ", name, &sizeOfVar) == 2) {
            if((sizeOfVar > 0) && strlen(name)) {
                SetSizeOfVar(name, sizeOfVar / 8);
                Ok = true;
            }
        }
        if(sscanf(line, " %d bytes %s ", &sizeOfVar, name) == 2) {
            if((sizeOfVar > 0) && strlen(name)) {
                SetSizeOfVar(name, sizeOfVar, false);
                Ok = true;
            }
        }
        if(!Ok) {
            THROW_COMPILER_EXCEPTION_FMT(_("Error reading 'VAR LIST' section from .ld file!\nError in line:\n'%s'."), strspacer(line));
            return false;
        }
    }
    return false;
}
//-----------------------------------------------------------------------------
// Allocate or retrieve the bit of memory assigned to an internal relay or
// other thing that requires a single bit of storage.
//-----------------------------------------------------------------------------
static void MemForBitInternal(const NameArray& name, DWORD *addr, int *bit, bool writeTo)
{
    int i;
    for(i = 0; i < InternalRelayCount; i++) {
        if(strcmp(name.c_str(), InternalRelays[i].name) == 0)
            break;
    }
    if(i >= MAX_IO) {
        THROW_COMPILER_EXCEPTION(_("Internal limit exceeded (number of relay)"));
    }
    if(i == InternalRelayCount) {
        InternalRelayCount++;
        strcpy(InternalRelays[i].name, name.c_str());
        AllocBitRam(&InternalRelays[i].addr, &InternalRelays[i].bit);
        InternalRelays[i].assignedTo = false;
    }

    *addr = InternalRelays[i].addr;
    *bit = InternalRelays[i].bit;
    if(writeTo) {
        InternalRelays[i].assignedTo = true;
    }
}

//-----------------------------------------------------------------------------
// Retrieve the bit to read to determine whether a set of contacts is open
// or closed. Contacts could be internal relay, output pin, or input pin,
// or one of the internal state variables ($xxx) from the int code generator.
//-----------------------------------------------------------------------------
void MemForSingleBit(const NameArray& name, bool forRead, DWORD *addr, int *bit)
{
    *addr = -1;
    *bit = -1;
    if(name.length() == 0) {
        return;
    }
    switch(name[0]) {
        case 'I':
        case 'X':
            if(!forRead)
                oops();
            MemForPin(name, addr, bit, true);
            break;

        case 'P':
        case 'Y':
            MemForPin(name, addr, bit, false);
            break;

        case 'R':
        case '$':
            MemForBitInternal(name, addr, bit, !forRead);
            break;

        default:
            THROW_COMPILER_EXCEPTION_FMT(_("Unknown name >%s<"), name);
            break;
    }
}

void MemForSingleBit(const NameArray& name, DWORD *addr, int *bit)
{
    MemForSingleBit(name, false, addr, bit);
}

//-----------------------------------------------------------------------------
int isPinAssigned(const NameArray &name)
{
    int res = 0;
    if((Prog.mcu()) && ((Prog.mcu()->whichIsa == ISA_AVR) || (Prog.mcu()->whichIsa == ISA_PIC16)))
        switch(name[0]) {
            case 'A':
            case 'I':
            case 'X':
            case 'Y': {
                auto assign = std::find_if(Prog.io.assignment,
                                           Prog.io.assignment + Prog.io.count,
                                           [name](const PlcProgramSingleIo &io) { return (name == io.name); });
                if(assign == (Prog.io.assignment + Prog.io.count))
                    THROW_COMPILER_EXCEPTION(_("Can't find right assign."));

                int pin = assign->pin;
                if(name[0] == 'A') {
                    auto info = std::find_if(Prog.mcu()->adcInfo,
                                             Prog.mcu()->adcInfo + Prog.mcu()->adcCount,
                                             [pin](const McuAdcPinInfo &info) { return (info.pin == pin); });
                    if(info != (Prog.mcu()->adcInfo + Prog.mcu()->adcCount))
                        res = 1;
                } else {
                    auto info = std::find_if(Prog.mcu()->pinInfo,
                                             Prog.mcu()->pinInfo + Prog.mcu()->pinCount,
                                             [pin](const McuIoPinInfo &info) { return (info.pin == pin); });
                    if(info != (Prog.mcu()->pinInfo + Prog.mcu()->pinCount))
                        res = 1;
                }
                break;
            }

            case 'R':
            case '$':
                //MemForBitInternal(name, addr, bit, !forRead);
                break;

            default:
                //ooops(">%s<", name);
                break;
        }
    return res;
}

//-----------------------------------------------------------------------------
// Retrieve the bit to write to set the state of an output.
//-----------------------------------------------------------------------------
/*
void MemForCoil(char *name, DWORD *addr, int *bit)
{
    switch(name[0]) {
        case 'Y':
            MemForPin(name, addr, bit, false);
            break;

        case 'R':
            MemForBitInternal(name, addr, bit, true);
            break;

        default:
            oops();
            break;
    }
}
*/
//-----------------------------------------------------------------------------
// Do any post-compilation sanity checks necessary.
//-----------------------------------------------------------------------------
void MemCheckForErrorsPostCompile()
{
    for(int i = 0; i < InternalRelayCount; i++) {
        if(!InternalRelays[i].assignedTo) {
            THROW_COMPILER_EXCEPTION_FMT(_("Internal relay '%s' never assigned; add its coil somewhere."), InternalRelays[i].name);
        }
    }
}

//-----------------------------------------------------------------------------
// From the I/O list, determine which pins are inputs and which pins are
// outputs, and pack that in 8-bit format as we will need to write to the
// TRIS or DDR registers. ADC pins are neither inputs nor outputs.
//-----------------------------------------------------------------------------
///// Prototype modified by JG to have 8 / 16 bit ports
/////   void BuildDirectionRegisters(BYTE *isInput, BYTE *isAnsel, BYTE *isOutput, bool raiseError)
void BuildDirectionRegisters(WORD *isInput, WORD *isAnsel, WORD *isOutput, bool raiseError)
{
    if(!Prog.mcu())
        THROW_COMPILER_EXCEPTION(_("Invalid MCU"));

    ///// memset() modified by JG
    memset(isOutput, 0x0000, 2*MAX_IO_PORTS);
    memset(isAnsel, 0x0000, 2*MAX_IO_PORTS);
    memset(isInput, 0x0000, 2*MAX_IO_PORTS);

    bool usedUart = UartFunctionUsed();

    int i;
    for(i = 0; i < Prog.io.count; i++) {
        int pin = Prog.io.assignment[i].pin;
        int type = Prog.io.assignment[i].type;

        if(type == IO_TYPE_READ_ADC) {
            auto iop = std::find_if(Prog.mcu()->pinInfo,
                                    Prog.mcu()->pinInfo + Prog.mcu()->pinCount,
                                    [pin](const McuIoPinInfo &pi) { return (pi.pin == pin); });
            if(iop != (Prog.mcu()->pinInfo + Prog.mcu()->pinCount))
                isAnsel[iop->port - 'A'] |= (1 << iop->bit);
        }

        if(type == IO_TYPE_DIG_OUTPUT || //
           type == IO_TYPE_PWM_OUTPUT || //
           type == IO_TYPE_INT_INPUT ||  //
           type == IO_TYPE_DIG_INPUT) {

            auto iop = std::find_if(Prog.mcu()->pinInfo,
                                    Prog.mcu()->pinInfo + Prog.mcu()->pinCount,
                                    [pin](const McuIoPinInfo &pi) { return (pi.pin == pin); });
            if(iop == (Prog.mcu()->pinInfo + Prog.mcu()->pinCount)) {
                THROW_COMPILER_EXCEPTION_FMT(_("Must assign pins for all I/O.\r\n\r\n'%s' is not assigned."),
                                             Prog.io.assignment[i].name);
            }
            if((type == IO_TYPE_DIG_OUTPUT) || (type == IO_TYPE_PWM_OUTPUT)) {
                isOutput[iop->port - 'A'] |= (1 << iop->bit);
            } else {
                isInput[iop->port - 'A'] |= (1 << iop->bit);
            }

            if(raiseError) {
                if(usedUart && (pin == Prog.mcu()->uartNeeds.rxPin || pin == Prog.mcu()->uartNeeds.txPin)) {
                    THROW_COMPILER_EXCEPTION_FMT(_("UART in use; pins %d and %d reserved for that."),
                                                 Prog.mcu()->uartNeeds.rxPin,
                                                 Prog.mcu()->uartNeeds.txPin);
                }
            }
        }
    }
    if(usedUart) {
        McuIoPinInfo *iop;
        iop = PinInfo(Prog.mcu()->uartNeeds.txPin);
        if(iop)
            isOutput[iop->port - 'A'] |= (1 << iop->bit);
        else
            THROW_COMPILER_EXCEPTION(_("Invalid TX pin."));

        iop = PinInfo(Prog.mcu()->uartNeeds.rxPin);
        if(iop)
            isInput[iop->port - 'A'] |= (1 << iop->bit);
        else
            THROW_COMPILER_EXCEPTION(_("Invalid RX pin."));
    }
    if(McuAs("Microchip PIC16F877 ")) {
        // This is a nasty special case; one of the extra bits in TRISE
        // enables the PSP, and must be kept clear (set here as will be
        // inverted).
        isOutput[4] |= 0xf8; // TRISE
    }
}

///// Modified by JG to have 8 & 16 bit ports
void BuildDirectionRegisters(BYTE *isInput, BYTE *isAnsel, BYTE *isOutput)      ///// 8-bit ports
{
    WORD isInp[MAX_IO_PORTS], isAns[MAX_IO_PORTS], isOut[MAX_IO_PORTS];

    BuildDirectionRegisters(isInp, isAns, isOut, true);

    for (int i = 0 ; i < MAX_IO_PORTS ; i++)
    {
        isInput[i]= (BYTE) isInp[i];
        isAnsel[i]= (BYTE) isAns[i];
        isOutput[i]= (BYTE) isOut[i];
    }
}

void BuildDirectionRegisters(WORD *isInput, WORD *isAnsel, WORD *isOutput)      ///// 16-bit ports
{
    BuildDirectionRegisters(isInput, isAnsel, isOutput, true);
}
/////

//-----------------------------------------------------------------------------
// Display our boilerplate warning that the baud rate error is too high.
//-----------------------------------------------------------------------------
void ComplainAboutBaudRateError(int divisor, double actual, double err)
{
    Error(_("UART baud rate generator: divisor=%d actual=%.4f for %.2f%% error.\r\n\r\n"
            "This is too large; try a different baud rate (slower "
            "probably), or a crystal frequency chosen to be divisible "
            "by many common baud rates (e.g. 3.6864 MHz, 14.7456 MHz).\r\n\r\n"
            "Code will be generated anyways but serial may be unreliable or completely broken."),
          divisor,
          actual,
          err);
}

//-----------------------------------------------------------------------------
// Display our boilerplate warning that the baud rate is too slow (making
// for an overflowed divisor).
//-----------------------------------------------------------------------------
void ComplainAboutBaudRateOverflow()
{
    Error(
        _("UART baud rate generator: too slow, divisor overflows. "
          "Use a slower crystal or a faster baud rate.\r\n\r\n"
          "Code will be generated anyways but serial will likely be "
          "completely broken."));
}
//-----------------------------------------------------------------------------
/*
    International System (SI) prefixes.
           10 power
yotta   Y    24     septillion      quadrillion
zetta   Z    21     sextillion      thousand trillion
exa     E    18     quintillion     trillion
peta    P    15     quadrillion     thousand billion
tera    T    12     trillion        billion
giga    G    9      billion         thousand million
mega    M    6      million
kilo    k    3      thousand
hecto   h    2      hundred
deca    da   1      ten
             0      one
deci    d   -1      tenth
centi   c   -2      hundredth
milli   m   -3      thousandth
micro   u   -6      millionth
nano    n   -9      billionth       thousand millionth
pico    p   -12     trillionth      billionth
femto   f   -15     quadrillionth   thousand billionth
atto    a   -18     quintillionth   trillionth
zepto   z   -21     sextillionth    thousand trillionth
yocto   y   -24     septillionth    quadrillionth
*/
double SIprefix(double val, char *prefix, int en_1_2)
{
    //dbp("SIprefix val=%f",val);
    if(val >= 1e24) {
        strcpy(prefix, "Y");
        return val / 1e24;
    } else if(val >= 1e21) {
        strcpy(prefix, "Z");
        return val / 1e21;
    } else if(val >= 1e18) {
        strcpy(prefix, "E");
        return val / 1e18;
    } else if(val >= 1e15) {
        strcpy(prefix, "P");
        return val / 1e15;
    } else if(val >= 1e12) {
        strcpy(prefix, "T");
        return val / 1e12;
    } else if(val >= 1e9) {
        strcpy(prefix, "G");
        return val / 1e9;
    } else if(val >= 1e6) {
        strcpy(prefix, "M");
        return val / 1e6;
    } else if(val >= 1e3) {
        strcpy(prefix, "k");
        return val / 1e3;
    } else if((val >= 1e2) && (en_1_2)) {
        strcpy(prefix, "h");
        return val / 1e2;
    } else if((val >= 1e1) && (en_1_2)) {
        strcpy(prefix, "da");
        return val / 1e1;
    } else if(val >= 1.0) {
        strcpy(prefix, "");
        return val;
    } else if(val == 0.0) {
        strcpy(prefix, "");
        return val;
    } else if(val < 1e-21) {
        strcpy(prefix, "y");
        return val * 1e21 * 1e3;
    } else if(val < 1e-18) {
        strcpy(prefix, "z");
        return val * 1e18 * 1e3;
    } else if(val < 1e-15) {
        strcpy(prefix, "a");
        return val * 1e15 * 1e3;
    } else if(val < 1e-12) {
        strcpy(prefix, "f");
        return val * 1e12 * 1e3;
    } else if(val < 1e-9) {
        strcpy(prefix, "p");
        return val * 1e9 * 1e3;
    } else if(val < 1e-6) {
        strcpy(prefix, "n");
        return val * 1e6 * 1e3;
    } else if(val < 1e-3) {
        strcpy(prefix, "u");
        return val * 1e3 * 1e3;
        /**/
    } else if((val <= 1e-2) && en_1_2) {
        strcpy(prefix, "c");
        return val * 1e2;
    } else if((val <= 1e-1) && en_1_2) {
        strcpy(prefix, "d");
        return val * 1e1;
        /**/
    } else if(val < 1.0) {
        strcpy(prefix, "m"); //10 ms= 0.010 s
        return val * 1e3;
    } else {
        oops();
        return 0;
    }
}
double SIprefix(double val, char *prefix)
{
    return SIprefix(val, prefix, 0);
}
