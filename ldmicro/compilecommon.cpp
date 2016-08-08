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
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"
#include "intcode.h"

// If we encounter an error while compiling then it's convenient to break
// out of the possibly-deeply-recursed function we're in.
jmp_buf CompileErrorBuf;

// Assignment of the internal relays to memory, efficient, one bit per
// relay.
static struct {
    char    name[MAX_NAME_LEN];
    DWORD   addr;
    int     bit;
    BOOL    assignedTo;
} InternalRelays[MAX_IO];
static int InternalRelayCount;
/*
VariablesList moved to ldmicro.h

// Assignment of the `variables,' used for timers, counters, arithmetic, and
// other more general things. Allocate 2 octets (16 bits) per.
static struct {
    char    name[MAX_NAME_LEN];
    DWORD   addrl;
    DWORD   addrh;
} Variables[MAX_IO];
*/
VariablesList Variables[MAX_IO];
int VariableCount = 0;

#define NO_MEMORY   0xffffffff
static DWORD    NextBitwiseAllocAddr;
static int      NextBitwiseAllocBit;
static int      MemOffset;

//-----------------------------------------------------------------------------
void PrintVariables(FILE *f)
{
    fprintf(f, "\n");
    fprintf(f, ";|Name          | SizeOfVar | addrl |\n");
    int i;
    for(i = 0; i < VariableCount; i++) {
        fprintf(f, ";%s \t %d \t 0x%04x = %d\n", Variables[i].name, Variables[i].SizeOfVar, Variables[i].addrl, Variables[i].addrl);
    }
    fprintf(f, "\n");
}
//-----------------------------------------------------------------------------
static void ClrInternalData(void)
{
    MemOffset = 0;
//  VariableCount = 0;
    int i;
    for(i = 0; i < VariableCount; i++) {
        Variables[i].Allocated = 0;
        Variables[i].addrl = 0;
        Variables[i].addrh = 0;
    }
}
//-----------------------------------------------------------------------------
// Forget what memory has been allocated on the target, so we start from
// everything free.
//-----------------------------------------------------------------------------
void AllocStart(void)
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
DWORD AllocOctetRam(void)
{
    if(!Prog.mcu)
        return 0;
    if(MemOffset >= Prog.mcu->ram[0].len) {
        Error(_("Out of memory; simplify program or choose "
            "microcontroller with more memory."));
        CompileError();
    }

    MemOffset++;
    return Prog.mcu->ram[0].start + MemOffset - 1;
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
static void MemForPin(char *name, DWORD *addr, int *bit, BOOL asInput)
{
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, name)==0)
            break;
    }
    if(i >= Prog.io.count) oops();

    if(asInput && Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT) oops();
    if(!asInput && Prog.io.assignment[i].type != IO_TYPE_DIG_OUTPUT) oops();

    *addr = -1;
    *bit = -1;
    if(Prog.mcu) {
        /*
        int pin = Prog.io.assignment[i].pin;
        for(i = 0; i < Prog.mcu->pinCount; i++) {
            if(Prog.mcu->pinInfo[i].pin == pin)
                break;
        }

        if(i >= Prog.mcu->pinCount) {
            Error(_("Must assign pins for all I/O.\r\n\r\n"
                "'%s' is not assigned."), name);
            //CompileError();
        } else {
            McuIoPinInfo *iop = &Prog.mcu->pinInfo[i];

            if(asInput) {
                *addr = Prog.mcu->inputRegs[iop->port - 'A'];
            } else {
                *addr = Prog.mcu->outputRegs[iop->port - 'A'];
            }
            *bit = iop->bit;
        }
        */
        McuIoPinInfo *iop = PinInfo(Prog.io.assignment[i].pin);
        if(iop) {
            if(asInput) {
                *addr = Prog.mcu->inputRegs[iop->port - 'A'];
            } else {
                *addr = Prog.mcu->outputRegs[iop->port - 'A'];
            }
            *bit = iop->bit;
        } else {
            Error(_("Must assign pins for all I/O.\r\n\r\n"
                "'%s' is not assigned."), name);
        }
    }
}

