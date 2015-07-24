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
// An AVR assembler, for our own internal use, plus routines to generate
// code from the ladder logic structure, plus routines to generate the
// runtime needed to schedule the cycles.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include "ldmicro.h"
#include "intcode.h"

// not complete; just what I need
typedef enum AvrOpTag {
    OP_VACANT,
    OP_ADC,
    OP_ADD,
    OP_ASR,
    OP_BRCC,
    OP_BRCS,
    OP_BREQ,
    OP_BRGE,
    OP_BRLO,
    OP_BRLT,
    OP_BRNE,
    OP_CBR,
    OP_CLC,
    OP_CLR,
    OP_COM,
    OP_CP,
    OP_CPC,
    OP_DEC,
    OP_EOR,
    OP_ICALL,
    OP_IJMP,
    OP_INC,
    OP_LDI,
    OP_LD_X,
    OP_MOV,
    OP_OUT,
    OP_RCALL,
    OP_RET,
    OP_RETI,
    OP_RJMP,
    OP_ROR,
    OP_SEC,
    OP_SBC,
    OP_SBCI,
    OP_SBR,
    OP_SBRC,
    OP_SBRS,
    OP_ST_X,
    OP_SUB,
    OP_SUBI,
    OP_TST,
    OP_WDR,
} AvrOp;

typedef struct AvrInstructionTag {
    AvrOp       op;
    DWORD       arg1;
    DWORD       arg2;
} AvrInstruction;

#define MAX_PROGRAM_LEN 128*1024
static AvrInstruction AvrProg[MAX_PROGRAM_LEN];
static DWORD AvrProgWriteP;

// For yet unresolved references in jumps
static DWORD FwdAddrCount;

// Fancier: can specify a forward reference to the high or low octet of a
// 16-bit address, which is useful for indirect jumps.
#define FWD_LO(x) ((x) | 0x20000000)
#define FWD_HI(x) ((x) | 0x40000000)

// Address to jump to when we finish one PLC cycle
static DWORD BeginningOfCycleAddr;

// Address of the multiply subroutine, and whether we will have to include it
static DWORD MultiplyAddress;
static BOOL MultiplyUsed;
// and also divide
static DWORD DivideAddress;
static BOOL DivideUsed;

// For EEPROM: we queue up characters to send in 16-bit words (corresponding
// to the integer variables), but we can actually just program 8 bits at a
// time, so we need to store the high byte somewhere while we wait.
static DWORD EepromHighByte;
static DWORD EepromHighByteWaitingAddr;
static int EepromHighByteWaitingBit;

// Some useful registers, unfortunately many of which are in different places
// on different AVRs! I consider this a terrible design choice by Atmel.
static DWORD REG_TIMSK;
static DWORD REG_TIFR;
#define REG_OCR1AH  0x4b
#define REG_OCR1AL  0x4a
#define REG_TCCR1A  0x4f
#define REG_TCCR1B  0x4e
#define REG_SPH     0x5e
#define REG_SPL     0x5d
#define REG_ADMUX   0x27
#define REG_ADCSRA  0x26
#define REG_ADCL    0x24
#define REG_ADCH    0x25

static DWORD REG_UBRRH;
static DWORD REG_UBRRL;
static DWORD REG_UCSRB;
static DWORD REG_UCSRA;
static DWORD REG_UDR;

#define REG_OCR2    0x43
#define REG_TCCR2   0x45

#define REG_EEARH   0x3f
#define REG_EEARL   0x3e
#define REG_EEDR    0x3d
#define REG_EECR    0x3c


static int IntPc;

static void CompileFromIntermediate(void);

//-----------------------------------------------------------------------------
// Wipe the program and set the write pointer back to the beginning. Also
// flush all the state of the register allocators etc.
//-----------------------------------------------------------------------------
static void WipeMemory(void)
{
    memset(AvrProg, 0, sizeof(AvrProg));
    AvrProgWriteP = 0;
}

//-----------------------------------------------------------------------------
// Store an instruction at the next spot in program memory.  Error condition
// if this spot is already filled. We don't actually assemble to binary yet;
// there may be references to resolve.
//-----------------------------------------------------------------------------
static void Instruction(AvrOp op, DWORD arg1, DWORD arg2)
{
    if(AvrProg[AvrProgWriteP].op != OP_VACANT) oops();

    AvrProg[AvrProgWriteP].op = op;
    AvrProg[AvrProgWriteP].arg1 = arg1;
    AvrProg[AvrProgWriteP].arg2 = arg2;
    AvrProgWriteP++;
}

//-----------------------------------------------------------------------------
// Allocate a unique descriptor for a forward reference. Later that forward
// reference gets assigned to an absolute address, and we can go back and
// fix up the reference.
//-----------------------------------------------------------------------------
static DWORD AllocFwdAddr(void)
{
    FwdAddrCount++;
    return 0x80000000 | FwdAddrCount;
}

//-----------------------------------------------------------------------------
// Go back and fix up the program given that the provided forward address
// corresponds to the next instruction to be assembled.
//-----------------------------------------------------------------------------
static void FwdAddrIsNow(DWORD addr)
{
    if(!(addr & 0x80000000)) oops();

    DWORD i;
    for(i = 0; i < AvrProgWriteP; i++) {
        if(AvrProg[i].arg1 == addr) {
            AvrProg[i].arg1 = AvrProgWriteP;
        } else if(AvrProg[i].arg2 == FWD_LO(addr)) {
            AvrProg[i].arg2 = (AvrProgWriteP & 0xff);
        } else if(AvrProg[i].arg2 == FWD_HI(addr)) {
            AvrProg[i].arg2 = AvrProgWriteP >> 8;
        }
    }
}

