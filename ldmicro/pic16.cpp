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
// A PIC16... assembler, for our own internal use, plus routines to generate
// code from the ladder logic structure, plus routines to generate the
// runtime needed to schedule the cycles.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

#include "ldmicro.h"
#include "intcode.h"

// not complete; just what I need
typedef enum Pic16OpTag {
    OP_VACANT,
    OP_ADDWF,
    OP_ANDWF,
    OP_CALL,
    OP_BSF,
    OP_BCF,
    OP_BTFSC,
    OP_BTFSS,
    OP_GOTO,
    OP_CLRF,
    OP_CLRWDT,
    OP_COMF,
    OP_DECF,
    OP_DECFSZ,
    OP_INCF,
    OP_INCFSZ,
    OP_IORWF,
    OP_MOVLW,
    OP_MOVF,
    OP_MOVWF,
    OP_NOP,
    OP_RETFIE,
    OP_RETURN,
    OP_RLF,
    OP_RRF,
    OP_SUBLW,
    OP_SUBWF,
    OP_XORWF,
} Pic16Op;

#define DEST_F 1
#define DEST_W 0

#define STATUS_RP1  6
#define STATUS_RP0  5
#define STATUS_Z    2
#define STATUS_C    0

typedef struct Pic16InstructionTag {
    Pic16Op     op;
    DWORD       arg1;
    DWORD       arg2;
} Pic16Instruction;

#define MAX_PROGRAM_LEN 128*1024
static Pic16Instruction PicProg[MAX_PROGRAM_LEN];
static DWORD PicProgWriteP;

// Scratch variables, for temporaries
static DWORD Scratch0;
static DWORD Scratch1;
static DWORD Scratch2;
static DWORD Scratch3;
static DWORD Scratch4;
static DWORD Scratch5;
static DWORD Scratch6;
static DWORD Scratch7;

// The extra byte to program, for the EEPROM (because we can only set
// up one byte to program at a time, and we will be writing two-byte
// variables, in general).
static DWORD EepromHighByte;
static DWORD EepromHighByteWaitingAddr;
static int EepromHighByteWaitingBit;

// Subroutines to do multiply/divide
static DWORD MultiplyRoutineAddress;
static DWORD DivideRoutineAddress;
static BOOL MultiplyNeeded;
static BOOL DivideNeeded;

// For yet unresolved references in jumps
static DWORD FwdAddrCount;

// As I start to support the paging; it is sometimes necessary to pick
// out the high vs. low portions of the address, so that the high portion
// goes in PCLATH, and the low portion is just used for the jump.
#define FWD_LO(x)   ((x) | 0x20000000)
#define FWD_HI(x)   ((x) | 0x40000000)

// Some useful registers, which I think are mostly in the same place on
// all the PIC16... devices.
#define REG_INDF      0x00
#define REG_STATUS    0x03
#define REG_FSR       0x04
#define REG_PCLATH    0x0a
#define REG_INTCON    0x0b
#define REG_PIR1      0x0c
#define REG_PIE1      0x8c
#define REG_TMR1L     0x0e
#define REG_TMR1H     0x0f
#define REG_T1CON     0x10
#define REG_CCPR1L    0x15
#define REG_CCPR1H    0x16
#define REG_CCP1CON   0x17
#define REG_CMCON     0x1f

#define REG_TXSTA     0x98
#define REG_RCSTA     0x18
#define REG_SPBRG     0x99
#define REG_TXREG     0x19
#define REG_RCREG     0x1a

#define REG_ADRESH    0x1e
#define REG_ADRESL    0x9e
#define REG_ADCON0    0x1f
#define REG_ADCON1    0x9f

#define REG_T2CON     0x12
#define REG_CCPR2L    0x1b
#define REG_CCP2CON   0x1d
#define REG_PR2       0x92

// These move around from device to device.
static DWORD REG_EECON1;
static DWORD REG_EECON2;
static DWORD REG_EEDATA;
static DWORD REG_EEADR;
static DWORD REG_ANSEL;
static DWORD REG_ANSELH;

static int IntPc;

static void CompileFromIntermediate(BOOL topLevel);

//-----------------------------------------------------------------------------
// A convenience function, whether we are using a particular MCU.
//-----------------------------------------------------------------------------
static BOOL McuIs(char *str)
{
    return strcmp(Prog.mcu->mcuName, str) == 0;
}

//-----------------------------------------------------------------------------
// Wipe the program and set the write pointer back to the beginning.
//-----------------------------------------------------------------------------
static void WipeMemory(void)
{
    memset(PicProg, 0, sizeof(PicProg));
    PicProgWriteP = 0;
}

//-----------------------------------------------------------------------------
// Store an instruction at the next spot in program memory.  Error condition
// if this spot is already filled. We don't actually assemble to binary yet;
// there may be references to resolve.
//-----------------------------------------------------------------------------
static void Instruction(Pic16Op op, DWORD arg1, DWORD arg2)
{
    if(PicProg[PicProgWriteP].op != OP_VACANT) oops();

    PicProg[PicProgWriteP].op = op;
    PicProg[PicProgWriteP].arg1 = arg1;
    PicProg[PicProgWriteP].arg2 = arg2;
    PicProgWriteP++;
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

    BOOL seen = FALSE;
    DWORD i;
    for(i = 0; i < PicProgWriteP; i++) {
        if(PicProg[i].arg1 == addr) {
            // Insist that they be in the same page, but otherwise assume
            // that PCLATH has already been set up appropriately.
            if((i >> 11) != (PicProgWriteP >> 11)) {
                Error(_("Internal error relating to PIC paging; make program "
                    "smaller or reshuffle it."));
                CompileError();
            }
            PicProg[i].arg1 = PicProgWriteP;
            seen = TRUE;
        } else if(PicProg[i].arg1 == FWD_LO(addr)) {
            PicProg[i].arg1 = (PicProgWriteP & 0x7ff);
            seen = TRUE;
        } else if(PicProg[i].arg1 == FWD_HI(addr)) {
            PicProg[i].arg1 = (PicProgWriteP >> 8);
        }
    }
    if(!seen) oops();
}