//-----------------------------------------------------------------------------
int SingleBitAssigned(char *name)
{
    int pin = 0;
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, name)==0)
            break;
    }
    if(i >= Prog.io.count) oops();

    if(Prog.mcu) {
        pin = Prog.io.assignment[i].pin;
        for(i = 0; i < Prog.mcu->pinCount; i++) {
            if(Prog.mcu->pinInfo[i].pin == pin)
                break;
        }

        if(i >= Prog.mcu->pinCount) {
            pin = 0;
        }
    }
    return pin;
}
//-----------------------------------------------------------------------------
// Determine the mux register settings to read a particular ADC channel.
//-----------------------------------------------------------------------------
BYTE MuxForAdcVariable(char *name)
{
    int res = 0;
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, name)==0)
            break;
    }
    if(i >= Prog.io.count) oops();

    if(Prog.mcu) {
        int j;
        for(j = 0; j < Prog.mcu->adcCount; j++) {
            if(Prog.mcu->adcInfo[j].pin == Prog.io.assignment[i].pin) {
                break;
            }
        }
        if(j == Prog.mcu->adcCount) {
            Error(_("Must assign pins for all ADC inputs (name '%s')."), name);
            CompileError();
        }
        res = Prog.mcu->adcInfo[j].muxRegValue;
    }

    return res;
}

//-----------------------------------------------------------------------------
int byteNeeded(SDWORD i)
{
    if((-0x80<=i) && (i<=0x7f))
        return 1;
    else if((-0x8000<=i) && (i<=0x7FFF))
        return 2;
    else if((-0x800000<=i) && (i<=0x7FffFF))
        return 3;
    else
        return 4; // not implamanted for LDmicro
}
//-----------------------------------------------------------------------------
int TestByteNeeded(int count, SDWORD *vals)
{
    int res = -1;
    int i, r;
    for(i=0; i<count; i++){
         r = byteNeeded(vals[i]);
         if(res < r)
             res = r;
    }
    return res;
}
//-----------------------------------------------------------------------------
// Allocate the two octets (16-bit count) for a variable, used for a variety
// of purposes.
//-----------------------------------------------------------------------------
void MemForVariable(char *name, DWORD *addrl, DWORD *addrh)
{
    if(strlenalnum(name)==0) {
        Error(_("Empty variable name '%s'.\nrungNow=%d"), name, rungNow);
        CompileError();
    }

    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(name, Variables[i].name)==0) break;
    }
    if(i >= MAX_IO) {
        Error(_("Internal limit exceeded (number of vars)"));
        CompileError();
    }
    if(i == VariableCount) {
        VariableCount++;
        strcpy(Variables[i].name, name);
        Variables[i].SizeOfVar = 0;
        Variables[i].Allocated = 0;
    }
    if(addrl) { // Allocate SRAM

        if(Variables[i].Allocated == 0) {
            Variables[i].addrl = AllocOctetRam();
            Variables[i].addrh = AllocOctetRam();
        }
        Variables[i].Allocated = 2;
    }
    if(addrl)
        *addrl = Variables[i].addrl;
    if(addrh)
        *addrh = Variables[i].addrh;
}

int MemForVariable(char *name, DWORD *addrl)
{
    MemForVariable(name, addrl, NULL);
    return 2;
}

//-----------------------------------------------------------------------------
int GetVariableType(char *name)
{
    if(strlenalnum(name)==0) {
        Error(_("Empty variable name '%s'.\nrungNow=%d"), name, rungNow);
        CompileError();
    }

    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(name, Variables[i].name)==0) break;
    }
    if(i >= MAX_IO) {
        Error(_("Internal limit exceeded (number of vars)"));
        CompileError();
    }
    if(i < VariableCount) {
        return Variables[i].type;
    }
    return IO_TYPE_PENDING;
}