//-----------------------------------------------------------------------------
// Given an opcode and its operands, assemble the 16-bit instruction for the
// AVR. Check that the operands do not have more bits set than is meaningful;
// it is an internal error if they do not. Needs to know what address it is
// being assembled to so that it generate relative jumps; internal error if
// a relative jump goes out of range.
//-----------------------------------------------------------------------------
static DWORD Assemble(DWORD addrAt, AvrOp op, DWORD arg1, DWORD arg2)
{
#define CHECK(v, bits) if((v) != ((v) & ((1 << (bits))-1))) oops()
    switch(op) {
        case OP_ASR:
            CHECK(arg1, 5); CHECK(arg2, 0);
            return (9 << 12) | (2 << 9) | (arg1 << 4) | 5;

        case OP_ROR:
            CHECK(arg1, 5); CHECK(arg2, 0);
            return (9 << 12) | (2 << 9) | (arg1 << 4) | 7;

        case OP_ADD:
            CHECK(arg1, 5); CHECK(arg2, 5);
            return (3 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_ADC:
            CHECK(arg1, 5); CHECK(arg2, 5);
            return (7 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_EOR:
            CHECK(arg1, 5); CHECK(arg2, 5);
            return (9 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_SUB:
            CHECK(arg1, 5); CHECK(arg2, 5);
            return (6 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_SBC:
            CHECK(arg1, 5); CHECK(arg2, 5);
            return (2 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_CP:
            CHECK(arg1, 5); CHECK(arg2, 5);
            return (5 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_CPC:
            CHECK(arg1, 5); CHECK(arg2, 5);
            return (1 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_COM:
            CHECK(arg1, 5); CHECK(arg2, 0);
            return (9 << 12) | (2 << 9) | (arg1 << 4);

        case OP_SBR:
            CHECK(arg1, 5); CHECK(arg2, 8);
            if(!(arg1 & 0x10)) oops();
            arg1 &= ~0x10;
            return (6 << 12) | ((arg2 & 0xf0) << 4) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_CBR:
            CHECK(arg1, 5); CHECK(arg2, 8);
            if(!(arg1 & 0x10)) oops();
            arg1 &= ~0x10;
            arg2 = ~arg2;
            return (7 << 12) | ((arg2 & 0xf0) << 4) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_INC:
            CHECK(arg1, 5); CHECK(arg2, 0);
            return (0x4a << 9) | (arg1 << 4) | 3;

        case OP_DEC:
            CHECK(arg1, 5); CHECK(arg2, 0);
            return (0x4a << 9) | (arg1 << 4) | 10;

        case OP_SUBI:
            CHECK(arg1, 5); CHECK(arg2, 8);
            if(!(arg1 & 0x10)) oops();
            arg1 &= ~0x10;
            return (5 << 12) | ((arg2 & 0xf0) << 4) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_SBCI:
            CHECK(arg1, 5); CHECK(arg2, 8);
            if(!(arg1 & 0x10)) oops();
            arg1 &= ~0x10;
            return (4 << 12) | ((arg2 & 0xf0) << 4) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_TST:
            CHECK(arg1, 5); CHECK(arg2, 0);
            return (8 << 10) | ((arg1 & 0x10) << 4) | ((arg1 & 0x10) << 5) |
                ((arg1 & 0xf) << 4) | (arg1 & 0xf);

        case OP_SEC:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x9408;

        case OP_CLC:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x9488;

        case OP_IJMP:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x9409;

        case OP_ICALL:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x9509;

        case OP_RJMP:
            CHECK(arg2, 0);
            arg1 = arg1 - addrAt - 1;
            if(((int)arg1) > 2047 || ((int)arg1) < -2048) oops();
            arg1 &= (4096-1);
            return (12 << 12) | arg1;

        case OP_RCALL:
            CHECK(arg2, 0);
            arg1 = arg1 - addrAt - 1;
            if(((int)arg1) > 2047 || ((int)arg1) < -2048) oops();
            arg1 &= (4096-1);
            return (13 << 12) | arg1;

        case OP_RETI:
            return 0x9518;

        case OP_RET:
            return 0x9508;

        case OP_SBRC:
            CHECK(arg1, 5); CHECK(arg2, 3);
            return (0x7e << 9) | (arg1 << 4) | arg2;

        case OP_SBRS:
            CHECK(arg1, 5); CHECK(arg2, 3);
            return (0x7f << 9) | (arg1 << 4) | arg2;

        case OP_BREQ:
            CHECK(arg2, 0);
            arg1 = arg1 - addrAt - 1;
            if(((int)arg1) > 63 || ((int)arg1) < -64) oops();
            arg1 &= (128-1);
            return (0xf << 12) | (arg1 << 3) | 1;

        case OP_BRNE:
            CHECK(arg2, 0);
            arg1 = arg1 - addrAt - 1;
            if(((int)arg1) > 63 || ((int)arg1) < -64) oops();
            arg1 &= (128-1);
            return (0xf << 12) | (1 << 10) | (arg1 << 3) | 1;

        case OP_BRLO:
            CHECK(arg2, 0);
            arg1 = arg1 - addrAt - 1;
            if(((int)arg1) > 63 || ((int)arg1) < -64) oops();
            arg1 &= (128-1);
            return (0xf << 12) | (arg1 << 3);

        case OP_BRGE:
            CHECK(arg2, 0);
            arg1 = arg1 - addrAt - 1;
            if(((int)arg1) > 63 || ((int)arg1) < -64) oops();
            arg1 &= (128-1);
            return (0xf << 12) | (1 << 10) | (arg1 << 3) | 4;

        case OP_BRLT:
            CHECK(arg2, 0);
            arg1 = arg1 - addrAt - 1;
            if(((int)arg1) > 63 || ((int)arg1) < -64) oops();
            arg1 &= (128-1);
            return (0xf << 12) | (arg1 << 3) | 4;

        case OP_BRCC:
            CHECK(arg2, 0);
            arg1 = arg1 - addrAt - 1;
            if(((int)arg1) > 63 || ((int)arg1) < -64) oops();
            arg1 &= (128-1);
            return (0xf << 12) | (1 << 10) | (arg1 << 3);

        case OP_BRCS:
            CHECK(arg2, 0);
            arg1 = arg1 - addrAt - 1;
            if(((int)arg1) > 63 || ((int)arg1) < -64) oops();
            arg1 &= (128-1);
            return (0xf << 12) | (arg1 << 3);

        case OP_MOV:
            CHECK(arg1, 5); CHECK(arg2, 5);
            return (0xb << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_LDI:
            CHECK(arg1, 5); CHECK(arg2, 8);
            if(!(arg1 & 0x10)) oops();
            arg1 &= ~0x10;
            return (0xe << 12) | ((arg2 & 0xf0) << 4) | (arg1 << 4) |
                (arg2 & 0x0f);

        case OP_LD_X:
            CHECK(arg1, 5); CHECK(arg2, 0);
            return (9 << 12) | (arg1 << 4) | 12;

        case OP_ST_X:
            CHECK(arg1, 5); CHECK(arg2, 0);
            return (0x49 << 9) | (arg1 << 4) | 12;

        case OP_WDR:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x95a8;

        default:
            oops();
            break;
    }
}

//-----------------------------------------------------------------------------
// Write an intel IHEX format description of the program assembled so far.
// This is where we actually do the assembly to binary format.
//-----------------------------------------------------------------------------
static void WriteHexFile(FILE *f)
{
    BYTE soFar[16];
    int soFarCount = 0;
    DWORD soFarStart = 0;

    DWORD i;
    for(i = 0; i < AvrProgWriteP; i++) {
        DWORD w = Assemble(i, AvrProg[i].op, AvrProg[i].arg1, AvrProg[i].arg2);

        if(soFarCount == 0) soFarStart = i;
        soFar[soFarCount++] = (BYTE)(w & 0xff);
        soFar[soFarCount++] = (BYTE)(w >> 8);

        if(soFarCount >= 0x10 || i == (AvrProgWriteP-1)) {
            StartIhex(f);
            WriteIhex(f, soFarCount);
            WriteIhex(f, (BYTE)((soFarStart*2) >> 8));
            WriteIhex(f, (BYTE)((soFarStart*2) & 0xff));
            WriteIhex(f, 0x00);
            int j;
            for(j = 0; j < soFarCount; j++) {
                WriteIhex(f, soFar[j]);
            }
            FinishIhex(f);
            soFarCount = 0;
        }
    }

    // end of file record
    fprintf(f, ":00000001FF\n");
}

//-----------------------------------------------------------------------------
// Make sure that the given address is loaded in the X register; might not
// have to update all of it.
//-----------------------------------------------------------------------------
static void LoadXAddr(DWORD addr)
{
    Instruction(OP_LDI, 27, (addr >> 8));
    Instruction(OP_LDI, 26, (addr & 0xff));
}

//-----------------------------------------------------------------------------
// Generate code to write an 8-bit value to a particular register.
//-----------------------------------------------------------------------------
static void WriteMemory(DWORD addr, BYTE val)
{
    LoadXAddr(addr);
    // load r16 with the data
    Instruction(OP_LDI, 16, val);
    // do the store
    Instruction(OP_ST_X, 16, 0);
}

//-----------------------------------------------------------------------------
// Copy just one bit from one place to another.
//-----------------------------------------------------------------------------
static void CopyBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc)
{
    LoadXAddr(addrSrc); Instruction(OP_LD_X, 16, 0);
    LoadXAddr(addrDest); Instruction(OP_LD_X, 17, 0);
    Instruction(OP_SBRS, 16, bitSrc);
    Instruction(OP_CBR, 17, (1 << bitDest));
    Instruction(OP_SBRC, 16, bitSrc);
    Instruction(OP_SBR, 17, (1 << bitDest));

    Instruction(OP_ST_X, 17, 0);
}

//-----------------------------------------------------------------------------
// Execute the next instruction only if the specified bit of the specified
// memory location is clear (i.e. skip if set).
//-----------------------------------------------------------------------------
static void IfBitClear(DWORD addr, int bit)
{
    LoadXAddr(addr);
    Instruction(OP_LD_X, 16, 0);
    Instruction(OP_SBRS, 16, bit);
}

//-----------------------------------------------------------------------------
// Execute the next instruction only if the specified bit of the specified
// memory location is set (i.e. skip if clear).
//-----------------------------------------------------------------------------
static void IfBitSet(DWORD addr, int bit)
{
    LoadXAddr(addr);
    Instruction(OP_LD_X, 16, 0);
    Instruction(OP_SBRC, 16, bit);
}

//-----------------------------------------------------------------------------
// Set a given bit in an arbitrary (not necessarily I/O memory) location in
// memory.
//-----------------------------------------------------------------------------
static void SetBit(DWORD addr, int bit)
{
    LoadXAddr(addr);
    Instruction(OP_LD_X, 16, 0);
    Instruction(OP_SBR, 16, (1 << bit));
    Instruction(OP_ST_X, 16, 0);
}

//-----------------------------------------------------------------------------
// Clear a given bit in an arbitrary (not necessarily I/O memory) location in
// memory.
//-----------------------------------------------------------------------------
static void ClearBit(DWORD addr, int bit)
{
    LoadXAddr(addr);
    Instruction(OP_LD_X, 16, 0);
    Instruction(OP_CBR, 16, (1 << bit));
    Instruction(OP_ST_X, 16, 0);
}

//-----------------------------------------------------------------------------
// Configure AVR 16-bit Timer1 to do the timing for us.
//-----------------------------------------------------------------------------
static void ConfigureTimer1(int cycleTimeMicroseconds)
{
    int divisor = 1;
    int countsPerCycle;
    while(divisor <= 1024) {
        int timerRate = (Prog.mcuClock / divisor); // hertz
        double timerPeriod = 1e6 / timerRate; // timer period, us
        countsPerCycle = ((int)(cycleTimeMicroseconds / timerPeriod)) - 1;

        if(countsPerCycle < 1000) {
            Error(_("Cycle time too fast; increase cycle time, or use faster "
                "crystal."));
            CompileError();
        } else if(countsPerCycle > 0xffff) {
            if(divisor >= 1024) {
                Error(
                    _("Cycle time too slow; decrease cycle time, or use slower "
                    "crystal."));
                CompileError();
            }
        } else {
            break;
        }

        if(divisor == 1) divisor = 8;
        else if(divisor == 8) divisor = 64;
        else if(divisor == 64) divisor = 256;
        else if(divisor == 256) divisor = 1024;
    }
    WriteMemory(REG_TCCR1A, 0x00); // WGM11=0, WGM10=0

    int csn;
    switch(divisor) {
        case    1: csn = 1; break;
        case    8: csn = 2; break;
        case   64: csn = 3; break;
        case  256: csn = 4; break;
        case 1024: csn = 5; break;
        default: oops();
    }
    
    WriteMemory(REG_TCCR1B, (1<<3) | csn); // WGM13=0, WGM12=1

    // `the high byte must be written before the low byte'
    WriteMemory(REG_OCR1AH, (countsPerCycle - 1) >> 8);
    WriteMemory(REG_OCR1AL, (countsPerCycle - 1) & 0xff);
    
    // Okay, so many AVRs have a register called TIFR, but the meaning of
    // the bits in that register varies from device to device...
    if(strcmp(Prog.mcu->mcuName, "Atmel AVR ATmega162 40-PDIP")==0) {
        WriteMemory(REG_TIMSK, (1 << 6));
    } else {
        WriteMemory(REG_TIMSK, (1 << 4));
    }
}

//-----------------------------------------------------------------------------
// Write the basic runtime. We set up our reset vector, configure all the
// I/O pins, then set up the timer that does the cycling. Next instruction
// written after calling WriteRuntime should be first instruction of the
// timer loop (i.e. the PLC logic cycle).
//-----------------------------------------------------------------------------
static void WriteRuntime(void)
{
    DWORD resetVector = AllocFwdAddr();

    int i;
    Instruction(OP_RJMP, resetVector, 0);       // $0000, RESET
    for(i = 0; i < 34; i++)
        Instruction(OP_RETI, 0, 0);

    FwdAddrIsNow(resetVector);

    // set up the stack, which we use only when we jump to multiply/divide
    // routine
    WORD topOfMemory = (WORD)Prog.mcu->ram[0].start + Prog.mcu->ram[0].len - 1;
    WriteMemory(REG_SPH, topOfMemory >> 8);
    WriteMemory(REG_SPL, topOfMemory & 0xff);

    // zero out the memory used for timers, internal relays, etc.
    LoadXAddr(Prog.mcu->ram[0].start + Prog.mcu->ram[0].len);
    Instruction(OP_LDI, 16, 0);
    Instruction(OP_LDI, 18, (Prog.mcu->ram[0].len) & 0xff);
    Instruction(OP_LDI, 19, (Prog.mcu->ram[0].len) >> 8);

    DWORD loopZero = AvrProgWriteP;
    Instruction(OP_SUBI, 26, 1);
    Instruction(OP_SBCI, 27, 0);
    Instruction(OP_ST_X, 16, 0);
    Instruction(OP_SUBI, 18, 1);
    Instruction(OP_SBCI, 19, 0);
    Instruction(OP_TST, 18, 0);
    Instruction(OP_BRNE, loopZero, 0);
    Instruction(OP_TST, 19, 0);
    Instruction(OP_BRNE, loopZero, 0);
    

    // set up I/O pins
    BYTE isInput[MAX_IO_PORTS], isOutput[MAX_IO_PORTS];
    BuildDirectionRegisters(isInput, isOutput);

    if(UartFunctionUsed()) {
        if(Prog.baudRate == 0) {
            Error(_("Zero baud rate not possible."));
            return;
        }

        // bps = Fosc/(16*(X+1))
        // bps*16*(X + 1) = Fosc
        // X = Fosc/(bps*16)-1
        // and round, don't truncate
        int divisor = (Prog.mcuClock + Prog.baudRate*8)/(Prog.baudRate*16) - 1;

        double actual = Prog.mcuClock/(16.0*(divisor+1));
        double percentErr = 100*(actual - Prog.baudRate)/Prog.baudRate;

        if(fabs(percentErr) > 2) {
            ComplainAboutBaudRateError(divisor, actual, percentErr);
        }
        if(divisor > 4095) ComplainAboutBaudRateOverflow();
        
        WriteMemory(REG_UBRRH, divisor >> 8);
        WriteMemory(REG_UBRRL, divisor & 0xff);
        WriteMemory(REG_UCSRB, (1 << 4) | (1 << 3)); // RXEN, TXEN

        for(i = 0; i < Prog.mcu->pinCount; i++) {
            if(Prog.mcu->pinInfo[i].pin == Prog.mcu->uartNeeds.txPin) {
                McuIoPinInfo *iop = &(Prog.mcu->pinInfo[i]);
                isOutput[iop->port - 'A'] |= (1 << iop->bit);
                break;
            }
        }
        if(i == Prog.mcu->pinCount) oops();
    }

    if(PwmFunctionUsed()) {
        for(i = 0; i < Prog.mcu->pinCount; i++) {
            if(Prog.mcu->pinInfo[i].pin == Prog.mcu->pwmNeedsPin) {
                McuIoPinInfo *iop = &(Prog.mcu->pinInfo[i]);
                isOutput[iop->port - 'A'] |= (1 << iop->bit);
                break;
            }
        }
        if(i == Prog.mcu->pinCount) oops();
    }

    for(i = 0; Prog.mcu->dirRegs[i] != 0; i++) {
        if(Prog.mcu->dirRegs[i] == 0xff && Prog.mcu->outputRegs[i] == 0xff) {
            // skip this one, dummy entry for MCUs with I/O ports not
            // starting from A
        } else {
            WriteMemory(Prog.mcu->dirRegs[i], isOutput[i]);
            // turn on the pull-ups, and drive the outputs low to start
            WriteMemory(Prog.mcu->outputRegs[i], isInput[i]);
        }
    }


    ConfigureTimer1(Prog.cycleTime);

    // and now the generated PLC code will follow
    BeginningOfCycleAddr = AvrProgWriteP;

    // Okay, so many AVRs have a register called TIFR, but the meaning of
    // the bits in that register varies from device to device...
    int tifrBitForOCF1A;
    if(strcmp(Prog.mcu->mcuName, "Atmel AVR ATmega162 40-PDIP")==0) {
        tifrBitForOCF1A = 6;
    } else {
        tifrBitForOCF1A = 4;
    }

    DWORD now = AvrProgWriteP;
    IfBitClear(REG_TIFR, tifrBitForOCF1A);
    Instruction(OP_RJMP, now, 0);

    SetBit(REG_TIFR, tifrBitForOCF1A);

    Instruction(OP_WDR, 0, 0);
}

//-----------------------------------------------------------------------------
// Handle an IF statement. Flow continues to the first instruction generated
// by this function if the condition is true, else it jumps to the given
// address (which is an FwdAddress, so not yet assigned). Called with IntPc
// on the IF statement, returns with IntPc on the END IF.
//-----------------------------------------------------------------------------
static void CompileIfBody(DWORD condFalse)
{
    IntPc++;
    CompileFromIntermediate();
    if(IntCode[IntPc].op == INT_ELSE) {
        IntPc++;
        DWORD endBlock = AllocFwdAddr();
        Instruction(OP_RJMP, endBlock, 0);

        FwdAddrIsNow(condFalse);
        CompileFromIntermediate();
        FwdAddrIsNow(endBlock);
    } else {
        FwdAddrIsNow(condFalse);
    }

    if(IntCode[IntPc].op != INT_END_IF) oops();
}

//-----------------------------------------------------------------------------
// Call a subroutine, using either an rcall or an icall depending on what
// the processor supports or requires.
//-----------------------------------------------------------------------------
static void CallSubroutine(DWORD addr)
{
    if(Prog.mcu->avrUseIjmp) {
        Instruction(OP_LDI, 30, FWD_LO(addr));
        Instruction(OP_LDI, 31, FWD_HI(addr));
        Instruction(OP_ICALL, 0, 0);
    } else {
        Instruction(OP_RCALL, addr, 0);
    }
}

//-----------------------------------------------------------------------------
// Compile the intermediate code to AVR native code.
//-----------------------------------------------------------------------------
static void CompileFromIntermediate(void)
{   
    DWORD addr, addr2;
    int bit, bit2;
    DWORD addrl, addrh;
    DWORD addrl2, addrh2;

    for(; IntPc < IntCodeLen; IntPc++) {
        IntOp *a = &IntCode[IntPc];
        switch(a->op) {
            case INT_SET_BIT:   
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                SetBit(addr, bit);
                break;

            case INT_CLEAR_BIT:
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                ClearBit(addr, bit);
                break;

            case INT_COPY_BIT_TO_BIT:
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                MemForSingleBit(a->name2, FALSE, &addr2, &bit2);
                CopyBit(addr, bit, addr2, bit2);
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                MemForVariable(a->name1, &addrl, &addrh);
                WriteMemory(addrl, a->literal & 0xff);
                WriteMemory(addrh, a->literal >> 8);
                break;

            case INT_INCREMENT_VARIABLE: {
                MemForVariable(a->name1, &addrl, &addrh);
                LoadXAddr(addrl);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrh);
                Instruction(OP_LD_X, 17, 0);
                // increment
                Instruction(OP_INC, 16, 0);
                DWORD noCarry = AllocFwdAddr();
                Instruction(OP_BRNE, noCarry, 0);
                Instruction(OP_INC, 17, 0);
                FwdAddrIsNow(noCarry);
                // X is still addrh
                Instruction(OP_ST_X, 17, 0);
                LoadXAddr(addrl);
                Instruction(OP_ST_X, 16, 0);
                break;
            }
            case INT_IF_BIT_SET: {
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                IfBitClear(addr, bit);
                Instruction(OP_RJMP, condFalse, 0);
                CompileIfBody(condFalse);
                break;
            }
            case INT_IF_BIT_CLEAR: {
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                IfBitSet(addr, bit);
                Instruction(OP_RJMP, condFalse, 0);
                CompileIfBody(condFalse);
                break;
            }
            case INT_IF_VARIABLE_LES_LITERAL: {
                DWORD notTrue = AllocFwdAddr();

                MemForVariable(a->name1, &addrl, &addrh);
                LoadXAddr(addrl);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrh);
                Instruction(OP_LD_X, 17, 0);

                Instruction(OP_LDI, 18, (a->literal & 0xff));
                Instruction(OP_LDI, 19, (a->literal >> 8));

                Instruction(OP_CP, 16, 18);
                Instruction(OP_CPC, 17, 19);
                Instruction(OP_BRGE, notTrue, 0);

                CompileIfBody(notTrue);
                break;
            }
            case INT_IF_VARIABLE_GRT_VARIABLE:
            case INT_IF_VARIABLE_EQUALS_VARIABLE: {
                DWORD notTrue = AllocFwdAddr();

                MemForVariable(a->name1, &addrl, &addrh);
                LoadXAddr(addrl);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrh);
                Instruction(OP_LD_X, 17, 0);
                MemForVariable(a->name2, &addrl, &addrh);
                LoadXAddr(addrl);
                Instruction(OP_LD_X, 18, 0);
                LoadXAddr(addrh);
                Instruction(OP_LD_X, 19, 0);

                if(a->op == INT_IF_VARIABLE_EQUALS_VARIABLE) {
                    Instruction(OP_CP, 16, 18);
                    Instruction(OP_CPC, 17, 19);
                    Instruction(OP_BRNE, notTrue, 0);
                } else if(a->op == INT_IF_VARIABLE_GRT_VARIABLE) {
                    DWORD isTrue = AllocFwdAddr();

                    // true if op1 > op2
                    // false if op1 >= op2
                    Instruction(OP_CP, 18, 16);
                    Instruction(OP_CPC, 19, 17);
                    Instruction(OP_BRGE, notTrue, 0);
                } else oops();
                CompileIfBody(notTrue);
                break;
            }
            case INT_SET_VARIABLE_TO_VARIABLE:
                MemForVariable(a->name1, &addrl, &addrh);
                MemForVariable(a->name2, &addrl2, &addrh2);

                LoadXAddr(addrl2);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrl);
                Instruction(OP_ST_X, 16, 0);

                LoadXAddr(addrh2);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrh);
                Instruction(OP_ST_X, 16, 0);
                break;

            case INT_SET_VARIABLE_DIVIDE:
                // Do this one separately since the divide routine uses
                // slightly different in/out registers and I don't feel like
                // modifying it.
                MemForVariable(a->name2, &addrl, &addrh);
                MemForVariable(a->name3, &addrl2, &addrh2);

                LoadXAddr(addrl2);
                Instruction(OP_LD_X, 18, 0);
                LoadXAddr(addrh2);
                Instruction(OP_LD_X, 19, 0);

                LoadXAddr(addrl);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrh);
                Instruction(OP_LD_X, 17, 0);

                CallSubroutine(DivideAddress);
                DivideUsed = TRUE;
                
                MemForVariable(a->name1, &addrl, &addrh);

                LoadXAddr(addrl);
                Instruction(OP_ST_X, 16, 0);
                LoadXAddr(addrh);
                Instruction(OP_ST_X, 17, 0);
                break;

            case INT_SET_VARIABLE_ADD:
            case INT_SET_VARIABLE_SUBTRACT:
            case INT_SET_VARIABLE_MULTIPLY:
                MemForVariable(a->name2, &addrl, &addrh);
                MemForVariable(a->name3, &addrl2, &addrh2);

                LoadXAddr(addrl);
                Instruction(OP_LD_X, 18, 0);
                LoadXAddr(addrh);
                Instruction(OP_LD_X, 19, 0);

                LoadXAddr(addrl2);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrh2);
                Instruction(OP_LD_X, 17, 0);

                if(a->op == INT_SET_VARIABLE_ADD) {
                    Instruction(OP_ADD, 18, 16);
                    Instruction(OP_ADC, 19, 17);
                } else if(a->op == INT_SET_VARIABLE_SUBTRACT) {
                    Instruction(OP_SUB, 18, 16);
                    Instruction(OP_SBC, 19, 17);
                } else if(a->op == INT_SET_VARIABLE_MULTIPLY) {
                    CallSubroutine(MultiplyAddress);
                    MultiplyUsed = TRUE;
                } else oops();

                MemForVariable(a->name1, &addrl, &addrh);

                LoadXAddr(addrl);
                Instruction(OP_ST_X, 18, 0);
                LoadXAddr(addrh);
                Instruction(OP_ST_X, 19, 0);
                break;

            case INT_SET_PWM: {
                int target = atoi(a->name2);

                // PWM frequency is 
                //   target = xtal/(256*prescale)
                // so not a lot of room for accurate frequency here

                int prescale;
                int bestPrescale;
                int bestError = INT_MAX;
                int bestFreq;
                for(prescale = 1;;) {
                    int freq = (Prog.mcuClock + prescale*128)/(prescale*256);

                    int err = abs(freq - target);
                    if(err < bestError) {
                        bestError = err;
                        bestPrescale = prescale;
                        bestFreq = freq;
                    }
                    
                    if(prescale == 1) {
                        prescale = 8;
                    } else if(prescale == 8) {
                        prescale = 64;
                    } else if(prescale == 64) {
                        prescale = 256;
                    } else if(prescale == 256) {
                        prescale = 1024;
                    } else {
                        break;
                    }
                }

                if(((double)bestError)/target > 0.05) {
                    Error(_("Target frequency %d Hz, closest achievable is "
                        "%d Hz (warning, >5%% error)."), target, bestFreq);
                }

                DivideUsed = TRUE; MultiplyUsed = TRUE;
                MemForVariable(a->name1, &addrl, &addrh);
                LoadXAddr(addrl);
                Instruction(OP_LD_X, 16, 0);
                Instruction(OP_LDI, 17, 0);
                Instruction(OP_LDI, 19, 0);
                Instruction(OP_LDI, 18, 255);
                CallSubroutine(MultiplyAddress);
                Instruction(OP_MOV, 17, 19);
                Instruction(OP_MOV, 16, 18);
                Instruction(OP_LDI, 19, 0);
                Instruction(OP_LDI, 18, 100);
                CallSubroutine(DivideAddress);
                LoadXAddr(REG_OCR2);
                Instruction(OP_ST_X, 16, 0);

                // Setup only happens once
                MemForSingleBit("$pwm_init", FALSE, &addr, &bit);
                DWORD skip = AllocFwdAddr();
                IfBitSet(addr, bit);
                Instruction(OP_RJMP, skip, 0);
                SetBit(addr, bit);

                BYTE cs;
                switch(bestPrescale) {
                    case    1: cs = 1; break;
                    case    8: cs = 2; break;
                    case   64: cs = 3; break;
                    case  256: cs = 4; break;
                    case 1024: cs = 5; break;
                    default: oops(); break;
                }
    
                // fast PWM mode, non-inverted operation, given prescale
                WriteMemory(REG_TCCR2, (1 << 6) | (1 << 3) | (1 << 5) | cs);

                FwdAddrIsNow(skip);

                break;
            }
            case INT_EEPROM_BUSY_CHECK: {
                MemForSingleBit(a->name1, FALSE, &addr, &bit);

                DWORD isBusy = AllocFwdAddr();
                DWORD done = AllocFwdAddr();
                IfBitSet(REG_EECR, 1);
                Instruction(OP_RJMP, isBusy, 0);

                IfBitClear(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
                Instruction(OP_RJMP, done, 0);

                // Just increment EEARH:EEARL, to point to the high byte of
                // whatever we just wrote the low byte for.
                LoadXAddr(REG_EEARL);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(REG_EEARH);
                Instruction(OP_LD_X, 17, 0);
                Instruction(OP_INC, 16, 0);
                DWORD noCarry = AllocFwdAddr();
                Instruction(OP_BRNE, noCarry, 0);
                Instruction(OP_INC, 17, 0);
                FwdAddrIsNow(noCarry);
                // X is still REG_EEARH
                Instruction(OP_ST_X, 17, 0);
                LoadXAddr(REG_EEARL);
                Instruction(OP_ST_X, 16, 0);

                LoadXAddr(EepromHighByte);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(REG_EEDR);
                Instruction(OP_ST_X, 16, 0);
                LoadXAddr(REG_EECR);
                Instruction(OP_LDI, 16, 0x04);
                Instruction(OP_ST_X, 16, 0);
                Instruction(OP_LDI, 16, 0x06);
                Instruction(OP_ST_X, 16, 0);

                ClearBit(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);

                FwdAddrIsNow(isBusy);
                SetBit(addr, bit);
                FwdAddrIsNow(done);
                break;
            }
            case INT_EEPROM_READ: {
                MemForVariable(a->name1, &addrl, &addrh);
                int i;
                for(i = 0; i < 2; i++) {
                    WriteMemory(REG_EEARH, ((a->literal+i) >> 8));
                    WriteMemory(REG_EEARL, ((a->literal+i) & 0xff));
                    WriteMemory(REG_EECR, 0x01);
                    LoadXAddr(REG_EEDR);
                    Instruction(OP_LD_X, 16, 0);
                    if(i == 0) {
                        LoadXAddr(addrl);
                    } else {
                        LoadXAddr(addrh);
                    }
                    Instruction(OP_ST_X, 16, 0);
                }
                break;
            }
            case INT_EEPROM_WRITE:
                MemForVariable(a->name1, &addrl, &addrh);
                SetBit(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
                LoadXAddr(addrh);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(EepromHighByte);
                Instruction(OP_ST_X, 16, 0);

                WriteMemory(REG_EEARH, (a->literal >> 8));
                WriteMemory(REG_EEARL, (a->literal & 0xff));
                LoadXAddr(addrl);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(REG_EEDR);
                Instruction(OP_ST_X, 16, 0);
                LoadXAddr(REG_EECR);
                Instruction(OP_LDI, 16, 0x04);
                Instruction(OP_ST_X, 16, 0);
                Instruction(OP_LDI, 16, 0x06);
                Instruction(OP_ST_X, 16, 0);
                break;
            
            case INT_READ_ADC: {
                MemForVariable(a->name1, &addrl, &addrh);

                WriteMemory(REG_ADMUX, 
                    (0 << 6) |              // AREF, internal Vref odd
                    (0 << 5) |              // right-adjusted
                    MuxForAdcVariable(a->name1));

                // target something around 200 kHz for the ADC clock, for
                // 25/(200k) or 125 us conversion time, reasonable
                int divisor = (Prog.mcuClock / 200000);
                int j = 0;
                for(j = 1; j <= 7; j++) {
                    if((1 << j) > divisor) break;
                }

                BYTE adcsra = 
                    (1 << 7) |              // ADC enabled
                    (0 << 5) |              // not free running
                    (0 << 3) |              // no interrupt enabled
                    j;                      // prescaler setup
            
                WriteMemory(REG_ADCSRA, adcsra);
                WriteMemory(REG_ADCSRA, (BYTE)(adcsra | (1 << 6)));

                DWORD waitForFinsh = AvrProgWriteP;
                IfBitSet(REG_ADCSRA, 6);
                Instruction(OP_RJMP, waitForFinsh, 0);

                LoadXAddr(REG_ADCL);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrl);
                Instruction(OP_ST_X, 16, 0);

                LoadXAddr(REG_ADCH);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrh);
                Instruction(OP_ST_X, 16, 0);

                break;
            }
            case INT_UART_SEND: {
                MemForVariable(a->name1, &addrl, &addrh);
                MemForSingleBit(a->name2, TRUE, &addr, &bit);

                DWORD noSend = AllocFwdAddr();
                IfBitClear(addr, bit);
                Instruction(OP_RJMP, noSend, 0);

                LoadXAddr(addrl);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(REG_UDR);
                Instruction(OP_ST_X, 16, 0);

                FwdAddrIsNow(noSend);

                ClearBit(addr, bit);
                DWORD dontSet = AllocFwdAddr();
                IfBitSet(REG_UCSRA, 5); // UDRE, is 1 when tx buffer is empty
                Instruction(OP_RJMP, dontSet, 0);
                SetBit(addr, bit);
                FwdAddrIsNow(dontSet);

                break;
            }
            case INT_UART_RECV: {
                MemForVariable(a->name1, &addrl, &addrh);
                MemForSingleBit(a->name2, TRUE, &addr, &bit);
        
                ClearBit(addr, bit);

                DWORD noChar = AllocFwdAddr();
                IfBitClear(REG_UCSRA, 7);
                Instruction(OP_RJMP, noChar, 0);

                SetBit(addr, bit);
                LoadXAddr(REG_UDR);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addrl);
                Instruction(OP_ST_X, 16, 0);
                
                LoadXAddr(addrh);
                Instruction(OP_LDI, 16, 0);
                Instruction(OP_ST_X, 16, 0);

                FwdAddrIsNow(noChar);
                break;
            }
            case INT_END_IF:
            case INT_ELSE:
                return;

            case INT_SIMULATE_NODE_STATE:
            case INT_COMMENT:
                break;

            default:
                oops();
                break;
        }
    }
}