//-----------------------------------------------------------------------------
// Given an opcode and its operands, assemble the 14-bit instruction for the
// PIC. Check that the operands do not have more bits set than is meaningful;
// it is an internal error if they do.
//-----------------------------------------------------------------------------
static DWORD Assemble(Pic16Op op, DWORD arg1, DWORD arg2)
{
#define CHECK(v, bits) if((v) != ((v) & ((1 << (bits))-1))) oops()
    switch(op) {
        case OP_ADDWF:
            CHECK(arg2, 1); CHECK(arg1, 7);
            return (7 << 8) | (arg2 << 7) | arg1;

        case OP_ANDWF:
            CHECK(arg2, 1); CHECK(arg1, 7);
            return (5 << 8) | (arg2 << 7) | arg1;

        case OP_BSF:
            CHECK(arg2, 3); CHECK(arg1, 7);
            return (5 << 10) | (arg2 << 7) | arg1;

        case OP_BCF:
            CHECK(arg2, 3); CHECK(arg1, 7);
            return (4 << 10) | (arg2 << 7) | arg1;

        case OP_BTFSC:
            CHECK(arg2, 3); CHECK(arg1, 7);
            return (6 << 10) | (arg2 << 7) | arg1;

        case OP_BTFSS:
            CHECK(arg2, 3); CHECK(arg1, 7);
            return (7 << 10) | (arg2 << 7) | arg1;

        case OP_CLRF:
            CHECK(arg1, 7); CHECK(arg2, 0);
            return (3 << 7) | arg1;

        case OP_CLRWDT:
            return 0x0064;

        case OP_COMF:
            CHECK(arg2, 1); CHECK(arg1, 7);
            return (9 << 8) | (arg2 << 7) | arg1;

        case OP_DECF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            return (3 << 8) | (arg2 << 7) | arg1;

        case OP_DECFSZ:
            CHECK(arg1, 7); CHECK(arg2, 1);
            return (11 << 8) | (arg2 << 7) | arg1;

        case OP_GOTO:
            // Very special case: we will assume that the PCLATH stuff has
            // been taken care of already.
            arg1 &= 0x7ff;
            CHECK(arg1, 11); CHECK(arg2, 0);
            return (5 << 11) | arg1;

        case OP_CALL:
            CHECK(arg1, 11); CHECK(arg2, 0);
            return (4 << 11) | arg1;

        case OP_INCF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            return (10 << 8) | (arg2 << 7) | arg1;

        case OP_INCFSZ:
            CHECK(arg1, 7); CHECK(arg2, 1);
            return (15 << 8) | (arg2 << 7) | arg1;

        case OP_IORWF:
            CHECK(arg2, 1); CHECK(arg1, 7);
            return (4 << 8) | (arg2 << 7) | arg1;

        case OP_MOVLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            return (3 << 12) | arg1;

        case OP_MOVF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            return (8 << 8) | (arg2 << 7) | arg1;

        case OP_MOVWF:
            CHECK(arg1, 7); CHECK(arg2, 0);
            return (1 << 7) | arg1;

        case OP_NOP:
            return 0x0000;

        case OP_RETURN:
            return 0x0008;

        case OP_RETFIE:
            return 0x0009;

        case OP_RLF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            return (13 << 8) | (arg2 << 7) | arg1;

        case OP_RRF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            return (12 << 8) | (arg2 << 7) | arg1;

        case OP_SUBLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            return (15 << 9) | arg1;

        case OP_SUBWF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            return (2 << 8) | (arg2 << 7) | arg1;

        case OP_XORWF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            return (6 << 8) | (arg2 << 7) | arg1;

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

    // always start from address 0
    fprintf(f, ":020000040000FA\n");

    DWORD i;
    for(i = 0; i < PicProgWriteP; i++) {
        DWORD w = Assemble(PicProg[i].op, PicProg[i].arg1, PicProg[i].arg2);

        if(soFarCount == 0) soFarStart = i;
        soFar[soFarCount++] = (BYTE)(w & 0xff);
        soFar[soFarCount++] = (BYTE)(w >> 8);

        if(soFarCount >= 0x10 || i == (PicProgWriteP-1)) {
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

    StartIhex(f);
    // Configuration words start at address 0x2007 in program memory; and the
    // hex file addresses are by bytes, not words, so we start at 0x400e.
    // There may be either 16 or 32 bits of conf word, depending on the part.
    if(McuIs("Microchip PIC16F887 40-PDIP") ||
       McuIs("Microchip PIC16F886 28-PDIP or 28-SOIC"))
    {
        WriteIhex(f, 0x04);
        WriteIhex(f, 0x40);
        WriteIhex(f, 0x0E);
        WriteIhex(f, 0x00);
        WriteIhex(f, (Prog.mcu->configurationWord >>  0) & 0xff);
        WriteIhex(f, (Prog.mcu->configurationWord >>  8) & 0xff);
        WriteIhex(f, (Prog.mcu->configurationWord >> 16) & 0xff);
        WriteIhex(f, (Prog.mcu->configurationWord >> 24) & 0xff);
    } else {
        if(Prog.mcu->configurationWord & 0xffff0000) oops();
        WriteIhex(f, 0x02);
        WriteIhex(f, 0x40);
        WriteIhex(f, 0x0E);
        WriteIhex(f, 0x00);
        WriteIhex(f, (Prog.mcu->configurationWord >>  0) & 0xff);
        WriteIhex(f, (Prog.mcu->configurationWord >>  8) & 0xff);
    }
    FinishIhex(f);

    // end of file record
    fprintf(f, ":00000001FF\n");
}

//-----------------------------------------------------------------------------
// Generate code to write an 8-bit value to a particular register. Takes care
// of the bank switching if necessary; assumes that code is called in bank
// 0.
//-----------------------------------------------------------------------------
static void WriteRegister(DWORD reg, BYTE val)
{
    if(reg & 0x080) Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
    if(reg & 0x100) Instruction(OP_BSF, REG_STATUS, STATUS_RP1);

    Instruction(OP_MOVLW, val, 0);
    Instruction(OP_MOVWF, (reg & 0x7f), 0);

    if(reg & 0x080) Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
    if(reg & 0x100) Instruction(OP_BCF, REG_STATUS, STATUS_RP1);
}

//-----------------------------------------------------------------------------
// Call a subroutine, that might be in an arbitrary page, and then put
// PCLATH back where we want it.
//-----------------------------------------------------------------------------
static void CallWithPclath(DWORD addr)
{
    // Set up PCLATH for the jump, and then do it.
    Instruction(OP_MOVLW, FWD_HI(addr), 0);
    Instruction(OP_MOVWF, REG_PCLATH, 0);
    Instruction(OP_CALL, FWD_LO(addr), 0);

    // Restore PCLATH to something appropriate for our page. (We have
    // already made fairly sure that we will never try to compile across
    // a page boundary.)
    Instruction(OP_MOVLW, (PicProgWriteP >> 8), 0);
    Instruction(OP_MOVWF, REG_PCLATH, 0);
}

// Note that all of these are single instructions on the PIC; this is not the
// case for their equivalents on the AVR!
#define SetBit(reg, b)      Instruction(OP_BSF, reg, b)
#define ClearBit(reg, b)    Instruction(OP_BCF, reg, b)
#define IfBitClear(reg, b)  Instruction(OP_BTFSS, reg, b)
#define IfBitSet(reg, b)    Instruction(OP_BTFSC, reg, b)
static void CopyBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc)
{
    IfBitSet(addrSrc, bitSrc);
    SetBit(addrDest, bitDest);
    IfBitClear(addrSrc, bitSrc);
    ClearBit(addrDest, bitDest);
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
    CompileFromIntermediate(FALSE);
    if(IntCode[IntPc].op == INT_ELSE) {
        IntPc++;
        DWORD endBlock = AllocFwdAddr();
        Instruction(OP_GOTO, endBlock, 0);

        FwdAddrIsNow(condFalse);
        CompileFromIntermediate(FALSE);
        FwdAddrIsNow(endBlock);
    } else {
        FwdAddrIsNow(condFalse);
    }

    if(IntCode[IntPc].op != INT_END_IF) oops();
}

//-----------------------------------------------------------------------------
// Compile the intermediate code to PIC16 native code.
//-----------------------------------------------------------------------------
static void CompileFromIntermediate(BOOL topLevel)
{
    DWORD addr, addr2;
    int bit, bit2;
    DWORD addrl, addrh;
    DWORD addrl2, addrh2;
    DWORD addrl3, addrh3;

    // Keep track of which 2k section we are using. When it looks like we
    // are about to run out, fill with nops and move on to the next one.
    DWORD section = 0;

    for(; IntPc < IntCodeLen; IntPc++) {
        // Try for a margin of about 400 words, which is a little bit
        // wasteful but considering that the formatted output commands
        // are huge, probably necessary. Of course if we are in our
        // last section then it is silly to do that, either we make it
        // or we're screwed...
        if(topLevel && (((PicProgWriteP + 400) >> 11) != section) &&
            ((PicProgWriteP + 400) < Prog.mcu->flashWords))
        {
            // Jump to the beginning of the next section
            Instruction(OP_MOVLW, (PicProgWriteP >> 8) + (1<<3), 0);
            Instruction(OP_MOVWF, REG_PCLATH, 0);
            Instruction(OP_GOTO, 0, 0);
            // Then, just burn the last of this section with NOPs.
            while((PicProgWriteP >> 11) == section) {
                Instruction(OP_MOVLW, 0xab, 0);
            }
            section = (PicProgWriteP >> 11);
            // And now PCLATH is set up, so everything in our new section
            // should just work
        }
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
                WriteRegister(addrl, a->literal & 0xff);
                WriteRegister(addrh, a->literal >> 8);
                break;

            case INT_INCREMENT_VARIABLE: {
                MemForVariable(a->name1, &addrl, &addrh);
                DWORD noCarry = AllocFwdAddr();
                Instruction(OP_INCFSZ, addrl, DEST_F);
                Instruction(OP_GOTO, noCarry, 0);
                Instruction(OP_INCF, addrh, DEST_F);
                FwdAddrIsNow(noCarry);
                break;
            }
            case INT_IF_BIT_SET: {
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                IfBitClear(addr, bit);
                Instruction(OP_GOTO, condFalse, 0);
                CompileIfBody(condFalse);
                break;
            }
            case INT_IF_BIT_CLEAR: {
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                IfBitSet(addr, bit);
                Instruction(OP_GOTO, condFalse, 0);
                CompileIfBody(condFalse);
                break;
            }
            case INT_IF_VARIABLE_LES_LITERAL: {
                DWORD notTrue = AllocFwdAddr();
                DWORD isTrue = AllocFwdAddr();
                DWORD lsbDecides = AllocFwdAddr();
                
                // V = Rd7*(Rr7')*(R7') + (Rd7')*Rr7*R7 ; but only one of the
                // product terms can be true, and we know which at compile
                // time
                BYTE litH = (a->literal >> 8);
                BYTE litL = (a->literal & 0xff);

                MemForVariable(a->name1, &addrl, &addrh);

                // var - lit
                Instruction(OP_MOVLW, litH, 0);
                Instruction(OP_SUBWF, addrh, DEST_W);
                IfBitSet(REG_STATUS, STATUS_Z);
                Instruction(OP_GOTO, lsbDecides, 0);
                Instruction(OP_MOVWF, Scratch0, 0);
                if(litH & 0x80) {
                    Instruction(OP_COMF, addrh, DEST_W);
                    Instruction(OP_ANDWF, Scratch0, DEST_W);
                    Instruction(OP_XORWF, Scratch0, DEST_F);
                } else {
                    Instruction(OP_COMF, Scratch0, DEST_W);
                    Instruction(OP_ANDWF, addrh, DEST_W);
                    Instruction(OP_XORWF, Scratch0, DEST_F);
                }
                IfBitSet(Scratch0, 7); // var - lit < 0, var < lit
                Instruction(OP_GOTO, isTrue, 0);
                Instruction(OP_GOTO, notTrue, 0);

                FwdAddrIsNow(lsbDecides);

                // var - lit < 0
                // var < lit
                Instruction(OP_MOVLW, litL, 0);
                Instruction(OP_SUBWF, addrl, DEST_W);
                IfBitClear(REG_STATUS, STATUS_C);
                Instruction(OP_GOTO, isTrue, 0);
                Instruction(OP_GOTO, notTrue, 0);

                FwdAddrIsNow(isTrue);
                CompileIfBody(notTrue);
                break;
            }
            case INT_IF_VARIABLE_EQUALS_VARIABLE: {
                DWORD notEqual = AllocFwdAddr();

                MemForVariable(a->name1, &addrl, &addrh);
                MemForVariable(a->name2, &addrl2, &addrh2);
                Instruction(OP_MOVF, addrl, DEST_W);
                Instruction(OP_SUBWF, addrl2, DEST_W);
                IfBitClear(REG_STATUS, STATUS_Z);
                Instruction(OP_GOTO, notEqual, 0);
                Instruction(OP_MOVF, addrh, DEST_W);
                Instruction(OP_SUBWF, addrh2, DEST_W);
                IfBitClear(REG_STATUS, STATUS_Z);
                Instruction(OP_GOTO, notEqual, 0);
                CompileIfBody(notEqual);
                break;
            }
            case INT_IF_VARIABLE_GRT_VARIABLE: {
                DWORD notTrue = AllocFwdAddr();
                DWORD isTrue = AllocFwdAddr();
                DWORD lsbDecides = AllocFwdAddr();

                MemForVariable(a->name1, &addrl, &addrh);
                MemForVariable(a->name2, &addrl2, &addrh2);

                // first, a signed comparison of the high octets, which is
                // a huge pain on the PIC16
                DWORD iu = addrh2, ju = addrh;
                DWORD signa = Scratch0;
                DWORD signb = Scratch1;

                Instruction(OP_COMF, ju, DEST_W);
                Instruction(OP_MOVWF, signb, 0);

                Instruction(OP_ANDWF, iu, DEST_W);
                Instruction(OP_MOVWF, signa, 0);

                Instruction(OP_MOVF, iu, DEST_W);
                Instruction(OP_IORWF, signb, DEST_F);
                Instruction(OP_COMF, signb, DEST_F);

                Instruction(OP_MOVF, ju, DEST_W);
                Instruction(OP_SUBWF, iu, DEST_W);
                IfBitSet(REG_STATUS, STATUS_Z);
                Instruction(OP_GOTO, lsbDecides, 0);

                Instruction(OP_ANDWF, signb, DEST_F);
                Instruction(OP_MOVWF, Scratch2, 0);
                Instruction(OP_COMF, Scratch2, DEST_W);
                Instruction(OP_ANDWF, signa, DEST_W);
                Instruction(OP_IORWF, signb, DEST_W);
                Instruction(OP_XORWF, Scratch2, DEST_F);
                IfBitSet(Scratch2, 7);
                Instruction(OP_GOTO, isTrue, 0);

                Instruction(OP_GOTO, notTrue, 0);

                FwdAddrIsNow(lsbDecides);
                Instruction(OP_MOVF, addrl, DEST_W);
                Instruction(OP_SUBWF, addrl2, DEST_W);
                IfBitClear(REG_STATUS, STATUS_C);
                Instruction(OP_GOTO, isTrue, 0);

                Instruction(OP_GOTO, notTrue, 0);

                FwdAddrIsNow(isTrue);
                CompileIfBody(notTrue);
                break;
            }
            case INT_SET_VARIABLE_TO_VARIABLE:
                MemForVariable(a->name1, &addrl, &addrh);
                MemForVariable(a->name2, &addrl2, &addrh2);

                Instruction(OP_MOVF, addrl2, DEST_W);
                Instruction(OP_MOVWF, addrl, 0);

                Instruction(OP_MOVF, addrh2, DEST_W);
                Instruction(OP_MOVWF, addrh, 0);
                break;

            // The add and subtract routines must be written to return correct
            // results if the destination and one of the operands happen to
            // be the same registers (e.g. for B = A - B).

            case INT_SET_VARIABLE_ADD:
                MemForVariable(a->name1, &addrl, &addrh);
                MemForVariable(a->name2, &addrl2, &addrh2);
                MemForVariable(a->name3, &addrl3, &addrh3);

                Instruction(OP_MOVF, addrl2, DEST_W);
                Instruction(OP_ADDWF, addrl3, DEST_W);
                Instruction(OP_MOVWF, addrl, 0);
                ClearBit(Scratch0, 0);
                IfBitSet(REG_STATUS, STATUS_C);
                SetBit(Scratch0, 0);

                Instruction(OP_MOVF, addrh2, DEST_W);
                Instruction(OP_ADDWF, addrh3, DEST_W);
                Instruction(OP_MOVWF, addrh, 0);
                IfBitSet(Scratch0, 0);
                Instruction(OP_INCF, addrh, DEST_F);
                break;

            case INT_SET_VARIABLE_SUBTRACT:
                MemForVariable(a->name1, &addrl, &addrh);
                MemForVariable(a->name2, &addrl2, &addrh2);
                MemForVariable(a->name3, &addrl3, &addrh3);

                Instruction(OP_MOVF, addrl3, DEST_W);
                Instruction(OP_SUBWF, addrl2, DEST_W);
                Instruction(OP_MOVWF, addrl, 0);
                ClearBit(Scratch0, 0);
                IfBitSet(REG_STATUS, STATUS_C);
                SetBit(Scratch0, 0);

                Instruction(OP_MOVF, addrh3, DEST_W);
                Instruction(OP_SUBWF, addrh2, DEST_W);
                Instruction(OP_MOVWF, addrh, 0);
                IfBitClear(Scratch0, 0); // bit is carry / (not borrow)
                Instruction(OP_DECF, addrh, DEST_F);
                break;

            case INT_SET_VARIABLE_MULTIPLY:
                MultiplyNeeded = TRUE;
                
                MemForVariable(a->name1, &addrl, &addrh);
                MemForVariable(a->name2, &addrl2, &addrh2);
                MemForVariable(a->name3, &addrl3, &addrh3);

                Instruction(OP_MOVF, addrl2, DEST_W);
                Instruction(OP_MOVWF, Scratch0, 0);
                Instruction(OP_MOVF, addrh2, DEST_W);
                Instruction(OP_MOVWF, Scratch1, 0);

                Instruction(OP_MOVF, addrl3, DEST_W);
                Instruction(OP_MOVWF, Scratch2, 0);
                Instruction(OP_MOVF, addrh3, DEST_W);
                Instruction(OP_MOVWF, Scratch3, 0);

                CallWithPclath(MultiplyRoutineAddress);

                Instruction(OP_MOVF, Scratch2, DEST_W);
                Instruction(OP_MOVWF, addrl, 0);
                Instruction(OP_MOVF, Scratch3, DEST_W);
                Instruction(OP_MOVWF, addrh, 0);
                break;

            case INT_SET_VARIABLE_DIVIDE:
                DivideNeeded = TRUE;

                MemForVariable(a->name1, &addrl, &addrh);
                MemForVariable(a->name2, &addrl2, &addrh2);
                MemForVariable(a->name3, &addrl3, &addrh3);

                Instruction(OP_MOVF, addrl2, DEST_W);
                Instruction(OP_MOVWF, Scratch0, 0);
                Instruction(OP_MOVF, addrh2, DEST_W);
                Instruction(OP_MOVWF, Scratch1, 0);

                Instruction(OP_MOVF, addrl3, DEST_W);
                Instruction(OP_MOVWF, Scratch2, 0);
                Instruction(OP_MOVF, addrh3, DEST_W);
                Instruction(OP_MOVWF, Scratch3, 0);

                CallWithPclath(DivideRoutineAddress);

                Instruction(OP_MOVF, Scratch0, DEST_W);
                Instruction(OP_MOVWF, addrl, 0);
                Instruction(OP_MOVF, Scratch1, DEST_W);
                Instruction(OP_MOVWF, addrh, 0);
                break;

            case INT_UART_SEND: {
                MemForVariable(a->name1, &addrl, &addrh);
                MemForSingleBit(a->name2, TRUE, &addr, &bit);

                DWORD noSend = AllocFwdAddr();
                IfBitClear(addr, bit);
                Instruction(OP_GOTO, noSend, 0);

                Instruction(OP_MOVF, addrl, DEST_W);
                Instruction(OP_MOVWF, REG_TXREG, 0);

                FwdAddrIsNow(noSend);
                ClearBit(addr, bit);

                DWORD notBusy = AllocFwdAddr();
                Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
                Instruction(OP_BTFSC, REG_TXSTA ^ 0x80, 1);
                Instruction(OP_GOTO, notBusy, 0);
                
                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
                SetBit(addr, bit);

                FwdAddrIsNow(notBusy);
                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);

                break;
            }
            case INT_UART_RECV: {
                MemForVariable(a->name1, &addrl, &addrh);
                MemForSingleBit(a->name2, TRUE, &addr, &bit);

                ClearBit(addr, bit);
    
                // If RCIF is still clear, then there's nothing to do; in that
                // case jump to the end, and leave the rung-out clear.
                DWORD done = AllocFwdAddr();
                IfBitClear(REG_PIR1, 5);
                Instruction(OP_GOTO, done, 0);

                // RCIF is set, so we have a character. Read it now.
                Instruction(OP_MOVF, REG_RCREG, DEST_W);
                Instruction(OP_MOVWF, addrl, 0);
                Instruction(OP_CLRF, addrh, 0);
                // and set rung-out true
                SetBit(addr, bit);

                // And check for errors; need to reset the UART if yes.
                DWORD yesError = AllocFwdAddr();
                IfBitSet(REG_RCSTA, 1); // overrun error
                Instruction(OP_GOTO, yesError, 0);
                IfBitSet(REG_RCSTA, 2); // framing error
                Instruction(OP_GOTO, yesError, 0);

                // Neither FERR nor OERR is set, so we're good.
                Instruction(OP_GOTO, done, 0);

                FwdAddrIsNow(yesError);
                // An error did occur, so flush the FIFO.
                Instruction(OP_MOVF, REG_RCREG, DEST_W);
                Instruction(OP_MOVF, REG_RCREG, DEST_W);
                // And clear and then set CREN, to clear the error flags.
                ClearBit(REG_RCSTA, 4);
                SetBit(REG_RCSTA, 4);

                FwdAddrIsNow(done);
                break;
            }
            case INT_SET_PWM: {
                int target = atoi(a->name2);

                // So the PWM frequency is given by 
                //    target = xtal/(4*prescale*pr2)
                //    xtal/target = 4*prescale*pr2
                // and pr2 should be made as large as possible to keep
                // resolution, so prescale should be as small as possible

                int pr2;
                int prescale;
                for(prescale = 1;;) {
                    int dv = 4*prescale*target;
                    pr2 = (Prog.mcuClock + (dv/2))/dv;
                    if(pr2 < 3) {
                        Error(_("PWM frequency too fast."));
                        CompileError();
                    }
                    if(pr2 >= 256) {
                        if(prescale == 1) {
                            prescale = 4;
                        } else if(prescale == 4) {
                            prescale = 16;
                        } else {
                            Error(_("PWM frequency too slow."));
                            CompileError();
                        }
                    } else {
                        break;
                    }
                }

                // First scale the input variable from percent to timer units,
                // with a multiply and then a divide.
                MultiplyNeeded = TRUE; DivideNeeded = TRUE;
                MemForVariable(a->name1, &addrl, &addrh);
                Instruction(OP_MOVF, addrl, DEST_W);
                Instruction(OP_MOVWF, Scratch0, 0);
                Instruction(OP_CLRF, Scratch1, 0);

                Instruction(OP_MOVLW, pr2, 0);
                Instruction(OP_MOVWF, Scratch2, 0);
                Instruction(OP_CLRF, Scratch3, 0);

                CallWithPclath(MultiplyRoutineAddress);

                Instruction(OP_MOVF, Scratch3, DEST_W);
                Instruction(OP_MOVWF, Scratch1, 0);
                Instruction(OP_MOVF, Scratch2, DEST_W);
                Instruction(OP_MOVWF, Scratch0, 0);
                Instruction(OP_MOVLW, 100, 0);
                Instruction(OP_MOVWF, Scratch2, 0);
                Instruction(OP_CLRF, Scratch3, 0);

                CallWithPclath(DivideRoutineAddress);

                Instruction(OP_MOVF, Scratch0, DEST_W);
                Instruction(OP_MOVWF, REG_CCPR2L, 0);

                // Only need to do the setup stuff once
                MemForSingleBit("$pwm_init", FALSE, &addr, &bit);
                DWORD skip = AllocFwdAddr();
                IfBitSet(addr, bit);
                Instruction(OP_GOTO, skip, 0);
                SetBit(addr, bit);

                // Set up the CCP2 and TMR2 peripherals.
                WriteRegister(REG_PR2, (pr2-1));
                WriteRegister(REG_CCP2CON, 0x0c); // PWM mode, ignore lsbs

                BYTE t2con = (1 << 2); // timer 2 on
                if(prescale == 1)
                    t2con |= 0;
                else if(prescale == 4)
                    t2con |= 1;
                else if(prescale == 16)
                    t2con |= 2;
                else oops();
                WriteRegister(REG_T2CON, t2con);

                FwdAddrIsNow(skip);
                break;
            }

// A quick helper macro to set the banksel bits correctly; this is necessary
// because the EEwhatever registers are all over in the memory maps.
#define EE_REG_BANKSEL(r) \
    if((r) & 0x80) { \
        if(!(m & 0x80)) { \
            m |= 0x80; \
            Instruction(OP_BSF, REG_STATUS, STATUS_RP0); \
        } \
    } else { \
        if(m & 0x80) { \
            m &= ~0x80; \
            Instruction(OP_BCF, REG_STATUS, STATUS_RP0); \
        } \
    } \
    if((r) & 0x100) { \
        if(!(m & 0x100)) { \
            m |= 0x100; \
            Instruction(OP_BSF, REG_STATUS, STATUS_RP1); \
        } \
    } else { \
        if(m & 0x100) { \
            m &= ~0x100; \
            Instruction(OP_BCF, REG_STATUS, STATUS_RP1); \
        } \
    }

            case INT_EEPROM_BUSY_CHECK: {
                DWORD isBusy = AllocFwdAddr();
                DWORD done = AllocFwdAddr();
                MemForSingleBit(a->name1, FALSE, &addr, &bit);

                WORD m = 0;
               
                EE_REG_BANKSEL(REG_EECON1);
                IfBitSet(REG_EECON1 ^ m, 1);
                Instruction(OP_GOTO, isBusy, 0);
                EE_REG_BANKSEL(0);

                IfBitClear(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
                Instruction(OP_GOTO, done, 0);

                // So there is not a write pending, but we have another
                // character to transmit queued up.

                EE_REG_BANKSEL(REG_EEADR);
                Instruction(OP_INCF, REG_EEADR ^ m, DEST_F);
                EE_REG_BANKSEL(0);
                Instruction(OP_MOVF, EepromHighByte, DEST_W);
                EE_REG_BANKSEL(REG_EEDATA);
                Instruction(OP_MOVWF, REG_EEDATA ^ m, 0);
                EE_REG_BANKSEL(REG_EECON1);
                Instruction(OP_BCF, REG_EECON1 ^ m, 7);
                Instruction(OP_BSF, REG_EECON1 ^ m, 2);
                Instruction(OP_MOVLW, 0x55, 0);
                Instruction(OP_MOVWF, REG_EECON2 ^ m, 0);
                Instruction(OP_MOVLW, 0xaa, 0);
                Instruction(OP_MOVWF, REG_EECON2 ^ m, 0);
                Instruction(OP_BSF, REG_EECON1 ^ m, 1);

                EE_REG_BANKSEL(0);

                ClearBit(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);

                FwdAddrIsNow(isBusy);
                // Have to do these explicitly; m is out of date due to jump.
                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
                Instruction(OP_BCF, REG_STATUS, STATUS_RP1);
                SetBit(addr, bit);

                FwdAddrIsNow(done);
                break;
            }
            case INT_EEPROM_WRITE: {
                MemForVariable(a->name1, &addrl, &addrh);

                WORD m = 0;

                SetBit(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
                Instruction(OP_MOVF, addrh, DEST_W);
                Instruction(OP_MOVWF, EepromHighByte, 0);

                EE_REG_BANKSEL(REG_EEADR);
                Instruction(OP_MOVLW, a->literal, 0);
                Instruction(OP_MOVWF, REG_EEADR ^ m, 0);
                EE_REG_BANKSEL(0);
                Instruction(OP_MOVF, addrl, DEST_W);
                EE_REG_BANKSEL(REG_EEDATA);
                Instruction(OP_MOVWF, REG_EEDATA ^ m, 0);
                EE_REG_BANKSEL(REG_EECON1);
                Instruction(OP_BCF, REG_EECON1 ^ m, 7);
                Instruction(OP_BSF, REG_EECON1 ^ m, 2);
                Instruction(OP_MOVLW, 0x55, 0);
                Instruction(OP_MOVWF, REG_EECON2 ^ m, 0);
                Instruction(OP_MOVLW, 0xaa, 0);
                Instruction(OP_MOVWF, REG_EECON2 ^ m, 0);
                Instruction(OP_BSF, REG_EECON1 ^ m, 1);

                EE_REG_BANKSEL(0);
                break;
            }
            case INT_EEPROM_READ: {
                int i;
                MemForVariable(a->name1, &addrl, &addrh);
                WORD m = 0;
                for(i = 0; i < 2; i++) {
                    EE_REG_BANKSEL(REG_EEADR);
                    Instruction(OP_MOVLW, a->literal+i, 0);
                    Instruction(OP_MOVWF, REG_EEADR ^ m, 0);
                    EE_REG_BANKSEL(REG_EECON1);
                    Instruction(OP_BCF, REG_EECON1 ^ m, 7);
                    Instruction(OP_BSF, REG_EECON1 ^ m, 0);
                    EE_REG_BANKSEL(REG_EEDATA);
                    Instruction(OP_MOVF, REG_EEDATA ^ m , DEST_W);
                    EE_REG_BANKSEL(0);
                    if(i == 0) {
                        Instruction(OP_MOVWF, addrl, 0);
                    } else {
                        Instruction(OP_MOVWF, addrh, 0);
                    }
                }
                break;
            }
            case INT_READ_ADC: {
                BYTE adcs;

                MemForVariable(a->name1, &addrl, &addrh);

                if(Prog.mcuClock > 5000000) {
                    adcs = 2; // 32*Tosc
                } else if(Prog.mcuClock > 1250000) {
                    adcs = 1; // 8*Tosc
                } else {
                    adcs = 0; // 2*Tosc
                }

                int goPos, chsPos;
                if(McuIs("Microchip PIC16F887 40-PDIP") ||
                   McuIs("Microchip PIC16F886 28-PDIP or 28-SOIC"))
                {
                    goPos = 1;
                    chsPos = 2;
                } else {
                    goPos = 2;
                    chsPos = 3;
                }
                WriteRegister(REG_ADCON0, (BYTE)
                    ((adcs << 6) |
                     (MuxForAdcVariable(a->name1) << chsPos) |
                     (0 << goPos) |  // don't start yet
                                     // bit 1 unimplemented
                     (1 << 0))       // A/D peripheral on
                );

                WriteRegister(REG_ADCON1,
                    (1 << 7) |      // right-justified
                    (0 << 0)        // for now, all analog inputs
                );
                if(strcmp(Prog.mcu->mcuName,
                    "Microchip PIC16F88 18-PDIP or 18-SOIC")==0)
                {
                    WriteRegister(REG_ANSEL, 0x7f);
                }
                if(McuIs("Microchip PIC16F887 40-PDIP") ||
                   McuIs("Microchip PIC16F886 28-PDIP or 28-SOIC"))
                {
                    WriteRegister(REG_ANSEL, 0xff);
                    WriteRegister(REG_ANSELH, 0x3f);
                }

                // need to wait Tacq (about 20 us) for mux, S/H etc. to settle
                int cyclesToWait = ((Prog.mcuClock / 4) * 20) / 1000000;
                cyclesToWait /= 3;
                if(cyclesToWait < 1) cyclesToWait = 1;

                Instruction(OP_MOVLW, cyclesToWait, 0);
                Instruction(OP_MOVWF, Scratch1, 0);
                DWORD wait = PicProgWriteP;
                Instruction(OP_DECFSZ, Scratch1, DEST_F);
                Instruction(OP_GOTO, wait, 0);
                
                SetBit(REG_ADCON0, goPos);
                DWORD spin = PicProgWriteP;
                IfBitSet(REG_ADCON0, goPos);
                Instruction(OP_GOTO, spin, 0);
                 
                Instruction(OP_MOVF, REG_ADRESH, DEST_W);
                Instruction(OP_MOVWF, addrh, 0);

                Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
                Instruction(OP_MOVF, REG_ADRESL ^ 0x80, DEST_W);
                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
                Instruction(OP_MOVWF, addrl, 0);

                // hook those pins back up to the digital inputs in case
                // some of them are used that way
                WriteRegister(REG_ADCON1,
                    (1 << 7) |      // right-justify A/D result
                    (6 << 0)        // all digital inputs
                );
                if(strcmp(Prog.mcu->mcuName,
                    "Microchip PIC16F88 18-PDIP or 18-SOIC")==0)
                {
                    WriteRegister(REG_ANSEL, 0x00);
                }
                if(McuIs("Microchip PIC16F887 40-PDIP") ||
                   McuIs("Microchip PIC16F886 28-PDIP or 28-SOIC"))
                {
                    WriteRegister(REG_ANSEL, 0x00);
                    WriteRegister(REG_ANSELH, 0x00);
                }
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
        if(((PicProgWriteP >> 11) != section) && topLevel) {
            // This is particularly prone to happening in the last section,
            // if the program doesn't fit (since we won't have attempted
            // to add padding).
            Error(_("Internal error relating to PIC paging; make program "
                    "smaller or reshuffle it."));
            CompileError();
        }
    }
}

//-----------------------------------------------------------------------------
// Configure Timer1 and Ccp1 to generate the periodic `cycle' interrupt
// that triggers all the ladder logic processing. We will always use 16-bit
// Timer1, with the prescaler configured appropriately.
//-----------------------------------------------------------------------------
static void ConfigureTimer1(int cycleTimeMicroseconds)
{
    int divisor = 1;
    int countsPerCycle;

    while(divisor < 16) {
        int timerRate = (Prog.mcuClock / (4*divisor)); // hertz
        double timerPeriod = 1e6 / timerRate; // timer period, us
        countsPerCycle = (int)(cycleTimeMicroseconds / timerPeriod);

        if(countsPerCycle < 1000) {
            Error(_("Cycle time too fast; increase cycle time, or use faster "
                "crystal."));
            CompileError();
        } else if(countsPerCycle > 0xffff) {
            if(divisor >= 8) {
                Error(
                    _("Cycle time too slow; decrease cycle time, or use slower "
                    "crystal."));
                CompileError();
            }
        } else {
            break;
        }
        divisor *= 2;
    }

    WriteRegister(REG_CCPR1L, countsPerCycle & 0xff);
    WriteRegister(REG_CCPR1H, countsPerCycle >> 8);

    WriteRegister(REG_TMR1L, 0);
    WriteRegister(REG_TMR1H, 0);

    BYTE t1con = 0;
    // set up prescaler
    if(divisor == 1)        t1con |= 0x00;
    else if(divisor == 2)   t1con |= 0x10;
    else if(divisor == 4)   t1con |= 0x20;
    else if(divisor == 8)   t1con |= 0x30;
    else oops();
    // enable clock, internal source
    t1con |= 0x01;
    WriteRegister(REG_T1CON, t1con);

    BYTE ccp1con;
    // compare mode, reset TMR1 on trigger
    ccp1con = 0x0b;
    WriteRegister(REG_CCP1CON, ccp1con);
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 16x16 signed multiply. One operand in
// Scratch1:Scratch0, other in Scratch3:Scratch2, result in Scratch3:Scratch2.
//-----------------------------------------------------------------------------
static void WriteMultiplyRoutine(void)
{
    DWORD result3 = Scratch5;
    DWORD result2 = Scratch4;
    DWORD result1 = Scratch3;
    DWORD result0 = Scratch2;

    DWORD multiplicand0 = Scratch0;
    DWORD multiplicand1 = Scratch1;

    DWORD counter = Scratch6;

    DWORD dontAdd = AllocFwdAddr();
    DWORD top;

    FwdAddrIsNow(MultiplyRoutineAddress);

    Instruction(OP_CLRF, result3, 0);
    Instruction(OP_CLRF, result2, 0);
    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_RRF, result1, DEST_F);
    Instruction(OP_RRF, result0, DEST_F);

    Instruction(OP_MOVLW, 16, 0);
    Instruction(OP_MOVWF, counter, 0);

    top = PicProgWriteP;
    Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, dontAdd, 0);
    Instruction(OP_MOVF, multiplicand0, DEST_W);
    Instruction(OP_ADDWF, result2, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
    Instruction(OP_INCF, result3, DEST_F);
    Instruction(OP_MOVF, multiplicand1, DEST_W);
    Instruction(OP_ADDWF, result3, DEST_F);
    FwdAddrIsNow(dontAdd);


    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_RRF, result3, DEST_F);
    Instruction(OP_RRF, result2, DEST_F);
    Instruction(OP_RRF, result1, DEST_F);
    Instruction(OP_RRF, result0, DEST_F);

    Instruction(OP_DECFSZ, counter, DEST_F);
    Instruction(OP_GOTO, top, 0);

    Instruction(OP_RETURN, 0, 0);
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 16/16 signed divide. Call with dividend in
// Scratch1:0, divisor in Scratch3:2, and get the result in Scratch1:0.
//-----------------------------------------------------------------------------
static void WriteDivideRoutine(void)
{
    DWORD dividend0 = Scratch0;
    DWORD dividend1 = Scratch1;

    DWORD divisor0 = Scratch2;
    DWORD divisor1 = Scratch3;

    DWORD remainder0 = Scratch4;
    DWORD remainder1 = Scratch5;

    DWORD counter = Scratch6;
    DWORD sign = Scratch7;

    DWORD dontNegateDivisor = AllocFwdAddr();
    DWORD dontNegateDividend = AllocFwdAddr();
    DWORD done = AllocFwdAddr();
    DWORD notNegative = AllocFwdAddr();
    DWORD loop;
    
    FwdAddrIsNow(DivideRoutineAddress);
    Instruction(OP_MOVF, dividend1, DEST_W);
    Instruction(OP_XORWF, divisor1, DEST_W);
    Instruction(OP_MOVWF, sign, 0);

    Instruction(OP_BTFSS, divisor1, 7);
    Instruction(OP_GOTO, dontNegateDivisor, 0);
    Instruction(OP_COMF, divisor0, DEST_F);
    Instruction(OP_COMF, divisor1, DEST_F);
    Instruction(OP_INCF, divisor0, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_Z);
    Instruction(OP_INCF, divisor1, DEST_F);
    FwdAddrIsNow(dontNegateDivisor);

    Instruction(OP_BTFSS, dividend1, 7);
    Instruction(OP_GOTO, dontNegateDividend, 0);
    Instruction(OP_COMF, dividend0, DEST_F);
    Instruction(OP_COMF, dividend1, DEST_F);
    Instruction(OP_INCF, dividend0, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_Z);
    Instruction(OP_INCF, dividend1, DEST_F);
    FwdAddrIsNow(dontNegateDividend);

    Instruction(OP_CLRF, remainder1, 0);
    Instruction(OP_CLRF, remainder0, 0);

    Instruction(OP_BCF, REG_STATUS, STATUS_C);

    Instruction(OP_MOVLW, 17, 0);
    Instruction(OP_MOVWF, counter, 0);
    
    loop = PicProgWriteP;
    Instruction(OP_RLF, dividend0, DEST_F);
    Instruction(OP_RLF, dividend1, DEST_F);

    Instruction(OP_DECF, counter, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_Z);
    Instruction(OP_GOTO, done, 0);

    Instruction(OP_RLF, remainder0, DEST_F);
    Instruction(OP_RLF, remainder1, DEST_F);

    Instruction(OP_MOVF, divisor0, DEST_W);
    Instruction(OP_SUBWF, remainder0, DEST_F);
    Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
    Instruction(OP_DECF, remainder1, DEST_F);
    Instruction(OP_MOVF, divisor1, DEST_W);
    Instruction(OP_SUBWF, remainder1, DEST_F);

    Instruction(OP_BTFSS, remainder1, 7);
    Instruction(OP_GOTO, notNegative, 0);

    Instruction(OP_MOVF, divisor0, DEST_W);
    Instruction(OP_ADDWF, remainder0, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
    Instruction(OP_INCF, remainder1, DEST_F);
    Instruction(OP_MOVF, divisor1, DEST_W);
    Instruction(OP_ADDWF, remainder1, DEST_F);

    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, loop, 0);

    FwdAddrIsNow(notNegative);
    Instruction(OP_BSF, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, loop, 0);

    FwdAddrIsNow(done);
    Instruction(OP_BTFSS, sign, 7);
    Instruction(OP_RETURN, 0, 0);

    Instruction(OP_COMF, dividend0, DEST_F);
    Instruction(OP_COMF, dividend1, DEST_F);
    Instruction(OP_INCF, dividend0, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_Z);
    Instruction(OP_INCF, dividend1, DEST_F);
    Instruction(OP_RETURN, 0, 0);
}

//-----------------------------------------------------------------------------
// Compile the program to PIC16 code for the currently selected processor
// and write it to the given file. Produce an error message if we cannot
// write to the file, or if there is something inconsistent about the
// program.
//-----------------------------------------------------------------------------
void CompilePic16(char *outFile)
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

    WipeMemory();

    AllocStart();
    Scratch0 = AllocOctetRam();
    Scratch1 = AllocOctetRam();
    Scratch2 = AllocOctetRam();
    Scratch3 = AllocOctetRam();
    Scratch4 = AllocOctetRam();
    Scratch5 = AllocOctetRam();
    Scratch6 = AllocOctetRam();
    Scratch7 = AllocOctetRam();
   
    // Allocate the register used to hold the high byte of the EEPROM word
    // that's queued up to program, plus the bit to indicate that it is
    // valid.
    EepromHighByte = AllocOctetRam();
    AllocBitRam(&EepromHighByteWaitingAddr, &EepromHighByteWaitingBit);

    DWORD progStart = AllocFwdAddr();
    // Our boot vectors; not necessary to do it like this, but it lets
    // bootloaders rewrite the beginning of the program to do their magic.
    // PCLATH is init to 0, but apparently some bootloaders want to see us
    // initialize it again.
    Instruction(OP_BCF, REG_PCLATH, 3);
    Instruction(OP_BCF, REG_PCLATH, 4);
    Instruction(OP_GOTO, progStart, 0);
    Instruction(OP_NOP, 0, 0);
    Instruction(OP_NOP, 0, 0);
    Instruction(OP_NOP, 0, 0);
    Instruction(OP_NOP, 0, 0);
    Instruction(OP_NOP, 0, 0);
    FwdAddrIsNow(progStart);

    // Now zero out the RAM
    Instruction(OP_MOVLW, Prog.mcu->ram[0].start + 8, 0);
    Instruction(OP_MOVWF, REG_FSR, 0);
    Instruction(OP_MOVLW, Prog.mcu->ram[0].len - 8, 0);
    Instruction(OP_MOVWF, Scratch0, 0);

    DWORD zeroMem = PicProgWriteP;
    Instruction(OP_CLRF, REG_INDF, 0);
    Instruction(OP_INCF, REG_FSR, DEST_F);
    Instruction(OP_DECFSZ, Scratch0, DEST_F);
    Instruction(OP_GOTO, zeroMem, 0);

    DivideRoutineAddress = AllocFwdAddr();
    DivideNeeded = FALSE;
    MultiplyRoutineAddress = AllocFwdAddr();
    MultiplyNeeded = FALSE;

    ConfigureTimer1(Prog.cycleTime);

    // Set up the TRISx registers (direction). 1 means tri-stated (input).
    BYTE isInput[MAX_IO_PORTS], isOutput[MAX_IO_PORTS];
    BuildDirectionRegisters(isInput, isOutput);

    if(McuIs("Microchip PIC16F877 40-PDIP") ||
       McuIs("Microchip PIC16F819 18-PDIP or 18-SOIC") ||
       McuIs("Microchip PIC16F88 18-PDIP or 18-SOIC") ||
       McuIs("Microchip PIC16F876 28-PDIP or 28-SOIC") ||
       McuIs("Microchip PIC16F887 40-PDIP") ||
       McuIs("Microchip PIC16F886 28-PDIP or 28-SOIC"))
    {
        REG_EECON1  = 0x18c;
        REG_EECON2  = 0x18d;
        REG_EEDATA  = 0x10c;
        REG_EEADR   = 0x10d;
    } else if(McuIs("Microchip PIC16F628 18-PDIP or 18-SOIC")) {
        REG_EECON1  = 0x9c;
        REG_EECON2  = 0x9d;
        REG_EEDATA  = 0x9a;
        REG_EEADR   = 0x9b;
    } else {
        oops();
    }

    if(McuIs("Microchip PIC16F887 40-PDIP") ||
       McuIs("Microchip PIC16F886 28-PDIP or 28-SOIC"))
    {
        REG_ANSEL = 0x188;
        REG_ANSELH = 0x189;
    } else {
        REG_ANSEL = 0x9b;
    }

    if(strcmp(Prog.mcu->mcuName, "Microchip PIC16F877 40-PDIP")==0) {
        // This is a nasty special case; one of the extra bits in TRISE
        // enables the PSP, and must be kept clear (set here as will be
        // inverted).
        isOutput[4] |= 0xf8;
    }

    if(strcmp(Prog.mcu->mcuName, "Microchip PIC16F877 40-PDIP")==0 ||
       strcmp(Prog.mcu->mcuName, "Microchip PIC16F819 18-PDIP or 18-SOIC")==0 ||
       strcmp(Prog.mcu->mcuName, "Microchip PIC16F876 28-PDIP or 28-SOIC")==0)
    {
        // The GPIOs that can also be A/D inputs default to being A/D
        // inputs, so turn that around
        WriteRegister(REG_ADCON1,
            (1 << 7) |      // right-justify A/D result
            (6 << 0)        // all digital inputs
        );
    }

    if(strcmp(Prog.mcu->mcuName, "Microchip PIC16F88 18-PDIP or 18-SOIC")==0) {
        WriteRegister(REG_ANSEL, 0x00); // all digital inputs
    }

    if(strcmp(Prog.mcu->mcuName, "Microchip PIC16F628 18-PDIP or 18-SOIC")==0) {
        // This is also a nasty special case; the comparators on the
        // PIC16F628 are enabled by default and need to be disabled, or
        // else the PORTA GPIOs don't work.
        WriteRegister(REG_CMCON, 0x07);
    }

    if(McuIs("Microchip PIC16F887 40-PDIP") ||
       McuIs("Microchip PIC16F886 28-PDIP or 28-SOIC"))
    {
        WriteRegister(REG_ANSEL, 0x00);     // all digital inputs
        WriteRegister(REG_ANSELH, 0x00);    // all digital inputs
    }

    if(PwmFunctionUsed()) {
        // Need to clear TRIS bit corresponding to PWM pin
        int i;
        for(i = 0; i < Prog.mcu->pinCount; i++) {
            if(Prog.mcu->pinInfo[i].pin == Prog.mcu->pwmNeedsPin) {
                McuIoPinInfo *iop = &(Prog.mcu->pinInfo[i]);
                isOutput[iop->port - 'A'] |= (1 << iop->bit);
                break;
            }
        }
        if(i == Prog.mcu->pinCount) oops();
    }

    int i;
    for(i = 0; Prog.mcu->dirRegs[i] != 0; i++) {
        WriteRegister(Prog.mcu->outputRegs[i], 0x00);
        WriteRegister(Prog.mcu->dirRegs[i], ~isOutput[i]);
    }

    if(UartFunctionUsed()) {
        if(Prog.baudRate == 0) {
            Error(_("Zero baud rate not possible."));
            fclose(f);
            return;
        }

        // So now we should set up the UART. First let us calculate the
        // baud rate; there is so little point in the fast baud rates that
        // I won't even bother, so
        // bps = Fosc/(64*(X+1))
        // bps*64*(X + 1) = Fosc
        // X = Fosc/(bps*64)-1
        // and round, don't truncate
        int divisor = (Prog.mcuClock + Prog.baudRate*32)/(Prog.baudRate*64) - 1;

        double actual = Prog.mcuClock/(64.0*(divisor+1));
        double percentErr = 100*(actual - Prog.baudRate)/Prog.baudRate;

        if(fabs(percentErr) > 2) {
            ComplainAboutBaudRateError(divisor, actual, percentErr);
        }
        if(divisor > 255) ComplainAboutBaudRateOverflow();
        
        WriteRegister(REG_SPBRG, divisor);
        WriteRegister(REG_TXSTA, 0x20); // only TXEN set
        WriteRegister(REG_RCSTA, 0x90); // only SPEN, CREN set
    }

    DWORD top = PicProgWriteP;

    IfBitClear(REG_PIR1, 2);
    Instruction(OP_GOTO, PicProgWriteP - 1, 0);

    Instruction(OP_BCF, REG_PIR1, 2);

    Instruction(OP_CLRWDT, 0, 0);
    IntPc = 0;
    CompileFromIntermediate(TRUE);

    MemCheckForErrorsPostCompile();

    // This is probably a big jump, so give it PCLATH.
    Instruction(OP_CLRF, REG_PCLATH, 0);
    Instruction(OP_GOTO, top, 0);

    // Once again, let us make sure not to put stuff on a page boundary
    if((PicProgWriteP >> 11) != ((PicProgWriteP + 150) >> 11)) {
        DWORD section = (PicProgWriteP >> 11);
        // Just burn the last of this section with NOPs.
        while((PicProgWriteP >> 11) == section) {
            Instruction(OP_MOVLW, 0xab, 0);
        }
    }

    if(MultiplyNeeded) WriteMultiplyRoutine();
    if(DivideNeeded) WriteDivideRoutine();

    WriteHexFile(f);
    fclose(f);

    char str[MAX_PATH+500];
    sprintf(str, _("Compile successful; wrote IHEX for PIC16 to '%s'.\r\n\r\n"
        "Configuration word (fuses) has been set for crystal oscillator, BOD "
        "enabled, LVP disabled, PWRT enabled, all code protection off.\r\n\r\n" 
        "Used %d/%d words of program flash (chip %d%% full)."),
            outFile, PicProgWriteP, Prog.mcu->flashWords, 
            (100*PicProgWriteP)/Prog.mcu->flashWords);
    CompileSuccessfulMessage(str);
}