int SetVariableType(char *name, int type)
{
    if(strlenalnum(name)==0) {
        Error(_("Empty variable name '%s'.\nrungNow=%d"), name, rungNow);
        CompileError();
    }
    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(name, Variables[i].name)==0) break;
    }
    if(i >= MAX_IO) {
        Error(_("Internal limit exceeded (number of vars)"));
        CompileError();
    }
    if(i == VariableCount) {
        VariableCount++;
        strcpy(Variables[i].name, name);
        Variables[i].SizeOfVar = 0;
        Variables[i].Allocated = 0;
        Variables[i].type = type;
    } else {
        Variables[i].type = type;
    }
    return type;
}
//-----------------------------------------------------------------------------
int SetSizeOfVar(char *name, int sizeOfVar)
{
    return 2;
}

int SizeOfVar(char *name)
{
    return 2;
}

int AllocOfVar(char *name)
{
    if(strlenalnum(name)==0) {
        Error(_("Empty variable name '%s'.\nrungNow=%d"), name, rungNow);
        CompileError();
    }

    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(name, Variables[i].name)==0) break;
    }
    if(i >= MAX_IO) {
        Error(_("Internal limit exceeded (number of vars)"));
        CompileError();
    }
    if(i < VariableCount) {
        return Variables[i].Allocated;
    }
    return 0;
}
//-----------------------------------------------------------------------------
// Compare function to qsort() the I/O list.
//-----------------------------------------------------------------------------
static int CompareIo(const void *av, const void *bv)
{
    VariablesList *a = (VariablesList *)av;
    VariablesList *b = (VariablesList *)bv;

    return strcmp(a->name, b->name);
}