//-----------------------------------------------------------------------------
// 16x16 signed multiply, code from Atmel app note AVR200. op1 in r17:16,
// op2 in r19:18, result low word goes into r19:18.
//-----------------------------------------------------------------------------
static void MultiplyRoutine(void)
{
    FwdAddrIsNow(MultiplyAddress);

    DWORD m16s_1;
    DWORD m16s_2 = AllocFwdAddr();

    Instruction(OP_SUB, 21, 21);
    Instruction(OP_SUB, 20, 20);
    Instruction(OP_LDI, 22, 16);
    m16s_1 = AvrProgWriteP; Instruction(OP_BRCC, m16s_2, 0);
    Instruction(OP_ADD, 20, 16);
    Instruction(OP_ADC, 21, 17);
    FwdAddrIsNow(m16s_2); Instruction(OP_SBRC, 18, 0);
    Instruction(OP_SUB, 20, 16);
    Instruction(OP_SBRC, 18, 0);
    Instruction(OP_SBC, 21, 17);
    Instruction(OP_ASR, 21, 0);
    Instruction(OP_ROR, 20, 0);
    Instruction(OP_ROR, 19, 0);
    Instruction(OP_ROR, 18, 0);
    Instruction(OP_DEC, 22, 0);
    Instruction(OP_BRNE, m16s_1, 0);
    Instruction(OP_RET, 0, 0);
}