//-----------------------------------------------------------------------------
void SaveVarListToFile(FILE *f)
{
    qsort(Variables, VariableCount, sizeof(Variables[0]),
        CompareIo);

    int i;
    for(i = 0; i < VariableCount; i++)
      if((Variables[i].type != IO_TYPE_INT_INPUT)
      && (Variables[i].type != IO_TYPE_DIG_INPUT)
      && (Variables[i].type != IO_TYPE_DIG_OUTPUT)
      && (Variables[i].type != IO_TYPE_INTERNAL_RELAY))
    if(Variables[i].name[0] != '$') {
          fprintf(f, "  %3d bytes %s %s\n",
            SizeOfVar(Variables[i].name),
            Variables[i].name,
           (Variables[i].Allocated?"":"\tNow not used !!!"));
        }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Allocate or retrieve the bit of memory assigned to an internal relay or
// other thing that requires a single bit of storage.
//-----------------------------------------------------------------------------
static void MemForBitInternal(char *name, DWORD *addr, int *bit, BOOL writeTo)
{
    int i;
    for(i = 0; i < InternalRelayCount; i++) {
        if(strcmp(name, InternalRelays[i].name)==0)
            break;
    }
    if(i >= MAX_IO) {
        Error(_("Internal limit exceeded (number of relay)"));
        CompileError();
    }
    if(i == InternalRelayCount) {
        InternalRelayCount++;
        strcpy(InternalRelays[i].name, name);
        AllocBitRam(&InternalRelays[i].addr, &InternalRelays[i].bit);
        InternalRelays[i].assignedTo = FALSE;
    }

    *addr = InternalRelays[i].addr;
    *bit = InternalRelays[i].bit;
    if(writeTo) {
        InternalRelays[i].assignedTo = TRUE;
    }
}

//-----------------------------------------------------------------------------
// Retrieve the bit to read to determine whether a set of contacts is open
// or closed. Contacts could be internal relay, output pin, or input pin,
// or one of the internal state variables ($xxx) from the int code generator.
//-----------------------------------------------------------------------------
void MemForSingleBit(char *name, BOOL forRead, DWORD *addr, int *bit)
{
    switch(name[0]) {
        case 'I':
        case 'X':
            if(!forRead) oops();
            MemForPin(name, addr, bit, TRUE);
            break;

        case 'Y':
            MemForPin(name, addr, bit, FALSE);
            break;

        case 'R':
        case '$':
            MemForBitInternal(name, addr, bit, !forRead);
            break;

        default:
            ooops(">%s<", name);
            break;
    }
}

//-----------------------------------------------------------------------------
int isPinAssigned(char *name)
{
    int res = 0;
    if((Prog.mcu) &&
      ((Prog.mcu->whichIsa == ISA_AVR)
    || (Prog.mcu->whichIsa == ISA_PIC16)))
    switch(name[0]) {
        case 'A':
        case 'I':
        case 'X':
        case 'Y': {
            int i;
            for(i = 0; i < Prog.io.count; i++) {
                if(strcmp(Prog.io.assignment[i].name, name)==0)
                    break;
            }
            if(i >= Prog.io.count) oops();

            //if(asInput && Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT) oops();
            //if(!asInput && Prog.io.assignment[i].type != IO_TYPE_DIG_OUTPUT) oops();

            int pin = Prog.io.assignment[i].pin;
            if(name[0] == 'A') {
                for(i = 0; i < Prog.mcu->adcCount; i++)
                    if(Prog.mcu->adcInfo[i].pin == pin)
                        break;
                if(i < Prog.mcu->adcCount)
                    res = 1;
            } else {
                for(i = 0; i < Prog.mcu->pinCount; i++)
                    if(Prog.mcu->pinInfo[i].pin == pin)
                        break;
                if(i < Prog.mcu->pinCount)
                    res = 1;
            }
            if(!res)
                Error(_("Must assign pins for all I/O.\r\n\r\n"
                    "'%s' is not assigned."), name);
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
            MemForPin(name, addr, bit, FALSE);
            break;

        case 'R':
            MemForBitInternal(name, addr, bit, TRUE);
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
void MemCheckForErrorsPostCompile(void)
{
    int i;
    for(i = 0; i < InternalRelayCount; i++) {
        if(!InternalRelays[i].assignedTo) {
            Error(
               _("Internal relay '%s' never assigned; add its coil somewhere."),
                InternalRelays[i].name);
            //CompileError();
        }
    }
}

//-----------------------------------------------------------------------------
// From the I/O list, determine which pins are inputs and which pins are
// outputs, and pack that in 8-bit format as we will need to write to the
// TRIS or DDR registers. ADC pins are neither inputs nor outputs.
//-----------------------------------------------------------------------------
void BuildDirectionRegisters(BYTE *isInput, BYTE *isOutput, BOOL raiseError)
{
    memset(isOutput, 0x00, MAX_IO_PORTS);
    memset(isInput, 0x00, MAX_IO_PORTS);

    BOOL usedUart = UartFunctionUsed();
    BOOL usedPwm  = PwmFunctionUsed();

    int i;
    for(i = 0; i < Prog.io.count; i++) {
        int pin = Prog.io.assignment[i].pin;

        if(Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_PWM_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_INT_INPUT ||
           Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT)
        {
            int j;
            for(j = 0; j < Prog.mcu->pinCount; j++) {
                McuIoPinInfo *iop = &Prog.mcu->pinInfo[j];
                if(iop && (iop->pin == pin)) {
                    if((Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT)
                    || (Prog.io.assignment[i].type == IO_TYPE_PWM_OUTPUT)) {
                        isOutput[iop->port - 'A'] |= (1 << iop->bit);
                    } else {
                        isInput[iop->port - 'A'] |= (1 << iop->bit);
                    }
                    break;
                }
            }
          if(Prog.mcu && raiseError) {
            if(j >= Prog.mcu->pinCount) {
                Error(_("Must assign pins for all I/O.\r\n\r\n"
                    "'%s' is not assigned."),
                    Prog.io.assignment[i].name);
                if(raiseError)
                    CompileError();
            }

            if(usedUart &&
                (pin == Prog.mcu->uartNeeds.rxPin ||
                 pin == Prog.mcu->uartNeeds.txPin))
            {
                Error(_("UART in use; pins %d and %d reserved for that."),
                    Prog.mcu->uartNeeds.rxPin, Prog.mcu->uartNeeds.txPin);
                if(raiseError)
                    CompileError();
            }
            /*
            if(usedPwm && pin == Prog.mcu->pwmNeedsPin) {
                Error(_("PWM in use; pin %d reserved for that."),
                    Prog.mcu->pwmNeedsPin);
                if(raiseError)
                    CompileError();
            }
            */
          }
        }
    }
}

void BuildDirectionRegisters(BYTE *isInput, BYTE *isOutput)
{
    BuildDirectionRegisters(isInput, isOutput, TRUE);
}

//-----------------------------------------------------------------------------
// Display our boilerplate warning that the baud rate error is too high.
//-----------------------------------------------------------------------------
void ComplainAboutBaudRateError(int divisor, double actual, double err)
{
    Error(_("UART baud rate generator: divisor=%d actual=%.4f for %.2f%% "
          "error.\r\n"
          "\r\n"
          "This is too large; try a different baud rate (slower "
          "probably), or a crystal frequency chosen to be divisible "
          "by many common baud rates (e.g. 3.6864 MHz, 14.7456 MHz).\r\n"
          "\r\n"
          "Code will be generated anyways but serial may be "
          "unreliable or completely broken."), divisor, actual, err);
}

//-----------------------------------------------------------------------------
// Display our boilerplate warning that the baud rate is too slow (making
// for an overflowed divisor).
//-----------------------------------------------------------------------------
void ComplainAboutBaudRateOverflow(void)
{
    Error(_("UART baud rate generator: too slow, divisor overflows. "
        "Use a slower crystal or a faster baud rate.\r\n"
        "\r\n"
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
double SIprefix(double val, char* prefix, int en_1_2)
{
    //dbp("SIprefix val=%f",val);
    if(val >=        1e24) {
        strcpy(prefix,"Y");
        return val / 1e24;
    } else if(val >= 1e21) {
        strcpy(prefix,"Z");
        return val / 1e21;
    } else if(val >= 1e18) {
        strcpy(prefix,"E");
        return val / 1e18;
    } else if(val >= 1e15) {
        strcpy(prefix,"P");
        return val / 1e15;
    } else if(val >= 1e12) {
        strcpy(prefix,"T");
        return val / 1e12;
    } else if(val >= 1e9) {
        strcpy(prefix,"G");
        return val / 1e9;
    } else if(val >= 1e6) {
        strcpy(prefix,"M");
        return val / 1e6;
    } else if(val >= 1e3) {
        strcpy(prefix,"k");
        return val / 1e3;
    } else if((val >= 1e2)&&(en_1_2)) {
        strcpy(prefix,"h");
        return val / 1e2;
    } else if((val >= 1e1)&&(en_1_2)) {
        strcpy(prefix,"da");
        return val / 1e1;
    } else if(val >= 1.0) {
        strcpy(prefix,"");
        return val;
    } else if(val == 0.0) {
        strcpy(prefix,"");
        return val;
    } else if(val < 1e-21) {
        strcpy(prefix,"y");
        return val * 1e21 * 1e3;
    } else if(val < 1e-18) {
        strcpy(prefix,"z");
        return val * 1e18 * 1e3;
    } else if(val < 1e-15) {
        strcpy(prefix,"a");
        return val * 1e15 * 1e3;
    } else if(val < 1e-12) {
        strcpy(prefix,"f");
        return val * 1e12 * 1e3;
    } else if(val < 1e-9) {
        strcpy(prefix,"p");
        return val * 1e9 * 1e3;
    } else if(val < 1e-6) {
        strcpy(prefix,"n");
        return val * 1e6 * 1e3;
    } else if(val < 1e-3) {
        strcpy(prefix,"u");
        return val * 1e3 * 1e3;
/*
    } else if((val <= 1e-2)&&(en_1_2)) {
        strcpy(prefix,"c");
        return val * 1e2;
    } else if((val <= 1e-1)&&(en_1_2)) {
        strcpy(prefix,"d");
        return val * 1e1;
*/
    } else if(val < 1) {
        strcpy(prefix,"m");           //10 ms= 0.010 s
        return val * 1e3;
    } else {
        oops();
        return 0;
    }
}
double SIprefix(double val, char* prefix)
{
    return SIprefix(val, prefix, 0);
}