//-----------------------------------------------------------------------------
// 16/16 signed divide, code from the same app note. Dividend in r17:16,
// divisor in r19:18, result goes in r17:16 (and remainder in r15:14).
//-----------------------------------------------------------------------------
static void DivideRoutine(void)
{
    FwdAddrIsNow(DivideAddress);

    DWORD d16s_1 = AllocFwdAddr();
    DWORD d16s_2 = AllocFwdAddr();
    DWORD d16s_3;
    DWORD d16s_4 = AllocFwdAddr();
    DWORD d16s_5 = AllocFwdAddr();
    DWORD d16s_6 = AllocFwdAddr();

    Instruction(OP_MOV, 13, 17);
    Instruction(OP_EOR, 13, 19);
    Instruction(OP_SBRS, 17, 7);
    Instruction(OP_RJMP, d16s_1, 0);
    Instruction(OP_COM, 17, 0);
    Instruction(OP_COM, 16, 0);
    Instruction(OP_SUBI, 16, 0xff);
    Instruction(OP_SBCI, 17, 0xff);
    FwdAddrIsNow(d16s_1); Instruction(OP_SBRS, 19, 7);
    Instruction(OP_RJMP, d16s_2, 0);
    Instruction(OP_COM, 19, 0);
    Instruction(OP_COM, 18, 0);
    Instruction(OP_SUBI, 18, 0xff);
    Instruction(OP_SBCI, 19, 0xff);
    FwdAddrIsNow(d16s_2); Instruction(OP_EOR, 14, 14);
    Instruction(OP_SUB, 15, 15);
    Instruction(OP_LDI, 20, 17);

    d16s_3 = AvrProgWriteP; Instruction(OP_ADC, 16, 16);
    Instruction(OP_ADC, 17, 17);
    Instruction(OP_DEC, 20, 0);
    Instruction(OP_BRNE, d16s_5, 0);
    Instruction(OP_SBRS, 13, 7);
    Instruction(OP_RJMP, d16s_4, 0);
    Instruction(OP_COM, 17, 0);
    Instruction(OP_COM, 16, 0);
    Instruction(OP_SUBI, 16, 0xff);
    Instruction(OP_SBCI, 17, 0xff);
    FwdAddrIsNow(d16s_4); Instruction(OP_RET, 0, 0);
    FwdAddrIsNow(d16s_5); Instruction(OP_ADC, 14, 14);
    Instruction(OP_ADC, 15, 15);
    Instruction(OP_SUB, 14, 18);
    Instruction(OP_SBC, 15, 19);
    Instruction(OP_BRCC, d16s_6, 0);
    Instruction(OP_ADD, 14, 18);
    Instruction(OP_ADC, 15, 19);
    Instruction(OP_CLC, 0, 0);
    Instruction(OP_RJMP, d16s_3, 0);
    FwdAddrIsNow(d16s_6); Instruction(OP_SEC, 0, 0);
    Instruction(OP_RJMP, d16s_3, 0);
}

//-----------------------------------------------------------------------------
// Compile the program to REG code for the currently selected processor
// and write it to the given file. Produce an error message if we cannot
// write to the file, or if there is something inconsistent about the
// program.
//-----------------------------------------------------------------------------
void CompileAvr(char *outFile)
{
    FILE *f = fopen(outFile, "w");
    if(!f) {
        Error(_("Couldn't open file '%s'"), outFile);
        return;
    }

    if(setjmp(CompileErrorBuf) != 0) {
        fclose(f);
        return;
    }

    // Here we must set up the addresses of some registers that for some
    // stupid reason move around from AVR to AVR.
    if(strcmp(Prog.mcu->mcuName, "Atmel AVR ATmega16 40-PDIP")==0  ||
       strcmp(Prog.mcu->mcuName, "Atmel AVR ATmega32 40-PDIP")==0 ||
       strcmp(Prog.mcu->mcuName, "Atmel AVR ATmega162 40-PDIP")==0 ||
       strcmp(Prog.mcu->mcuName, "Atmel AVR ATmega8 28-PDIP")==0)
    {
        REG_TIMSK = 0x59;
        REG_TIFR = 0x58;
        REG_UBRRH = 0x40;
        REG_UBRRL = 0x29;
        REG_UCSRB = 0x2a;
        REG_UCSRA = 0x2b;
        REG_UDR = 0x2c;
    } else {
        REG_TIMSK = 0x57;
        REG_TIFR = 0x56;
        REG_UBRRH = 0x98;
        REG_UBRRL = 0x99;
        REG_UCSRB = 0x9a;
        REG_UCSRA = 0x9b;
        REG_UDR = 0x9c;
    }

    WipeMemory();
    MultiplyUsed = FALSE;
    MultiplyAddress = AllocFwdAddr();
    DivideUsed = FALSE;
    DivideAddress = AllocFwdAddr();
    AllocStart();

    // Where we hold the high byte to program in EEPROM while the low byte
    // programs.
    EepromHighByte = AllocOctetRam();
    AllocBitRam(&EepromHighByteWaitingAddr, &EepromHighByteWaitingBit);

    WriteRuntime();
    IntPc = 0;
    CompileFromIntermediate();

    if(Prog.mcu->avrUseIjmp) {
        Instruction(OP_LDI, 30, (BeginningOfCycleAddr & 0xff));
        Instruction(OP_LDI, 31, (BeginningOfCycleAddr >> 8));
        Instruction(OP_IJMP, 0, 0);
    } else {
        Instruction(OP_RJMP, BeginningOfCycleAddr, 0);
    }

    MemCheckForErrorsPostCompile();

    if(MultiplyUsed) MultiplyRoutine();
    if(DivideUsed) DivideRoutine();

    WriteHexFile(f);
    fclose(f);

    char str[MAX_PATH+500];
    sprintf(str, _("Compile successful; wrote IHEX for AVR to '%s'.\r\n\r\n"
        "Remember to set the processor configuration (fuses) correctly. "
        "This does not happen automatically."), outFile);
    CompileSuccessfulMessage(str);
}
