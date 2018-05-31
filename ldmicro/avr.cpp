//-----------------------------------------------------------------------------
// Copyright 2007 Jonathan Westhues
// Copyright 2015 Nehrutsa Ihor
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
#include "stdafx.h"
#include "filetracker.hpp"

#define ASM_LABEL 1
//                0 - no labels
//              * 1 - only if GOTO or CALL operations need a label
//                2 - always, all line is labeled

#define USE_WDT
#define USE_MUL

#include "ldmicro.h"
#include "intcode.h"

#define r0 0 // used muls. Don't use elsewhere!!!
#define r1 1 // used muls. Don't use elsewhere!!!
#define r2 2 // used in MultiplyRoutine, RANDOM
#define r3 3 // used in CopyBit, XorBit, _SWAP etc.

#define r4 4 // WriteLiteralToMemory
#define r5 5 // WriteLiteralToMemory

#define r6 6
#define r7 7 // used as Sign Register (BIT7) in DivideRoutine
#define r8 8
#define r9 9 // used ONLY in QuadEncodInterrupt to save REG_SREG. Don't use elsewhere!!!

#define r10 10 //used as op1 copy in MultiplyRoutine24
#define r11 11
#define r12 12
#define r13 13
#define r14 14
#define r15 15
#define r16 16 //used as op2
#define r17 17 //...
#define r18 18 //...
#define r19 19 //used as op2+3

#define r20 20 //used as op1 and result
#define r21 21 //...
#define r22 22 //...
#define r23 23 //used as op1+3 and result+3

#define r24 24
#define r25 25 // used in WriteLiteralToMemory, WriteMemory, CopyBit, SetBit, IfBitClear, etc.
//      r25    // used as Loop counter in MultiplyRoutine, DivideRoutine, etc.
/*
#define r26 26 // X
#define r27 27
#define r28 28 // Y used as pointer to op2
#define r29 29
#define r30 30 // Z used as pointer to op1 and result
#define r31 31
*/
/* Pointer definition   */
#define XL 26
#define XH 27
#define YL 28
#define YH 29
#define ZL 30
#define ZH 31
static DWORD REG_EIND = 0; // EIND:ZH:ZL indirect addres for EICALL, EIJMP

#define MAX_PROGRAM_LEN 128 * 1024
static PicAvrInstruction AvrProg[MAX_PROGRAM_LEN];
static DWORD             AvrProgWriteP;

DWORD AvrProgLdLen = 0;

static int IntPcNow = -INT_MAX; //must be static

RungAddr AddrOfRungN[MAX_RUNGS];

#define OP_XOR OP_EOR

// For yet unresolved references in jumps
static DWORD FwdAddrCount;

// clang-format off

// Fancier: can specify a forward reference to the high or low octet of a
// 16-bit or more address, which is useful for indirect jumps.
//                         0x003fFFFF == 4194303 ==   4M address space
//                         0x001fFFFF == 2097151 ==   2M address
//                         0x000fFFFF == 1048575 ==   1M address
//                         0x0007FFFF ==  524287 == 512K address
//                         0x0003FFFF ==  262143 == 256K address
//                         0x0001FFFF ==  131071 == 128K address
//                         0x0000FFFF ==   65535 ==  65K address
//                         0x00000FFF ==    4095 ==   4K address
#define FWD(x)      ((x) | 0x80000000)
#define FWD_HI(x)   ((x) | 0x40000000)
#define FWD_LO(x)   ((x) | 0x20000000)
#define FWD_EIND(x) ((x) | 0x10000000)

// Address to jump to when we finish one PLC cycle
static DWORD BeginOfPLCCycle;

// Address of the multiply subroutine, and whether we will have to include it
static DWORD MultiplyAddress;
static bool  MultiplyUsed;
static DWORD MultiplyAddress8;
static bool  MultiplyUsed8;
static DWORD MultiplyAddress24;
static bool  MultiplyUsed24;
static DWORD MultiplyAddress32;
static bool  MultiplyUsed32;
// and also divide
static DWORD DivideAddress;
static bool  DivideUsed;
static DWORD DivideAddress8;
static bool  DivideUsed8;
static DWORD DivideAddress24;
static bool  DivideUsed24;

// For EEPROM: we queue up characters to send in 16-bit words (corresponding
// to the integer variables), but we can actually just program 8 bits at a
// time, so we need to store the high byte somewhere while we wait.
static DWORD EepromHighByte; // Allocate high bytes needed for 16-24-32 bit integers variables.
//static DWORD AllocateNextByte; // Allocate 2 bytes needed for 24 bit integers variables.
//static DWORD EepromHighByteWaitingAddr; // obsolete
//static int   EepromHighByteWaitingBit;  // obsolete
static DWORD EepromHighBytesCounter;

// Some useful registers, unfortunately many of which are in different places
// on different AVRs! I consider this a terrible design choice by Atmel.
// 0 means not defined.
static DWORD REG_TIMSK  = 0;
static int       OCIE1A = -1; // Timer/Counter1, Output Compare A Match Interrupt Enable
static int       TOIE1  = -1; // Timer/Counter1 Overflow Interrupt Enable
static int       TOIE0  = -1; // Timer/Counter0 Overflow Interrupt Enable

static DWORD REG_TIFR1  = 0;
static int       OCF1A  = -1; // Timer/Counter1, Output Compare A Match Flag
static int       TOV1   = -1; // Timer/Counter1 Overflow Flag
static DWORD REG_TIFR0  = 0;
static int       OCF0A  = -1; // Timer/Counter0, Output Compare A Match Flag
static int       TOV0   = -1; // Timer/Counter0 Overflow Flag

#define REG_SREG    0x5f
#define     SREG_C  0
#define     SREG_Z  1
#define     SREG_N  2
#define     SREG_V  3
#define     SREG_S  4
#define     SREG_H  5
#define     SREG_T  6
#define     SREG_I  7
#define REG_SPH     0x5e
#define REG_SPL     0x5d

static DWORD REG_ADMUX  = 0;
static DWORD REG_ADCSRA = 0;
#define          ADEN     BIT7
#define          ADSC     BIT6
#define          ADFR     BIT5 // ADATE
#define          ADIE     BIT3
static DWORD REG_ADCSRB = 0;
#define          ACME     BIT6
#define          MUX5     BIT3
static DWORD REG_ADCH   = 0;
static DWORD REG_ADCL   = 0;

// USART
static DWORD REG_UBRRH  = 0;
static DWORD REG_UBRRL  = 0;
#define          UCSZ0    1
#define          UCSZ1    2
#define          URSEL    7
static DWORD REG_UCSRB  = 0;
#define          RXEN   BIT4
#define          TXEN   BIT3
static DWORD REG_UCSRA  = 0;
#define          RXC    BIT7 // USART Receive Complete
                        // This flag bit is set when there are unread data
                        //   in the receive buffer and
                        //   cleared when the receive buffer is empty.
#define          TXC    BIT6 // USART Transmit Complete
/*
This flag bit is set when the entire frame in the Transmit Shift Register
has been shifted out and there are no new data currently present in the
transmit buffer (UDR). The TXC Flag bit is automatically cleared
when a transmit complete interrupt is executed, or it can be cleared
by writing a one to its bit location.
*/
#define          UDRE   BIT5 // bit is 1 when tx buffer is empty

#define          FE     BIT4 // Frame Error
#define          DOR    BIT3 // Data OverRun
#define          PE     BIT2 // Parity Error

static DWORD REG_UDR = 0;

// WDT
static DWORD REG_WDTCR  = 0; // 0x41 or 0x60
#define          WDP3    BIT5
#define          WDCE    BIT4
#define          WDE     BIT3
#define          WDP2    BIT2
#define          WDP1    BIT1
#define          WDP0    BIT0

// Timer1
static DWORD REG_OCR1AH = 0; // 0x4b
static DWORD REG_OCR1AL = 0; // 0x4a
static DWORD REG_TCCR1A = 0; // 0x4f
static DWORD REG_TCCR1B = 0; // 0x4e
#define          WGM13    4
#define          WGM12    3
#define          WGM11    1
#define          WGM10    0

// Timer0
static DWORD REG_TCCR0A = 0;    //
static int       WGM00  = -1;   // 0 or 6 or not
static int       WGM01  = -1;   // 1 or 3 or not
static DWORD REG_TCCR0B = 0;    // TCCR0 // cs
//static int       WGM02  = -1; // 3 or not // always = 0
//static int       WGM03  = -1; // not
static DWORD REG_TCNT0  = 0;    //
static DWORD REG_OCR0A  = 0;    //

// Timer2
static DWORD REG_OCR2   = 0;
static DWORD REG_TCCR2  = 0; // TCCR2A
static DWORD REG_TCCR2B = 0;
static int       WGM20  = -1;
static int       WGM21  = -1;
static int       WGM22  = -1;
static int       WGM23  = -1;
static int       COM21  = BIT5;
static int       COM20  = BIT4;

//EEPROM
static DWORD REG_EEARH     = 0;
static DWORD REG_EEARL     = 0;
static DWORD REG_EEDR      = 0;
static DWORD REG_EECR      = 0;
#define          EERE   BIT0
#define          EEWE   BIT1
#define          EEMWE  BIT2
#define          EERIE  BIT3

// I2C support for ATmega8,16,32,64,128
#define TWCR    0x56
#define TWDR    0x23
#define TWAR    0x22
#define TWSR    0x21
#define TWBR    0x20
/*
// I2C support for atmega48,88,168,328,164,324,644,1284,2560
#define TWAMR   0xBD
#define TWCR    0xBC
#define TWDR    0xBB
#define TWAR    0xBA
#define TWSR    0xB9
#define TWBR    0xB8
*/

/*
//SPI
static DWORD REG_SPCR = 0; // SSPCON  // SPI Control Register
static DWORD REG_SPSR = 0; // SSPSTAT // SPI Status Register
static DWORD REG_SPDR = 0; // SSPBUF  // SPI Data Register
*/

// Interrupt Vectors Table
static DWORD Int0Addr         = 0;
static DWORD Int1Addr         = 0;
static DWORD Timer0OvfAddr    = 0;
static DWORD Timer1OvfAddr    = 0;
static DWORD Timer1CompAAddr  = 0;

// General Timer/Counter Control Register
static DWORD REG_GTCCR = 0;
#define          PSRASY  BIT1
// Special Function IO Register
static DWORD REG_SFIOR = 0;
static int       PSR2  = -1;

// Power Reduction Register
static DWORD REG_PRR   = 0;

// Sleep Mode Control Register (redirected) =================================
static DWORD REG_sleep = 0;  // maybe REG_SMCR or REG_MCUCR !!!
static int       SE    = -1; // maybe in REG_SMCR or REG_MCUCR !!!
static int       SM0   = -1; // maybe in REG_SMCR or REG_MCUCR or REG_EMCUCR!!!
static int       SM1   = -1; // maybe in REG_SMCR or REG_MCUCR !!!
static int       SM2   = -1; // maybe in REG_SMCR or REG_MCUCR or REG_MCUCSR!!!

// Sleep Mode Control Register
//static DWORD REG_SMCR  = 0; // 0x53;

// MCU Control Register
//static DWORD REG_MCUCR  = 0; // 0x55;
static DWORD REG_EMCUCR = 0;
static DWORD REG_MCUCSR = 0;

// clang-format on

// External Interrupts support (redirected) =================================
static DWORD REG_int_sup = 0; // maybe REG_EICRA or REG_MCUCR !!!

// External Interrupt Control Registers
//static DWORD REG_EICRB = 0;
//static DWORD REG_EICRA = 0;

//               Interrupt Sense Control
static int ISC11 = BIT3;
static int ISC10 = BIT2;
static int ISC01 = BIT1;
static int ISC00 = BIT0; //maybe in REG_EICRA or REG_MCUCR !!!

// Interrupt Control Register
static DWORD REG_int_en = 0; // maybe REG_GICR or REG_EIMSK !!!
//static DWORD REG_GICR  = 0;
// External Interrupt Mask Register
//static DWORD REG_EIMSK = 0;
//               External Interrupt Request Enable
static int INT1 = -1;
static int INT0 = -1;

// Interrupt Flag Register
static DWORD REG_int_flag = 0; // maybe REG_GIFR or REG_EIFR !!!
//static DWORD REG_GIFR  = 0;
// External Interrupt Flag Register
//static DWORD REG_EIFR  = 0;
//               External Interrupt Flag is cleared when the interrupt routine is executed.
static int INTF1 = -1;
static int INTF0 = -1;
//===========================================================================
//used in NPulseTimerOverflowInterrupt in ELEM_NPULSE
static DWORD NPulseTimerOverflowVector;
static int   tcntNPulse = 0;
static DWORD NPulseTimerOverflowRegAddr;
static int   NPulseTimerOverflowBit;
static DWORD NPulseTimerOverflowCounter;
static int   sovNPulseTimerOverflowCounter;

static uint32_t IntPc;

static void CompileFromIntermediate();

//-----------------------------------------------------------------------------
// Wipe the program and set the write pointer back to the beginning. Also
// flush all the state of the register allocators etc.
//-----------------------------------------------------------------------------
static void WipeMemory()
{
    memset(AvrProg, 0, sizeof(AvrProg));
    AvrProgWriteP = 0;
}

//-----------------------------------------------------------------------------
// Store an instruction at the next spot in program memory.  Error condition
// if this spot is already filled. We don't actually assemble to binary yet;
// there may be references to resolve.
//-----------------------------------------------------------------------------
static void _Instruction(int l, const char *f, const char *args, AvrOp op, DWORD arg1, DWORD arg2,
                         const char *comment) //, IntOp *IntCode)
{
    if(AvrProg[AvrProgWriteP].opAvr != OP_VACANT)
        THROW_COMPILER_EXCEPTION("Internal error.");

    if(op == OP_COMMENTINT) {
        if(comment) {
            if(strlen(AvrProg[AvrProgWriteP].commentInt))
                strncatn(AvrProg[AvrProgWriteP].commentInt, "\n    ; ", MAX_COMMENT_LEN);
            strncatn(AvrProg[AvrProgWriteP].commentInt, comment, MAX_COMMENT_LEN);
        }
        return;
    }

    if(AvrProg[AvrProgWriteP].opAvr != OP_VACANT)
        THROW_COMPILER_EXCEPTION("Internal error.");
    //vvv  same
    AvrProg[AvrProgWriteP].opAvr = op;
    AvrProg[AvrProgWriteP].arg1 = arg1;
    AvrProg[AvrProgWriteP].arg2 = arg2;
    if(args) {
        if(strlen(AvrProg[AvrProgWriteP].commentAsm))
            strncatn(AvrProg[AvrProgWriteP].commentAsm, " ; ", MAX_COMMENT_LEN);
        strncatn(AvrProg[AvrProgWriteP].commentAsm, "(", MAX_COMMENT_LEN);
        strncatn(AvrProg[AvrProgWriteP].commentAsm, args, MAX_COMMENT_LEN);
        strncatn(AvrProg[AvrProgWriteP].commentAsm, ")", MAX_COMMENT_LEN);
    }
    if(comment) {
        if(strlen(AvrProg[AvrProgWriteP].commentAsm))
            strncatn(AvrProg[AvrProgWriteP].commentAsm, " ; ", MAX_COMMENT_LEN);
        strncatn(AvrProg[AvrProgWriteP].commentAsm, comment, MAX_COMMENT_LEN);
    }
    AvrProg[AvrProgWriteP].rung = rungNow;
    AvrProg[AvrProgWriteP].IntPc = IntPcNow;
    AvrProg[AvrProgWriteP].l = l;
    strcpy(AvrProg[AvrProgWriteP].f, f);
    //^^^ same
    AvrProgWriteP++;
    if(AvrProgWriteP >= MAX_PROGRAM_LEN) {
        THROW_COMPILER_EXCEPTION(_("Internal limit exceeded (MAX_PROGRAM_LEN)"));
    }
}

static void _Instruction(int l, const char *f, const char *args, AvrOp op, DWORD arg1, DWORD arg2)
{
    _Instruction(l, f, args, op, arg1, arg2, nullptr);
}

static void _Instruction(int l, const char *f, const char *args, AvrOp op, DWORD arg1)
{
    _Instruction(l, f, args, op, arg1, 0, nullptr);
}

static void _Instruction(int l, const char *f, const char *args, AvrOp op)
{
    _Instruction(l, f, args, op, 0, 0, nullptr);
}
static void _Instruction(int l, const char *f, const char *args, AvrOp op, const char *comment)
{
    _Instruction(l, f, args, op, 0, 0, comment);
}

// And use macro for bugtracking
#define Instruction(...) _Instruction(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)
//-----------------------------------------------------------------------------
static void _SetInstruction(int l, char *f, char *args, DWORD addr, AvrOp op, DWORD arg1, DWORD arg2)
//for setiing interrupt vector
{
    if(addr == 0) {
        THROW_COMPILER_EXCEPTION(_("Direct Addr error"));
    }
    if(addr >= MAX_PROGRAM_LEN) {
        THROW_COMPILER_EXCEPTION(_("Internal limit exceeded (MAX_PROGRAM_LEN)"));
    }
    //vvv  same
    AvrProg[addr].opAvr = op;
    AvrProg[addr].arg1 = arg1;
    AvrProg[addr].arg2 = arg2;

    if(args) {
        if(strlen(AvrProg[addr].commentAsm))
            strncatn(AvrProg[addr].commentAsm, " ; ", MAX_COMMENT_LEN);
        strncatn(AvrProg[addr].commentAsm, "(", MAX_COMMENT_LEN);
        strncatn(AvrProg[addr].commentAsm, args, MAX_COMMENT_LEN);
        strncatn(AvrProg[addr].commentAsm, ")", MAX_COMMENT_LEN);
    }
    AvrProg[addr].rung = rungNow;
    AvrProg[addr].IntPc = IntPcNow;
    AvrProg[addr].l = l;
    strcpy(AvrProg[addr].f, f);
    //^^^ same
}

#define SetInstruction(...) _SetInstruction(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)

//-----------------------------------------------------------------------------
// printf-like comment function
//-----------------------------------------------------------------------------
static void Comment(const char *str, ...)
{
    if(asm_comment_level) {
        va_list f;
        char    buf[MAX_COMMENT_LEN];
        va_start(f, str);
        vsnprintf(buf, MAX_COMMENT_LEN, str, f);
        Instruction(OP_COMMENTINT, buf);
    }
}

//-----------------------------------------------------------------------------
// Allocate a unique descriptor for a forward reference. Later that forward
// reference gets assigned to an absolute address, and we can go back and
// fix up the reference.
//-----------------------------------------------------------------------------
static DWORD AllocFwdAddr()
{
    FwdAddrCount++;
    return FWD(FwdAddrCount);
}

//-----------------------------------------------------------------------------
// Go back and fix up the program given that the provided forward address
// corresponds to the next instruction to be assembled.
//-----------------------------------------------------------------------------
static void FwdAddrIsNow(DWORD addr)
{
    if(!(addr & FWD(0)))
        THROW_COMPILER_EXCEPTION("Internal error.");

    WORD  seen = 0;
    DWORD i;
    for(i = 0; i < AvrProgWriteP; i++) {
        if(AvrProg[i].arg1 == FWD(addr)) { // Its a FWD addr
            AvrProg[i].arg1 = AvrProgWriteP;
            seen = 0x10;
        } else if(AvrProg[i].arg2 == FWD_LO(addr)) {
            AvrProg[i].arg2 = AvrProgWriteP & 0xff;
            seen = 0x01;
        } else if(AvrProg[i].arg2 == FWD_HI(addr)) {
            AvrProg[i].arg2 = (AvrProgWriteP >> 8) & 0xff;
            seen = 0x02;
        } else if(AvrProg[i].arg2 == FWD_EIND(addr)) {
            AvrProg[i].arg2 = (AvrProgWriteP >> 16) & 0xff;
            seen = 0x04;
        }
    }
}

//-----------------------------------------------------------------------------
#define IS_SKIP 2
#define IS_BANK 1
#define IS_ANY_BANK 0
#define IS_RETS -1
#define IS_PAGE -2
#define IS_GOTO -2
#define IS_CALL -3

static int IsOperation(AvrOp op)
{
    switch(op) {
            /*
        case OP_BTFSC:
        case OP_BTFSS:
        case OP_DECFSZ:
        case OP_INCFSZ:
            return OP_SKIP; // can need to change bank
        case OP_ADDWF:
        case OP_ANDWF:
        case OP_BSF:
        case OP_BCF:
        case OP_CLRF:
        case OP_COMF:
        case OP_DECF:
        case OP_INCF:
        case OP_IORWF:
        case OP_MOVF:
        case OP_MOVWF:
        case OP_RLF:
        case OP_RRF:
        case OP_SUBWF:
        case OP_XORWF:
            return OP_BANK; // can need to change bank
        case OP_CLRWDT:
        case OP_MOVLW:
        case OP_MOVLB:
        case OP_MOVLP:
        case OP_NOP_:
        case OP_COMMENT_:
        case OP_SUBLW:
        case OP_IORLW:
            return 0;       // not need to change bank
        case OP_RETURN:
        case OP_RETFIE:
            return OP_RET;  // not need to change bank
        case OP_GOTO:
*/
        case OP_BREQ:
        case OP_BRNE:
        case OP_BRLO:
        case OP_BRGE:
        case OP_BRLT:
        case OP_BRCC:
        case OP_BRCS:
        case OP_BRMI:
            return IS_PAGE;
        case OP_EIJMP:
        case OP_IJMP:
        case OP_RJMP:
            return IS_GOTO; // can need to change page
        case OP_EICALL:
        case OP_ICALL:
        case OP_RCALL:
            return IS_CALL; // can need to change page
        default:
            return 0;
    }
}

//-----------------------------------------------------------------------------
static void AddrCheckForErrorsPostCompile()
{
    DWORD i;
    for(i = 0; i < AvrProgWriteP; i++) {
        if(IsOperation(AvrProg[i].opAvr) <= IS_PAGE) {
            if(AvrProg[i].arg1 & FWD(0)) {
                THROW_COMPILER_EXCEPTION_FMT("Every AllocFwdAddr needs FwdAddrIsNow.\ni=%d op=%d arg1=%d arg2=%d rung=%d",
                      i,
                      AvrProg[i].opAvr,
                      AvrProg[i].arg1,
                      AvrProg[i].arg2,
                      AvrProg[i].rung + 1);
            }
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
static DWORD Assemble(DWORD addrAt, AvrOp op, DWORD arg1, DWORD arg2, char *sAsm)
{
    PicAvrInstruction *AvrInstr = &AvrProg[addrAt];
    IntOp              intOp;
    if(AvrInstr->IntPc > -1 && static_cast<uint32_t>(AvrInstr->IntPc) < IntCode.size())
        intOp = IntCode[AvrInstr->IntPc];
    strcpy(sAsm, "");
/*
#define CHECK(v, bits) if((v) != ((v) & ((1 << (bits))-1))) oops()
*/
#define CHECK(v, bits)                                            \
    if((v) != ((v) & ((1 << (bits)) - 1)))                        \
    THROW_COMPILER_EXCEPTION_FMT("v=%d ((1 << (%d))-1)=%d\nat %d in %s %s\nat %d in %s", \
          v,                                                      \
          bits,                                                   \
          ((1 << (bits)) - 1),                                    \
          AvrInstr->l,                                            \
          AvrInstr->f,                                            \
          intOp.name1.c_str(),                                    \
          intOp.fileLine,                                         \
          intOp.fileName.c_str())
#define CHECK2(v, LowerRangeInclusive, UpperRangeInclusive)              \
    if(((int)v < LowerRangeInclusive) || ((int)v > UpperRangeInclusive)) \
    THROW_COMPILER_EXCEPTION_FMT("v=%d [%d..%d]\nat %d in %s %s\nat %d in %s",                  \
          (int)v,                                                        \
          LowerRangeInclusive,                                           \
          UpperRangeInclusive,                                           \
          AvrInstr->l,                                                   \
          AvrInstr->f,                                                   \
          intOp.name1.c_str(),                                           \
          intOp.fileLine,                                                \
          intOp.fileName.c_str())

    switch(op) {
        case OP_COMMENT:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            if(strlen(AvrInstr->commentInt))
                sprintf(sAsm, "nop \t; %s", AvrInstr->commentInt);
            else
                sprintf(sAsm, "nop \t \t");
            return 0;

        case OP_NOP:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "nop \t \t");
            return 0;

        case OP_SLEEP:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "sleep \t \t");
            return 0x9588;

        case OP_CPSE:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "cpse \t r%d, \t r%d", arg1, arg2);
            return 0x1000 | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_ASR:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "asr \t r%d \t", arg1);
            return 0x9000 | (2 << 9) | (arg1 << 4) | 5;

        case OP_ROR:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ror \t r%d \t", arg1);
            return 0x9000 | (2 << 9) | (arg1 << 4) | 7;

        case OP_ADD:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "add \t r%d, \t r%d", arg1, arg2);
            return (3 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_ADC:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "adc \t r%d, \t r%d", arg1, arg2);
            return (7 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_ADIW:
            if(!((arg1 == 24) || (arg1 == 26) || (arg1 == 28) || (arg1 == 30)))
                THROW_COMPILER_EXCEPTION("Internal error");
            CHECK2(arg2, 0, 64);
            sprintf(sAsm, "adiw \t r%d:r%d, %d", arg1 + 1, arg1, arg2);
            return 0x9600 | ((arg2 & 0x30) << 2) | ((arg1 & 0x06) << 3) | (arg2 & 0x0f);

        case OP_SBIW:
            if(!((arg1 == 24) || (arg1 == 26) || (arg1 == 28) || (arg1 == 30)))
                THROW_COMPILER_EXCEPTION("Internal error");
            CHECK2(arg2, 0, 64);
            sprintf(sAsm, "sbiw \t r%d:r%d, %d", arg1 + 1, arg1, arg2);
            return 0x9700 | ((arg2 & 0x30) << 2) | ((arg1 & 0x06) << 3) | (arg2 & 0x0f);

        case OP_EOR:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "eor \t r%d, \t r%d", arg1, arg2);
            return (9 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_CLR:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "clr \t r%d \t", arg1);
            return (9 << 10) | ((arg1 & 0x10) << 5) | (arg1 << 4) | (arg1 & 0x0f);

        case OP_SER:
            CHECK2(arg1, 16, 31);
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            if((arg1 < 16) || (31 < arg1))
                THROW_COMPILER_EXCEPTION("Internal error");
            sprintf(sAsm, "ser \t r%d \t", arg1);
            return 0xEF0F | (arg1 << 4);

        case OP_SUB:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "sub \t r%d, \t r%d", arg1, arg2);
            return (6 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_SBC:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "sbc \t r%d, \t r%d", arg1, arg2);
            return (2 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_CP:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "cp  \t r%d, \t r%d", arg1, arg2);
            return (5 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_CPC:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "cpc \t r%d, \t r%d", arg1, arg2);
            return (1 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_CPI:
            CHECK2(arg1, 16, 31);
            CHECK(arg2, 8);
            sprintf(sAsm, "cpi \t r%d, \t 0x%X", arg1, arg2);
            return 0x3000 | ((arg2 & 0xF0) << 4) | ((arg1 & 0x0F) << 4) | (arg2 & 0x0F);

        case OP_COM:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "com \t r%d \t", arg1);
            return 0x9000 | (2 << 9) | (arg1 << 4);

        case OP_SBR:
            CHECK2(arg1, 16, 31);
            CHECK(arg2, 8);
            sprintf(sAsm, "sbr \t r%d, \t 0x%X", arg1, arg2);
            return 0x6000 | ((arg2 & 0xf0) << 4) | ((arg1 & 0x0F) << 4) | (arg2 & 0x0f);

        case OP_CBR:
            CHECK2(arg1, 16, 31);
            CHECK(arg2, 8);
            sprintf(sAsm, "cbr \t r%d, \t 0x%X", arg1, arg2);
            arg2 = ~arg2;
            return 0x7000 | ((arg2 & 0xf0) << 4) | ((arg1 & 0x0F) << 4) | (arg2 & 0x0f);

        case OP_INC:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "inc \t r%d \t", arg1);
            return 0x9400 | (arg1 << 4) | 3;

        case OP_DEC:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "dec \t r%d \t", arg1);
            return 0x9400 | (arg1 << 4) | 10;

        case OP_SUBI:
            CHECK2(arg1, 16, 31);
            CHECK2(arg2, -256, 255);
            sprintf(sAsm, "subi \t r%d, \t %d", arg1, arg2);
            return 0x5000 | ((arg2 & 0XF0) << 4) | ((arg1 & 0x0F) << 4) | (arg2 & 0X0F);

        case OP_SBCI:
            CHECK2(arg1, 16, 31);
            CHECK(arg2, 8);
            sprintf(sAsm, "sbci \t r%d, \t 0x%X", arg1, arg2);
            return 0x4000 | ((arg2 & 0xf0) << 4) | ((arg1 & 0x0F) << 4) | (arg2 & 0x0f);

        case OP_TST:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "tst \t r%d \t", arg1);
            return 0x2000 | ((arg1 & 0x10) << 4) | ((arg1 & 0x10) << 5) | ((arg1 & 0x0f) << 4) | (arg1 & 0x0f);

        case OP_SEC:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "sec \t \t");
            return 0x9408;

        case OP_CLC:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "clc \t \t");
            return 0x9488;

        case OP_IJMP:
            //CHECK(arg1, 0); // arg1 used for label
            CHECK(arg2, 0);
            sprintf(sAsm, "ijmp \t \t");
            return 0x9409;

        case OP_EIJMP:
            //CHECK(arg1, 0); // arg1 used for label
            CHECK(arg2, 0);
            sprintf(sAsm, "eijmp \t \t");
            return 0x9419;

        case OP_ICALL:
            //CHECK(arg1, 0); // arg1 used for label
            CHECK(arg2, 0);
            sprintf(sAsm, "icall \t \t");
            return 0x9509;

        case OP_EICALL:
            //CHECK(arg1, 0); // arg1 used for label
            CHECK(arg2, 0);
            sprintf(sAsm, "eicall \t \t");
            return 0x9519;

        case OP_RJMP:
            CHECK(arg2, 0);
            sprintf(sAsm, "rjmp \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -2048, 2047); // $fff !!!
            if(((int)arg1) > 2047 || ((int)arg1) < -2048)
                THROW_COMPILER_EXCEPTION("Internal error");
            arg1 &= (4096 - 1); // $fff !!!
            return 0xC000 | arg1;

        case OP_RCALL:
            CHECK(arg2, 0);
            sprintf(sAsm, "rcall \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -2048, 2047); //$fff !!!
            if(((int)arg1) > 2047 || ((int)arg1) < -2048)
                THROW_COMPILER_EXCEPTION("Internal error");
            arg1 &= (4096 - 1);
            return 0xD000 | arg1;

        case OP_RETI:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "reti \t \t");
            return 0x9518;

        case OP_RET:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "ret \t \t");
            return 0x9508;

        case OP_SBRC:
            CHECK(arg1, 5);
            CHECK(arg2, 3);
            sprintf(sAsm, "sbrc \t r%d, \t %d", arg1, arg2);
            return (0x7e << 9) | (arg1 << 4) | arg2;

        case OP_SBRS:
            CHECK(arg1, 5);
            CHECK(arg2, 3);
            sprintf(sAsm, "sbrs \t r%d, \t %d", arg1, arg2);
            return (0x7f << 9) | (arg1 << 4) | arg2;

        case OP_BREQ:
            CHECK(arg2, 0);
            sprintf(sAsm, "breq \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -64, 63);
            return 0xf001 | ((arg1 & 0x7f) << 3);

        case OP_BRNE:
            CHECK(arg2, 0);
            sprintf(sAsm, "brne \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -64, 63);
            return 0xf401 | ((arg1 & 0x7f) << 3);

        case OP_BRLO:
            CHECK(arg2, 0);
            sprintf(sAsm, "brlo \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -64, 63);
            return 0xf000 | ((arg1 & 0x7f) << 3);

        case OP_BRGE:
            CHECK(arg2, 0);
            sprintf(sAsm, "brge \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -64, 63);
            return 0xf404 | ((arg1 & 0x7f) << 3);

        case OP_BRLT:
            CHECK(arg2, 0);
            sprintf(sAsm, "brlt \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -64, 63);
            return 0xf004 | ((arg1 & 0x7f) << 3);

        case OP_BRCC:
            CHECK(arg2, 0);
            sprintf(sAsm, "brcc \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -64, 63);
            return 0xf400 | ((arg1 & 0x7f) << 3);

        case OP_BRCS:
            CHECK(arg2, 0);
            sprintf(sAsm, "brcs \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -64, 63);
            return 0xf000 | ((arg1 & 0x7f) << 3);

        case OP_BRMI:
            CHECK(arg2, 0);
            sprintf(sAsm, "brmi \t l_%06x ", arg1);
            arg1 = arg1 - addrAt - 1;
            CHECK2(arg1, -64, 63);
            return 0xf002 | ((arg1 & 0x7f) << 3);

        case OP_MOV:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "mov \t r%d, \t r%d", arg1, arg2);
            return (0xb << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_SWAP:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "swap \t r%d \t", arg1);
            return 0x9402 | (arg1 << 4);

        case OP_LDI:
            CHECK2(arg1, 16, 31);
            CHECK(arg2, 8);
            sprintf(sAsm, "ldi \t r%d, \t 0x%X", arg1, arg2);
            return 0xE000 | ((arg2 & 0xF0) << 4) | ((arg1 & 0x0F) << 4) | (arg2 & 0x0F);

        case OP_LD_X:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ld  \t r%d, \t X", arg1);
            return 0x9000 | (arg1 << 4) | 12;

        case OP_LD_XP:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ld  \t r%d, \t X+", arg1);
            return 0x9000 | (arg1 << 4) | 13;

        case OP_LD_XS:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ld  \t r%d, \t -X", arg1);
            return 0x9000 | (arg1 << 4) | 14;

        case OP_LD_Y:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ld  \t r%d, \t Y", arg1);
            return 0x8000 | (arg1 << 4) | 8;

        case OP_LD_YP:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ld  \t r%d, \t Y+", arg1);
            return 0x9000 | (arg1 << 4) | 9;

        case OP_LD_YS:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ld  \t r%d, \t -Y", arg1);
            return 0x9000 | (arg1 << 4) | 10;

        case OP_LDD_Y:
            CHECK(arg1, 5);
            CHECK(arg2, 6);
            sprintf(sAsm, "ldd  \t r%d, \t Y+%d", arg1, arg2);
            return 0x8008 | (arg1 << 4) | ((arg2 & 0x20) << 8) | ((arg2 & 0x18) << 7) | (arg2 & 0x7);

        case OP_LD_Z:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ld  \t r%d, \t Z", arg1);
            return 0x8000 | (arg1 << 4) | 0;

        case OP_LD_ZP:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ld  \t r%d, \t Z+", arg1);
            return 0x9000 | (arg1 << 4) | 1;

        case OP_LD_ZS:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "ld  \t r%d, \t -Z", arg1);
            return 0x9000 | (arg1 << 4) | 2;

        case OP_LDD_Z:
            CHECK(arg1, 5);
            CHECK(arg2, 6);
            sprintf(sAsm, "ldd  \t r%d, \t Z+%d", arg1, arg2);
            return 0x8000 | (arg1 << 4) | ((arg2 & 0x20) << 8) | ((arg2 & 0x18) << 7) | (arg2 & 0x7);

        case OP_LPM_0Z:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "lpm \t \t");
            return 0x95C8;

        case OP_LPM_Z:
            CHECK2(arg1, 0, 31);
            CHECK(arg2, 0);
            sprintf(sAsm, "lpm \t r%d, \t Z", arg1);
            return (0x9004) | (arg1 << 4);

        case OP_LPM_ZP:
            CHECK2(arg1, 0, 31);
            CHECK(arg2, 0);
            sprintf(sAsm, "lpm \t r%d, \t Z+", arg1);
            return (0x9005) | (arg1 << 4);

        case OP_ST_X:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "st  \t X, \t r%d", arg1);
            return 0x9200 | (arg1 << 4) | 12;

        case OP_ST_XP:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "st  \t X+, \t r%d", arg1);
            return 0x9200 | (arg1 << 4) | 13;

        case OP_ST_XS:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "st  \t -X, \t r%d", arg1);
            return 0x9200 | (arg1 << 4) | 14;

        case OP_ST_Y:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "st  \t Y, \t r%d", arg1);
            return 0x8200 | (arg1 << 4) | 8;

        case OP_ST_YP:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "st  \t Y+, \t r%d", arg1);
            return 0x9200 | (arg1 << 4) | 9;

        case OP_ST_YS:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "st  \t -Y, \t r%d", arg1);
            return 0x9200 | (arg1 << 4) | 10;

        case OP_ST_Z:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "st  \t Z, \t r%d", arg1);
            return 0x8200 | (arg1 << 4) | 0;

        case OP_ST_ZP:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "st  \t Z+, \t r%d", arg1);
            return 0x9200 | (arg1 << 4) | 1;

        case OP_ST_ZS:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "st  \t -Z, \t r%d", arg1);
            return 0x9200 | (arg1 << 4) | 2;

        case OP_WDR:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "wdr \t \t");
            return 0x95a8;

        case OP_ANDI:
            CHECK2(arg1, 16, 31);
            CHECK(arg2, 8);
            sprintf(sAsm, "andi \t r%d, \t 0x%X", arg1, arg2);
            return 0x7000 | ((arg2 & 0xF0) << 4) | ((arg1 & 0x0F) << 4) | (arg2 & 0x0F);

        case OP_ORI:
            CHECK2(arg1, 16, 31);
            CHECK(arg2, 8);
            sprintf(sAsm, "ori \t r%d, \t 0x%X", arg1, arg2);
            return 0x6000 | ((arg2 & 0xF0) << 4) | ((arg1 & 0x0F) << 4) | (arg2 & 0x0F);

        case OP_AND:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "and \t r%d, \t r%d", arg1, arg2);
            return (0x8 << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_OR:
            CHECK(arg1, 5);
            CHECK(arg2, 5);
            sprintf(sAsm, "or  \t r%d, \t r%d", arg1, arg2);
            return (0xA << 10) | ((arg2 & 0x10) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_LSL:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "lsl \t r%d \t", arg1);
            return (3 << 10) | ((arg1 & 0x10) << 5) | (arg1 << 4) | (arg1 & 0x0f);

        case OP_LSR:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "lsr \t r%d \t", arg1);
            return (0x9406) | (arg1 << 4);

        case OP_ROL:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            sprintf(sAsm, "rol \t r%d \t", arg1);
            return (7 << 10) | ((arg1 & 0x10) << 5) | (arg1 << 4) | (arg1 & 0x0f);

        case OP_MOVW:
            CHECK2(arg1, 0, 30);
            CHECK2(arg2, 0, 30);
            if(arg1 & 1)
                THROW_COMPILER_EXCEPTION("Internal error");
            if(arg2 & 1)
                THROW_COMPILER_EXCEPTION("Internal error");
            sprintf(sAsm, "movw \t r%d:r%d, \t r%d:r%d", arg1 + 1, arg1, arg2 + 1, arg2);
            return (0x0100) | ((arg1 >> 1) << 4) | ((arg2 >> 1));

#ifdef USE_MUL
        case OP_MUL:
            CHECK2(arg1, 0, 31);
            CHECK2(arg2, 0, 31);
            sprintf(sAsm, "mul \t r%d, \t r%d", arg1, arg2);
            return (0x9C00) | (arg1 << 4) | ((arg2 & 0x10) << 5) | (arg2 & 0x0f);

        case OP_MULS:
            CHECK2(arg1, 16, 31);
            CHECK2(arg2, 16, 31);
            sprintf(sAsm, "muls \t r%d, \t r%d", arg1, arg2);
            return (0x0200) | ((arg1 & 0x0f) << 4) | (arg2 & 0x0f);

        case OP_MULSU:
            CHECK2(arg1, 16, 23);
            CHECK2(arg2, 16, 23);
            sprintf(sAsm, "mulsu \t r%d, \t r%d", arg1, arg2);
            return (0x0300) | ((arg1 & 0x07) << 4) | (arg2 & 0x07);
#endif

#if USE_IO_REGISTERS == 1
        case OP_IN:
            CHECK2(arg1, 0, 31);
            CHECK2(arg2, 0, 63);
            sprintf(sAsm, "in  \t r%d, \t 0x%02X", arg1, arg2);
            return 0xB000 | ((arg2 & 0x30) << 5) | (arg1 << 4) | (arg2 & 0x0f);

        case OP_OUT:
            CHECK2(arg1, 0, 63);
            CHECK2(arg2, 0, 31);
            sprintf(sAsm, "out \t 0x%02X, \t r%d", arg1, arg2);
            return 0xB800 | ((arg1 & 0x30) << 5) | (arg2 << 4) | (arg1 & 0x0f);

        case OP_SBI:
            CHECK2(arg1, 0, 31);
            CHECK2(arg2, 0, 7);
            sprintf(sAsm, "sbi \t 0x%02X, \t %d", arg1, arg2);
            return 0x9A00 | (arg1 << 3) | arg2;

        case OP_CBI:
            CHECK2(arg1, 0, 31);
            CHECK2(arg2, 0, 7);
            sprintf(sAsm, "cbi \t 0x%02X, \t %d", arg1, arg2);
            return 0x9800 | (arg1 << 3) | arg2;

        case OP_SBIC:
            CHECK2(arg1, 0, 31);
            CHECK2(arg2, 0, 7);
            sprintf(sAsm, "sbic \t 0x%02X, \t %d", arg1, arg2);
            return 0x9900 | (arg1 << 3) | arg2;

        case OP_SBIS:
            CHECK2(arg1, 0, 31);
            CHECK2(arg2, 0, 7);
            sprintf(sAsm, "sbis \t 0x%02X, \t %d", arg1, arg2);
            return 0x9b00 | (arg1 << 3) | arg2;
#endif
        case OP_BST:
            CHECK2(arg1, 0, 31);
            CHECK2(arg2, 0, 7);
            sprintf(sAsm, "bst \t r%d, \t %d", arg1, arg2);
            return 0xFA00 | (arg1 << 4) | arg2;

        case OP_BLD:
            CHECK2(arg1, 0, 31);
            CHECK2(arg2, 0, 7);
            sprintf(sAsm, "bld \t r%d, \t %d", arg1, arg2);
            return 0xF800 | (arg1 << 4) | arg2;

        case OP_PUSH:
            CHECK2(arg1, 0, 31);
            CHECK(arg2, 0);
            sprintf(sAsm, "push \t r%d \t", arg1);
            return (0x920F) | (arg1 << 4);

        case OP_POP:
            CHECK2(arg1, 0, 31);
            CHECK(arg2, 0);
            sprintf(sAsm, "pop \t r%d \t", arg1);
            return (0x900F) | (arg1 << 4);

        case OP_CLI:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "cli \t \t");
            return 0x94f8;

        case OP_SEI:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "sei \t \t");
            return 0x9478;

        case OP_DB:
            CHECK2(BYTE(arg1), 0, 255);
            CHECK(arg2, 0);
            //      CHECK2(arg1, -128, 127); CHECK(arg2, 0);
            sprintf(sAsm, ".db  \t 0x%02X \t", BYTE(arg1));
            return BYTE(arg1);

        case OP_DB2:
            CHECK2(BYTE(arg1), 0, 255);
            CHECK2(BYTE(arg2), 0, 255);
            //      CHECK2(arg1, -128, 127); CHECK2(arg2, -128, 127);
            sprintf(sAsm, ".db  \t 0x%02X, \t 0x%02X", BYTE(arg1), BYTE(arg2));
            return (BYTE(arg2) << 8) | BYTE(arg1);

        case OP_DW:
            CHECK2(WORD(arg1), 0, 0xffff);
            CHECK(arg2, 0);
            sprintf(sAsm, ".dw  \t 0x%04X \t", WORD(arg1));
            return WORD(arg1);

        default:
            sprintf(sAsm, "0x%X OP_%d %d %d", addrAt, op, arg1, arg2);
            THROW_COMPILER_EXCEPTION_FMT("Invalid AVR OpCode %i", static_cast<int>(op));
            return 0;
    }
}

//-----------------------------------------------------------------------------
// Write an intel IHEX format description of the program assembled so far.
// This is where we actually do the assembly to binary format.
//-----------------------------------------------------------------------------
static void WriteHexFile(FILE *f, FILE *fAsm)
{
    static int prevRung = INT_MAX;
    static int prevL = INT_MAX;
    static int prevIntPcL = INT_MAX;
    char       sAsm[1024];
    const int  n = 1; // 1 - VMLAB and (avrasm2.exe or avrasm32.exe)
        // 2 - increases the length of the data string to be compatible with "avrdude.exe -U flash:r:saved_Intel_Hex.hex:i"
    BYTE  soFar[16 * n];
    int   soFarCount = 0;
    DWORD soFarStart = 0;

    // always start from address 0
    fprintf(f, ":020000020000FC\n");
    //fprintf(f, ":020000040000FA\n");

    DWORD ExtendedSegmentAddress = 0;
    DWORD i;
    for(i = 0; i < AvrProgWriteP; i++) {
        AvrProg[i].label = false;
    }

    for(i = 0; i < AvrProgWriteP; i++) {
        if(IsOperation(AvrProg[i].opAvr) <= IS_PAGE)
            AvrProg[AvrProg[i].arg1].label = true;
    }

    for(i = 1; i < AvrProgWriteP; i++) {
        if((AvrProg[i].opAvr == OP_DB) && (AvrProg[i - 1].opAvr != OP_DB))
            AvrProg[i].label = true;
        if((AvrProg[i].opAvr == OP_DB2) && (AvrProg[i - 1].opAvr != OP_DB2))
            AvrProg[i].label = true;
        if((AvrProg[i].opAvr == OP_DW) && (AvrProg[i - 1].opAvr != OP_DW))
            AvrProg[i].label = true;
    }

    for(i = 0; i < AvrProgWriteP; i++) {
        DWORD w = Assemble(i, AvrProg[i].opAvr, AvrProg[i].arg1, AvrProg[i].arg2, sAsm);

        if(strlen(AvrProg[i].commentInt)) {
            fprintf(fAsm, "    ; %s\n", AvrProg[i].commentInt);
        }

        if(strlen(sAsm)) {

#if ASM_LABEL > 0
            if(AvrProg[i].label || (ASM_LABEL == 2))
                fprintf(fAsm, "l_%06x: %s", i, sAsm);
            else
                fprintf(fAsm, "          %s", sAsm);
#else
            fprintf(fAsm, "          %s", sAsm);
#endif

            if(asm_comment_level >= 3) {
                fprintf(fAsm, "\t");
                if(1 || (prevRung != AvrProg[i].rung)) {
                    fprintf(fAsm, " ; rung=%d", AvrProg[i].rung + 1);
                    prevRung = AvrProg[i].rung;
                } else {
                    fprintf(fAsm, " \t");
                }
            }

            if(asm_comment_level >= 4) {
                if(1 || (prevL != AvrProg[i].l)) {
                    fprintf(fAsm, " ; line %d in %s", AvrProg[i].l, AvrProg[i].f);
                    prevL = AvrProg[i].l;
                }
            }

            if(asm_comment_level >= 5) {
                if((AvrProg[i].IntPc >= 0) && (AvrProg[i].IntPc < IntCode.size())) {
                    fprintf(fAsm, "\t");
                    if(IntCode[AvrProg[i].IntPc].which != INT_MAX) {
                        fprintf(fAsm, " ; ELEM_0x%X", IntCode[AvrProg[i].IntPc].which);
                    }
                    if(1 || (prevIntPcL != IntCode[AvrProg[i].IntPc].fileLine)) {
                        fprintf(fAsm, " ; line %d in %s", IntCode[AvrProg[i].IntPc].fileLine, IntCode[AvrProg[i].IntPc].fileName.c_str());
                        prevIntPcL = IntCode[AvrProg[i].IntPc].fileLine;
                    }
                }
            }

            if(asm_comment_level >= 2)
                if(strlen(AvrProg[i].commentAsm)) {
                    fprintf(fAsm, "\t ; %s", AvrProg[i].commentAsm);
                }

            fprintf(fAsm, "\n");
        } else
            THROW_COMPILER_EXCEPTION_FMT("op=%d, i=%d", AvrProg[i].opAvr, i);

        if(ExtendedSegmentAddress != (i & ~0x7fff)) {
            //if(i == 0x8000) {
            //    fprintf(f, ":020000021000EC\n"); // TT->Record Type -> 02 is Extended Segment Address 0x10000 + 0xffff
            ExtendedSegmentAddress = (i & ~0x7fff);
            StartIhex(f);    // ':'->Colon
            WriteIhex(f, 2); // LL->Record Length
            WriteIhex(f, 0); // AA->Address as big endian values HI()
            WriteIhex(f, 0); // AA->Address as big endian values LO()
            WriteIhex(f, 2); // TT->Record Type -> 02 is Extended Segment Address
            WriteIhex(f, (BYTE)((ExtendedSegmentAddress >> 3) >> 8));   // AA->Address as big endian values HI()
            WriteIhex(f, (BYTE)((ExtendedSegmentAddress >> 3) & 0xff)); // AA->Address as big endian values LO()
            FinishIhex(f);                                              // CC->Checksum
        }
        if(soFarCount == 0)
            soFarStart = i;
        soFar[soFarCount++] = (BYTE)(w & 0xff);
        soFar[soFarCount++] = (BYTE)(w >> 8);

        if(soFarCount >= 0x10 * n || i == (AvrProgWriteP - 1)) {
            StartIhex(f);                                  // ':'->Colon
            WriteIhex(f, (BYTE)soFarCount);                // LL->Record Length
            WriteIhex(f, (BYTE)((soFarStart * 2) >> 8));   // AA->Address as big endian values HI()
            WriteIhex(f, (BYTE)((soFarStart * 2) & 0xff)); // AA->Address as big endian values LO()
            WriteIhex(f, 0x00);                            // TT->Record Type -> 00 is Data
            int j;
            for(j = 0; j < soFarCount; j++) {
                WriteIhex(f, soFar[j]); // DD->Data
            }
            FinishIhex(f); // CC->Checksum
            soFarCount = 0;
        }
    }

    // end of file record
    fprintf(f, ":00000001FF\n");
    if((Prog.mcu->flashWords) && (AvrProgWriteP >= Prog.mcu->flashWords)) {
        THROW_COMPILER_EXCEPTION_FMT(_(" Flash program memory size %d is exceed limit %d words\nfor %s."),
              AvrProgWriteP,
              Prog.mcu->flashWords,
              Prog.mcu->mcuName);
    }
}

//-----------------------------------------------------------------------------
// Make sure that the given address is loaded in the X register; might not
// have to update all of it.
//-----------------------------------------------------------------------------
static void LoadXAddr(DWORD addr, const char *comment)
//used X; Opcodes: 2
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Zero memory addres not allowed!\nLoadXAddr(%d) skiped!"), addr);
        //return;
    }
    if(addr > 0xffff) {
        THROW_COMPILER_EXCEPTION_FMT(_("Addres not allowed!\nLoadXAddr(%d) skiped!"), addr);
        //return;
    }
    Instruction(OP_LDI, XL, (addr & 0xff), comment); // X-register Low Byte
    Instruction(OP_LDI, XH, (addr >> 8));            // X-register High Byte
}

static void LoadXAddr(DWORD addr)
{
    LoadXAddr(addr, nullptr);
}
//----
static void LoadYAddr(DWORD addr, const char *comment = nullptr);
static void LoadYAddr(DWORD addr, const char *comment)
//used Y; Opcodes: 2
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Zero memory addres not allowed!\nLoadYAddr(%d) skiped!"), addr);
        //return;
    }
    if(addr > 0xffff) {
        THROW_COMPILER_EXCEPTION_FMT(_("Addres not allowed!\nLoadYAddr(%d) skiped!"), addr);
        //return;
    }
    Instruction(OP_LDI, YL, (addr & 0xff), comment); // Y-register Low Byte
    Instruction(OP_LDI, YH, (addr >> 8));            // Y-register High Byte
}
//----
static void LoadZAddr(DWORD addr, const char *comment)
//used Z; Opcodes: 2
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Zero memory addres not allowed!\nLoadZAddr(%d) skiped!"), addr);
        //return;
    }
    if(addr > 0xffff) {
        THROW_COMPILER_EXCEPTION_FMT(_("Addres not allowed!\nLoadZAddr(%d) skiped!"), addr);
        //return;
    }
    Instruction(OP_LDI, ZL, (addr & 0xff), comment); // Z-register Low Byte
    Instruction(OP_LDI, ZH, (addr >> 8));            // Z-register High Byte
}

static void LoadZAddr(DWORD addr)
{
    LoadZAddr(addr, nullptr);
}

//See WinAVR\avr\include\avr\sfr_defs.h
#define __SFR_OFFSET 0x20

//-----------------------------------------------------------------------------
//AVR001 appnote macro
/*
;*********************************************************
;*  BIT access anywhere in IO or lower $FF of data space
;*  SETB - SET Bit in IO of data space
;*********************************************************
*/
static void SETB(DWORD addr, int bit, int reg, const char *name)
{
    if(bit > 7) {
        THROW_COMPILER_EXCEPTION(_("Only values 0-7 allowed for Bit parameter"));
    }
    if((addr - __SFR_OFFSET > 0x3F) || (USE_IO_REGISTERS == 0)) {
#ifdef USE_LDS_STS
        Instruction(OP_LDS, reg, addr, name);
#else
        LoadZAddr(addr);
        Instruction(OP_LD_Z, reg, 0, name);
#endif
        Instruction(OP_SBR, reg, 1 << bit);
#ifdef USE_LDS_STS
        Instruction(OP_STS, addr, reg);
#else
        Instruction(OP_ST_Z, reg);
#endif
    } else if((addr - __SFR_OFFSET > 0x1F) && (USE_IO_REGISTERS == 1)) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_IN, reg, addr - __SFR_OFFSET, name);
        Instruction(OP_SBR, reg, 1 << bit);
        Instruction(OP_OUT, addr - __SFR_OFFSET, reg);
#endif
    } else if(USE_IO_REGISTERS == 1) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_SBI, addr - __SFR_OFFSET, bit, name);
#endif
    } else
        THROW_COMPILER_EXCEPTION("Internal error");
}

static void SETB(DWORD addr, int bit, const char *name)
{
    SETB(addr, bit, r25, name);
}

static void SETB(DWORD addr, int bit, const NameArray &name)
{
    SETB(addr, bit, r25, name.c_str());
}

static void SETB(DWORD addr, int bit)
{
    SETB(addr, bit, r25, nullptr);
}

/*
;*********************************************************
;*  BIT access anywhere in IO or lower $FF of data space
;*  CLRB - CLeaR Bit in IO of data space
;*********************************************************
*/
static void CLRB(DWORD addr, int bit, int reg, const char *name)
{
    if(bit > 7) {
        THROW_COMPILER_EXCEPTION(_("Only values 0-7 allowed for Bit parameter"));
    }
    if((addr - __SFR_OFFSET > 0x3F) || (USE_IO_REGISTERS == 0)) {
#ifdef USE_LDS_STS
        Instruction(OP_LDS, reg, addr, name);
#else
        LoadZAddr(addr);
        Instruction(OP_LD_Z, reg, 0, name);
#endif
        Instruction(OP_CBR, reg, 1 << bit);
#ifdef USE_LDS_STS
        Instruction(OP_STS, addr, reg);
#else
        Instruction(OP_ST_Z, reg);
#endif
    } else if((addr - __SFR_OFFSET > 0x1F) && (USE_IO_REGISTERS == 1)) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_IN, reg, addr - __SFR_OFFSET, name);
        Instruction(OP_CBR, reg, 1 << bit);
        Instruction(OP_OUT, addr - __SFR_OFFSET, reg);
#endif
    } else if(USE_IO_REGISTERS == 1) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_CBI, addr - __SFR_OFFSET, bit, name);
#endif
    } else
        THROW_COMPILER_EXCEPTION("Internal error");
}

static void CLRB(DWORD addr, int bit, const char *name)
{
    CLRB(addr, bit, r25, name);
}

static void CLRB(DWORD addr, int bit, const NameArray &name)
{
    CLRB(addr, bit, r25, name.c_str());
}

static void CLRB(DWORD addr, int bit)
{
    CLRB(addr, bit, r25, nullptr);
}
/*
;*********************************************************
;*  Bit test anywhere in IO or in lower $FF of data space
;*  SKBS : SKip if Bit Set
;*  RETURN: Used Instruction Word
;*********************************************************
*/
static DWORD SKBS(DWORD addr, int bit, int reg)
{
    DWORD i = AvrProgWriteP;
    if(bit > 7) {
        THROW_COMPILER_EXCEPTION(_("Only values 0-7 allowed for Bit parameter"));
    }
    if((addr - __SFR_OFFSET > 0x3F) || (USE_IO_REGISTERS == 0)) {
#ifdef USE_LDS_STS
        Instruction(OP_LDS, reg, addr);
#else
        LoadZAddr(addr);
        Instruction(OP_LD_Z, reg);
#endif
        Instruction(OP_SBRS, reg, bit);
    } else if((addr - __SFR_OFFSET > 0x1F) && (USE_IO_REGISTERS == 1)) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_IN, reg, addr - __SFR_OFFSET);
        Instruction(OP_SBRS, reg, bit);
#endif
    } else if(USE_IO_REGISTERS == 1) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_SBIS, addr - __SFR_OFFSET, bit);
#endif
    } else
        THROW_COMPILER_EXCEPTION("Internal error");
    return AvrProgWriteP - i;
}

static DWORD SKBS(DWORD addr, int bit)
{
    return SKBS(addr, bit, r25);
}

/*
;*********************************************************
;*  Bit test anywhere in IO or in lower $FF of data space
;*  SKBC : SKip if Bit Cleared
;*********************************************************
*/
static DWORD SKBC(DWORD addr, int bit, int reg)
{
    DWORD i = AvrProgWriteP;
    if(bit > 7) {
        THROW_COMPILER_EXCEPTION(_("Only values 0-7 allowed for Bit parameter"));
    }
    if((addr - __SFR_OFFSET > 0x3F) || (USE_IO_REGISTERS == 0)) {
#ifdef USE_LDS_STS
        Instruction(OP_LDS, reg, addr);
#else
        LoadZAddr(addr);
        Instruction(OP_LD_Z, reg);
#endif
        Instruction(OP_SBRC, reg, bit);
    } else if((addr - __SFR_OFFSET > 0x1F) && (USE_IO_REGISTERS == 1)) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_IN, reg, addr - __SFR_OFFSET);
        Instruction(OP_SBRC, reg, bit);
#endif
    } else if(USE_IO_REGISTERS == 1) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_SBIC, addr - __SFR_OFFSET, bit);
#endif
    } else
        THROW_COMPILER_EXCEPTION("Internal error");

    return AvrProgWriteP - i;
}

static DWORD SKBC(DWORD addr, int bit)
{
    return SKBC(addr, bit, r25);
}

/*
;*********************************************************
;*  Byte access anywhere in IO or lower $FF of data space
;*  STORE - Store register in IO or data space
;*  LOAD  - Load register from IO or data space
;*********************************************************
*/
static void STORE(DWORD addr, int reg)
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Zero memory addres not allowed!\nSTORE(%d, %d) skiped!"), addr, reg);
        return;
    }
    if(reg < 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Registers less zero not allowed!\nSTORE(%d, %d) skiped."), addr, reg);
        return;
    }
    if((addr - __SFR_OFFSET > 0x3F) || (USE_IO_REGISTERS == 0)) {
#ifdef USE_LDS_STS
        Instruction(OP_STS, addr, reg);
#else
        LoadZAddr(addr);
        Instruction(OP_ST_Z, reg);
#endif
    } else if(USE_IO_REGISTERS == 1) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_OUT, addr - __SFR_OFFSET, reg);
#endif
    } else
        THROW_COMPILER_EXCEPTION("Internal error");
}

static void STOREval(DWORD addr, BYTE val)
//used ZL, r25; Opcodes: 2 or 4
{
    // load r25 with the data
    Instruction(OP_LDI, r25, val);
    // do the store
    STORE(addr, r25); // not a OP_ST_ZP !
}

static void LOAD(int reg, DWORD addr)
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Zero memory addres not allowed!\nLOAD(%d, %d) skiped!"), reg, addr);
        return;
    }
    if(reg < 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Registers less zero not allowed!\nLOAD(%d, %d) skiped."), reg, addr);
        return;
    }
    if((addr - __SFR_OFFSET > 0x3F) || (USE_IO_REGISTERS == 0)) {
#ifdef USE_LDS_STS
        Instruction(OP_LDS, reg, addr);
#else
        LoadZAddr(addr);
        Instruction(OP_LD_Z, reg);
#endif
    } else if(USE_IO_REGISTERS == 1) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_IN, reg, addr - __SFR_OFFSET);
#endif
    } else
        THROW_COMPILER_EXCEPTION("Internal error");
}
//AVR001 appnote macro

//-----------------------------------------------------------------------------
// Generate code to write/read an 8-bit value to a particular register.
//-----------------------------------------------------------------------------
#define WriteMemory(...) _WriteMemory(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)

static void _WriteMemory(int l, const char *f, const char *args, DWORD addr, BYTE val, const char *name, SDWORD literal)
//used ZL, r25; Opcodes: 4
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Zero memory addres not allowed!\nWriteMemory(0, %d) skiped! %s %s"), val, name, literal); //see TODO
        return;
    }
    char s[1024];
    sprintf(s, "(%s)", args);
    LoadZAddr(addr, s);
    char lit[1024];
    sprintf(lit, "val=%d lit=0x%X=%d", val, literal, literal);
    // load r25 with the data
    Instruction(OP_LDI, r25, val, lit);
    // do the store
    Instruction(OP_ST_ZP, r25, 0, name); // only OP_.._ZP, need for WriteMemoryNextAddr
}

static void _WriteMemory(int l, const char *f, const char *args, DWORD addr, BYTE val, const char *name)
{
    _WriteMemory(l, f, args, addr, val, name, 0);
}

static void _WriteMemory(int l, const char *f, const char *args, DWORD addr, BYTE val, SDWORD literal)
{
    _WriteMemory(l, f, args, addr, val, nullptr, literal);
}

static void _WriteMemory(int l, const char *f, const char *args, DWORD addr, BYTE val)
{
    _WriteMemory(l, f, args, addr, val, nullptr, 0);
}
//-----------------------------------------------------------------------------
// Use only after WriteMemory() !!!
//-----------------------------------------------------------------------------
static void WriteMemoryNextAddr(BYTE val)
//used ZL, r25; Opcodes: 2
{
    // Z was setted in WriteMemory()
    // load r25 with the data
    Instruction(OP_LDI, r25, val);
    // do the store
    Instruction(OP_ST_ZP, r25); // only OP_.._ZP
}

//-----------------------------------------------------------------------------
static void WriteMemoryStillAddr(DWORD addr, BYTE val)
//used ZL, r25; Opcodes: 4
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Zero memory addres not allowed!\nWriteMemoryStillAddr(0, %d) skiped!"), val); //see TODO
        return;
    }
    LoadZAddr(addr);
    // load r25 with the data
    Instruction(OP_LDI, r25, val);
    // do the store
    Instruction(OP_ST_Z, r25); // only OP_ST_Z, not a OP_ST_ZP !
}

//-----------------------------------------------------------------------------
static void WriteMemoryCurrAddr(BYTE val)
//used ZL, r25; Opcodes: 2
{
    // Z was setted in WriteMemory() or WriteMemoryStillAddr()
    // load r25 with the data
    Instruction(OP_LDI, r25, val);
    // do the store
    Instruction(OP_ST_Z, r25); // only OP_ST_Z, not a OP_ST_ZP !
}

//-----------------------------------------------------------------------------
static void LoadXAddrFromReg(int reg, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 2)
        THROW_COMPILER_EXCEPTION("Internal error");
    Instruction(OP_MOV, XL, reg);         // X-register Low Byte
    if(sov > 1)                           //
        Instruction(OP_MOV, XH, reg + 1); // X-register High Byte
    else                                  //
        Instruction(OP_LDI, XH, 0);       // X-register High Byte
}

//-----------------------------------------------------------------------------
static void LoadYAddrFromReg(int reg, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 2)
        THROW_COMPILER_EXCEPTION("Internal error");
    Instruction(OP_MOV, YL, reg);         // Y-register Low Byte
    if(sov > 1)                           //
        Instruction(OP_MOV, YH, reg + 1); // Y-register High Byte
    else                                  //
        Instruction(OP_LDI, YH, 0);       // Y-register High Byte
}

//-----------------------------------------------------------------------------
static void LoadZAddrFromReg(int reg, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 2)
        THROW_COMPILER_EXCEPTION("Internal error");
    Instruction(OP_MOV, ZL, reg);         // Z-register Low Byte
    if(sov > 1)                           //
        Instruction(OP_MOV, ZH, reg + 1); // Z-register High Byte
    else                                  //
        Instruction(OP_LDI, ZH, 0);       // Z-register High Byte
}

//-----------------------------------------------------------------------------
static void LdToReg(AvrOp op, int sov, int reg, int sovReg, bool signPropagation)
// used r25
// Address is preloaded to X,Y or Z.
{
    if(sovReg < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sovReg > 4)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error");

    if((op != OP_LD_XP) && (op != OP_LD_YP) && (op != OP_LD_ZP))
        THROW_COMPILER_EXCEPTION("Internal error");

    for(int i = 0; i < sovReg; i++) {
        if(i < sov)
            Instruction(op, reg + i);
        else {
            //Instruction(OP_CLR, reg + i); // BLINK
            Instruction(OP_LDI, r25, 0x00); // NO BLINK
            if(signPropagation) {
                Instruction(OP_SBRC, reg + sov - 1, BIT7); // Sign propagation
                Instruction(OP_LDI, r25, 0xFF);            // NO BLINK
                //Instruction(OP_COM, reg + i); // BLINK
            }
            Instruction(OP_MOV, reg + i, r25); // NO BLINK
        }
    }
}
//-----------------------------------------------------------------------------
static void WriteLiteralToMemory(DWORD addr, int sov, SDWORD literal, const char *name)
// used r4, r5, r25
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION(name);
    if(sov > 4)
        THROW_COMPILER_EXCEPTION(name);

    LoadZAddr(addr, name); // load direct addres
    if(name && IsAddrInVar(name)) {
        int sovA = SizeOfVar(&name[1]);
        LdToReg(OP_LD_ZP, sovA, r4, 2, false); // as address
        LoadZAddrFromReg(r4, 2);               // reload indirect addres
    }
    DWORD lNow, lPrev;
    lNow = literal & 0xff;
    lPrev = ~lNow;
    for(int i = 0; i < sov; i++) {
        lNow = (literal >> (8 * i)) & 0xff;
        if(lNow != lPrev)
            Instruction(OP_LDI, r25, lNow);
        Instruction(OP_ST_ZP, r25);
        lPrev = lNow;
    }
    /*
    DWORD l1, l2;
    l1 = (literal & 0xff);
    Instruction(OP_LDI, r25, BYTE(l1), name);
    Instruction(OP_ST_ZP, r25, 0, name);

    if(sov >= 2) {
        l2 = ((literal >> 8) & 0xff);
        if(l1 != l2)
            Instruction(OP_LDI, r25, l2);
        Instruction(OP_ST_ZP, r25);

        if(sov >= 3) {
            l1 = ((literal >> 16) & 0xff);
            if(l1 != l2)
                Instruction(OP_LDI, r25, l1);
            Instruction(OP_ST_ZP, r25);

            if(sov >= 4) {
                l2 = ((literal >> 24) & 0xff);
                if(l1 != l2)
                    Instruction(OP_LDI, r25, l2);
                Instruction(OP_ST_ZP, r25);
            }
        }
    }
    */
}

static void WriteLiteralToMemory(DWORD addr, int sov, SDWORD literal, const NameArray &name)
{
    WriteLiteralToMemory(addr, sov, literal, name.c_str());
}

//-----------------------------------------------------------------------------
static void OrMemory(DWORD addr, BYTE val, char *name1, char *literal)
//used ZL, r25; Opcodes: 4
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Zero memory addres not allowed!\nOrMemory(0, %d) skiped!"), val); //see TODO
        return;
    }
    LoadZAddr(addr);
    Instruction(OP_LD_Z, r25);
    Instruction(OP_ORI, r25, val, literal);
    // do the store
    Instruction(OP_ST_Z, r25, 0, name1); // not a OP_ST_ZP !
}

static void OrMemory(DWORD addr, BYTE val)
{
    OrMemory(addr, val, nullptr, nullptr);
}
//-----------------------------------------------------------------------------
static void AndMemory(DWORD addr, BYTE val, char *name1, char *literal)
//used ZL, r25; Opcodes: 4
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION_FMT(_("Zero memory addres not allowed!\nAndMemory(0, %d) skiped!"), val); //see TODO
        return;
    }
    LoadZAddr(addr);
    Instruction(OP_LD_Z, r25);
    Instruction(OP_ANDI, r25, val, literal);
    // do the store
    Instruction(OP_ST_Z, r25, 0, name1); // not a OP_ST_ZP !
}

static void AndMemory(DWORD addr, BYTE val)
{
    AndMemory(addr, val, nullptr, nullptr);
}
//-----------------------------------------------------------------------------
static void WriteRegToIO(DWORD addr, BYTE reg)
//   used ZL; Opcodes: 3
//or used   ; Opcodes: 1
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION(_("Zero memory addres not allowed!\nWriteRegToIO skiped.")); //see TODO
        return;
    }
    if(reg < 0) {
        THROW_COMPILER_EXCEPTION(_("Registers less zero not allowed!\nWriteRegToIO skiped.")); //see TODO
        return;
    }
    LoadZAddr(addr);
    Instruction(OP_ST_Z, reg);
}
//-----------------------------------------------------------------------------
static void ReadIoToReg(BYTE reg, DWORD addr)
//   used ZL; Opcodes: 3
//or used   ; Opcodes: 1
{
    if(addr <= 0) {
        THROW_COMPILER_EXCEPTION(_("Zero memory addres not allowed!\nReadIoToReg skiped.")); //see TODO
        return;
    }
    if(reg < 0) {
        THROW_COMPILER_EXCEPTION(_("Registers less zero not allowed!\nReadIoToReg skiped.")); //see TODO
        return;
    }
    LoadZAddr(addr);
    Instruction(OP_LD_Z, reg);
}
//-----------------------------------------------------------------------------
// Copy just one bit from one place to another.
//-----------------------------------------------------------------------------
static void CopyBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc, const char *name1, const char *name2)
{
    if((addrDest == addrSrc) && (bitDest == bitSrc))
        THROW_COMPILER_EXCEPTION("Internal error");
    char s[10];
    char d[10];
    sprintf(s, "BIT%d", bitSrc);
    sprintf(d, "BIT%d", bitDest);
    /*
//used ZL, r25, r3; Opcodes: 11
    LoadZAddr(addrSrc);
    Instruction(OP_LD_Z, r3, 0, name2);
    if(addrDest != addrSrc)
      LoadZAddr(addrDest);
    Instruction(OP_LD_Z, r25, 0, name1);
    Instruction(OP_SBRS, r3, bitSrc, s);
    Instruction(OP_CBR, r25, 1 << bitDest, d);
    Instruction(OP_SBRC, r3, bitSrc, s);
    Instruction(OP_SBR, r25, 1 << bitDest, d);
    Instruction(OP_ST_Z, r25, 0, name1);
/**/
    //used ZL, r25; Opcodes: 9
    LoadZAddr(addrSrc);
    Instruction(OP_LD_Z, r25, 0, name2);
    Instruction(OP_BST, r25, bitSrc, s);
    if(addrDest != addrSrc) {
        LoadZAddr(addrDest);
        Instruction(OP_LD_Z, r25);
    }
    Instruction(OP_BLD, r25, bitDest, d);
    Instruction(OP_ST_Z, r25, 0, name1);
    /**/
}

static void CopyBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc)
{
    CopyBit(addrDest, bitDest, addrSrc, bitSrc, "", "");
}

//-----------------------------------------------------------------------------
static void CopyNotBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc, const char *name1, const char *name2)
{
    if((addrDest == addrSrc) && (bitDest == bitSrc))
        THROW_COMPILER_EXCEPTION("Internal error");
    char s[10];
    char d[10];
    sprintf(s, "BIT%d", bitSrc);
    sprintf(d, "BIT%d", bitDest);
    //used ZL, r25, r3; Opcodes: 11
    LoadZAddr(addrSrc);
    Instruction(OP_LD_Z, r3, 0, name2);
    if(addrDest != addrSrc)
        LoadZAddr(addrDest);
    Instruction(OP_LD_Z, r25, 0, name1);
    Instruction(OP_SBRS, r3, bitSrc, s);
    Instruction(OP_SBR, r25, 1 << bitDest, d);
    Instruction(OP_SBRC, r3, bitSrc, s);
    Instruction(OP_CBR, r25, 1 << bitDest, d);
    Instruction(OP_ST_Z, r25, 0, name1);
}

static void CopyNotBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc)
{
    CopyNotBit(addrDest, bitDest, addrSrc, bitSrc, "", "");
}

//-----------------------------------------------------------------------------
static void GetUartSendReady(DWORD addr, int bit)
{
    /*
    ClearBit(addr, bit); // UART ready
    DWORD dontSet = AllocFwdAddr();
    IfBitSet(REG_UCSRA, UDRE); // UDRE, is 1 when tx buffer is empty
    Instruction(OP_RJMP, dontSet);
    SetBit(addr, bit); // Set UART busy
    FwdAddrIsNow(dontSet);
    */
    //  CopyBit(addr, bit, REG_UCSRA, TXC); // TXC, is 1 when hift buffer is empty
    CopyBit(addr, bit, REG_UCSRA, UDRE); // UDRE, is 1 when tx buffer is empty and ready
}
//-----------------------------------------------------------------------------
static void GetUartSendBusy(DWORD addr, int bit)
{
    //  CopyNotBit(addr, bit, REG_UCSRA, TXC); // TXC, is 1 when hift buffer is empty
    CopyNotBit(addr, bit, REG_UCSRA, UDRE); // UDRE, is 1 when tx buffer is empty and ready
}
//-----------------------------------------------------------------------------
// Execute the next instruction only if the specified bit of the specified
// memory location is clear (i.e. skip if set).
//-----------------------------------------------------------------------------
static void IfBitClear(DWORD addr, int bit, BYTE reg, const char *name)
//used ZL, r25 // bit in [0..7]
{
    char b[10];
    sprintf(b, "BIT%d", bit);
    if(bit > 7) {
        THROW_COMPILER_EXCEPTION(_("Only values 0-7 allowed for Bit parameter"));
    }
    if((addr - __SFR_OFFSET > 0x3F) || (USE_IO_REGISTERS == 0)) {
        LoadZAddr(addr);
        Instruction(OP_LD_Z, reg, 0, name);
        Instruction(OP_SBRS, reg, bit, b);
    } else if((addr - __SFR_OFFSET > 0x1F) && (USE_IO_REGISTERS == 1)) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_IN, reg, addr - __SFR_OFFSET, name);
        Instruction(OP_SBRS, reg, bit, b);
#endif
    } else if(USE_IO_REGISTERS == 1) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_SBIS, addr - __SFR_OFFSET, bit, name);
#endif
    } else
        THROW_COMPILER_EXCEPTION("Internal error");
}

static void IfBitClear(DWORD addr, int bit, const char *name)
{
    IfBitClear(addr, bit, r25, name);
}

static void IfBitClear(DWORD addr, int bit, const NameArray &name)
{
    IfBitClear(addr, bit, r25, name.c_str());
}

static void IfBitClear(DWORD addr, int bit)
{
    IfBitClear(addr, bit, r25, nullptr);
}

//-----------------------------------------------------------------------------
// Execute the next instruction only if the specified bit of the specified
// memory location is set (i.e. skip if clear).
//-----------------------------------------------------------------------------
static void IfBitSet(DWORD addr, int bit, BYTE reg, const char *name)
//used ZL, r25 // bit in [0..7]
{
    char b[10];
    sprintf(b, "BIT%d", bit);
    if(bit > 7) {
        THROW_COMPILER_EXCEPTION(_("Only values 0-7 allowed for Bit parameter"));
    }
    if((addr - __SFR_OFFSET > 0x3F) || (USE_IO_REGISTERS == 0)) {
        LoadZAddr(addr);
        Instruction(OP_LD_Z, r25, 0, name);
        Instruction(OP_SBRC, r25, bit, b);
    } else if((addr - __SFR_OFFSET > 0x1F) && (USE_IO_REGISTERS == 1)) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_IN, reg, addr - __SFR_OFFSET, name);
        Instruction(OP_SBRC, reg, bit, b);
#endif
    } else if(USE_IO_REGISTERS == 1) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_SBIC, addr - __SFR_OFFSET, bit, name);
#endif
    } else
        THROW_COMPILER_EXCEPTION("Internal error");
}

static void IfBitSet(DWORD addr, int bit, const char *name)
{
    IfBitSet(addr, bit, r25, name);
}

static void IfBitSet(DWORD addr, int bit, const NameArray &name)
{
    IfBitSet(addr, bit, r25, name.c_str());
}

static void IfBitSet(DWORD addr, int bit)
{
    IfBitSet(addr, bit, r25, nullptr);
}

//-----------------------------------------------------------------------------
// Set a given bit in an arbitrary (not necessarily I/O memory) location in
// memory.
//-----------------------------------------------------------------------------
/*
static void SetBit(DWORD addr, int bit, char *name)
//used ZL, r25 // Opcodes: 5
{

//  LoadZAddr(addr);
//  Instruction(OP_LD_Z, r25);
//  Instruction(OP_SBR, r25, 1 << bit);
//  Instruction(OP_ST_Z, r25);

    SETB(addr, bit, name);
}
static void SetBit(DWORD addr, int bit)
{
    SetBit(addr, bit, nullptr)
}
*/
#define SetBit(...) SETB(__VA_ARGS__)

//-----------------------------------------------------------------------------
// Clear a given bit in an arbitrary (not necessarily I/O memory) location in
// memory.
//-----------------------------------------------------------------------------
/*
static void ClearBit(DWORD addr, int bit)
//used ZL, r25; Opcodes: 5
{

//  LoadZAddr(addr);
//  Instruction(OP_LD_Z, r25);
//  Instruction(OP_CBR, r25, 1 << bit);
//  Instruction(OP_ST_Z, r25);

    CLRB(addr, bit);
}
*/
#define ClearBit(...) CLRB(__VA_ARGS__)

//-----------------------------------------------------------------------------
static bool TstAddrBitReg(DWORD addr, int bit, int reg)
{
    bool b = true;
    if((addr <= 0) || (addr > 0xFFFF)) {
        THROW_COMPILER_EXCEPTION_FMT(_("Only values 0-0xFFFF allowed for Addres parameter.\naddres=0x%4X"), addr);
        b = false;
    }
    if((bit < 0) || (bit > 7)) {
        THROW_COMPILER_EXCEPTION_FMT(_("Only values 0-7 allowed for Bit parameter.\nbit=%d"), bit);
        b = false;
    }
    if((reg < 0) || (reg > 0x1F)) {
        THROW_COMPILER_EXCEPTION_FMT(_("Only values 0-0x1F allowed for Register parameter.\nreg=0x%02X"), reg);
        b = false;
    }
    return b;
}
//-----------------------------------------------------------------------------
static void nops()
{
    Instruction(OP_NOP); // Ok
    Instruction(OP_NOP); // Ok
    //  Instruction(OP_NOP); // uncomment this string to increase pulse duration
    //  Instruction(OP_NOP); // uncomment this string to increase pulse duration
    //  Instruction(OP_NOP); // uncomment this string to increase pulse duration
}

static void PulseBit(DWORD addr, int bit, int reg)
//used ZL, r25 // Opcodes: 7
{
    if((addr - __SFR_OFFSET > 0x3F) || (USE_IO_REGISTERS == 0)) {
        LoadZAddr(addr);
        Instruction(OP_LD_Z, r25);
        Instruction(OP_SBR, r25, 1 << bit);
        Instruction(OP_ST_Z, r25);
        nops();
        Instruction(OP_LD_Z, r25); // its more correct wiht volatile vars
        Instruction(OP_CBR, r25, 1 << bit);
        Instruction(OP_ST_Z, r25);
    } else if((addr - __SFR_OFFSET > 0x1F) && (USE_IO_REGISTERS == 1)) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_IN, reg, addr - __SFR_OFFSET);
        Instruction(OP_SBR, reg, 1 << bit);
        Instruction(OP_OUT, addr - __SFR_OFFSET, reg);
        nops();
        Instruction(OP_IN, reg, addr - __SFR_OFFSET); // its more correct wiht volatile vars
        Instruction(OP_CBR, reg, 1 << bit);
        Instruction(OP_OUT, addr - __SFR_OFFSET, reg);
#endif
    } else if(USE_IO_REGISTERS == 1) {
#if USE_IO_REGISTERS == 1
        Instruction(OP_SBI, addr - __SFR_OFFSET, bit); // its full correct wiht volatile vars
        nops();
        Instruction(OP_CBI, addr - __SFR_OFFSET, bit);
#endif
    } else
        THROW_COMPILER_EXCEPTION("Internal error");
}

static void PulseBit(DWORD addr, int bit)
{
    PulseBit(addr, bit, r25);
}

//-----------------------------------------------------------------------------
// Calc AVR 8-bit Timer0
// or   AVR 16-bit Timer1 to do the timing for NPulse generator.
static bool CalcAvrTimerNPulse(double target, int *bestPrescaler, BYTE *cs, int *bestDivider, int *bestError,
                               double *bestTarget)
{
    int max_tmr;
    if(Prog.cycleTimer == 0)
        max_tmr = 0x10000; // used Timer1 for NPulse
    else
        max_tmr = 0x100; // used Timer0 for NPulse
    // frequency (HZ) is
    // target = mcuClock / (divider * prescaler)
    // divider = mcuClock / (target * prescaler)
    //dbp("target=%.3f", target);
    *bestDivider = -INT_MAX;
    *bestError = INT_MAX;
    double freq;
    int    err;
    int    divider;
    int    prescaler;
    for(prescaler = 1;;) {
        divider = int(round((double)Prog.mcuClock / (target * prescaler)));
        freq = (double)Prog.mcuClock / (prescaler * divider);

        //dbp("prescaler=%d divider=%d freq=%f Hz",prescaler, divider, freq);

        err = (int)fabs(freq - target);
        if((err <= *bestError) && (*bestDivider < divider)) {
            if(divider <= max_tmr) {
                *bestError = err;
                *bestPrescaler = prescaler;
                *bestDivider = divider;
                *bestTarget = freq;
            }
        }
        if(prescaler == 1)
            prescaler = 8;
        else if(prescaler == 8)
            prescaler = 64;
        else if(prescaler == 64)
            prescaler = 256;
        else if(prescaler == 256)
            prescaler = 1024;
        else
            break;
    }
    switch(*bestPrescaler) {
        case 1:
            *cs = 1;
            break;
        case 8:
            *cs = 2;
            break;
        case 64:
            *cs = 3;
            break;
        case 256:
            *cs = 4;
            break;
        case 1024:
            *cs = 5;
            break;
        default:
            THROW_COMPILER_EXCEPTION("Internal error");
            break;
    }

    //dbp("bestPrescaler=%d bestDivider=%d bestTarget=%d Hz", *bestPrescaler, *bestDivider, *bestTarget);

    return true;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Calc AVR 16-bit Timer1 or 8-bit Timer0  to do the timing of PLC cycle.
bool CalcAvrPlcCycle(long long int cycleTimeMicroseconds, DWORD AvrProgLdLen)
{
    //memset(plcTmr, 0, sizeof(plcTmr));
    plcTmr.softDivisor = 1;
    plcTmr.prescaler = 1;
    plcTmr.cs = 0;
    plcTmr.cycleTimeMin = (int)floor(1e6 * PLC_CLOCK_MIN * 1 / Prog.mcuClock + 0.5);
    //                                     ^min_divider    ^min_prescaler
    long int max_tmr, max_prescaler, max_softDivisor;
    if(Prog.cycleTimer == 0) {
        max_tmr = 0xFF;         //+1;
        max_prescaler = 1024;   // 1,8,64,256,1024
        max_softDivisor = 0xFF; // 1..0xFFFF
    } else {
        max_tmr = 0xFFFF;       //+1;
        max_prescaler = 1024;   // 1,8,64,256,1024
        max_softDivisor = 0xFF; // 1..0xFF
    }
    plcTmr.cycleTimeMax = (long long int)floor(1e6 * max_tmr * max_prescaler * max_softDivisor / Prog.mcuClock + 0.5);
    if(cycleTimeMicroseconds <= 0)
        return false;
    plcTmr.ticksPerCycle = (long long int)floor(1.0 * Prog.mcuClock * cycleTimeMicroseconds / 1000000 + 0.5);
    long int      bestTmr = LONG_MIN;
    long int      bestPrescaler = LONG_MAX;
    long int      bestSoftDivisor;
    long long int bestErr = LLONG_MAX;
    long long int err;
    while(plcTmr.softDivisor <= max_softDivisor) {
        plcTmr.prescaler = max_prescaler;
        while(plcTmr.prescaler >= 1) {
            for(plcTmr.tmr = 1; plcTmr.tmr <= max_tmr; plcTmr.tmr++) {
                err = plcTmr.ticksPerCycle - (long long int)plcTmr.tmr * plcTmr.prescaler * plcTmr.softDivisor;
                if(err < 0)
                    err = -err;

                if((bestErr > err) || ((bestErr == err) && (bestPrescaler < plcTmr.prescaler))) {
                    bestErr = err;
                    bestSoftDivisor = plcTmr.softDivisor;
                    bestPrescaler = plcTmr.prescaler;
                    bestTmr = plcTmr.tmr;
                    if(err == 0)
                        goto err0;
                }
            }
            if(plcTmr.prescaler == 1)
                break;
            else if(plcTmr.prescaler == 8)
                plcTmr.prescaler = 1;
            else if(plcTmr.prescaler == 64)
                plcTmr.prescaler = 8;
            else if(plcTmr.prescaler == 256)
                plcTmr.prescaler = 64;
            else if(plcTmr.prescaler == 1024)
                plcTmr.prescaler = 256;
            else
                THROW_COMPILER_EXCEPTION("Internal error");
        }
        if(plcTmr.softDivisor == max_softDivisor)
            break;
        plcTmr.softDivisor++;
    }
err0:
    plcTmr.softDivisor = bestSoftDivisor;
    plcTmr.prescaler = bestPrescaler;
    plcTmr.tmr = bestTmr;

    plcTmr.Fcycle = 1.0 * Prog.mcuClock / (1.0 * plcTmr.softDivisor * plcTmr.prescaler * plcTmr.tmr);
    plcTmr.TCycle = 1.0 * plcTmr.prescaler * plcTmr.softDivisor * plcTmr.tmr / (1.0 * Prog.mcuClock);
    switch(plcTmr.prescaler) {
        case 1:
            plcTmr.cs = 1;
            break;
        case 8:
            plcTmr.cs = 2;
            break;
        case 64:
            plcTmr.cs = 3;
            break;
        case 256:
            plcTmr.cs = 4;
            break;
        case 1024:
            plcTmr.cs = 5;
            break;
        default:
            THROW_COMPILER_EXCEPTION_FMT("plcTmr.prescaler=%d", plcTmr.prescaler);
    }

    if(plcTmr.tmr > max_tmr) {
        THROW_COMPILER_EXCEPTION_FMT("PLC cycle time more then %lld ms not valid.", plcTmr.cycleTimeMax / 1000);
        return false;
    } else if((plcTmr.prescaler * plcTmr.tmr) < PLC_CLOCK_MIN) {
        THROW_COMPILER_EXCEPTION_FMT("PLC cycle time less then %d us not valid.", plcTmr.cycleTimeMin);
        return false;
    }
    return true;
}
//-----------------------------------------------------------------------------
// Configure AVR 16-bit Timer1 or 8-bit Timer0  to do the timing of PLC cycle.
//static DWORD  PlcCycleTimerOverflowVector;
static long int tcnt0PlcCycle = 0;
static void     ConfigureTimerForPlcCycle(long long int cycleTimeMicroseconds)
{
    CalcAvrPlcCycle(cycleTimeMicroseconds, AvrProgLdLen);

    if(Prog.cycleTimer == 0) {
        if((WGM01 == -1)) {                       // ATmega8
            tcnt0PlcCycle = 256 - plcTmr.tmr + 0; // + 0 DONE 1000Hz
            if(tcnt0PlcCycle < 0)
                tcnt0PlcCycle = 0;
            if(tcnt0PlcCycle > 255)
                tcnt0PlcCycle = 255;

            Instruction(OP_LDI, r25, tcnt0PlcCycle);
            WriteRegToIO(REG_TCNT0, r25); // set divider

            WriteMemory(REG_TCCR0B, (BYTE)plcTmr.cs); // set prescaler
            SetBit(REG_TIFR0, TOV0);                  // Clear TOV0/ clear pending interrupts
            //To clean a bit in the register TIFR need write 1 in the corresponding bit!
        } else {
            WriteMemory(REG_TCCR0A, 1 << WGM01);       // WGM01=1, WGM00=0 // CTC mode
            WriteMemory(REG_TCCR0B, plcTmr.cs & 0xff); //  WGM02=0 // CTC mode

            int counter = plcTmr.tmr - 1; // -1 DONE 1000Hz
            // the counter is less than the divisor at 1
            if(counter < 0)
                counter = 0;
            if(counter > 0xff)
                counter = 0xff;

            WriteMemory(REG_OCR0A, counter & 0xff);
        }
    } else {                                                        // Timer1
        WriteMemory(REG_TCCR1A, 0x00);                              // WGM11=0, WGM10=0 // CTC mode
        WriteMemory(REG_TCCR1B, ((1 << WGM12) | plcTmr.cs) & 0xff); // WGM13=0, WGM12=1 // CTC mode

        long int counter = plcTmr.tmr - 1; // -1 DONE 1000Hz
        // the counter is less than the divisor at 1
        // ArduinoMega2560 has an error: 2000Hz instead 1000Hz !!!

        // the high byte must be written before the low byte
        WriteMemory(REG_OCR1AH, BYTE((counter >> 8) & 0xff));
        WriteMemory(REG_OCR1AL, BYTE(counter & 0xff));
        /*
        Bug .. no interupt for timer1 need..

        // Okay, so many AVRs have a register called TIMSK, but the meaning of
        // the bits in that register varies from device to device...
        if(strcmp(Prog.mcu->mcuName, "Atmel AVR AT90USB647 64-TQFP")==0) {
            WriteMemory(REG_TIMSK, (1 << 1));
        }
        else
        if(strcmp(Prog.mcu->mcuName, "Atmel AVR ATmega162 40-PDIP")==0) {
            WriteMemory(REG_TIMSK, (1 << 6));
        } else {
            WriteMemory(REG_TIMSK, (1 << 4));
        }
        */
    }
}

#ifdef TABLE_IN_FLASH
//-----------------------------------------------------------------------------
static void InitTable(IntOp *a)
{
    //  DWORD saveAvrProgWriteP = AvrProgWriteP;
    DWORD addrOfTable = 0;
    MemOfVar(a->name1, &addrOfTable);

    if(addrOfTable == 0) {
        Comment("TABLE %s", a->name1.c_str());
        if(AvrProgWriteP % 2)
            Instruction(OP_NOP);
        addrOfTable = AvrProgWriteP; // << 1; //see LPM // data stored in flash

        SetMemForVariable(a->name1, addrOfTable, a->literal);

        int sovElement = a->literal2;
        //sovElement = 1;
        int i;
        if(sovElement == 2) {
            for(i = 0; i < a->literal; i++) {
                //dbp("i=%d %d",i,a->data[i]);
                Instruction(OP_DW, a->data[i]);
            }
        } else if(sovElement == 1) {
            for(i = 0; i < a->literal; i = i + 2) {
                //dbp("i=%d %d %d", i, a->data[i], a->data[i+1]);
                Instruction(OP_DB2, a->data[i], i + 1 < a->literal ? a->data[i + 1] : 0);
            }
            /*
            for(i=0; i < a->literal; i++){
              //dbp("i=%d %d", i, a->data[i]);
              Instruction(OP_DB, a->data[i]); // BAD! Hi byte of flash word is 0!
            }
            */
        } else if(sovElement == 4) {
            for(i = 0; i < a->literal; i++) {
                Instruction(OP_DW, a->data[i]);
                Instruction(OP_DW, a->data[i] >> 16);
            }
        } else if(sovElement == 3) {
            for(i = 0; i < a->literal; i = i + 2) {
                Instruction(OP_DW, a->data[i]);
                Instruction(OP_DB2, a->data[i] >> 16, (i + 1 < a->literal ? a->data[i + 1] : 0) & 0xFF);
                Instruction(OP_DW, (i + 1 < a->literal ? a->data[i + 1] : 0) >> 8);
            }
        } else
            THROW_COMPILER_EXCEPTION("Internal error");
        Comment("TABLE %s END", a->name1.c_str());
    }

    //  if((saveAvrProgWriteP >> 11) != (AvrProgWriteP >> 11)) oops();
}

//-----------------------------------------------------------------------------
static void InitTables()
{
    for(IntPc = 0; IntPc < IntCode.size(); IntPc++) {
        IntPcNow = IntPc;
        IntOp *a = &IntCode[IntPc];
        rungNow = a->rung;
        switch(a->op) {
            case INT_FLASH_INIT:
                InitTable(a);
                break;
            default:
                break;
        }
    }
}
#endif

//-----------------------------------------------------------------------------
// Call a subroutine, using either an rcall or an icall depending on what
// the processor supports or requires.
//-----------------------------------------------------------------------------
static void CallSubroutine(DWORD addr)
//used ZL, r25
{
    if(addr & FWD(0)) {
        if(Prog.mcu->core >= EnhancedCore4M) {
            Instruction(OP_LDI, ZL, REG_EIND & 0xff);        // Z-register Low Byte
            Instruction(OP_LDI, ZH, (REG_EIND >> 8) & 0xff); // Z-register High Byte
            // load r25 with the data
            Instruction(OP_LDI, r25, FWD_EIND(addr));
            // do the store
            Instruction(OP_ST_Z, r25, 0); // 1

            Instruction(OP_LDI, ZL, FWD_LO(addr)); // 2
            Instruction(OP_LDI, ZH, FWD_HI(addr));
            Instruction(OP_EICALL, FWD(addr)); // arg1 used for label
        } else if(Prog.mcu->core >= ClassicCore8K) {
            Instruction(OP_LDI, ZL, FWD_LO(addr));
            Instruction(OP_LDI, ZH, FWD_HI(addr));
            Instruction(OP_ICALL, FWD(addr)); // arg1 used for label
        } else {
            Instruction(OP_RCALL, FWD(addr));
        }
    } else {
        if((-2048 <= (addr - AvrProgWriteP - 1)) && ((addr - AvrProgWriteP - 1) <= 2047)) {
            Instruction(OP_RCALL, addr);
        } else if((0 <= addr) && (addr <= 0xFFFF) && (Prog.mcu->core >= ClassicCore8K)) {
            Instruction(OP_LDI, ZL, addr & 0xff);
            Instruction(OP_LDI, ZH, (addr >> 8) & 0xff);
            Instruction(OP_ICALL, addr); // arg1 used for label
        } else if((0 <= addr) && (addr <= 0x3fFFFF) && (Prog.mcu->core >= EnhancedCore4M)) {
            WriteMemory(REG_EIND, (BYTE)(addr >> 16) & 0xff); // 1
            Instruction(OP_LDI, ZL, addr & 0xff);             // 2
            Instruction(OP_LDI, ZH, (addr >> 8) & 0xff);
            Instruction(OP_EICALL, addr); // arg1 used for label
        } else
            THROW_COMPILER_EXCEPTION("Internal error");
    }
}

//-----------------------------------------------------------------------------
static void InstructionJMP(DWORD addr)
//used ZL, r25
{
    if(addr & FWD(0)) {
        if(Prog.mcu->core >= EnhancedCore4M) {
            Instruction(OP_LDI, ZL, REG_EIND & 0xff);        // Z-register Low Byte
            Instruction(OP_LDI, ZH, (REG_EIND >> 8) & 0xff); // Z-register High Byte
            // load r25 with the data
            Instruction(OP_LDI, r25, FWD_EIND(addr));
            // do the store
            Instruction(OP_ST_Z, r25, 0); // 1

            Instruction(OP_LDI, ZL, FWD_LO(addr)); // 2
            Instruction(OP_LDI, ZH, FWD_HI(addr));
            Instruction(OP_EIJMP, FWD(addr)); // arg1 used for label
        } else if(Prog.mcu->core >= ClassicCore8K) {
            Instruction(OP_LDI, ZL, FWD_LO(addr));
            Instruction(OP_LDI, ZH, FWD_HI(addr));
            Instruction(OP_IJMP, FWD(addr)); // arg1 used for label
        } else {
            Instruction(OP_RJMP, FWD(addr));
        }
    } else {
        if((-2048 <= (addr - AvrProgWriteP - 1)) && ((addr - AvrProgWriteP - 1) <= 2047)) {
            Instruction(OP_RJMP, addr);
        } else if((0 <= addr) && (addr <= 0xFFFF) && (Prog.mcu->core >= ClassicCore8K)) {
            Instruction(OP_LDI, ZL, addr & 0xff);
            Instruction(OP_LDI, ZH, (addr >> 8) & 0xff);
            Instruction(OP_IJMP, addr); // arg1 used for label
        } else if((0 <= addr) && (addr <= 0x3fFFFF) && (Prog.mcu->core >= EnhancedCore4M)) {
            WriteMemory(REG_EIND, (BYTE)(addr >> 16) & 0xff); // 1
            Instruction(OP_LDI, ZL, addr & 0xff);             // 2
            Instruction(OP_LDI, ZH, (addr >> 8) & 0xff);
            Instruction(OP_EIJMP, addr); // arg1 used for label
        } else
            THROW_COMPILER_EXCEPTION("Internal error");
    }
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
    IntPcNow = IntPc;
    CompileFromIntermediate();
    if(IntCode[IntPc].op == INT_ELSE) {
        IntPc++;
        IntPcNow = IntPc;
        DWORD endBlock = AllocFwdAddr();
        InstructionJMP(endBlock);

        FwdAddrIsNow(condFalse);
        CompileFromIntermediate();
        FwdAddrIsNow(endBlock);
    } else {
        FwdAddrIsNow(condFalse);
    }

    if(IntCode[IntPc].op != INT_END_IF)
        THROW_COMPILER_EXCEPTION_FMT("Invalid intcode %d.", IntCode[IntPc].op);
}

//-----------------------------------------------------------------------------
static void ShlReg(int reg, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error");
    Instruction(OP_LSL, reg);
    if(sov >= 2)
        Instruction(OP_ROL, reg + 1);
    if(sov >= 3)
        Instruction(OP_ROL, reg + 2);
    if(sov >= 4)
        Instruction(OP_ROL, reg + 3);
}

static void RolReg(int reg, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error");
    Instruction(OP_CLC);
    Instruction(OP_LSL, reg);
    if(sov >= 2)
        Instruction(OP_ROL, reg + 1);
    if(sov >= 3)
        Instruction(OP_ROL, reg + 2);
    if(sov >= 4)
        Instruction(OP_ROL, reg + 3);
    IfBitSet(REG_SREG, SREG_C);
    Instruction(OP_SBR, reg, 1);
}

//-----------------------------------------------------------------------------
static void CpseReg(int op1, int sov, int op2, DWORD condFalse)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error");
    Instruction(OP_AND, op1, op2);
    Instruction(OP_CPSE, op1, op2);
    Instruction(OP_RJMP, condFalse);
    if(sov >= 2) {
        Instruction(OP_AND, op1 + 1, op2 + 1);
        Instruction(OP_CPSE, op1 + 1, op2 + 1);
        Instruction(OP_RJMP, condFalse);
    }
    if(sov >= 3) {
        Instruction(OP_AND, op1 + 2, op2 + 2);
        Instruction(OP_CPSE, op1 + 2, op2 + 2);
        Instruction(OP_RJMP, condFalse);
    }
    if(sov >= 4) {
        Instruction(OP_AND, op1 + 3, op2 + 3);
        Instruction(OP_CPSE, op1 + 3, op2 + 3);
        Instruction(OP_RJMP, condFalse);
    }
}
//-----------------------------------------------------------------------------
static void OrReg(int reg, int sov, int op2)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error");
    Instruction(OP_OR, reg, op2);
    if(sov >= 2)
        Instruction(OP_OR, reg + 1, op2 + 1);
    if(sov >= 3)
        Instruction(OP_OR, reg + 2, op2 + 2);
    if(sov >= 4)
        Instruction(OP_OR, reg + 3, op2 + 3);
}

//-----------------------------------------------------------------------------
static void AndReg(int reg, int sov, int op2)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error");
    Instruction(OP_AND, reg, op2);
    if(sov >= 2)
        Instruction(OP_AND, reg + 1, op2 + 1);
    if(sov >= 3)
        Instruction(OP_AND, reg + 2, op2 + 2);
    if(sov >= 4)
        Instruction(OP_AND, reg + 3, op2 + 3);
}

//-----------------------------------------------------------------------------
static void CopyLitToReg(int reg, int sov, SDWORD literal, const char *comment)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov >= 1)
        Instruction(OP_LDI, reg, literal & 0xff, comment);
    if(sov >= 2)
        Instruction(OP_LDI, reg + 1, (literal >> 8) & 0xff);
    if(sov >= 3)
        Instruction(OP_LDI, reg + 2, (literal >> 16) & 0xff);
    if(sov >= 4)
        Instruction(OP_LDI, reg + 3, (literal >> 24) & 0xff);
}

static void CopyLitToReg(int reg, int sov, SDWORD literal)
{
    CopyLitToReg(reg, sov, literal, nullptr);
}

//-----------------------------------------------------------------------------
static void CopyVarToReg(int reg, int sovReg, const char *var)
{
    DWORD addr;
    int   sov = SizeOfVar(var);
    if(sov != sovReg)
        dbp("reg=%d sovReg=%d <- var=%s sov=%d", reg, sovReg, var, sov);

    MemForVariable(var, &addr);
    LoadXAddr(addr, var); // load direct addres

    if(IsAddrInVar(var)) {
        int sovA = SizeOfVar(&var[1]);
        LdToReg(OP_LD_XP, sovA, r4, 2, false); // as address
        LoadXAddrFromReg(r4, 2);               // reload indirect addres
    }
    LdToReg(OP_LD_XP, sov, reg, sovReg, true); // as data
}

static void CopyVarToReg(int reg, int sovReg, const NameArray &var)
{
    CopyVarToReg(reg, sovReg, var.c_str());
}

//-----------------------------------------------------------------------------
static void CopyArgToReg(int reg, int sovReg, const char *var)
{
    if(IsNumber(var))
        CopyLitToReg(reg, sovReg, hobatoi(var));
    else
        CopyVarToReg(reg, sovReg, var);
}

static void CopyArgToReg(int reg, int sovReg, const NameArray &var)
{
    CopyArgToReg(reg, sovReg, var.c_str());
}

//-----------------------------------------------------------------------------
static void StFromReg(AvrOp op, int sov, int reg, int sovReg, bool signPropagation)
// used r25
// Address is preloaded to X,Y or Z.
{
    if(sovReg < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sovReg > 4)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error");

    if((op != OP_ST_XP) && (op != OP_ST_YP) && (op != OP_ST_ZP))
        THROW_COMPILER_EXCEPTION("Internal error");

    for(int i = 0; i < sov; i++) {
        if(i < sovReg)
            Instruction(op, reg + i);
        else {
            Instruction(OP_CLR, r25); // NO BLINK
            if(signPropagation) {
                Instruction(OP_SBRC, reg + sovReg - 1, BIT7); // Sign propagation
                Instruction(OP_COM, r25);                     // NO BLINK
            }
            Instruction(op, r25);
        }
    }
}
//-----------------------------------------------------------------------------
static void _CopyRegToVar(int l, const char *f, const char *args, const char *var, int reg, int sovReg)
{
    DWORD addr;
    int   sov;

    if(IsAddrInVar(var)) {
        MemForVariable(&var[1], &addr);
    } else {
        MemForVariable(var, &addr);
    }
    sov = SizeOfVar(var);
    LoadXAddr(addr, var); // load direct addres

    if(IsAddrInVar(var)) {
        int sovA = SizeOfVar(&var[1]);
        LdToReg(OP_LD_XP, sovA, r4, 2, false); // as address
        LoadXAddrFromReg(r4, 2);               // reload indirect addres
    }
    StFromReg(OP_ST_XP, sov, reg, sovReg, true); // as data
}

static void _CopyRegToVar(int l, const char *f, const char *args, const NameArray &var, int reg, int sovReg)
{
    _CopyRegToVar(l, f, args, var.c_str(), reg, sovReg);
}

#define CopyRegToVar(...) _CopyRegToVar(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)
//-----------------------------------------------------------------------------
static void Decrement(DWORD addr, int sov)
//used ZL, r25
{
    LoadZAddr(addr);           //2  instructions
    Instruction(OP_LD_Z, r25); //1 instruction
    Instruction(OP_SUBI, r25, 1);
    Instruction(OP_ST_ZP, r25);
    if(sov >= 2) {
        DWORD noCarry = AllocFwdAddr();
        Instruction(OP_BRCC, noCarry);
        Instruction(OP_LD_Z, r25); // now Z is addr+1
        Instruction(OP_SUBI, r25, 1);
        Instruction(OP_ST_ZP, r25);
        if(sov >= 3) {
            Instruction(OP_BRCC, noCarry);
            Instruction(OP_LD_Z, r25); // now Z is addr+2
            Instruction(OP_SUBI, r25, 1);
            Instruction(OP_ST_ZP, r25);
            if(sov >= 4) {
                Instruction(OP_BRCC, noCarry);
                Instruction(OP_LD_Z, r25); // now Z is addr+3
                Instruction(OP_SUBI, r25, 1);
                Instruction(OP_ST_ZP, r25);
            }
        }
        FwdAddrIsNow(noCarry); //5 or (6-9) or (6-10-13) instruction
    }
    // 5 instructions for 8 bit var;
    // 6 or 9 instructions for 16 bit var;
    // 6 or 10 or 13 instructions for 24 bit var;
}
//-----------------------------------------------------------------------------
static void Increment(DWORD addr, int sov, char *name)
//used ZL, r25
{
    LoadZAddr(addr);           //2  instructions
    Instruction(OP_LD_Z, r25); //1 instruction
    Instruction(OP_INC, r25, 0, name);
    //Instruction(OP_SUBI, r25, 0xFF); // r25 = r25 + 1
    Instruction(OP_ST_ZP, r25);
    if(sov >= 2) {
        DWORD noCarry = AllocFwdAddr();
        Instruction(OP_BRNE, noCarry);
        //Instruction(OP_BRCS, noCarry);
        Instruction(OP_LD_Z, r25); // now Z is addl+1
        Instruction(OP_INC, r25, 0, name);
        //Instruction(OP_SUBI, r25, 0xFF); // r25 = r25 + 1
        Instruction(OP_ST_ZP, r25);
        if(sov >= 3) {
            Instruction(OP_BRNE, noCarry);
            //Instruction(OP_BRCS, noCarry);
            Instruction(OP_LD_Z, r25); // now Z is addr+2
            Instruction(OP_INC, r25, 0, name);
            //Instruction(OP_SUBI, r25, 0xFF); // r25 = r25 + 1
            Instruction(OP_ST_ZP, r25);
            if(sov >= 4) {
                Instruction(OP_BRNE, noCarry);
                //Instruction(OP_BRCS, noCarry);
                Instruction(OP_LD_Z, r25); // now Z is addr+2
                Instruction(OP_INC, r25, 0, name);
                //Instruction(OP_SUBI, r25, 0xFF); // r25 = r25 + 1
                Instruction(OP_ST_ZP, r25);
            }
        }
        FwdAddrIsNow(noCarry); //5 or (6-9) or (6-10-13) instruction
    }
    // 5 instructions for 8 bit var;
    // 6 or 9 instructions for 16 bit var;
    // 6 or 10 or 13 instructions for 24 bit var;
    // ? instructions for 32 bit var;
}

static void Increment(DWORD addr, int sov)
{
    Increment(addr, sov, nullptr);
}
//-----------------------------------------------------------------------------
static void IncrementReg(int reg, int sov)
{
    Instruction(OP_INC, reg);
    if(sov >= 2) {
        DWORD noCarry = AllocFwdAddr();
        Instruction(OP_BRNE, noCarry);
        Instruction(OP_INC, reg + 1);
        if(sov >= 3) {
            Instruction(OP_BRNE, noCarry);
            Instruction(OP_INC, reg + 2);
            if(sov >= 4) {
                Instruction(OP_BRNE, noCarry);
                Instruction(OP_INC, reg + 3);
            }
        }
        FwdAddrIsNow(noCarry);
    }
    // 1 instructions for 8 bit reg;
    // 2 or 3 instructions for 16 bit reg;
    // 2 or 3 or 5 instructions for 24 bit reg;
    // 2 or 3 or 5 or 7 instructions for 32 bit reg;
}
//-----------------------------------------------------------------------------
static void IfNotZeroGoto(DWORD addrVar, int sov, DWORD addrGoto)
//used ZL, r25
//if value(addrVar) == 0 GOTO addrGoto
{
    LoadZAddr(addrVar);
    Instruction(OP_LD_ZP, r25);
    Instruction(OP_TST, r25);
    Instruction(OP_BRNE, addrGoto);
    if(sov >= 2) {
        Instruction(OP_LD_ZP, r25);
        Instruction(OP_TST, r25);
        Instruction(OP_BRNE, addrGoto);
        if(sov >= 3) {
            Instruction(OP_LD_ZP, r25);
            Instruction(OP_TST, r25);
            Instruction(OP_BRNE, addrGoto);
            if(sov >= 4) {
                Instruction(OP_LD_ZP, r25);
                Instruction(OP_TST, r25);
                Instruction(OP_BRNE, addrGoto);
            }
        }
    }
}

//-----------------------------------------------------------------------------
int calcAvrUsart(int *divisor, double *actual, double *percentErr)
{
    // bps = Fosc/(16*(X+1))
    // bps*16*(X + 1) = Fosc
    // X = Fosc/(bps*16)-1
    // and round, don't truncate
    *divisor = (Prog.mcuClock + Prog.baudRate * 8) / (Prog.baudRate * 16) - 1;

    *actual = 1.0 * Prog.mcuClock / (16.0 * (*divisor + 1));
    *percentErr = 100.0 * (*actual - Prog.baudRate) / Prog.baudRate;
    return *divisor;
}

int testAvrUsart(int divisor, double actual, double percentErr)
{
    if(fabs(percentErr) > 2) {
        ComplainAboutBaudRateError(divisor, actual, percentErr);
    }
    if(divisor > 4095)
        ComplainAboutBaudRateOverflow();
    return divisor;
}

//-----------------------------------------------------------------------------
// Write the basic runtime. We set up our reset vector, configure all the
// I/O pins, then set up the timer that does the cycling. Next instruction
// written after calling WriteRuntime should be first instruction of the
// timer loop (i.e. the PLC logic cycle).
//-----------------------------------------------------------------------------
static DWORD addrDuty;
static int   bitDuty;
static void  WriteRuntime()
{
    DWORD resetVector = AllocFwdAddr();

    int i;
#ifdef TABLE_IN_FLASH
    InstructionJMP(resetVector); // $0000, RESET
#else
    Instruction(OP_RJMP, resetVector); // $0000, RESET
#endif
    for(i = 0; i < 34; i++)
        Instruction(OP_RETI);
    Comment("Interrupt table end.");
#ifdef TABLE_IN_FLASH
    InitTables();
#endif

    FwdAddrIsNow(resetVector);
    Comment("This is Reset Vector"); // 1
    if(Prog.cycleTimer >= 0) {       // 2
        Comment("ConfigureTimerForPlcCycle");
        ConfigureTimerForPlcCycle(Prog.cycleTime);
    }
    Comment("Watchdog on"); // 3
    Instruction(OP_CLI);
    Instruction(OP_WDR);
    //  OrMemory(REG_WDTCR, (1<<WDCE) | (1<<WDE));
    //  WriteMemoryStillAddr(REG_WDTCR, (1<<WDCE) | (1<<WDE));
#ifdef USE_WDT
    STOREval(REG_WDTCR, (1 << WDCE) | (1 << WDE));
#endif
    Comment("- Got only four cycles to set the new values from here! -");
    WriteMemoryCurrAddr((1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0)); // 2s
    //  WriteMemoryCurrAddr((1<<WDE) | (1<<WDP3) | (1<<WDP0)); // 8s
    ////STOREval(REG_WDTCR, (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0)); // 2s BAD, more then four cycles
    ////WriteMemory(REG_WDTCR, (1<<WDE) | (1<<WDP3) | (1<<WDP0)); // BAD, more then four cycles
    Instruction(OP_SEI);

    Comment("Set up the stack, which we use only when we jump to multiply/divide routine"); // 4
    WORD topOfMemory = (WORD)(Prog.mcu->ram[0].start + Prog.mcu->ram[0].len - 1);
    WriteMemory(REG_SPH, topOfMemory >> 8, topOfMemory);
    WriteMemory(REG_SPL, topOfMemory & 0xff, topOfMemory);

    Comment("Zero out the memory used for timers, internal relays, etc."); // 5
    LoadXAddr(Prog.mcu->ram[0].start + Prog.mcu->ram[0].len);
    Instruction(OP_LDI, 16, 0);
    Instruction(OP_LDI, r24, (Prog.mcu->ram[0].len) & 0xff);
    Instruction(OP_LDI, r25, (Prog.mcu->ram[0].len) >> 8);

    DWORD loopZero = AvrProgWriteP;
    //  Instruction(OP_SUBI, 26, 1);
    //  Instruction(OP_SBCI, 27, 0);
    //  Instruction(OP_ST_X, 16);
    Instruction(OP_ST_XS, 16);
    //  Instruction(OP_SUBI, 18, 1);
    //  Instruction(OP_SBCI, 19, 0);
    //  Instruction(OP_TST, 18, 0);
    //  Instruction(OP_BRNE, loopZero, 0);
    //  Instruction(OP_TST, 19, 0);
    //  Instruction(OP_BRNE, loopZero, 0);
    Instruction(OP_SBIW, r24, 1);
    Instruction(OP_BRNE, loopZero);

    if(plcTmr.softDivisor > 1) { // RAM used, after zero out // 5
        Comment("Configure PLC Timer softDivisor");
        MemForVariable("$softDivisor", &plcTmr.softDivisorAddr);
        WriteLiteralToMemory(
            plcTmr.softDivisorAddr, byteNeeded(plcTmr.softDivisor), plcTmr.softDivisor, "plcTmr.softDivisor");
    }

    Comment("Set up I/O pins"); // 6
    BYTE isInput[MAX_IO_PORTS], isAnsel[MAX_IO_PORTS], isOutput[MAX_IO_PORTS];
    BuildDirectionRegisters(isInput, isAnsel, isOutput);

    if(UartFunctionUsed()) {
        if(Prog.baudRate == 0) {
            THROW_COMPILER_EXCEPTION(_("Zero baud rate not possible."));
            return;
        }

        Comment("UartFunctionUsed. UART setup");
        int    divisor;
        double actual;
        double percentErr;

        calcAvrUsart(&divisor, &actual, &percentErr);
        testAvrUsart(divisor, actual, percentErr);

        WriteMemory(REG_UBRRH, divisor >> 8);
        WriteMemory(REG_UBRRL, divisor & 0xff);
        WriteMemory(REG_UCSRB, (1 << RXEN) | (1 << TXEN));

        // UCSRC initial Value frame format: 8 data, parity - none, 1 stop bit.
        // Not need to set.
    }
    // All PWM outputs setted in BuildDirectionRegisters

    Comment("Turn on the pull-ups, and drive the outputs low to start");
    for(i = 0; i < MAX_IO_PORTS; i++) {
        if(!IS_MCU_REG(i)) {
            // skip this one, dummy entry for MCUs with I/O ports not
            // starting from A
        } else {
            WriteMemory(Prog.mcu->dirRegs[i], isOutput[i]);
            // turn on the pull-ups, and drive the outputs low to start
            WriteMemory(Prog.mcu->outputRegs[i], isInput[i]);
        }
    }
    //Comment("and now the generated PLC code will follow");
    Comment("Begin Of PLC Cycle");
    BeginOfPLCCycle = AvrProgWriteP;
    // ConfigureTimerForPlcCycle
    if(Prog.cycleTimer == 0) {
        if((WGM01 == -1)) { // ATmega8
            DWORD i = SKBS(REG_TIFR0, TOV0);
            Instruction(OP_RJMP, AvrProgWriteP - std::min(i, DWORD(2))); // Ladder cycle timing on Timer0/Counter

            SetBit(REG_TIFR0, TOV0); // Opcodes: 4+1+5 = 10
            //To clean a bit in the register TIFR need write 1 in the corresponding bit!

            STOREval(REG_TCNT0, BYTE(tcnt0PlcCycle + 0)); // + 0 DONE // reload Counter0
        } else {
            DWORD i = SKBS(REG_TIFR0, OCF0A);
            Instruction(OP_RJMP, AvrProgWriteP - std::min(i, DWORD(2))); // Ladder cycle timing on Timer0/Counter

            SetBit(REG_TIFR0, OCF0A);
            //To clean a bit in the register TIFR need write 1 in the corresponding bit!
        }
    } else if(Prog.cycleTimer == 1) {
        DWORD i = SKBS(REG_TIFR1, OCF1A);
        Instruction(OP_RJMP, AvrProgWriteP - std::min(i, DWORD(2))); // Ladder cycle timing on Timer1/Counter

        SetBit(REG_TIFR1, OCF1A);
        //To clean a bit in the register TIFR need write 1 in the corresponding bit!
    } else if(Prog.cycleTimer < 0) {
        ;
    } else
        THROW_COMPILER_EXCEPTION("Internal error");

    Comment("Watchdog reset");
    Instruction(OP_WDR);

#if 1 // 1
    if(plcTmr.softDivisor > 1) {
        Decrement(plcTmr.softDivisorAddr, byteNeeded(plcTmr.softDivisor));
        Instruction(OP_BRNE, BeginOfPLCCycle);
        WriteLiteralToMemory(
            plcTmr.softDivisorAddr, byteNeeded(plcTmr.softDivisor), plcTmr.softDivisor, "plcTmr.softDivisor");
    }
#endif

    if(Prog.cycleDuty) {
        Comment("SetBit YPlcCycleDuty");
        MemForSingleBit(YPlcCycleDuty, false, &addrDuty, &bitDuty);
        SetBit(addrDuty, bitDuty);
    }
}

//-----------------------------------------------------------------------------
/*
HLC2705
LS7184
LS7084
HCTL-2022
LS7166
A1230
A1232
A1233

Encoder outputs with bounce
                               <----   ---->
              -1           -0                    +0           +1

  ---------xxxx         xxxx---------------------xxxx         xxxx---------
A          xxxx         xxxx                     xxxx         xxxx
           xxxx---------xxxx                     xxxx---------xxxx

  --xxxx         xxxx---------xxxx         xxxx---------xxxx         xxxx---------
B   xxxx         xxxx         xxxx         xxxx         xxxx         xxxx
    xxxx---------xxxx         xxxx---------xxxx         xxxx---------xxxx


A_   B_   previous encoder input
A    B    new encoder input

Encoder lookup table
     B_ A_ B  A                           (B_A_ xor BA)     (B_ xor B)
0    0  0  0  0    0   not changed state         0               0
1    0  0  0  1   -1                             1               0      -
2    0  0  1  0   +1                             2               1      +
3    0  0  1  1    E   Error                     3 (B | A)       1
4    0  1  0  0   +1                             1               0      +
5    0  1  0  1    0   not changed state         0               0
6    0  1  1  0    E   Error                     3 (B | A)       1
7    0  1  1  1   -1                             2               1      -
8    1  0  0  0   -1                             2               1      -
9    1  0  0  1    E   Error                     3 (B | A)       0
A    1  0  1  0    0   not changed state         0               0
B    1  0  1  1   +1                             1               0      +
C    1  1  0  0    E   Error                     3 (B | A)       1
D    1  1  0  1   +1                             2               1      +
E    1  1  1  0   -1                             1               0      -
F    1  1  1  1    0   not changed state         0               0

{0,-1,1,E,1,0,E,-1,-1,E,0,1,E,1,-1,0};

                       (B_ xor A)   (A_ xor B)
2    0  0  1  0   +1        0           1
4    0  1  0  0   +1        0           1
B    1  0  1  1   +1        0           1
D    1  1  0  1   +1        0           1

1    0  0  0  1   -1        1           0
7    0  1  1  1   -1        1           0
8    1  0  0  0   -1        1           0
E    1  1  1  0   -1        1           0

Error:
Two outputs A and B of the quadrature encoder cannot changed together.
If an Error is detected, it means that lost one or more steps.

ISR (PCINT1_vect) {
    static unsigned char enc_last=0,enc_now;
    enc_now = (PINC & (3<<4))>>4;                   //read the port pins and shift result to bottom bits
    enc_dir = (enc_last & 1)^((enc_now & 2) >> 1);  //determine direction of rotation
    if(enc_dir==0) enc_pos++; else enc_pos--;       //update encoder position
    enc_last=enc_now;                               //remember last state
}
The algorithm is from Scott Edwards, in Nuts&Volts Vol 1 Oct. 1995 (Basic Stamp #8) and
is described in an article available on line at
http://www.parallax.com/dl/docs/cols/nv/vol1/col/nv8.pdf
*/

//-----------------------------------------------------------------------------
// Compile the intermediate code to AVR native code.
//-----------------------------------------------------------------------------
static void CompileFromIntermediate()
{
    DWORD addr = 0, addr1 = 0, addr2 = 0, addr3 = 0, addr4 = 0;
    int   bit = -1, bit1 = -1, bit2 = -1, bit3 = -1, bit4 = -1;
    int   sov = -1, sov1 = -1, sov2 = -1, sov12 = -1, sov23 = -1;

    for(; IntPc < IntCode.size(); IntPc++) {
        IntPcNow = IntPc;
        IntOp *a = &IntCode[IntPc];
        rungNow = a->rung;
        switch(a->op) {
            case INT_SET_BIT:
                Comment("INT_SET_BIT %s", a->name1.c_str());
                MemForSingleBit(a->name1, false, &addr1, &bit1);
                SetBit(addr1, bit1, a->name1.c_str());
                break;

            case INT_CLEAR_BIT:
                Comment("INT_CLEAR_BIT %s", a->name1.c_str());
                MemForSingleBit(a->name1, false, &addr1, &bit1);
                ClearBit(addr1, bit1, a->name1.c_str());
                break;

            case INT_COPY_BIT_TO_BIT:
                Comment("INT_COPY_BIT_TO_BIT %s:=%s", a->name1.c_str(), a->name2.c_str());
                MemForSingleBit(a->name1, false, &addr1, &bit1);
                MemForSingleBit(a->name2, false, &addr2, &bit2);
                CopyBit(addr1, bit1, addr2, bit2, a->name1.c_str(), a->name2.c_str());
                break;

            case INT_COPY_VAR_BIT_TO_VAR_BIT:
                Comment("INT_COPY_VAR_BIT_TO_VAR_BIT");
                break;

            case INT_SET_BCD2BIN:
                Comment("INT_SET_BCD2BIN");
                THROW_COMPILER_EXCEPTION_FMT("Invalid operation %i.", INT_SET_BCD2BIN);
                break;

            case INT_SET_BIN2BCD:
                // 0..99
                Comment("INT_SET_BIN2BCD");
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                Comment("INT_SET_VARIABLE_TO_LITERAL %s:=0x%X(%d)", a->name1.c_str(), a->literal, a->literal);
                if(IsAddrInVar(a->name1.c_str()))
                    MemForVariable(&a->name1[1], &addr1);
                else
                    MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);
                WriteLiteralToMemory(addr1, sov1, a->literal, a->name1);
                break;

            case INT_INCREMENT_VARIABLE: {
                Comment("INT_INCREMENT_VARIABLE %s", a->name1.c_str());
                sov1 = SizeOfVar(a->name1);
                MemForVariable(a->name1, &addr1);
                Increment(addr1, sov1);
                break;
            }
            case INT_DECREMENT_VARIABLE: {
                Comment("INT_DECREMENT_VARIABLE %s", a->name1.c_str());
                sov1 = SizeOfVar(a->name1);
                MemForVariable(a->name1, &addr1);
                Decrement(addr1, sov1);
                break;
            }
            case INT_IF_BIT_SET: {
                Comment("INT_IF_BIT_SET %s", a->name1.c_str());
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                IfBitClear(addr1, bit1, a->name1);
                Instruction(OP_RJMP, condFalse);
                CompileIfBody(condFalse);
                break;
            }
            case INT_IF_BIT_CLEAR: {
                Comment("INT_IF_BIT_CLEAR %s", a->name1.c_str());
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                IfBitSet(addr1, bit1, a->name1);
                Instruction(OP_RJMP, condFalse);
                CompileIfBody(condFalse);
                break;
            }
            case INT_VARIABLE_CLEAR_BIT: {
                Comment("INT_VARIABLE_CLEAR_BIT %s %s", a->name1.c_str(), a->name2.c_str());
                bit = hobatoi(a->name2.c_str());
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);
                if(IsNumber(a->name2)) {
                    if((0 <= bit) && (bit <= 7))
                        ClearBit(addr1, bit, a->name1);
                    else if((8 <= bit) && (bit <= 15) && (sov1 >= 2))
                        ClearBit(addr1 + 1, bit - 8, a->name1);
                    else if((16 <= bit) && (bit <= 23) && (sov1 >= 3))
                        ClearBit(addr1 + 2, bit - 16, a->name1);
                    else if((24 <= bit) && (bit <= 32) && (sov1 >= 4))
                        ClearBit(addr1 + 3, bit - 24, a->name1);
                    else
                        THROW_COMPILER_EXCEPTION_FMT("Invalid bit number %i", bit);
                } else {
                    CopyVarToReg(r3, 1, a->name2);
                    CopyLitToReg(r16, sov1, -2); // 0xF..FE
                    DWORD Skip = AllocFwdAddr();
                    DWORD Loop = AvrProgWriteP;
                    Instruction(OP_TST, r3);
                    Instruction(OP_BREQ, Skip);
                    Instruction(OP_DEC, r3);
                    RolReg(r16, sov1);
                    Instruction(OP_RJMP, Loop);
                    FwdAddrIsNow(Skip);

                    CopyVarToReg(r20, sov1, a->name1);
                    AndReg(r20, sov1, r16);
                    CopyRegToVar(a->name1, r20, sov1);
                }
                break;
            }
            case INT_VARIABLE_SET_BIT: {
                Comment("INT_VARIABLE_SET_BIT %s %s", a->name1.c_str(), a->name2.c_str());
                bit = hobatoi(a->name2.c_str());
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);
                if(IsNumber(a->name2)) {
                    if((0 <= bit) && (bit <= 7))
                        SetBit(addr1, bit, a->name1);
                    else if((8 <= bit) && (bit <= 15) && (sov1 >= 2))
                        SetBit(addr1 + 1, bit - 8, a->name1);
                    else if((16 <= bit) && (bit <= 23) && (sov1 >= 3))
                        SetBit(addr1 + 2, bit - 16, a->name1);
                    else if((24 <= bit) && (bit <= 32) && (sov1 >= 4))
                        SetBit(addr1 + 3, bit - 24, a->name1);
                    else
                        THROW_COMPILER_EXCEPTION_FMT("Invalid bit number %i", bit);
                } else {
                    CopyVarToReg(r3, 1, a->name2);
                    CopyLitToReg(r16, sov1, 0x01);
                    DWORD Skip = AllocFwdAddr();
                    DWORD Loop = AvrProgWriteP;
                    Instruction(OP_TST, r3);
                    Instruction(OP_BREQ, Skip);
                    Instruction(OP_DEC, r3);
                    ShlReg(r16, sov1);
                    Instruction(OP_RJMP, Loop);
                    FwdAddrIsNow(Skip);

                    CopyVarToReg(r20, sov1, a->name1);
                    OrReg(r20, sov1, r16);
                    CopyRegToVar(a->name1, r20, sov1);
                }
                break;
            }
            case INT_IF_BIT_SET_IN_VAR: {
                Comment("INT_IF_BIT_SET_IN_VAR %s %s", a->name1.c_str(), a->name2.c_str());
                DWORD ifAddr = AllocFwdAddr();
                DWORD endifAddr = AllocFwdAddr();
                sov1 = SizeOfVar(a->name1);
                if(IsNumber(a->name2)) {
                    MemForVariable(a->name1, &addr1);
                    bit = hobatoi(a->name2.c_str());
                    if((0 <= bit) && (bit <= 7))
                        IfBitClear(addr1, bit, a->name1);
                    else if((8 <= bit) && (bit <= 15) && (sov1 >= 2))
                        IfBitClear(addr1 + 1, bit - 8);
                    else if((16 <= bit) && (bit <= 23) && (sov1 >= 3))
                        IfBitClear(addr1 + 2, bit - 16);
                    else if((24 <= bit) && (bit <= 32) && (sov1 >= 4))
                        IfBitClear(addr1 + 3, bit - 24);
                    else
                        THROW_COMPILER_EXCEPTION_FMT("Invalid bit number %i", bit);
                    Instruction(OP_RJMP, endifAddr); // here bit is CLR
                } else {
                    CopyVarToReg(r3, 1, a->name2);
                    CopyLitToReg(r16, sov1, 0x01);
                    DWORD Skip = AllocFwdAddr();
                    DWORD Loop = AvrProgWriteP;
                    Instruction(OP_TST, r3);
                    Instruction(OP_BREQ, Skip);
                    Instruction(OP_DEC, r3);
                    ShlReg(r16, sov1);
                    Instruction(OP_RJMP, Loop);
                    FwdAddrIsNow(Skip);

                    CopyVarToReg(r20, sov1, a->name1);
                    CpseReg(r20, sov1, r16, endifAddr);
                    /*
                    Instruction(OP_AND,  r20, r16);
                    Instruction(OP_CPSE, r20, r16);
                    Instruction(OP_RJMP, endifAddr);     // here r20 != r16, bit is CLR
                    if(sov >= 2) {
                        Instruction(OP_AND,  r20+1, r16+1);
                        Instruction(OP_CPSE, r20+1, r16+1);
                        Instruction(OP_RJMP, endifAddr); // here r20 != r16, bit is CLR
                    }
                    if(sov >= 3) {
                        Instruction(OP_AND,  r20+2, r16+2);
                        Instruction(OP_CPSE, r20+2, r16+2);
                        Instruction(OP_RJMP, endifAddr); // here r20 != r16, bit is CLR
                    }
                    if(sov >= 4) {
                        Instruction(OP_AND,  r20+3, r16+3);
                        Instruction(OP_CPSE, r20+3, r16+3);
                        Instruction(OP_RJMP, endifAddr); // here r20 != r16, bit is CLR
                    }
                  //Instruction(OP_RJMP, ifAddr);     // here r20 == r16, bit is SET
*/
                }
                FwdAddrIsNow(ifAddr);     // 1
                CompileIfBody(endifAddr); // 2
                break;
            }
                /*
        case ELEM_IF_BIT_CLEAR:
            Comment(3, "ELEM_IF_BIT_CLEAR");
            Op(INT_IF_BIT_SET_IN_VAR, l->d.cmp.op1, l->d.cmp.op2);
                Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
*/
            case INT_IF_BIT_CLEAR_IN_VAR: {
                Comment("INT_IF_BIT_CLEAR_IN_VAR %s %s", a->name1.c_str(), a->name2.c_str());
                DWORD ifAddr = AllocFwdAddr();
                DWORD endifAddr = AllocFwdAddr();
                sov1 = SizeOfVar(a->name1);
                if(IsNumber(a->name2)) {
                    MemForVariable(a->name1, &addr1);
                    bit = hobatoi(a->name2.c_str());
                    if((0 <= bit) && (bit <= 7))
                        IfBitSet(addr1, bit);
                    else if((8 <= bit) && (bit <= 15) && (sov1 >= 2))
                        IfBitSet(addr1 + 1, bit - 8);
                    else if((16 <= bit) && (bit <= 23) && (sov1 >= 3))
                        IfBitSet(addr1 + 2, bit - 16);
                    else if((24 <= bit) && (bit <= 32) && (sov1 >= 4))
                        IfBitSet(addr1 + 3, bit - 24);
                    else
                        THROW_COMPILER_EXCEPTION_FMT("Invalid bit number %i", bit);
                    Instruction(OP_RJMP, endifAddr); // here bit is SET
                } else {
                    CopyVarToReg(r3, 1, a->name2);
                    CopyLitToReg(r16, sov1, 0x01);
                    DWORD Skip = AllocFwdAddr();
                    DWORD Loop = AvrProgWriteP;
                    Instruction(OP_TST, r3);
                    Instruction(OP_BREQ, Skip);
                    Instruction(OP_DEC, r3);
                    ShlReg(r16, sov1);
                    Instruction(OP_RJMP, Loop);
                    FwdAddrIsNow(Skip);

                    CopyVarToReg(r20, sov1, a->name1);
                    CpseReg(r20, sov1, r16, ifAddr);
                    /*
                    Instruction(OP_AND,  r20, r16);
                    Instruction(OP_CPSE, r20, r16);
                    Instruction(OP_RJMP, ifAddr);     // here r20 != r16, bit is CLR
                    if(sov >= 2) {
                        Instruction(OP_AND,  r20+1, r16+1);
                        Instruction(OP_CPSE, r20+1, r16+1);
                        Instruction(OP_RJMP, ifAddr); // here r20 != r16, bit is CLR
                    }
                    if(sov >= 3) {
                        Instruction(OP_AND,  r20+2, r16+2);
                        Instruction(OP_CPSE, r20+2, r16+2);
                        Instruction(OP_RJMP, ifAddr); // here r20 != r16, bit is CLR
                    }
                    if(sov >= 4) {
                        Instruction(OP_AND,  r20+3, r16+3);
                        Instruction(OP_CPSE, r20+3, r16+3);
                        Instruction(OP_RJMP, ifAddr); // here r20 != r16, bit is CLR
                    }
*/
                    Instruction(OP_RJMP, endifAddr); // here r20 == r16, bit is SET
                }
                FwdAddrIsNow(ifAddr);     // 1
                CompileIfBody(endifAddr); // 2
                break;
            }
                /*
        case ELEM_IF_BIT_SET:
            Comment(3, "ELEM_IF_BIT_SET");
            Op(INT_IF_BIT_CLEAR_IN_VAR, l->d.cmp.op1, l->d.cmp.op2);
                Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
*/
            //
            //case INT_IF_BITS_CLEAR_IN_VAR: TODO
            case INT_IF_BITS_SET_IN_VAR: {
                Comment("INT_IF_BITS_SET_IN_VAR %s", a->name1.c_str());
            }
            case INT_SET_OPPOSITE:
                Comment("INT_SET_OPPOSITE %s", a->name1.c_str());
                THROW_COMPILER_EXCEPTION_FMT("Invalid AVR OpCode %i", INT_SET_OPPOSITE);
                break;

            case INT_SET_SWAP:
                Comment(" %s", a->name1.c_str());
                //MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                CopyArgToReg(r16, sov1, a->name2);
                if(sov2 == 1) {
                    Instruction(OP_SWAP, r16);
                } else if(sov2 == 2) {
                    Instruction(OP_MOV, r3, r16);
                    Instruction(OP_MOV, r16, r17);
                    Instruction(OP_MOV, r17, r3);
                } else if(sov2 == 3) {
                    Instruction(OP_MOV, r3, r16);
                    Instruction(OP_MOV, r16, r18); // r17 stay same
                    Instruction(OP_MOV, r18, r3);
                } else if(sov2 == 4) {
                    Instruction(OP_MOV, r3, r16);
                    Instruction(OP_MOV, r16, r19);
                    Instruction(OP_MOV, r19, r3);

                    Instruction(OP_MOV, r3, r17);
                    Instruction(OP_MOV, r17, r18);
                    Instruction(OP_MOV, r18, r3);
                } else
                    THROW_COMPILER_EXCEPTION("Invalid var size in swap.");

                        CopyRegToVar(a->name1, r16, sov1);
                break;

#ifndef NEW_CMP
            case INT_IF_VARIABLE_LES_LITERAL: {
                Comment("INT_IF_VARIABLE_LES_LITERAL %s < 0x%X(%d)", a->name1.c_str(), a->literal, a->literal);
                DWORD notTrue = AllocFwdAddr();

                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);

                DWORD l1, l2;
                l1 = a->literal & 0xff;

                //Instruction(OP_LDI, 20, (a->literal & 0xff));
                Instruction(OP_LDI, 20, l1);
                Instruction(OP_LD_XP, 16);
                Instruction(OP_CP, 16, 20);

                sov1 = SizeOfVar(a->name1);
                if(sov1 >= 2) {
                    //Instruction(OP_LDI, 20, (a->literal >> 8) & 0xff);
                    l2 = (a->literal >> 8) & 0xff;
                    if(l1 != l2)
                        Instruction(OP_LDI, r20, l2);
                    Instruction(OP_LD_XP, 17);
                    Instruction(OP_CPC, 17, 20);

                    if(sov1 >= 3) {
                        //Instruction(OP_LDI, 20, (a->literal >> 16) & 0xff);
                        l1 = (a->literal >> 16) & 0xff;
                        if(l1 != l2)
                            Instruction(OP_LDI, r20, l1);
                        Instruction(OP_LD_XP, 18);
                        Instruction(OP_CPC, 18, 20);

                        if(sov1 >= 4) {
                            //Instruction(OP_LDI, 20, (a->literal >> 24) & 0xff);
                            l2 = (a->literal >> 24) & 0xff;
                            if(l1 != l2)
                                Instruction(OP_LDI, r20, l2);
                            Instruction(OP_LD_XP, 19);
                            Instruction(OP_CPC, 19, 20);

                            if(sov1 > 4)
                                oops();
                        }
                    }
                }
                if(a->op == INT_IF_VARIABLE_LES_LITERAL)
                    Instruction(OP_BRGE, notTrue, 0);

                CompileIfBody(notTrue);
                break;
            }

            case INT_IF_VARIABLE_GRT_VARIABLE:
            case INT_IF_VARIABLE_EQUALS_VARIABLE: {
                DWORD notTrue = AllocFwdAddr();

                sov = std::max(SizeOfVar(a->name1), SizeOfVar(a->name2));
                if(a->op == INT_IF_VARIABLE_GRT_VARIABLE) {
                    //Interchange Rd and Rr in the operation before the test, i.e., CP Rd,Rr
                    CopyArgToReg(r20, sov, a->name1);
                    CopyArgToReg(r16, sov, a->name2);
                } else {
                    CopyArgToReg(r16, sov, a->name1);
                    CopyArgToReg(r20, sov, a->name2);
                }

                Instruction(OP_CP, 16, 20);
                if(sov >= 2)
                    Instruction(OP_CPC, 17, 21);
                if(sov >= 3)
                    Instruction(OP_CPC, 18, 22);
                if(sov >= 4)
                    Instruction(OP_CPC, 19, 23);

                if(a->op == INT_IF_VARIABLE_EQUALS_VARIABLE) {
                    Instruction(OP_BRNE, notTrue, 0);
                } else if(a->op == INT_IF_VARIABLE_GRT_VARIABLE) {
                    // true if op1 > op2
                    // false if op1 <= op2
                    Instruction(OP_BRGE, notTrue, 0);
                } else
                    oops();
                CompileIfBody(notTrue);
                break;
            }
#endif

#ifdef NEW_CMP
            case INT_IF_GRT:
            case INT_IF_GEQ:
            case INT_IF_LES:
            case INT_IF_LEQ:
            case INT_IF_NEQ:
            case INT_IF_EQU: {
                DWORD notTrue = AllocFwdAddr();
                sov = std::max(SizeOfVar(a->name1), SizeOfVar(a->name2));
                CopyArgToReg(r20, sov, a->name1);
                CopyArgToReg(r16, sov, a->name2);

                switch(a->op) {
                    case INT_IF_LEQ: // * op2 - op1
                    case INT_IF_GRT: // *
                        Instruction(OP_CP, 16, 20);
                        if(sov >= 2)
                            Instruction(OP_CPC, 17, 21);
                        if(sov >= 3)
                            Instruction(OP_CPC, 18, 22);
                        if(sov >= 4)
                            Instruction(OP_CPC, 19, 23);
                        break;
                    case INT_IF_EQU: // op1 - op2
                        Instruction(OP_CP, 20, 16);
                        Instruction(OP_BRNE, notTrue, 0);
                        if(sov >= 2) {
                            Instruction(OP_CPC, 21, 17);
                            Instruction(OP_BRNE, notTrue, 0);
                        }
                        if(sov >= 3) {
                            Instruction(OP_CPC, 22, 18);
                            Instruction(OP_BRNE, notTrue, 0);
                        }
                        if(sov >= 4) {
                            Instruction(OP_CPC, 23, 19);
                            Instruction(OP_BRNE, notTrue, 0);
                        }
                    default: // op1 - op2
                        Instruction(OP_CP, 20, 16);
                        if(sov >= 2)
                            Instruction(OP_CPC, 21, 17);
                        if(sov >= 3)
                            Instruction(OP_CPC, 22, 18);
                        if(sov >= 4)
                            Instruction(OP_CPC, 23, 19);
                }
                switch(a->op) {
                    case INT_IF_LEQ: // *
                    case INT_IF_GEQ:
                        Instruction(OP_BRLT, notTrue, 0);
                        break;
                    case INT_IF_GRT: // *
                    case INT_IF_LES:
                        Instruction(OP_BRGE, notTrue, 0);
                        break;
                    case INT_IF_NEQ:
                        Instruction(OP_BREQ, notTrue, 0);
                        break;
                    case INT_IF_EQU:
                        Instruction(OP_BRNE, notTrue, 0);
                        break;
                    default:
                        THROW_COMPILER_EXCEPTION("Invalid compare.");
                }
                CompileIfBody(notTrue);
                break;
            }
#endif

#ifdef USE_SFR
            // Sepcial function
            case INT_READ_SFR_LITERAL: {
                MemForVariable(a->name1, &addr1);
                Instruction(OP_LDI, 26, (a->literal & 0xff));
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(addr1);
                Instruction(OP_ST_X, 16, 0);
                break;
            }
            case INT_READ_SFR_VARIABLE: {
                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                LoadXAddr(addr1);
                Instruction(OP_LD_XP, 28, 0); //Y-register
                Instruction(OP_LD_X, 29, 0);
                Instruction(OP_LD_Y, 16, 0);
                LoadXAddr(addr2);
                Instruction(OP_ST_X, 16, 0); //8
                break;
            }
            case INT_WRITE_SFR_LITERAL_L: {
                Comment("INT_WRITE_SFR_LITERAL_L");
                //MemForVariable(a->name1, &addr1); // name not used
                Instruction(OP_LDI, 28, (a->literal2 & 0xff)); //op
                Instruction(OP_LDI, 26, (a->literal & 0xff));  //sfr
                Instruction(OP_LDI, 27, (a->literal >> 8));    //sfr
                Instruction(OP_ST_X, 28, 0);
                break;
            }
            case INT_WRITE_SFR_VARIABLE_L: {
                Comment("INT_WRITE_SFR_VARIABLE_L");
                CopyArgToReg(ZL, 2, a->name1);                //sfr
                Instruction(OP_LDI, 28, (a->literal & 0xff)); //op
                Instruction(OP_ST_Z, 28, 0);
                break;
            }
            case INT_WRITE_SFR_LITERAL: {
                Comment("INT_WRITE_SFR_LITERAL");
                MemForVariable(a->name1, &addr1); //op
                LoadXAddr(addr1);
                Instruction(OP_LD_X, 15, 0);
                Instruction(OP_LDI, 26, (a->literal & 0xff)); //sfr
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_ST_X, 15, 0);
                break;
            }
            case INT_WRITE_SFR_VARIABLE: {
                Comment("INT_WRITE_SFR_VARIABLE");
                CopyArgToReg(ZL, 2, a->name1);    //sfr
                MemForVariable(a->name2, &addr2); //op
                LoadXAddr(addr2);
                Instruction(OP_LD_X, 15, 0);
                Instruction(OP_ST_Z, 15, 0);
                break;
            }
            case INT_SET_SFR_LITERAL_L: {
                MemForVariable(a->name1, &addr1);
                Instruction(OP_LDI, 28, (a->literal2 & 0xff));
                Instruction(OP_LDI, 26, (a->literal & 0xff));
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_OR, 1, 28);  // logic OR by R1,R0 result is in R1
                Instruction(OP_ST_X, 1, 0);
                break;
            }
            case INT_SET_SFR_VARIABLE_L: {
                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);
                Instruction(OP_LD_XP, 16, 0);
                Instruction(OP_LD_XP, 17, 0);
                Instruction(OP_MOV, 26, 16);
                Instruction(OP_MOV, 27, 17);
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_LDI, 28, (a->literal & 0xff));
                Instruction(OP_OR, 1, 28); // logic OR by R1,R0 result is in R1
                Instruction(OP_ST_X, 1, 0);
                break;
            }
            case INT_SET_SFR_LITERAL: {
                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);
                Instruction(OP_LD_X, 0, 0); // read byte from variable to r0
                Instruction(OP_LDI, 26, (a->literal & 0xff));
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_OR, 1, 0);   // logic OR by R1,R0 result is in R1
                Instruction(OP_ST_X, 1, 0); // store R1 back to SFR
                break;
            }
            case INT_SET_SFR_VARIABLE: {
                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                LoadXAddr(addr2);
                Instruction(OP_LD_X, 0, 0); // read byte from variable to r0
                LoadXAddr(addr1);
                Instruction(OP_LD_XP, 16, 0);
                Instruction(OP_LD_XP, 17, 0);
                Instruction(OP_MOV, 26, 16);
                Instruction(OP_MOV, 27, 17);
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_OR, 1, 0);   // logic OR by R1,R0 result is in R1
                Instruction(OP_ST_X, 1, 0); // store R1 back to SFR
                break;
            }
            case INT_CLEAR_SFR_LITERAL_L: {
                MemForVariable(a->name1, &addr1);
                Instruction(OP_LDI, 28, (a->literal2 & 0xff));
                Instruction(OP_LDI, 26, (a->literal & 0xff));
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_COM, 0, 0);  //
                Instruction(OP_AND, 1, 28); //
                Instruction(OP_ST_X, 1, 0);
                break;
            }
            case INT_CLEAR_SFR_VARIABLE_L: {
                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);
                Instruction(OP_LD_XP, 16, 0);
                Instruction(OP_LD_XP, 17, 0);
                Instruction(OP_MOV, 26, 16);
                Instruction(OP_MOV, 27, 17);
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_LDI, 28, (a->literal & 0xff));
                Instruction(OP_COM, 0, 0);  //
                Instruction(OP_AND, 1, 28); //
                Instruction(OP_ST_X, 1, 0);
                break;
            }
            case INT_CLEAR_SFR_LITERAL: {
                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);
                Instruction(OP_LD_X, 0, 0); // read byte from variable to r0
                Instruction(OP_LDI, 26, (a->literal & 0xff));
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_COM, 0, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_AND, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_ST_X, 1, 0); // store R1 back to SFR
                break;
            }
            case INT_CLEAR_SFR_VARIABLE: {
                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                LoadXAddr(addr2);
                Instruction(OP_LD_X, 0, 0); // read byte from variable to r0
                LoadXAddr(addr1);
                Instruction(OP_LD_XP, 16, 0);
                Instruction(OP_LD_XP, 17, 0);
                Instruction(OP_MOV, 26, 16);
                Instruction(OP_MOV, 27, 17);
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_COM, 0, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_AND, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_ST_X, 1, 0); // store R1 back to SFR
                break;
            }
            case INT_TEST_SFR_LITERAL_L: {
                DWORD notTrue = AllocFwdAddr();
                MemForVariable(a->name1, &addr1);
                Instruction(OP_LDI, 28, (a->literal2 & 0xff));
                Instruction(OP_LDI, 26, (a->literal & 0xff));
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_AND, 1, 28); // logic OR by R1,R0 result is in R1
                Instruction(OP_EOR, 1, 28); // logic OR by R1,R0 result is in R1
                Instruction(OP_TST, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_BRNE, notTrue, 0);
                CompileIfBody(notTrue);
                break;
            }
            case INT_TEST_SFR_VARIABLE_L: {
                DWORD notTrue = AllocFwdAddr();
                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);
                Instruction(OP_LD_XP, 16, 0);
                Instruction(OP_LD_XP, 17, 0);
                Instruction(OP_MOV, 26, 16);
                Instruction(OP_MOV, 27, 17);
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_LDI, 28, (a->literal & 0xff));
                Instruction(OP_AND, 1, 28); // logic OR by R1,R0 result is in R1
                Instruction(OP_EOR, 1, 28); // logic OR by R1,R0 result is in R1
                Instruction(OP_TST, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_BRNE, notTrue, 0);
                CompileIfBody(notTrue);
                break;
            }
            case INT_TEST_SFR_LITERAL: {
                DWORD notTrue = AllocFwdAddr();
                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);
                Instruction(OP_LD_X, 0, 0); // read byte from variable to r0
                Instruction(OP_LDI, 26, (a->literal & 0xff));
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_AND, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_EOR, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_TST, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_BRNE, notTrue, 0);
                CompileIfBody(notTrue);
                break;
            }
            case INT_TEST_SFR_VARIABLE: {
                DWORD notTrue = AllocFwdAddr();
                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                LoadXAddr(addr2);
                Instruction(OP_LD_X, 0, 0); // read byte from variable to r0
                LoadXAddr(addr1);
                Instruction(OP_LD_XP, 16, 0);
                Instruction(OP_LD_XP, 17, 0);
                Instruction(OP_MOV, 26, 16);
                Instruction(OP_MOV, 27, 17);
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_AND, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_EOR, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_TST, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_BRNE, notTrue, 0);
                CompileIfBody(notTrue);
                break;
            }
            case INT_TEST_C_SFR_LITERAL_L: {
                DWORD notTrue = AllocFwdAddr();
                MemForVariable(a->name1, &addr1);
                Instruction(OP_LDI, 28, (a->literal2 & 0xff));
                Instruction(OP_LDI, 26, (a->literal & 0xff));
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_COM, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_AND, 1, 28); // logic OR by R1,R0 result is in R1
                Instruction(OP_EOR, 1, 28); // logic OR by R1,R0 result is in R1
                Instruction(OP_TST, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_BRNE, notTrue, 0);
                CompileIfBody(notTrue);
                break;
            }
            case INT_TEST_C_SFR_VARIABLE_L: {
                DWORD notTrue = AllocFwdAddr();
                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);
                Instruction(OP_LD_XP, 16, 0);
                Instruction(OP_LD_XP, 17, 0);
                Instruction(OP_MOV, 26, 16);
                Instruction(OP_MOV, 27, 17);
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_LDI, 28, (a->literal & 0xff));
                Instruction(OP_COM, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_AND, 1, 28); // logic OR by R1,R0 result is in R1
                Instruction(OP_EOR, 1, 28); // logic OR by R1,R0 result is in R1
                Instruction(OP_TST, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_BRNE, notTrue, 0);
                CompileIfBody(notTrue);
                break;
            }
            case INT_TEST_C_SFR_LITERAL: {
                DWORD notTrue = AllocFwdAddr();
                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);
                Instruction(OP_LD_X, 0, 0); // read byte from variable to r0
                Instruction(OP_LDI, 26, (a->literal & 0xff));
                Instruction(OP_LDI, 27, (a->literal >> 8));
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_COM, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_AND, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_EOR, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_TST, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_BRNE, notTrue, 0);
                CompileIfBody(notTrue);
                break;
            }
            case INT_TEST_C_SFR_VARIABLE: {
                DWORD notTrue = AllocFwdAddr();
                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                LoadXAddr(addr2);
                Instruction(OP_LD_X, 0, 0); // read byte from variable to r0
                LoadXAddr(addr1);
                Instruction(OP_LD_XP, 16, 0);
                Instruction(OP_LD_XP, 17, 0);
                Instruction(OP_MOV, 26, 16);
                Instruction(OP_MOV, 27, 17);
                Instruction(OP_LD_X, 1, 0); // read byte from SFR
                Instruction(OP_COM, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_AND, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_EOR, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_TST, 1, 0);  // logic OR by R1,R0 result is in R1
                Instruction(OP_BRNE, notTrue, 0);
                CompileIfBody(notTrue);
                break;
            }
// ^^^ sfr funtions  ^^^
#endif

            case INT_SET_VARIABLE_TO_VARIABLE:
                Comment("INT_SET_VARIABLE_TO_VARIABLE %s = %s", a->name1.c_str(), a->name2.c_str());
                CopyVarToReg(r16, SizeOfVar(a->name2), a->name2);
                CopyRegToVar(a->name1, r16, SizeOfVar(a->name2));
                break;

            case INT_SET_VARIABLE_MOD:
            case INT_SET_VARIABLE_DIVIDE:
                // Do this one separately since the divide routine uses
                // slightly different in/out registers and I don't feel like
                // modifying it.
                sov = std::max(SizeOfVar(a->name2), SizeOfVar(a->name3));
                CopyArgToReg(r19, sov, a->name2);
                CopyArgToReg(r22, sov, a->name3);
                if(sov == 1) {
                    CallSubroutine(DivideAddress8);
                    DivideUsed8 = true;
                } else if(sov == 2) {
                    CallSubroutine(DivideAddress);
                    DivideUsed = true;
                } else if(sov == 3) {
                    CallSubroutine(DivideAddress24);
                    DivideUsed24 = true;
                } else
                    THROW_COMPILER_EXCEPTION("Invalid variable size.");
                if(a->op == INT_SET_VARIABLE_DIVIDE)
                    CopyRegToVar(a->name1, r19, sov);
                else
                    CopyRegToVar(a->name1, r19, sov); // mod
                break;

            case INT_SET_VARIABLE_MULTIPLY:
                sov = std::max(SizeOfVar(a->name2), SizeOfVar(a->name3));
                CopyArgToReg(r20, sov, a->name2);
                CopyArgToReg(r16, sov, a->name3);
                if(sov == 1) {
                    CallSubroutine(MultiplyAddress8);
                    MultiplyUsed8 = true;
                    sov1 = std::min(2, SizeOfVar(a->name1));
                } else if(sov == 2) {
                    CallSubroutine(MultiplyAddress);
                    MultiplyUsed = true;
                    sov1 = std::min(4, SizeOfVar(a->name1));
                } else if(sov == 3) {
                    CallSubroutine(MultiplyAddress24);
                    MultiplyUsed24 = true;
                    sov1 = std::min(6, SizeOfVar(a->name1));
                } else
                    THROW_COMPILER_EXCEPTION("Invalid variable size.");
                CopyRegToVar(a->name1, r20, sov1);
                break;

            case INT_SET_VARIABLE_ROL:
            case INT_SET_VARIABLE_ROR:
            case INT_SET_VARIABLE_SHL:
            case INT_SET_VARIABLE_SHR:
            case INT_SET_VARIABLE_NOT:
            case INT_SET_VARIABLE_AND:
            case INT_SET_VARIABLE_OR:
            case INT_SET_VARIABLE_XOR:
            case INT_SET_VARIABLE_NEG:
            case INT_SET_VARIABLE_SR0:
            case INT_SET_VARIABLE_ADD:
            case INT_SET_VARIABLE_SUBTRACT: {
                Comment("INT_SET_VARIABLE_xxx %s %s %s", a->name1.c_str(), a->name2.c_str(), a->name3.c_str());

                sov = SizeOfVar(a->name1);
                CopyArgToReg(r20, sov, a->name2);

                if(a->op != INT_SET_VARIABLE_NEG)
                    if(a->op != INT_SET_VARIABLE_NOT)
                        CopyArgToReg(r16, sov, a->name3);

                if(a->op == INT_SET_VARIABLE_ADD) {
                    Instruction(OP_ADD, r20, 16);
                    if(sov >= 2)
                        Instruction(OP_ADC, r21, 17);
                    if(sov >= 3)
                        Instruction(OP_ADC, r22, 18);
                    if(sov >= 4)
                        Instruction(OP_ADC, r23, 19);
                } else if(a->op == INT_SET_VARIABLE_SUBTRACT) {
                    Instruction(OP_SUB, r20, 16);
                    if(sov >= 2)
                        Instruction(OP_SBC, r21, 17);
                    if(sov >= 3)
                        Instruction(OP_SBC, r22, 18);
                    if(sov >= 4)
                        Instruction(OP_SBC, r23, 19);
                } else if(a->op == INT_SET_VARIABLE_NEG) {
                    Instruction(OP_COM, 20, 0);
                    Instruction(OP_SUBI, 20, 0xff);
                    if(sov >= 2) {
                        Instruction(OP_COM, 21, 0);
                        Instruction(OP_SBCI, 21, 0xff); //are equivalent to adding $0001 to the 16-bit number.
                    }
                    if(sov >= 3) {
                        Instruction(OP_COM, 22, 0);
                        Instruction(OP_SBCI, 22, 0xff);
                    }
                    if(sov >= 4) {
                        Instruction(OP_COM, 23, 0);
                        Instruction(OP_SBCI, 23, 0xff);
                    }
                } else if(a->op == INT_SET_VARIABLE_AND) {
                    AndReg(r20, sov, r16);
                } else if(a->op == INT_SET_VARIABLE_OR) {
                    OrReg(r20, sov, r16);
                } else if(a->op == INT_SET_VARIABLE_XOR) {
                    Instruction(OP_EOR, 20, 16);
                    if(sov >= 2)
                        Instruction(OP_EOR, 21, 17);
                    if(sov >= 3)
                        Instruction(OP_EOR, 22, 18);
                    if(sov >= 4)
                        Instruction(OP_EOR, 23, 19);
                } else if(a->op == INT_SET_VARIABLE_NOT) {
                    Instruction(OP_COM, 20, 0);
                    if(sov >= 2)
                        Instruction(OP_COM, 21, 0);
                    if(sov >= 3)
                        Instruction(OP_COM, 22, 0);
                    if(sov >= 4)
                        Instruction(OP_COM, 23, 0);
                } else if((a->op == INT_SET_VARIABLE_SHL) || (a->op == INT_SET_VARIABLE_SHR)
                          || (a->op == INT_SET_VARIABLE_SR0) || (a->op == INT_SET_VARIABLE_ROR)
                          || (a->op == INT_SET_VARIABLE_ROL)) {
                    DWORD Loop = AvrProgWriteP;
                    Instruction(OP_DEC, r16);
                    DWORD Skip = AllocFwdAddr();
                    Instruction(OP_BRMI, Skip, 0);

                    if(a->op == INT_SET_VARIABLE_SHL) {
                        ShlReg(r20, sov);
                    } else if(a->op == INT_SET_VARIABLE_SR0) {
                        Instruction(OP_CLC);
                        if(sov == 1) {
                            Instruction(OP_ROR, 20);
                        } else if(sov == 2) {
                            Instruction(OP_ROR, 21);
                            Instruction(OP_ROR, 20);
                        } else if(sov == 3) {
                            Instruction(OP_ROR, 22);
                            Instruction(OP_ROR, 21);
                            Instruction(OP_ROR, 20);
                        } else if(sov == 4) {
                            Instruction(OP_ROR, 23);
                            Instruction(OP_ROR, 22);
                            Instruction(OP_ROR, 21);
                            Instruction(OP_ROR, 20);
                        } else
                            THROW_COMPILER_EXCEPTION("Invalid variable size.");
                    } else if(a->op == INT_SET_VARIABLE_SHR) {
                        if(sov == 1) {
                            Instruction(OP_ASR, 20);
                        } else if(sov == 2) {
                            Instruction(OP_ASR, 21);
                            Instruction(OP_ROR, 20);
                        } else if(sov == 3) {
                            Instruction(OP_ASR, 22);
                            Instruction(OP_ROR, 21);
                            Instruction(OP_ROR, 20);
                        } else if(sov == 4) {
                            Instruction(OP_ASR, 23);
                            Instruction(OP_ROR, 22);
                            Instruction(OP_ROR, 21);
                            Instruction(OP_ROR, 20);
                        } else
                            THROW_COMPILER_EXCEPTION("Invalid variable size.");
                    } else if(a->op == INT_SET_VARIABLE_ROL) {
                        RolReg(r20, sov);
                    } else if(a->op == INT_SET_VARIABLE_ROR) {
                        Instruction(OP_CLC);
                        if(sov == 1) {
                            Instruction(OP_ROR, 20);
                            IfBitSet(REG_SREG, SREG_C);
                            Instruction(OP_SBR, 20, 0x80);
                        } else if(sov == 2) {
                            Instruction(OP_ROR, 21);
                            Instruction(OP_ROR, 20);
                            IfBitSet(REG_SREG, SREG_C);
                            Instruction(OP_SBR, 21, 0x80);
                        } else if(sov == 3) {
                            Instruction(OP_ROR, 22);
                            Instruction(OP_ROR, 21);
                            Instruction(OP_ROR, 20);
                            IfBitSet(REG_SREG, SREG_C);
                            Instruction(OP_SBR, 22, 0x80);
                        } else if(sov == 4) {
                            Instruction(OP_ROR, 23);
                            Instruction(OP_ROR, 22);
                            Instruction(OP_ROR, 21);
                            Instruction(OP_ROR, 20);
                            IfBitSet(REG_SREG, SREG_C);
                            Instruction(OP_SBR, 23, 0x80);
                        } else
                            THROW_COMPILER_EXCEPTION("Invalid vaiable size.");
                    } else
                        THROW_COMPILER_EXCEPTION("Invalid instruction.");
                    Instruction(OP_RJMP, Loop);
                    FwdAddrIsNow(Skip);
                } else
                    THROW_COMPILER_EXCEPTION("Invalid instruction.");

                CopyRegToVar(a->name1, r20, sov);
                break;
            }
            case INT_QUAD_ENCOD: {
                break;
            }
            case INT_SET_NPULSE: {
            }
            case INT_OFF_NPULSE: {
                break;
            }
            case INT_PWM_OFF: {
                McuPwmPinInfo *iop = PwmPinInfoForName(a->name1.c_str(), Prog.cycleTimer);
                if(!iop) {
                    THROW_COMPILER_EXCEPTION_FMT(_("Pin '%s': PWM output not available!"), a->name1.c_str());
                }
                if(iop->maxCS == 0) {
                    if(REG_TCCR2B > 0) {
                        iop->REG_TCCRnB = REG_TCCR2B;
                    }
                    iop->REG_TCCRnA = REG_TCCR2;
                }

                // No clock source (Timer/CounterN stopped)
                if(iop->REG_TCCRnB > 0)
                    WriteMemory(iop->REG_TCCRnB, 0);
                WriteMemory(iop->REG_TCCRnA, 0);

                MemForSingleBit(a->name1, false, &addr, &bit);
                ClearBit(addr, bit, a->name1);

                char storeName[MAX_NAME_LEN];
                sprintf(storeName, "$pwm_init_%s", a->name1.c_str());
                MemForSingleBit(storeName, false, &addr, &bit);
                ClearBit(addr, bit, storeName);
                break;
            }

            case INT_SET_PWM: {
                //Op(INT_SET_PWM, l->d.setPwm.duty_cycle, l->d.setPwm.targetFreq, l->d.setPwm.name, l->d.setPwm.resolution);
                Comment(
                    "INT_SET_PWM %s %s %s %s", a->name1.c_str(), a->name2.c_str(), a->name3.c_str(), a->name4.c_str());
                int resol = 7; // 0-100% (6.7 bit)
                int TOP = 0xFF;
                getResolution(a->name4.c_str(), &resol, &TOP);

                McuPwmPinInfo *iop;
                iop = PwmPinInfoForName(a->name3.c_str(), Prog.cycleTimer);
                if(!iop) {
                    THROW_COMPILER_EXCEPTION_FMT(_("Pin '%s': PWM output not available!"), a->name3.c_str());
                } else {
                    iop = PwmPinInfoForName(a->name3.c_str(), Prog.cycleTimer, std::max(resol, 8));
                    if(!iop) {
                        THROW_COMPILER_EXCEPTION_FMT(_("Pin '%s': PWM resolution not available!"), a->name3.c_str());
                    }
                }
                if(iop->maxCS == 0) {
                    if(REG_TCCR2B > 0) {
                        iop->REG_TCCRnB = REG_TCCR2B;
                        iop->maxCS = 7;
                    } else {
                        iop->maxCS = 5;
                    }
                    iop->REG_TCCRnA = REG_TCCR2;

                    iop->REG_OCRnxH = 0;
                    iop->REG_OCRnxL = REG_OCR2;
                }

                double target = hobatoi(a->name2.c_str());
                // PWM frequency is
                //   target = xtal/(256*prescale)
                // so not a lot of room for accurate frequency here

                int      prescale;
                long int bestPrescale;
                long int bestError = LONG_MAX;
                double   bestFreq;
                double   freq;
                double   freqSI;
                char     SI[10];
                char     freqStr[1024] = "";
                for(prescale = 1;;) {
                    freq = (1.0 * Prog.mcuClock) / (1.0 * TOP * prescale);

                    freqSI = SIprefix(freq, SI);
                    sprintf(freqStr, "%s%.3f %sHz    ", freqStr, freqSI, SI);

                    long int err = (long int)fabs(freq - target);
                    if(err < bestError) {
                        bestError = err;
                        bestPrescale = prescale;
                        bestFreq = freq;
                    }
                    if(iop->maxCS == 7) {
                        if(prescale == 1) {
                            prescale = 8;
                        } else if(prescale == 8) {
                            prescale = 32;
                        } else if(prescale == 32) {
                            prescale = 64;
                        } else if(prescale == 64) {
                            prescale = 128;
                        } else if(prescale == 128) {
                            prescale = 256;
                        } else if(prescale == 256) {
                            prescale = 1024;
                        } else {
                            break;
                        }
                    } else if(iop->maxCS == 15) {
                        if(prescale == 1) {
                            prescale = 2;
                        } else if(prescale == 2) {
                            prescale = 4;
                        } else if(prescale == 4) {
                            prescale = 8;
                        } else if(prescale == 8) {
                            prescale = 16;
                        } else if(prescale == 16) {
                            prescale = 32;
                        } else if(prescale == 32) {
                            prescale = 64;
                        } else if(prescale == 64) {
                            prescale = 128;
                        } else if(prescale == 128) {
                            prescale = 256;
                        } else if(prescale == 256) {
                            prescale = 512;
                        } else if(prescale == 512) {
                            prescale = 1024;
                        } else if(prescale == 1024) {
                            prescale = 2048;
                        } else if(prescale == 2048) {
                            prescale = 4096;
                        } else if(prescale == 4096) {
                            prescale = 8192;
                        } else if(prescale == 8192) {
                            prescale = 16384;
                        } else {
                            break;
                        }
                    } else if(iop->maxCS == 5) {
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
                    } else
                        THROW_COMPILER_EXCEPTION("Can't set prescale.");
                }

                if(((double)bestError) / target > 0.05) {
                    char str1[1024];
                    char str2[1024];
                    sprintf(str1,
                            _("Target PWM frequency %d Hz, closest achievable is "
                              "%d Hz (warning, >5%% error)."),
                            (int)target,
                            (int)bestFreq);
                    //need duble %
                    char *c = strchr(str1, '%');
                    char *s = str1 + strlen(str1) + 1;
                    *s = 0;
                    while(s != c) {
                        s--;
                        *(s + 1) = *s;
                    }

                    sprintf(str2,
                            "'%s' %s"
                            "\n\n"
                            "%s"
                            "\n"
                            "%s",
                            a->name3.c_str(),
                            str1,
                            _("Select the frequency from the possible values:"),
                            freqStr);
                    THROW_COMPILER_EXCEPTION(str2);
                }

                BYTE cs;
                if(iop->maxCS == 7) {
                    switch(bestPrescale) {
                        case 1:
                            cs = 1;
                            break;
                        case 8:
                            cs = 2;
                            break;
                        case 32:
                            cs = 3;
                            break;
                        case 64:
                            cs = 4;
                            break;
                        case 128:
                            cs = 5;
                            break;
                        case 256:
                            cs = 6;
                            break;
                        case 1024:
                            cs = 7;
                            break;
                        default:
                            THROW_COMPILER_EXCEPTION("Invalid prescale.");
                    }
                } else if(iop->maxCS == 5) {
                    switch(bestPrescale) {
                        case 1:
                            cs = 1;
                            break;
                        case 8:
                            cs = 2;
                            break;
                        case 64:
                            cs = 3;
                            break;
                        case 256:
                            cs = 4;
                            break;
                        case 1024:
                            cs = 5;
                            break;
                        default:
                            THROW_COMPILER_EXCEPTION("Invalid prescale.");
                    }
                } else
                    THROW_COMPILER_EXCEPTION("Can't set PWM.");

                if(resol == 7) {
                    DivideUsed = true;
                    MultiplyUsed = true;
                    CopyArgToReg(r20, 2, a->name1);
                    CopyLitToReg(r16, 2, 255); // Fast PWM

                    CallSubroutine(MultiplyAddress);

                    Instruction(OP_MOV, 19, 20);
                    Instruction(OP_MOV, 20, 21);

                    CopyLitToReg(r22, 2, 100);
                    CallSubroutine(DivideAddress);
                    // result in r20:r19
                } else {
                    CopyArgToReg(r19, 2, a->name1);
                }

                if(iop->REG_OCRnxH)
                    WriteRegToIO(iop->REG_OCRnxH, r20); // first HIGH
                WriteRegToIO(iop->REG_OCRnxL, r19);     // then LOW

                // Setup only happens once
                Comment("PWM init");
                char storeName[MAX_NAME_LEN];
                sprintf(storeName, "$pwm_init_%s", a->name3.c_str());
                DWORD addr;
                int   bit;
                MemForSingleBit(storeName, false, &addr, &bit);

                DWORD endInit = AllocFwdAddr();
                IfBitSet(addr, bit);
                Instruction(OP_RJMP, endInit, 0);
                SetBit(addr, bit, storeName);

                // Prescaler Reset Timer/Counter2
                if(iop->timer == 2) {
                    if(REG_SFIOR)
                        SetBit(REG_SFIOR, PSR2);
                    else if(REG_GTCCR)
                        SetBit(REG_GTCCR, PSRASY);
                }
                // see Timer/Counter2
                // fast PWM mode, non-inverting or inverting mode, given prescale
                if(iop->REG_TCCRnB > 0) {
                    if(iop->COMnx1) {
                        WriteMemory(iop->REG_TCCRnB, iop->WGMb | cs);
                        AndMemory(iop->REG_TCCRnA, ~(iop->WGMa | (1 << iop->COMnx1) | (1 << iop->COMnx0)));
                        OrMemory(iop->REG_TCCRnA,
                                 iop->WGMa | (1 << iop->COMnx1) | (a->name2[0] == '/' ? (1 << iop->COMnx0) : 0));
                    } else {
                        WriteMemory(REG_TCCR2B, cs);
                        WriteMemory(REG_TCCR2,
                                    (1 << WGM20) | (1 << WGM21) | (1 << COM21)
                                        | (a->name2[0] == '/' ? (1 << COM20) : 0));
                    }
                } else {
                    //TODO: test registers and bits define's
                    if(iop->COMnx1) {
                        WriteMemory(iop->REG_TCCRnA,
                                    iop->WGMa | (1 << iop->COMnx1) | (a->name2[0] == '/' ? (1 << iop->COMnx0) : 0)
                                        | cs);
                    } else {
                        WriteMemory(REG_TCCR2,
                                    (1 << WGM20) | (1 << WGM21) | (1 << COM21) | (a->name2[0] == '/' ? (1 << COM20) : 0)
                                        | cs);
                    }
                }
                FwdAddrIsNow(endInit);
                break;
            }
#if 0
            case INT_EEPROM_BUSY_CHECK: {
                MemForSingleBit(a->name1, false, &addr, &bit);

                DWORD isBusy = AllocFwdAddr();
                DWORD done = AllocFwdAddr();
                IfBitSet(REG_EECR, EEWE);
                Instruction(OP_RJMP, isBusy, 0);

              //IfBitClear(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
              //Instruction(OP_RJMP, done, 0);
                LoadXAddr(EepromHighBytesCounter);
                Instruction(OP_LD_X, r23); //r23 as EepromHighBytesCounter
                Instruction(OP_TST,  r23, 0);
                Instruction(OP_BREQ, done);
                Instruction(OP_DEC,  r23);
                Instruction(OP_ST_X, r23);

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
                Instruction(OP_LDI, 16, (1 << EEMWE)); // 0x04
                Instruction(OP_ST_X, 16, 0);
                Instruction(OP_LDI, 16, (1 << EEMWE) | (1 << EEWE)); // 0x06
                Instruction(OP_ST_X, 16, 0);

                FwdAddrIsNow(isBusy);
                SetBit(addr, bit);
                FwdAddrIsNow(done);
                break;
            }
#else
            case INT_EEPROM_BUSY_CHECK: {
                MemForSingleBit(a->name1, false, &addr, &bit);

                DWORD isBusy = AllocFwdAddr();
                DWORD done = AllocFwdAddr();
                IfBitSet(REG_EECR, EEWE);
                Instruction(OP_RJMP, isBusy);

                LoadXAddr(EepromHighBytesCounter);
                Instruction(OP_LD_X, r23); //r23 as EepromHighBytesCounter
                Instruction(OP_TST, r23, 0);
                Instruction(OP_BREQ, done);
                Instruction(OP_DEC, r23);
                Instruction(OP_ST_X, r23);

                // Just increment EEARH:EEARL, to point to the high byte of
                // whatever we just wrote the low byte for.
                LoadXAddr(REG_EEARL);
                Instruction(OP_LD_X, 16);
                Instruction(OP_INC, 16);
                DWORD noCarry = AllocFwdAddr();
                Instruction(OP_BRNE, noCarry, 0);
                LoadYAddr(REG_EEARH);
                Instruction(OP_LD_Y, 17);
                Instruction(OP_INC, 17);
                // Y is still REG_EEARH
                Instruction(OP_ST_Y, 17);
                FwdAddrIsNow(noCarry);
                // X is still REG_EEARL
                Instruction(OP_ST_X, 16);
                //
                LoadXAddr(EepromHighByte);

                Instruction(OP_TST, r23, 0); //r23 still EepromHighBytesCounter-1
                DWORD doOut = AllocFwdAddr();
                Instruction(OP_BRNE, doOut, 0);
                Instruction(OP_LD_XP, r16); //increment addres EepromHighByte in X register
                                            //skip middle byte
                FwdAddrIsNow(doOut);
                Instruction(OP_LD_X, r16); //EepromHighByte data to r16
                LoadXAddr(REG_EEDR);
                Instruction(OP_ST_X, 16);
                LoadXAddr(REG_EECR);
                Instruction(OP_LDI, 16, 0x04);
                Instruction(OP_ST_X, 16);
                Instruction(OP_LDI, 16, 0x06);
                Instruction(OP_ST_X, 16);

                FwdAddrIsNow(isBusy);
                SetBit(addr, bit);
                FwdAddrIsNow(done);
                break;
            }
#endif
#if 0
            case INT_EEPROM_READ: {
                MemForVariable(a->name1, &addr1);
                int i;
                for(i = 0; i < 2; i++) {
                    WriteMemory(REG_EEARH, BYTE((a->literal+i) >> 8) & 0xff);
                    WriteMemory(REG_EEARL, BYTE((a->literal+i) & 0xff));
                    WriteMemory(REG_EECR, 1 << EERE);
                    LoadXAddr(REG_EEDR);
                    Instruction(OP_LD_X, 16, 0);
                    if(i == 0) {
                        LoadXAddr(addr1);
                    } else {
                        LoadXAddr(addr1+1);
                    }
                    Instruction(OP_ST_X, 16, 0);
                }
                break;
            }
#else
            case INT_EEPROM_READ: {
                MemForVariable(a->name1, &addr1);
                LoadXAddr(addr1);
                LoadYAddr(REG_EEDR);
                sov = SizeOfVar(a->name1);
                int i;
                for(i = 0; i < sov; i++) {
                    WriteMemory(REG_EEARH, BYTE(((a->literal + i) >> 8) & 0xff));
                    WriteMemory(REG_EEARL, BYTE((a->literal + i) & 0xff));
                    WriteMemory(REG_EECR, 0x01);
                    Instruction(OP_LD_Y, 16);
                    Instruction(OP_ST_XP, 16);
                }
                break;
            }
#endif
#if 0
            case INT_EEPROM_WRITE:
                MemForVariable(a->name1, &addr1);
                SetBit(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
                LoadXAddr(addr1+1);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(EepromHighByte);
                Instruction(OP_ST_X, 16, 0);

                WriteMemory(REG_EEARH, BYTE(a->literal >> 8) & 0xff);
                WriteMemory(REG_EEARL, BYTE(a->literal & 0xff));
                LoadXAddr(addr1);
                Instruction(OP_LD_X, 16, 0);
                LoadXAddr(REG_EEDR);
                Instruction(OP_ST_X, 16, 0);
                LoadXAddr(REG_EECR);
                Instruction(OP_LDI, 16, (1 << EEMWE)); // 0x04
                Instruction(OP_ST_X, 16, 0);
                Instruction(OP_LDI, 16, (1 << EEMWE) | (1 << EEWE)); // 0x06
                Instruction(OP_ST_X, 16, 0);
                break;
#else
            case INT_EEPROM_WRITE:
                sov = SizeOfVar(a->name1);
                Instruction(OP_LDI, 16, sov - 1);
                LoadYAddr(EepromHighBytesCounter);
                Instruction(OP_ST_Y, 16);

                if(sov >= 2) {
                    MemForVariable(a->name1, &addr1);
                    LoadXAddr(addr1 + 1);
                    LoadYAddr(EepromHighByte);
                    Instruction(OP_LD_XP, 16);
                    Instruction(OP_ST_YP, 16);
                    if(sov >= 3) {
                        Instruction(OP_LD_XP, 16);
                        Instruction(OP_ST_YP, 16);
                        if(sov >= 4) {
                            Instruction(OP_LD_XP, 16);
                            Instruction(OP_ST_YP, 16);
                        }
                    }
                }
                break;
#endif
            case INT_READ_ADC: {
                MemForVariable(a->name1, &addr1);

                BYTE mux = MuxForAdcVariable(a->name1);
                if(mux > 0x0F)
                    THROW_COMPILER_EXCEPTION_FMT("mux=0x%x", mux);
                WriteMemory(
                    REG_ADMUX,
                    (0 << 7) |
                        //                  (0 << 6) |              // AREF, Internal Vref turned off.
                        (1 << 6)
                        | // AVCC as reference with external capacitor 100nF at AREF to GND pin. // Arduino compatible.
                        (0 << 5) | // result is right adjusted.
                        mux & 0x07);

                if(REG_ADCSRB) {
                    WriteMemory(REG_ADCSRB, 0 << ACME);
                    //ClearBit(REG_ADCSRB, ACME);
                    if(mux & 0x08) {
                        SetBit(REG_ADCSRB, MUX5);
                    }
                }

                // target something around 200 kHz for the ADC clock, for
                // 25/(200k) or 125 us conversion time, reasonable
                int divisor = (Prog.mcuClock / 200000);
                int adps = 0;
                for(adps = 1; adps <= 7; adps++) {
                    if((1 << adps) > divisor)
                        break;
                }
                BYTE adcsra = (BYTE)((1 << ADEN) | // ADC enabled
                                     (0 << ADFR) | // not free running
                                     (0 << ADIE) | // no interrupt enabled
                                     adps);        // prescale setup

                WriteMemory(REG_ADCSRA, adcsra);
                WriteMemory(REG_ADCSRA, (BYTE)(adcsra | (1 << ADSC)));

                DWORD waitForFinsh = AvrProgWriteP;
                IfBitSet(REG_ADCSRA, ADSC);
                Instruction(OP_RJMP, waitForFinsh);

                LoadXAddr(REG_ADCL);
                Instruction(OP_LD_X, 16);
                LoadYAddr(addr1);
                Instruction(OP_ST_YP, 16);

                LoadXAddr(REG_ADCH);
                Instruction(OP_LD_X, 16);
                Instruction(OP_ST_YP, 16);

                break;
            }
            case INT_UART_SEND_READY: {
                Comment("INT_UART_SEND_READY");
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                GetUartSendReady(addr1, bit1);
                break;
            }
            case INT_UART_SEND_BUSY: {
                Comment("INT_UART_SEND_BUSY");
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                GetUartSendBusy(addr1, bit1);
                break;
            }
            case INT_UART_RECV_AVAIL: {
                Comment("INT_UART_RECV_AVAIL");
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                CopyBit(addr1, bit1, REG_UCSRA, RXC);
                break;
            }

            case INT_UART_SEND: {
                Comment("INT_UART_SEND");
                MemForVariable(a->name1, &addr1);
                MemForSingleBit(a->name2, true, &addr2, &bit2);

                DWORD noSend = AllocFwdAddr();
                IfBitClear(addr2, bit2);
                Instruction(OP_RJMP, noSend);

                DWORD isBusy = AllocFwdAddr();
                //IfBitClear(REG_UCSRA, UDRE); // UDRE, is 1 when tx buffer is empty, if 0 is busy
                //Instruction(OP_RJMP, isBusy);

                LoadXAddr(addr1);
                Instruction(OP_LD_X, r16);
                LoadXAddr(REG_UDR);
                Instruction(OP_ST_X, r16);

                FwdAddrIsNow(noSend);

                CopyNotBit(addr2, bit2, REG_UCSRA, UDRE); // UDRE, is 1 when tx buffer is empty, if 0 is busy

                FwdAddrIsNow(isBusy);
                break;
            }
            case INT_UART_SEND1: {
                Comment("INT_UART_SEND1");
                MemForVariable(a->name1, &addr1);

                DWORD isBusy = AllocFwdAddr();
                IfBitClear(REG_UCSRA, UDRE); // UDRE, is 1 when tx buffer is empty, if 0 is busy
                Instruction(OP_RJMP, isBusy);

                LoadXAddr(addr1);
                Instruction(OP_LD_X, r16);
                LoadXAddr(REG_UDR);
                Instruction(OP_ST_X, r16);

                FwdAddrIsNow(isBusy);
                break;
            }
            case INT_UART_RECV: {
                //Receive one char/byte in a single PLC cycle.
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);
                MemForSingleBit(a->name2, true, &addr2, &bit2);

                ClearBit(addr2, bit2);

                DWORD noChar = AllocFwdAddr();
                IfBitClear(REG_UCSRA, RXC);
                Instruction(OP_RJMP, noChar);

                SetBit(addr2, bit2);
                LoadXAddr(REG_UDR);
                Instruction(OP_LD_X, r16);
                LoadXAddr(addr1);
                Instruction(OP_ST_XP, r16);

                Instruction(OP_LDI, r16, 0);
                for(int i = 1; i < sov1; i++)
                    Instruction(OP_ST_XP, r16);

                FwdAddrIsNow(noChar);
                break;
            }
            case INT_END_IF:
            case INT_ELSE:
                return;

            case INT_WRITE_STRING:
                THROW_COMPILER_EXCEPTION(_("Unsupported operation 'INT_WRITE_STRING' for target, skipped."));
            case INT_SIMULATE_NODE_STATE:
                break;

            case INT_COMMENT:
                Comment("%s", a->name1.c_str());
                break;

            case INT_AllocKnownAddr:
                //Comment("INT_AllocKnownAddr %d %08X", a->literal, AddrOfRungN[a->literal].KnownAddr);
                AddrOfRungN[a->literal].KnownAddr = AvrProgWriteP;
                break;

            case INT_AllocFwdAddr:
                //Comment("INT_AllocFwdAddr %d %08X", a->literal, AddrOfRungN[a->literal].FwdAddr);
                AddrOfRungN[a->literal].FwdAddr = AllocFwdAddr();
                break;

            case INT_FwdAddrIsNow:
                //Comment("INT_FwdAddrIsNow %d %08x", a->literal, AddrOfRungN[a->literal].FwdAddr);
                FwdAddrIsNow(AddrOfRungN[a->literal].FwdAddr);
                break;

            case INT_RETURN:
                Instruction(OP_RET);
                break;

            case INT_GOTO: {
                int rung = a->literal;
                Comment("INT_GOTO %s %d 0x%08X 0x%08X",
                        a->name1.c_str(),
                        rung,
                        AddrOfRungN[rung].FwdAddr,
                        AddrOfRungN[rung].KnownAddr);
                DWORD addr;
                if(rung < -1) {
                    addr = 0;
                } else if(rung == -1) {
                    addr = BeginOfPLCCycle;
                } else if(rung <= rungNow) {
                    addr = AddrOfRungN[rung].KnownAddr;
                } else {
                    addr = AddrOfRungN[rung].FwdAddr;
                }
                InstructionJMP(addr);
                break;
            }
            case INT_GOSUB: {
                int rung = a->literal;
                Comment("INT_GOSUB %s %d %d 0x%08X 0x%08X",
                        a->name1.c_str(),
                        rung + 1,
                        rungNow + 1,
                        AddrOfRungN[rung].FwdAddr,
                        AddrOfRungN[rung].KnownAddr);
                if(rung < rungNow) {
                    CallSubroutine(AddrOfRungN[rung].KnownAddr);
                } else if(rung > rungNow) {
                    CallSubroutine(AddrOfRungN[rung].FwdAddr);
                } else
                    THROW_COMPILER_EXCEPTION("Can't instantiate GOSUB.");
                break;
            }
#ifdef TABLE_IN_FLASH
            case INT_FLASH_INIT: {
                // Inited by InitTables()
                /*
                DWORD addrOfTable = 0;
                MemOfVar(a->name1, &addrOfTable);

                if(addrOfTable == 0) {
                    DWORD SkipData = AllocFwdAddr();
                    InstructionJMP(SkipData);
                    InitTable(a);
                    FwdAddrIsNow(SkipData);
                }
                */
                break;
            }
            case INT_FLASH_READ: {
                Comment("INT_FLASH_READ");
                int sovElement = a->literal2;
                //sovElement = 1;
                CopyArgToReg(r16, 2, a->name3);
                //Comment("Index in r16:r17");

                if(sovElement == 3) {
                    Instruction(OP_MOV, r14, r16);
                    Instruction(OP_MOV, r15, r17); // Save Index
                }
                if(sovElement >= 2) {
                    Instruction(OP_LSL, r16);
                    Instruction(OP_ROL, r17); // Index := Index * 2
                }
                if(sovElement == 3) {
                    Instruction(OP_ADD, r16, r14);
                    Instruction(OP_ADC, r17, r15); // Index := Index * 3
                }
                if(sovElement == 4) {
                    Instruction(OP_LSL, r16);
                    Instruction(OP_ROL, r17); // Index := Index * 4
                }

                DWORD addrOfTable = 0;
                MemOfVar(a->name2, &addrOfTable);

                CopyLitToReg(ZL, 2, addrOfTable << 1, "addrOfTable"); // see LPM // data stored in flash
                //Comment(" Z == DataAddr");

                Instruction(OP_ADD, ZL, r16);
                Instruction(OP_ADC, ZH, r17); // Z = DataAddr + Index
                //Comment(" Z == DataAddr + Index");

                Instruction(OP_LPM_ZP, r20);
                if(sovElement >= 2)
                    Instruction(OP_LPM_ZP, r21);
                if(sovElement >= 3)
                    Instruction(OP_LPM_ZP, r22);
                if(sovElement >= 4)
                    Instruction(OP_LPM_ZP, r23);
                CopyRegToVar(a->name1, r20, sovElement);
                break;
            }
#endif
            case INT_SET_VARIABLE_RANDOM: {
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);

                char seedName[MAX_NAME_LEN];
                sprintf(seedName, "$seed_%s", a->name1.c_str());
                SetSizeOfVar(seedName, 4);
                MemForVariable(seedName, &addr2);
                /*
                DWORD addr;
                int bit;
                char storeName[MAX_NAME_LEN];
                sprintf(storeName, "$seed_init_%s", a->name1.c_str());
                MemForSingleBit(storeName, false, &addr, &bit);
                */

                //https://en.m.wikipedia.org/wiki/Linear_congruential_generator
                // X[n+1] = (a * X[n] + c) mod m

                //VMS's MTH$RANDOM, old versions of glibc
                // a = 69069 ( 0x10DCD ) (1 00001101 11001101b)
                // c = 1
                // m = 2^32
                // X = (X * 0x10DCD + 1) % 0x100000000
                //-----------------------------------------------------------------------------
                // 32x24=32 unsigned multiply, code from Atmel app note AVR201.
                //           h:  M:  m:  l
                // seed in r19:r18:r17:r16,
                // a    in r23:r22:r21:r20, r23 == 0, r22 == 1,
                // temp result 4 low bytes goes into r13:r12:r11:r10.
                //-----------------------------------------------------------------------------
                //CopyLitToReg(r20, 4, 0x00010DCD, "a:=69069(0x10DCD)");
                CopyLitToReg(r20, 2, 0x0DCD, "a:=69069(0x10DCD)");
                CopyVarToReg(r16, 4, seedName);

                Instruction(OP_SUB, r2, r2);    // zero
                Instruction(OP_MOVW, r12, r16); // r12 = r16 * 2^16
                //
                Instruction(OP_MUL, r16, r20); //l * l
                Instruction(OP_MOVW, r10, r0);

                Instruction(OP_MUL, r17, r20); //m * l
                Instruction(OP_ADD, r11, r0);
                Instruction(OP_ADC, r12, r1);
                Instruction(OP_ADC, r13, r2);

                Instruction(OP_MUL, r18, r20); //M * l
                Instruction(OP_ADD, r12, r0);
                Instruction(OP_ADC, r13, r1);
                Instruction(OP_ADC, r13, r2);

                Instruction(OP_MUL, r19, r20); //h * l
                Instruction(OP_ADD, r13, r0);
                //
                Instruction(OP_MUL, r21, r16); //m * l
                Instruction(OP_ADD, r11, r0);
                Instruction(OP_ADC, r12, r1);
                Instruction(OP_ADC, r13, r2);

                Instruction(OP_MUL, r21, r17); //m * m
                Instruction(OP_ADD, r12, r0);
                Instruction(OP_ADC, r13, r1);
                Instruction(OP_ADC, r13, r2);

                Instruction(OP_MUL, r21, r18); //m * M
                Instruction(OP_ADD, r13, r0);
                //
                IncrementReg(r10, 4); // (seedName + 1) % 2^32

                CopyRegToVar(seedName, r10, 4);
                CopyRegToVar(a->name1, r10 + 4 - sov1, sov1); // highest bytes of seed
                break;
            }
            case INT_DELAY: {
// #define DELAY_TEST
#ifdef DELAY_TEST
                SetBit(0x25, 0);   // 2 clocks
                ClearBit(0x25, 0); // 2 clocks
                SetBit(0x25, 0);   // 2 clocks
                ClearBit(0x25, 0); // 2 clocks
#endif
                if(IsNumber(a->name1)) {
                    SDWORD clocks = CalcDelayClock(hobatoi(a->name1.c_str()));
                    SDWORD clocksSave = clocks;
                    Comment("INT_DELAY %s us = %lld clocks", a->name1.c_str(), clocks);

                    clocks = (clocks - 1) / 4;
                    if(clocks > 0x10000) {
                        clocks = 0x10000;
                        clocksSave = clocks * 4;
                        THROW_COMPILER_EXCEPTION_FMT(_(" The delay is too long!\n"
                                "The maximum possible delay is %lld us."),
                              (clocks * 4 + 1) * 1000000 / Prog.mcuClock);
                    }
                    if(clocks < 0)
                        clocks = 0;
                    if(clocks > 0) {
                        CopyLitToReg(ZL, 2, clocks);             // 4 clocks
                        Instruction(OP_SBIW, ZL, 1);             // 2 clocks
                        Instruction(OP_BRNE, AvrProgWriteP - 1); // 1/2 clocks
                        clocksSave -= clocks * 4 + 1;
                    }
                    int i;
                    for(i = 0; i < clocksSave; i++)
                        Instruction(OP_NOP); // 1 clocks
                } else {
                    Comment("INT_DELAY %s us", a->name1.c_str());
                    CopyVarToReg(ZL, 2, a->name1);           // 4 clocks
                    Instruction(OP_SBIW, ZL, 1);             // 2 clocks
                    Instruction(OP_BRNE, AvrProgWriteP - 1); // 1/2 clocks
                }
#ifdef DELAY_TEST
                SetBit(0x25, 0);   // 2 clocks
                ClearBit(0x25, 0); // 2 clocks
                SetBit(0x25, 0);   // 2 clocks
                                   //ClearBit(0x25,0); // 2 clocks
#endif
                break;
            }
            case INT_CLRWDT:
                Instruction(OP_WDR);
                break;
            case INT_LOCK:
                Instruction(OP_RJMP, AvrProgWriteP);
                break;
            case INT_SLEEP:
                if(REG_MCUCSR)
                    ClearBit(REG_MCUCSR, SM2); //Power-down Sleep Mode
                else
                    ClearBit(REG_sleep, SM2); //Power-down Sleep Mode

                SetBit(REG_sleep, SM1); //Power-down Sleep Mode

                if(REG_EMCUCR)
                    ClearBit(REG_EMCUCR, SM0); //Power-down Sleep Mode
                else
                    ClearBit(REG_sleep, SM0); //Power-down Sleep Mode

                SetBit(REG_sleep, SE); //Sleep Enable

                /*
                if(REG_EICRB)
                    WriteMemory(REG_EICRB, 0); // configured as a level interrupt
                if(REG_EICRA)
                    WriteMemory(REG_EICRA, 0); // configured as a level interrupt
                */
                Instruction(OP_CLI);
                ClearBit(REG_int_sup, ISC00); // configured as a level interrupt
                ClearBit(REG_int_sup, ISC01); // ...
                ClearBit(REG_int_sup, ISC10); // ...
                ClearBit(REG_int_sup, ISC11); // configured as a level interrupt
                /*
                if(REG_EIFR) {
                    WriteMemory(REG_EIFR, (1<<INTF1) | (1<<INTF0)); // the flag can be cleared by writing a logical one to it
                } else if(REG_GICR) {
                    WriteMemory(REG_GICR, (1<<INTF1) | (1<<INTF0));
                } else oops();
                */
                SetBit(REG_int_flag, INTF0); // the flag can be cleared by writing a logical one to it
                SetBit(REG_int_flag, INTF1);
                /*
                if(REG_EIMSK) {
                    WriteMemory(REG_EIMSK, (1<<INT1) | (1<<INT0)); // the external pin interrupt is enabled
                } else if(REG_GICR) {
                    WriteMemory(REG_GICR, (1<<INT1) | (1<<INT0));
                } else oops();
                */
                SetBit(REG_int_en, INT0); // the external pin interrupt is enabled
                SetBit(REG_int_en, INT1);
                /**/
                Instruction(OP_SEI);
                Instruction(OP_SLEEP); // stopped here
                Instruction(OP_CLI);
                /*
                if(REG_EIMSK) {
                    AndMemory(REG_EIMSK, ~((1<<INT1) | (1<<INT0))); // the external pin interrupt is disabled
                } else if(REG_GICR) {
                    AndMemory(REG_GICR, ~((1<<INT1) | (1<<INT0)));
                } else oops();
                */
                ClearBit(REG_int_en, INT0); // the external pin interrupt is disabled
                ClearBit(REG_int_en, INT1);
                break;
            default:
                THROW_COMPILER_EXCEPTION_FMT("INT_%d", a->op);
                break;
        }
    }
}
#ifndef USE_MUL
//-----------------------------------------------------------------------------
// 16 x 16 = 32 Signed Multiplication - "mpy16s"
// 16x16 signed multiply, code from Atmel app note AVR200.
// op1 in r21:r20,
// op2 in r17:r16, result low word goes into r21:r20.
// Signed 32bit result goes into     r23:r22:r21:r20.
//-----------------------------------------------------------------------------
static void MultiplyRoutine() //5.1 Algorithm Description
{
    Comment("MultiplyRoutine16");
    FwdAddrIsNow(MultiplyAddress);
    Instruction(OP_SUB, r23, r23); //1.Clear result High word (Bytes 2&3) and carry.
    Instruction(OP_SUB, r22, r22); //1.Clear result High word (Bytes 2&3) and carry.
    Instruction(OP_LDI, r25, 16);  //2.Load Loop counter with 16.
    DWORD m16s_1 = AvrProgWriteP;
    DWORD m16s_2 = AllocFwdAddr();
    Instruction(OP_BRCC, m16s_2, 0); //3.If carry (previous bit 0 of multiplier Low byte) set,
    Instruction(OP_ADD, r22, r16);   //  add multiplicand to result High word.
    Instruction(OP_ADC, r23, r17);   //  add multiplicand to result High word.
    FwdAddrIsNow(m16s_2);
    Instruction(OP_SBRC, r20, BIT0); //4.If current bit 0 of multiplier Low byte set,
    Instruction(OP_SUB, r22, r16);   //  subtract multiplicand from result High word.
    Instruction(OP_SBRC, r20, BIT0); //4.If current bit 0 of multiplier Low byte set,
    Instruction(OP_SBC, r23, r17);   //  subtract multiplicand from result High word.
    Instruction(OP_ASR, r23);        //5.Shift right result High word into result Low word/multiplier.
    Instruction(OP_ROR, r22);        //5.Shift right result High word into result Low word/multiplier.
    Instruction(OP_ROR, r21);        //6.Shift right Low word/multiplier.
    Instruction(OP_ROR, r20);        //6.Shift right Low word/multiplier.
    Instruction(OP_DEC, r25);        //7.Decrement Loop counter.
    Instruction(OP_BRNE, m16s_1, 0); //8.If Loop counter not zero, go to Step 3.
    Instruction(OP_RET);
}
//-----------------------------------------------------------------------------
// 24 x 24 = 48 Signed Multiplication - "mpy24s" is modified "mpy16s"
// 24x24 signed multiply, code from Atmel app note AVR200.
// op1 in r22:r21:r20,
// op2 in r18:r17:r16, result 3 low bytes goes into r22:r21:r20.
// Signed 48bit result goes into        r25:r24:r23:r22:r21:r20.
//-----------------------------------------------------------------------------
static void MultiplyRoutine24() //5.1 Algorithm Description
{
    Comment("MultiplyRoutine24");
    FwdAddrIsNow(MultiplyAddress24);
    Instruction(OP_SUB, r25, r25); //1.Clear result High word (Bytes 3&4&5) and carry.
    Instruction(OP_SUB, r24, r24); //1.Clear result High word (Bytes 3&4&5) and carry.
    Instruction(OP_SUB, r23, r23); //1.Clear result High word (Bytes 3&4&5) and carry.
    Instruction(OP_LDI, r19, 24);  //2.Load Loop counter with 24.
    DWORD m16s_1 = AvrProgWriteP;
    DWORD m16s_2 = AllocFwdAddr();
    Instruction(OP_BRCC, m16s_2, 0); //3.If carry (previous bit 0 of multiplier Low byte) set,
    Instruction(OP_ADD, r23, r16);   //  add multiplicand to result High word.
    Instruction(OP_ADC, r24, r17);   //  add multiplicand to result High word.
    Instruction(OP_ADC, r25, r18);   //  add multiplicand to result High word.
    FwdAddrIsNow(m16s_2);
    Instruction(OP_SBRC, r20, BIT0); //4.If current bit 0 of multiplier Low byte set,
    Instruction(OP_SUB, r23, r16);   //  subtract multiplicand from result High word.
    Instruction(OP_SBRC, r20, BIT0); //4.If current bit 0 of multiplier Low byte set,
    Instruction(OP_SBC, r24, r17);   //  subtract multiplicand from result High word.
    Instruction(OP_SBRC, r20, BIT0); //4.If current bit 0 of multiplier Low byte set,
    Instruction(OP_SBC, r25, r18);   //  subtract multiplicand from result High word.
    Instruction(OP_ASR, r25);        //5.Shift right result High word into result Low word/multiplier.
    Instruction(OP_ROR, r24);        //5.Shift right result High word into result Low word/multiplier.
    Instruction(OP_ROR, r23);        //5.Shift right result High word into result Low word/multiplier.
    Instruction(OP_ROR, r22);        //6.Shift right Low word/multiplier.
    Instruction(OP_ROR, r21);        //6.Shift right Low word/multiplier.
    Instruction(OP_ROR, r20);        //6.Shift right Low word/multiplier.
    Instruction(OP_DEC, r19);        //7.Decrement Loop counter.
    Instruction(OP_BRNE, m16s_1, 0); //8.If Loop counter not zero, go to Step 3.
    Instruction(OP_RET);
}
//-----------------------------------------------------------------------------
// 8 x 8 = 16 Signed Multiplication - "mpy8s" is modified "mpy16s"
// 8x8 signed multiply, code from Atmel app note AVR200.
// op1 in r20,
// op2 in r16, result word goes into r21:r20.
//-----------------------------------------------------------------------------
static void MultiplyRoutine8() //5.1 Algorithm Description
{
    Comment("MultiplyRoutine8");
    FwdAddrIsNow(MultiplyAddress8);
    Instruction(OP_SUB, r21, r21); //1.Clear result High byte and carry.
    Instruction(OP_LDI, r25, 8);   //2.Load Loop counter with 8.
    DWORD m8s_1 = AvrProgWriteP;
    DWORD m8s_2 = AllocFwdAddr();
    Instruction(OP_BRCC, m8s_2, 0); //3.If carry (previous bit 0 of multiplier Low byte) set,
    Instruction(OP_ADD, r21, r16);  //  add multiplicand to result High word.
    FwdAddrIsNow(m8s_2);
    Instruction(OP_SBRC, r20, BIT0); //4.If current bit 0 of multiplier Low byte set,
    Instruction(OP_SUB, r21, r16);   //  subtract multiplicand from result High word.
    Instruction(OP_ASR, r21);        //5.Shift right result High word into result Low word/multiplier.
    Instruction(OP_ROR, r20);        //6.Shift right Low word/multiplier.
    Instruction(OP_DEC, r25);        //7.Decrement Loop counter.
    Instruction(OP_BRNE, m8s_1, 0);  //8.If Loop counter not zero, go to Step 3.
    Instruction(OP_RET);
}
#else
//-----------------------------------------------------------------------------
// 8x8=16 signed multiply.
// op1 in r20,
// op2 in r16, result word goes into r21:r20.
//-----------------------------------------------------------------------------
static void MultiplyRoutine8()
{
    Comment("MultiplyRoutine8");
    FwdAddrIsNow(MultiplyAddress8);
    Instruction(OP_MULS, r20, r16);
    Instruction(OP_MOVW, r20, r0);
    Instruction(OP_RET);
}
/*
;******************************************************************************
;*
;* FUNCTION
;*  muls16x16_24
;* DECRIPTION
;*  Signed multiply of two 16bits numbers with 24bits result.
;* USAGE
;*  r18:r17:r16 = r23:r22 * r21:r20
;* STATISTICS
;*  Cycles :    14 + ret
;*  Words :     10 + ret
;*  Register usage: r0 to r1, r16 to r18 and r20 to r23 (9 registers)
;* NOTE
;*  The routine is non-destructive to the operands.
;*
;******************************************************************************

muls16x16_24:
    muls    r23, r21        ; (signed)ah * (signed)bh
    mov     r18, r0
    mul     r22, r20        ; al * bl
    movw    r17:r16, r1:r0
    mulsu   r23, r20        ; (signed)ah * bl
    add     r17, r0
    adc     r18, r1
    mulsu   r21, r22        ; (signed)bh * al
    add     r17, r0
    adc     r18, r1
    ret


*****************************************************************************
;*
;* FUNCTION
;*  muls16x16_32
;* DECRIPTION
;*  Signed multiply of two 16bits numbers with 32bits result.
;* USAGE
;*  r19:r18:r17:r16 = r23:r22 * r21:r20
;* STATISTICS
;*  Cycles :    19 + ret
;*  Words :     15 + ret
;*  Register usage: r0 to r2 and r16 to r23 (11 registers)
;* NOTE
;*  The routine is non-destructive to the operands.
;*
;******************************************************************************

muls16x16_32:
    clr r2
    muls    r23, r21        ; (signed)ah * (signed)bh
    movw    r19:r18, r1:r0
    mul     r22, r20        ; al * bl
    movw    r17:r16, r1:r0
    mulsu   r23, r20        ; (signed)ah * bl
    sbc     r19, r2
    add     r17, r0
    adc     r18, r1
    adc     r19, r2
    mulsu   r21, r22        ; (signed)bh * al
    sbc     r19, r2
    add     r17, r0
    adc     r18, r1
    adc     r19, r2
    ret


;******************************************************************************
*/
//-----------------------------------------------------------------------------
// 16 x 16 = 32 Signed Multiplication - "muls16x16_32"
// 16x16 signed multiply, code from Atmel app note AVR201.
// op1 in r21:r20,
// op1 in r19:r18, // save
// op2 in r17:r16, result low word goes into r21:r20.
// Signed 32bit result goes into     r23:r22:r21:r20.
//-----------------------------------------------------------------------------
static void MultiplyRoutine()
{
    Comment("MultiplyRoutine16");
    FwdAddrIsNow(MultiplyAddress);
    Instruction(OP_MOVW, r18, r20); // save op1; r19:r18 <- r21:r20
    Instruction(OP_CLR, r2, 0);
    Instruction(OP_MULS, r19, r17); //; (signed)ah * (signed)bh
    Instruction(OP_MOVW, r22, r0);
    Instruction(OP_MUL, r18, r16); //; al * bl
    Instruction(OP_MOVW, r20, r0);
    Instruction(OP_MULSU, r19, r16); //; (signed)ah * bl
    Instruction(OP_SBC, r23, r2);
    Instruction(OP_ADD, r21, r0);
    Instruction(OP_ADC, r22, r1);
    Instruction(OP_ADC, r23, r2);
    Instruction(OP_MULSU, r17, r18); //; (signed)bh * al
    Instruction(OP_SBC, r23, r2);
    Instruction(OP_ADD, r21, r0);
    Instruction(OP_ADC, r22, r1);
    Instruction(OP_ADC, r23, r2);
    Instruction(OP_RET); //17
}
//-----------------------------------------------------------------------------
// 24x24=24 signed multiply, code from Atmel app note AVR201.
// op1 in r22:r21:r20,
// op2 in r18:r17:r16, result 3 low bytes goes into r12:r11:r10.
//                     result 3 low bytes goes into r22:r21:r20.
//-----------------------------------------------------------------------------
static void MultiplyRoutine24()
{
    Comment("MultiplyRoutine24");
    FwdAddrIsNow(MultiplyAddress24);
    Instruction(OP_MUL, r20, r16); //l * l
    Instruction(OP_MOVW, r10, r0);
    Instruction(OP_MUL, r21, r17); //m * m
    Instruction(OP_MOV, r12, r0);
    Instruction(OP_MULSU, r22, r16); //h * l
    Instruction(OP_ADD, r12, r0);
    Instruction(OP_MULSU, r18, r20); //h * l
    Instruction(OP_ADD, r12, r0);
    Instruction(OP_MUL, r21, r16); //m * l
    Instruction(OP_ADD, r11, r0);
    Instruction(OP_ADC, r12, r1);
    Instruction(OP_MUL, r17, r20); //m * l
    Instruction(OP_ADD, r11, r0);
    Instruction(OP_ADC, r12, r1);
    Instruction(OP_MOVW, r20, r10);
    Instruction(OP_MOV, r22, r12);
    Instruction(OP_RET); //17
}
#endif
//-----------------------------------------------------------------------------
// 16 / 16 = 16 + 16 Signed Division - "div16s"
// 16/16 signed divide, code from the same app note.
// Dividend in r20:r19,
// divisor in  r23:r22, result goes in r20:r19 (and remainder in r17:r16).
//-----------------------------------------------------------------------------
static void DivideRoutine()
{
    Comment("DivideRoutine16");
    FwdAddrIsNow(DivideAddress);

    DWORD d16s_1 = AllocFwdAddr();
    DWORD d16s_2 = AllocFwdAddr();
    DWORD d16s_3;
    DWORD d16s_4 = AllocFwdAddr();
    DWORD d16s_5 = AllocFwdAddr();
    DWORD d16s_6 = AllocFwdAddr();

    Instruction(OP_MOV, r7, r20);    //1.XOR dividend and divisor High bytes and store in a Sign Register. r7
    Instruction(OP_EOR, r7, r23);    //1.XOR dividend and divisor High bytes and store in a Sign Register. r7
    Instruction(OP_SBRS, r20, BIT7); //2.If MSB of dividend High byte set,
    Instruction(OP_RJMP, d16s_1);
    Instruction(OP_COM, r19, 0);     // negate dividend.
    Instruction(OP_COM, r20, 0);     // negate dividend.
    Instruction(OP_SUBI, r19, 0xff); // negate dividend.
    Instruction(OP_SBCI, r20, 0xff); // negate dividend.
    FwdAddrIsNow(d16s_1);
    Instruction(OP_SBRS, r23, BIT7); //3.If MSB of divisor High byte set,
    Instruction(OP_RJMP, d16s_2);
    Instruction(OP_COM, r22, 0);     // negate divisor.
    Instruction(OP_COM, r23, 0);     // negate divisor.
    Instruction(OP_SUBI, r22, 0xff); // negate divisor.
    Instruction(OP_SBCI, r23, 0xff); // negate divisor.
    FwdAddrIsNow(d16s_2);
    Instruction(OP_EOR, r16, r16); //4.Clear remainder.
    Instruction(OP_SUB, r17, r17); //4.Clear remainder and carry.
    Instruction(OP_LDI, r25, 17);  //5.Load Loop counter with 17.

    d16s_3 = AvrProgWriteP;
    Instruction(OP_ADC, r19, r19);   //6.Shift left dividend into carry.
    Instruction(OP_ADC, r20, r20);   //6.Shift left dividend into carry.
    Instruction(OP_DEC, r25);        //7.Decrement Loop counter.
    Instruction(OP_BRNE, d16s_5, 0); //8.If Loop counter =? 0, go to step 11.
    Instruction(OP_SBRS, r7, BIT7);  //9.If MSB of Sign register set,
    Instruction(OP_RJMP, d16s_4);
    Instruction(OP_COM, r19, 0);     // negate result.
    Instruction(OP_COM, r20, 0);     // negate result.
    Instruction(OP_SUBI, r19, 0xff); // negate result.
    Instruction(OP_SBCI, r20, 0xff); // negate result.
    FwdAddrIsNow(d16s_4);
    Instruction(OP_RET); //10.Return
    FwdAddrIsNow(d16s_5);
    Instruction(OP_ADC, r16, r16);   //11.Shift left carry (from dividend/result) into remainder
    Instruction(OP_ADC, r17, r17);   //11.Shift left carry (from dividend/result) into remainder
    Instruction(OP_SUB, r16, r22);   //12.Subtract divisor from remainder.
    Instruction(OP_SBC, r17, r23);   //12.Subtract divisor from remainder.
    Instruction(OP_BRCC, d16s_6, 0); //13.If result negative,
    Instruction(OP_ADD, r16, r22);   // add back divisor,
    Instruction(OP_ADC, r17, r23);   // add back divisor,
    Instruction(OP_CLC, 0, 0);       // clear carry
    Instruction(OP_RJMP, d16s_3);    // and go to Step 6.
    FwdAddrIsNow(d16s_6);
    Instruction(OP_SEC, 0, 0);    //14. Set carry
    Instruction(OP_RJMP, d16s_3); // and go to Step 6.
}

//-----------------------------------------------------------------------------
// 24 / 24 = 24 + 24 Signed Division - "div24s"
// 24/24 signed divide, code from the same app note.
// Dividend in r21:r20:r19,
// divisor in  r24:r23:r22, result goes in r21:r20:r19 (and remainder in r18:r17:r16).
//-----------------------------------------------------------------------------
static void DivideRoutine24()
{
    Comment("DivideRoutine24");
    FwdAddrIsNow(DivideAddress24);

    DWORD d16s_1 = AllocFwdAddr();
    DWORD d16s_2 = AllocFwdAddr();
    DWORD d16s_3;
    DWORD d16s_4 = AllocFwdAddr();
    DWORD d16s_5 = AllocFwdAddr();
    DWORD d16s_6 = AllocFwdAddr();

    Instruction(OP_MOV, r7, r21);    //1.XOR dividend and divisor High bytes and store in a Sign Register. r7
    Instruction(OP_EOR, r7, r24);    //1.XOR dividend and divisor High bytes and store in a Sign Register. r7
    Instruction(OP_SBRS, r21, BIT7); //2.If MSB of dividend High byte set,
    Instruction(OP_RJMP, d16s_1);
    Instruction(OP_COM, r19, 0);     // negate dividend.
    Instruction(OP_COM, r20, 0);     // negate dividend.
    Instruction(OP_COM, r21, 0);     // negate dividend.
    Instruction(OP_SUBI, r19, 0xff); // negate dividend.
    Instruction(OP_SBCI, r20, 0xff); // negate dividend.
    Instruction(OP_SBCI, r21, 0xff); // negate dividend.
    FwdAddrIsNow(d16s_1);
    Instruction(OP_SBRS, r24, BIT7); //3.If MSB of divisor High byte set,
    Instruction(OP_RJMP, d16s_2);
    Instruction(OP_COM, r22, 0);     // negate divisor.
    Instruction(OP_COM, r23, 0);     // negate divisor.
    Instruction(OP_COM, r24, 0);     // negate divisor.
    Instruction(OP_SUBI, r22, 0xff); // negate divisor.
    Instruction(OP_SBCI, r23, 0xff); // negate divisor.
    Instruction(OP_SBCI, r24, 0xff); // negate divisor.
    FwdAddrIsNow(d16s_2);
    Instruction(OP_EOR, r16, r16); //4.Clear remainder.
    Instruction(OP_EOR, r17, r17); //4.Clear remainder.
    Instruction(OP_SUB, r18, r18); //4.Clear remainder and carry.
    Instruction(OP_LDI, r25, 25);  //5.Load Loop counter with 17+8.

    d16s_3 = AvrProgWriteP;
    Instruction(OP_ADC, r19, r19);   //6.Shift left dividend into carry.
    Instruction(OP_ADC, r20, r20);   //6.Shift left dividend into carry.
    Instruction(OP_ADC, r21, r21);   //6.Shift left dividend into carry.
    Instruction(OP_DEC, r25);        //7.Decrement Loop counter.
    Instruction(OP_BRNE, d16s_5, 0); //8.If Loop counter = 0, go to step 11.
    Instruction(OP_SBRS, r7, BIT7);  //9.If MSB of Sign register set,
    Instruction(OP_RJMP, d16s_4);
    Instruction(OP_COM, r19, 0);     // negate result.
    Instruction(OP_COM, r20, 0);     // negate result.
    Instruction(OP_COM, r21, 0);     // negate result.
    Instruction(OP_SUBI, r19, 0xff); // negate result.
    Instruction(OP_SBCI, r20, 0xff); // negate result.
    Instruction(OP_SBCI, r21, 0xff); // negate result.
    FwdAddrIsNow(d16s_4);
    Instruction(OP_RET); //10.Return
    FwdAddrIsNow(d16s_5);
    Instruction(OP_ADC, r16, r16);   //11.Shift left carry (from dividend/result) into remainder
    Instruction(OP_ADC, r17, r17);   //11.Shift left carry (from dividend/result) into remainder
    Instruction(OP_ADC, r18, r18);   //11.Shift left carry (from dividend/result) into remainder
    Instruction(OP_SUB, r16, r22);   //12.Subtract divisor from remainder.
    Instruction(OP_SBC, r17, r23);   //12.Subtract divisor from remainder.
    Instruction(OP_SBC, r18, r24);   //12.Subtract divisor from remainder.
    Instruction(OP_BRCC, d16s_6, 0); //13.If result negative,
    Instruction(OP_ADD, r16, r22);   // add back divisor,
    Instruction(OP_ADC, r17, r23);   // add back divisor,
    Instruction(OP_ADC, r18, r24);   // add back divisor,
    Instruction(OP_CLC, 0, 0);       // clear carry
    Instruction(OP_RJMP, d16s_3);    // and go to Step 6.
    FwdAddrIsNow(d16s_6);
    Instruction(OP_SEC, 0, 0);    //14. Set carry
    Instruction(OP_RJMP, d16s_3); // and go to Step 6.
}

//-----------------------------------------------------------------------------
// 8 / 8 = 8 + 8 Signed Division - "div8s"
// 8/8 signed divide, code from the same app note.
// Dividend in r19,
// divisor in  r22, result goes in r19 (and remainder in r18).
//-----------------------------------------------------------------------------
static void DivideRoutine8()
{
    Comment("DivideRoutine8");
    FwdAddrIsNow(DivideAddress8);

    DWORD d16s_1 = AllocFwdAddr();
    DWORD d16s_2 = AllocFwdAddr();
    DWORD d16s_3;
    DWORD d16s_4 = AllocFwdAddr();
    DWORD d16s_5 = AllocFwdAddr();
    DWORD d16s_6 = AllocFwdAddr();

    Instruction(OP_MOV, r7, r19);    //1.XOR dividend and divisor High bytes and store in a Sign Register. r7
    Instruction(OP_EOR, r7, r22);    //1.XOR dividend and divisor High bytes and store in a Sign Register. r7
    Instruction(OP_SBRS, r19, BIT7); //2.If MSB of dividend High byte set,
    Instruction(OP_RJMP, d16s_1);
    Instruction(OP_COM, r19, 0);     // negate dividend.
    Instruction(OP_SUBI, r19, 0xff); // negate dividend.
    FwdAddrIsNow(d16s_1);
    Instruction(OP_SBRS, r22, BIT7); //3.If MSB of divisor High byte set,
    Instruction(OP_RJMP, d16s_2);
    Instruction(OP_COM, r22, 0);     // negate divisor.
    Instruction(OP_SUBI, r22, 0xff); // negate divisor.
    FwdAddrIsNow(d16s_2);
    Instruction(OP_SUB, r18, r18); //4.Clear remainder and carry.
    Instruction(OP_LDI, r25, 9);   //5.Load Loop counter with 17-8.

    d16s_3 = AvrProgWriteP;
    Instruction(OP_ADC, r19, r19);   //6.Shift left dividend into carry.
    Instruction(OP_DEC, r25);        //7.Decrement Loop counter.
    Instruction(OP_BRNE, d16s_5, 0); //8.If Loop counter = 0, go to step 11.
    Instruction(OP_SBRS, r7, BIT7);  //9.If MSB of Sign register set,
    Instruction(OP_RJMP, d16s_4);
    Instruction(OP_COM, r19, 0);     // negate result.
    Instruction(OP_SUBI, r19, 0xff); // negate result.
    FwdAddrIsNow(d16s_4);
    Instruction(OP_RET); //10.Return
    FwdAddrIsNow(d16s_5);
    Instruction(OP_ADC, r18, r18);   //11.Shift left carry (from dividend/result) into remainder
    Instruction(OP_SUB, r18, r22);   //12.Subtract divisor from remainder.
    Instruction(OP_BRCC, d16s_6, 0); //13.If result negative,
    Instruction(OP_ADD, r18, r22);   // add back divisor,
    Instruction(OP_CLC, 0, 0);       // clear carry
    Instruction(OP_RJMP, d16s_3);    // and go to Step 6.
    FwdAddrIsNow(d16s_6);
    Instruction(OP_SEC, 0, 0);    //14. Set carry
    Instruction(OP_RJMP, d16s_3); // and go to Step 6.
}
//-----------------------------------------------------------------------------
// Compile the program to REG code for the currently selected processor
// and write it to the given file. Produce an error message if we cannot
// write to the file, or if there is something inconsistent about the
// program.
//-----------------------------------------------------------------------------
void CompileAvr(const char *outFile)
{
    rungNow = -100;
    FileTracker f(outFile, "w");
    if(!f) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), outFile);
        return;
    }

    char outFileAsm[MAX_PATH];
    SetExt(outFileAsm, outFile, ".asm");
    FileTracker fAsm(outFileAsm, "w");
    if(!fAsm) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), outFileAsm);
        return;
    }

    fprintf(fAsm,
            ";/* This is auto-generated ASM code from LDmicro. Do not edit this file!\n"
            ";   Go back to the LDmicro ladder diagram source for changes in the ladder logic. */\n"
            ";%s is the LDmicro target processor.\n"
            ".DEVICE %s\n"
            ";#pragma AVRPART ADMIN PART_NAME %s\n"
            ";.INCLUDE <%s.inc>\n"
            ".CSEG\n"
            ".ORG 0x0\n"
            ";TABSIZE = 8\n",
            Prog.mcu->mcuName,
            Prog.mcu->mcuList,
            Prog.mcu->mcuList,
            Prog.mcu->mcuInc);
    Comment("GOTO, progStart");

    //***********************************************************************
    // Interrupt Vectors Table
    if(McuAs(" AT90USB646 ")     //
       || McuAs(" AT90USB647 ")  //
       || McuAs(" AT90USB1286 ") //
       || McuAs(" AT90USB1287 ") ///
    ) {
        Int0Addr = 1;
        Int1Addr = 2;
        Timer1OvfAddr = 20;
        Timer0OvfAddr = 23;
        Timer1CompAAddr = 17;
    } else if(McuAs(" AT90USB82 ")     //
              || McuAs(" AT90USB162 ") //
    ) {
        Int0Addr = 1;
        Int1Addr = 2;
        Timer1OvfAddr = 18;
        Timer0OvfAddr = 21;
        Timer1CompAAddr = 15;
    } else if(McuAs(" ATmega161 ")    //
              || McuAs(" ATmega162 ") //
              || McuAs(" ATmega32 ")  //
              || McuAs(" ATmega323 ") //
    ) {
        Int0Addr = 2;
        Int1Addr = 4;
        Timer1OvfAddr = 0x0012;
        Timer0OvfAddr = 0x0016;
        Timer1CompAAddr = 0x000E;
    } else if(McuAs(" ATmega103 ") //
    ) {
        Int0Addr = 1;
        Int1Addr = 2;
        Timer1OvfAddr = 14;
        Timer0OvfAddr = 16;
        Timer1CompAAddr = 12;
    } else if(McuAs(" ATmega16 ")     //
              || McuAs(" ATmega163 ") //
    ) {
        Int0Addr = 2;
        Int1Addr = 4;
        Timer1OvfAddr = 0x0010;
        Timer0OvfAddr = 0x0012;
        Timer1CompAAddr = 0x000C;
    } else if(McuAs("Atmel AVR ATmega48 ") ||  //
              McuAs("Atmel AVR ATmega88 ") ||  //
              McuAs("Atmel AVR ATmega168 ") || //
              McuAs("Atmel AVR ATmega328 ")    //
    ) {
        Int0Addr = 1;
        Int1Addr = 2;
        Timer1OvfAddr = 0x000D;
        Timer0OvfAddr = 0x0010;
        Timer1CompAAddr = 0x000B;
    } else if(McuAs("Atmel AVR ATmega64 ")     //
              || McuAs("Atmel AVR ATmega128 ") //
    ) {
        Int0Addr = 2;
        Int1Addr = 4;
        Timer1OvfAddr = 0x001C;
        Timer0OvfAddr = 0x0020;
        Timer1CompAAddr = 0x0018;
    } else if(McuAs(" ATmega8515 ") //
    ) {
        Int0Addr = 1;
        Int1Addr = 2;
        Timer1OvfAddr = 0x0006;
        Timer0OvfAddr = 0x0007;
        Timer1CompAAddr = 0x0004;
    } else if(McuAs(" ATmega8 ")       //
              || McuAs(" ATmega8535 ") //
    ) {
        Int0Addr = 1;
        Int1Addr = 2;
        Timer1OvfAddr = 0x0008;
        Timer0OvfAddr = 0x0009;
        Timer1CompAAddr = 0x0006;
    }; // else oops();

    //***********************************************************************
    // Okay, so many AVRs have a register called TIFR, but the meaning of
    // the bits in that register varies from device to device...

    //***********************************************************************
    // Here we must set up the addresses of some registers that for some
    // stupid reason move around from AVR to AVR.
    /*
    if(McuAs(" AT90USB82 ")
    || McuAs(" AT90USB162 ")
    ){
        REG_TCCR0B = 0x45
            WGM00   = BIT;
            WGM01   = BIT;
        REG_TCNT0  = 0x46

      //TIFR bits
        OCF1A  = BIT1;
        TOV0   = BIT0;
      //TIFR1  bits
        TOV1   = BIT0;
      //TIMSK  bits
        OCIE1A = BIT1;
        TOIE1  = BIT0;
        TOIE0  = BIT0;
    } else
    */
    if(McuAs(" AT90USB646 ")     //
       || McuAs(" AT90USB647 ")  //
       || McuAs(" AT90USB1286 ") //
       || McuAs(" AT90USB1287 ") //
    ) {
        REG_OCR0A = 0x47;
        REG_TCCR0A = 0x44;
        REG_TCCR0B = 0x45;
        WGM00 = BIT0;
        WGM01 = BIT1;
        REG_TCNT0 = 0x46;

        REG_TIFR1 = 0x36;
        OCF1A = BIT1;
        TOV1 = BIT0;
        REG_TIFR0 = 0x35;
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_OCR2 = 0xB3; // OCR2A
        REG_TCCR2B = 0xB1;
        REG_TCCR2 = 0xB0; // TCCR2A
        WGM20 = BIT0;
        WGM21 = BIT1;
        COM21 = BIT7; // COM2A1
        COM20 = BIT6; // COM2A0

        REG_EEARH = 0x42;
        REG_EEARL = 0x41;
        REG_EEDR = 0x40;
        REG_EECR = 0x3F;

        REG_ADMUX = 0x7C;
        REG_ADCSRB = 0x7B;
        REG_ADCSRA = 0x7A;
        REG_ADCH = 0x79;
        REG_ADCL = 0x78;

        REG_WDTCR = 0x60;

        REG_OCR1AH = 0x89;
        REG_OCR1AL = 0x88;
        REG_TCCR1B = 0x81;
        REG_TCCR1A = 0x80;

        REG_TIMSK = 0x6F;
        OCIE1A = BIT1;
        TOIE1 = BIT0;
        TOIE0 = BIT0;

        REG_UDR = 0xCE;
        REG_UBRRH = 0xCD;
        REG_UBRRL = 0xCC;
        //      REG_UCSRC   = 0xCA;
        REG_UCSRB = 0xC9;
        REG_UCSRA = 0xC8;

        REG_GTCCR = 0x43;

        REG_sleep = 0x53; // REG_SMCR
        SE = BIT0;
        SM0 = BIT1;
        SM1 = BIT2;
        SM2 = BIT3;

        REG_int_sup = 0x69;
        //      REG_EICRA   = 0x69;
        //      REG_EICRB   = 0x6A;

        REG_int_en = 0x3D; // REG_EIMSK
        INT1 = BIT1;
        INT0 = BIT0;

        REG_int_flag = 0x3C; // REG_EIFR
        INTF1 = BIT1;
        INTF0 = BIT0;

    } else if(McuAs(" ATmega16U4 ") || //
              McuAs(" ATmega32U4 ")    //
    ) {
        REG_OCR0A = 0x47;
        REG_TCCR0A = 0x44;
        REG_TCCR0B = 0x45;
        WGM00 = BIT0;
        WGM01 = BIT1;
        REG_TCNT0 = 0x46;

        REG_TIFR1 = 0x36; // TIFR
        OCF1A = BIT1;
        TOV1 = BIT0;
        REG_TIFR0 = 0x35; // TIFR
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_EEARH = 0x42;
        REG_EEARL = 0x41;
        REG_EEDR = 0x40;
        REG_EECR = 0x3F;

        REG_ADMUX = 0x7C;
        REG_ADCSRB = 0x7B;
        REG_ADCSRA = 0x7A;
        REG_ADCH = 0x79;
        REG_ADCL = 0x78;

        REG_WDTCR = 0x60;

        REG_OCR1AH = 0x89;
        REG_OCR1AL = 0x88;
        REG_TCCR1B = 0x81;
        REG_TCCR1A = 0x80;

        REG_TIMSK = 0x6f;
        OCIE1A = BIT1;
        TOIE1 = BIT0;
        TOIE0 = BIT0;

        REG_UDR = 0xCE;
        REG_UBRRH = 0xCD;
        REG_UBRRL = 0xCC;
        //      REG_UCSRC   = 0xCA;
        REG_UCSRB = 0xC9;
        REG_UCSRA = 0xC8;

        REG_sleep = 0x53; // REG_SMCR
        SE = BIT0;
        SM0 = BIT1;
        SM1 = BIT2;
        SM2 = BIT3;

        REG_int_sup = 0x69;
        //      REG_EICRA   = 0x69;
        //      REG_EICRB   = 0x6A;

        REG_int_en = 0x3D; // REG_EIMSK
        INT1 = BIT1;
        INT0 = BIT0;

        REG_int_flag = 0x3C; // REG_EIFR
        INTF1 = BIT1;
        INTF0 = BIT0;

        //      REG_SFIOR   = 0x50;
        //          PSR2    = BIT1;

        REG_EIND = 0x5C;
    } else if(McuAs("Atmel AVR ATmega48 ") ||  //
              McuAs("Atmel AVR ATmega88 ") ||  //
              McuAs("Atmel AVR ATmega168 ") || //
              McuAs("Atmel AVR ATmega328 ")    //
    ) {
        REG_OCR0A = 0x47;
        REG_TCCR0A = 0x44;
        REG_TCCR0B = 0x45;
        WGM00 = BIT0;
        WGM01 = BIT1;
        REG_TCNT0 = 0x46;

        REG_TIFR1 = 0x36;
        OCF1A = BIT1;
        TOV1 = BIT0;
        REG_TIFR0 = 0x35;
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_OCR2 = 0xB3; // OCR2A
        REG_TCCR2B = 0xB1;
        REG_TCCR2 = 0xB0; // TCCR2A
        WGM20 = BIT0;
        WGM21 = BIT1;
        COM21 = BIT7; // COM2A1
        COM20 = BIT6; // COM2A0

        REG_EEARH = 0x42;
        REG_EEARL = 0x41;
        REG_EEDR = 0x40;
        REG_EECR = 0x3F;

        REG_ADMUX = 0x7C;
        REG_ADCSRB = 0x7B;
        REG_ADCSRA = 0x7A;
        REG_ADCH = 0x79;
        REG_ADCL = 0x78;

        REG_WDTCR = 0x60;

        REG_OCR1AH = 0x89;
        REG_OCR1AL = 0x88;
        REG_TCCR1B = 0x81;
        REG_TCCR1A = 0x80;

        REG_TIMSK = 0x6F; // TIMSK1

        REG_UDR = 0xC6;   // UDR0
        REG_UBRRH = 0xC5; // UBRR0H
        REG_UBRRL = 0xC4; // UBRR0L
                          //      REG_UCSRC   = 0xC2; // UCSR0C
        REG_UCSRB = 0xC1; // UCSR0B
        REG_UCSRA = 0xC0; // UCSR0A

        REG_GTCCR = 0x43;

        REG_sleep = 0x53; // REG_SMCR
        SE = BIT0;
        SM0 = BIT1;
        SM1 = BIT2;
        SM2 = BIT3;

        REG_int_sup = 0x69; // REG_EICRA
                            //      REG_EICRA   = 0x69;

        REG_int_en = 0x3D; // REG_EIMSK
        INT1 = BIT1;
        INT0 = BIT0;

        REG_int_flag = 0x3C; // REG_EIFR
        INTF1 = BIT1;
        INTF0 = BIT0;

    } else if(McuAs("Atmel AVR ATmega164 ") || //
              McuAs("Atmel AVR ATmega324 ") || //
              McuAs("Atmel AVR ATmega644 ") || //
              McuAs("Atmel AVR ATmega1284 ")   //
    ) {
        REG_OCR0A = 0x47;
        REG_TCCR0A = 0x44;
        REG_TCCR0B = 0x45;
        WGM00 = BIT0;
        WGM01 = BIT1;
        REG_TCNT0 = 0x46;

        REG_TIFR1 = 0x36;
        OCF1A = BIT1;
        TOV1 = BIT0;
        REG_TIFR0 = 0x35;
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_OCR2 = 0xB3; // OCR2A
        REG_TCCR2B = 0xB1;
        REG_TCCR2 = 0xB0; // TCCR2A
        WGM20 = BIT0;
        WGM21 = BIT1;
        COM21 = BIT7; // COM2A1
        COM20 = BIT6; // COM2A0

        REG_EEARH = 0x42;
        REG_EEARL = 0x41;
        REG_EEDR = 0x40;
        REG_EECR = 0x3F;

        REG_ADMUX = 0x7C;
        REG_ADCSRB = 0x7B;
        REG_ADCSRA = 0x7A;
        REG_ADCH = 0x79;
        REG_ADCL = 0x78;

        REG_WDTCR = 0x60;

        REG_OCR1AH = 0x89;
        REG_OCR1AL = 0x88;
        REG_TCCR1B = 0x81;
        REG_TCCR1A = 0x80;

        REG_TIMSK = 0x6F; // TIMSK1

        REG_UDR = 0xC6;   // UDR0
        REG_UBRRH = 0xC5; // UBRR0H
        REG_UBRRL = 0xC4; // UBRR0L
                          //      REG_UCSRC   = 0xC2; // UCSR0C
        REG_UCSRB = 0xC1; // UCSR0B
        REG_UCSRA = 0xC0; // UCSR0A

        REG_GTCCR = 0x43;

        REG_sleep = 0x53; // REG_SMCR
        SE = BIT0;
        SM0 = BIT1;
        SM1 = BIT2;
        SM2 = BIT3;

        REG_int_sup = 0x69;
        //      REG_EICRA   = 0x69;

        REG_int_en = 0x3D; // REG_EIMSK
        INT1 = BIT1;
        INT0 = BIT0;

        REG_int_flag = 0x3C; // REG_EIFR
        INTF1 = BIT1;
        INTF0 = BIT0;

    } else if(McuAs(" ATmega640 ") ||  //
              McuAs(" ATmega1280 ") || //
              McuAs(" ATmega1281 ") || //
              McuAs(" ATmega2560 ") || //
              McuAs(" ATmega2561 ")    //
    ) {
        REG_OCR0A = 0x47;
        REG_TCCR0A = 0x44;
        REG_TCCR0B = 0x45;
        WGM00 = BIT0;
        WGM01 = BIT1;
        REG_TCNT0 = 0x46;

        REG_TIFR1 = 0x36;
        OCF1A = BIT1;
        TOV1 = BIT0;
        REG_TIFR0 = 0x35;
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_OCR2 = 0xB3; // OCR2A
        REG_TCCR2B = 0xB1;
        REG_TCCR2 = 0xB0; // TCCR2A
        WGM20 = BIT0;
        WGM21 = BIT1;
        COM21 = BIT7; // COM2A1
        COM20 = BIT6; // COM2A0

        REG_EEARH = 0x42;
        REG_EEARL = 0x41;
        REG_EEDR = 0x40;
        REG_EECR = 0x3F;

        REG_ADMUX = 0x7C;
        REG_ADCSRB = 0x7B;
        REG_ADCSRA = 0x7A;
        REG_ADCH = 0x79;
        REG_ADCL = 0x78;

        REG_WDTCR = 0x60;

        REG_OCR1AH = 0x89;
        REG_OCR1AL = 0x88;
        REG_TCCR1B = 0x81;
        REG_TCCR1A = 0x80;

        REG_TIMSK = 0x6F; // TIMSK1

        REG_UDR = 0xC6;   // UDR0
        REG_UBRRH = 0xC5; // UBRR0H
        REG_UBRRL = 0xC4; // UBRR0L
                          //      REG_UCSRC   = 0xC2; // UCSR0C
        REG_UCSRB = 0xC1; // UCSR0B
        REG_UCSRA = 0xC0; // UCSR0A

        REG_GTCCR = 0x43;

        REG_sleep = 0x53; // REG_SMCR
        SE = BIT0;
        SM0 = BIT1;
        SM1 = BIT2;
        SM2 = BIT3;

        REG_int_sup = 0x69;
        //      REG_EICRA   = 0x69;
        //      REG_EICRB   = 0x6A;

        REG_int_en = 0x3D; // REG_EIMSK
        INT1 = BIT1;
        INT0 = BIT0;

        REG_int_flag = 0x3C; // REG_EIFR
        INTF1 = BIT1;
        INTF0 = BIT0;

        REG_EIND = 0x5C;
    } else if(McuAs(" ATmega161 ")             //
              || McuAs("Atmel AVR ATmega162 ") //
              || McuAs(" ATmega8515 ")         //
    ) {
        REG_OCR0A = 0x51;  // OCR0
        REG_TCCR0A = 0x53; // TCCR0
        REG_TCCR0B = 0x53; // TCCR0
        WGM00 = BIT6;
        WGM01 = BIT3;
        REG_TCNT0 = 0x52;

        REG_TIFR1 = 0x58; // TIFR
        OCF1A = BIT6;
        TOV1 = BIT7;
        REG_TIFR0 = 0x58; // TIFR
        OCF0A = BIT0;
        TOV0 = BIT1;

        if(McuAs(" ATmega161 ")             //
           || McuAs("Atmel AVR ATmega162 ") //
        ) {
            REG_OCR2 = 0x43;
            //REG_TCCR2B  = 0;
            REG_TCCR2 = 0x45;
            WGM20 = BIT6;
            WGM21 = BIT3;
            COM21 = BIT5;
            COM20 = BIT4;
        }

        REG_EEARH = 0x3F;
        REG_EEARL = 0x3E;
        REG_EEDR = 0x3D;
        REG_EECR = 0x3C;

        /*
        REG_ADMUX   = 0; // No ADC
        REG_ADCSRA  = 0;
    ////REG_ADCSRB  = 0;
        REG_ADCH    = 0;
        REG_ADCL    = 0;
        */
        REG_WDTCR = 0x41;

        REG_OCR1AH = 0x4B;
        REG_OCR1AL = 0x4A;
        REG_TCCR1A = 0x4F;
        REG_TCCR1B = 0x4E;

        REG_TIMSK = 0x59;
        TOIE1 = BIT7;
        OCIE1A = BIT6;
        TOIE0 = BIT1;

        //      REG_UCSRC   = 0x40;
        REG_UBRRH = 0x40;
        REG_UBRRL = 0x29;
        REG_UCSRB = 0x2a;
        REG_UCSRA = 0x2b;
        REG_UDR = 0x2c;

        REG_SFIOR = 0x50;
        PSR2 = BIT1;

        REG_sleep = 0x55; // REG_MCUCR
        SE = BIT5;
        SM1 = BIT4;
        REG_EMCUCR = 0x56;
        SM0 = BIT7;
        REG_MCUCSR = 0x54;
        SM2 = BIT5;

        REG_int_sup = 0x55; // REG_MCUCR

        REG_int_en = 0x5B; // REG_GICR
        INT1 = BIT7;
        INT0 = BIT6;

        REG_int_flag = 0x5A; // REG_GIFR
        INTF1 = BIT7;
        INTF0 = BIT6;

    } else if(McuAs("Atmel AVR ATmega8 ") //
    ) {
        REG_TCCR0A = 0x53; // TCCR0
        REG_TCCR0B = 0x53; // TCCR0
        REG_TCNT0 = 0x52;

        REG_TIFR1 = 0x58; // TIFR
        OCF1A = BIT4;
        TOV1 = BIT2;
        REG_TIFR0 = 0x58; // TIFR
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_OCR2 = 0x43;
        //REG_TCCR2B  = 0;
        REG_TCCR2 = 0x45;
        WGM20 = BIT6;
        WGM21 = BIT3;
        COM21 = BIT5;
        COM20 = BIT4;

        REG_EEARH = 0x3F;
        REG_EEARL = 0x3E;
        REG_EEDR = 0x3D;
        REG_EECR = 0x3C;

        REG_ADMUX = 0x27;
        REG_ADCSRA = 0x26;
        ////REG_ADCSRB  = 0;
        REG_ADCH = 0x25;
        REG_ADCL = 0x24;

        REG_WDTCR = 0x41;

        REG_OCR1AH = 0x4B;
        REG_OCR1AL = 0x4A;
        REG_TCCR1A = 0x4F;
        REG_TCCR1B = 0x4E;

        REG_TIMSK = 0x59;
        OCIE1A = BIT4;
        TOIE1 = BIT2;
        TOIE0 = BIT0;

        //      REG_UCSRC   = 0x40;
        REG_UBRRH = 0x40;
        REG_UBRRL = 0x29;
        REG_UCSRB = 0x2a;
        REG_UCSRA = 0x2b;
        REG_UDR = 0x2c;

        REG_SFIOR = 0x50;
        PSR2 = BIT1;

        REG_sleep = 0x55; // REG_MCUCR
        SE = BIT7;
        SM0 = BIT4;
        SM1 = BIT5;
        SM2 = BIT6;

        REG_int_sup = 0x55; // REG_MCUCR

        REG_int_en = 0x5B; // REG_GICR
        INT1 = BIT7;
        INT0 = BIT6;

        REG_int_flag = 0x5A; // REG_GIFR
        INTF1 = BIT7;
        INTF0 = BIT6;

    } else if(McuAs("Atmel AVR ATmega32 ") //
    ) {
        REG_OCR0A = 0x5C;  // OCR0
        REG_TCCR0A = 0x53; // TCCR0
        REG_TCCR0B = 0x53; // TCCR0
        WGM00 = BIT6;
        WGM01 = BIT3;
        REG_TCNT0 = 0x52;

        REG_TIFR1 = 0x58; // TIFR
        OCF1A = BIT4;
        TOV1 = BIT2;
        REG_TIFR0 = 0x58; // TIFR
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_OCR2 = 0x43;
        //REG_TCCR2B  = 0;
        REG_TCCR2 = 0x45;
        WGM20 = BIT6;
        WGM21 = BIT3;
        COM21 = BIT5;
        COM20 = BIT4;

        REG_EEARH = 0x3F;
        REG_EEARL = 0x3E;
        REG_EEDR = 0x3D;
        REG_EECR = 0x3C;

        REG_ADMUX = 0x27;
        REG_ADCSRA = 0x26;
        ////REG_ADCSRB  = 0;
        REG_ADCH = 0x25;
        REG_ADCL = 0x24;

        REG_WDTCR = 0x41;

        REG_OCR1AH = 0x4B;
        REG_OCR1AL = 0x4A;
        REG_TCCR1A = 0x4F;
        REG_TCCR1B = 0x4E;

        REG_TIMSK = 0x59;
        OCIE1A = BIT4;
        TOIE1 = BIT2;
        TOIE0 = BIT0;

        //      REG_UCSRC   = 0x40;
        REG_UBRRH = 0x40;
        REG_UBRRL = 0x29;
        REG_UCSRB = 0x2a;
        REG_UCSRA = 0x2b;
        REG_UDR = 0x2c;

        REG_SFIOR = 0x50;
        PSR2 = BIT1;

        REG_sleep = 0x55; // REG_MCUCR
        SE = BIT7;
        SM0 = BIT4;
        SM1 = BIT5;
        SM2 = BIT6;

        REG_int_sup = 0x55; // REG_MCUCR

        REG_int_en = 0x5B; // REG_GICR
        INT1 = BIT7;
        INT0 = BIT6;

        REG_int_flag = 0x5A; // REG_GIFR
        INTF1 = BIT7;
        INTF0 = BIT6;

    } else if(McuAs("Atmel AVR ATmega16 ") //
    ) {
        REG_OCR0A = 0x5C;  // OCR0
        REG_TCCR0A = 0x53; // TCCR0
        REG_TCCR0B = 0x53; // TCCR0
        WGM00 = BIT6;
        WGM01 = BIT3;
        REG_TCNT0 = 0x52;

        REG_TIFR1 = 0x58; // TIFR
        OCF1A = BIT4;
        TOV1 = BIT2;
        REG_TIFR0 = 0x58; // TIFR
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_OCR2 = 0x43;
        //REG_TCCR2B  = 0;
        REG_TCCR2 = 0x45;
        WGM20 = BIT6;
        WGM21 = BIT3;
        COM21 = BIT5;
        COM20 = BIT4;

        REG_EEARH = 0x3F;
        REG_EEARL = 0x3E;
        REG_EEDR = 0x3D;
        REG_EECR = 0x3C;

        REG_ADMUX = 0x27;
        REG_ADCSRA = 0x26;
        ////REG_ADCSRB  = 0;
        REG_ADCH = 0x25;
        REG_ADCL = 0x24;

        REG_WDTCR = 0x41;

        REG_OCR1AH = 0x4B;
        REG_OCR1AL = 0x4A;
        REG_TCCR1A = 0x4F;
        REG_TCCR1B = 0x4E;

        REG_TIMSK = 0x59;
        OCIE1A = BIT4;
        TOIE1 = BIT2;
        TOIE0 = BIT0;

        //      REG_UCSRC   = 0x40;
        REG_UBRRH = 0x40;
        REG_UBRRL = 0x29;
        REG_UCSRB = 0x2a;
        REG_UCSRA = 0x2b;
        REG_UDR = 0x2c;

        REG_SFIOR = 0x50;
        PSR2 = BIT1;

        REG_sleep = 0x55; // REG_MCUCR
        SE = BIT6;
        SM0 = BIT4;
        SM1 = BIT5;
        SM2 = BIT7;

        REG_int_sup = 0x55; // REG_MCUCR

        REG_int_en = 0x5B; // REG_GICR
        INT1 = BIT7;
        INT0 = BIT6;

        REG_int_flag = 0x5A; // REG_GIFR
        INTF1 = BIT7;
        INTF0 = BIT6;

    } else if(McuAs("Atmel AVR ATmega64 ") || //
              McuAs("Atmel AVR ATmega128 ")   //
    ) {
        REG_OCR0A = 0x51;  // OCR0
        REG_TCCR0A = 0x53; // TCCR0
        REG_TCCR0B = 0x53; // TCCR0
        WGM00 = BIT6;
        WGM01 = BIT3;
        REG_TCNT0 = 0x52;

        REG_TIFR1 = 0x56; // TIFR
        OCF1A = BIT4;
        TOV1 = BIT2;
        REG_TIFR0 = 0x56; // TIFR
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_OCR2 = 0x43;
        //REG_TCCR2B  = 0;
        REG_TCCR2 = 0x45;
        WGM20 = BIT6;
        WGM21 = BIT3;
        COM21 = BIT5;
        COM20 = BIT4;

        REG_EEARH = 0x3F;
        REG_EEARL = 0x3E;
        REG_EEDR = 0x3D;
        REG_EECR = 0x3C;

        REG_ADMUX = 0x27;
        REG_ADCSRA = 0x26;
        ////REG_ADCSRB  = 0x8E; // only ATmega64
        REG_ADCH = 0x25;
        REG_ADCL = 0x24;

        REG_WDTCR = 0x41;

        REG_OCR1AH = 0x4B;
        REG_OCR1AL = 0x4A;
        REG_TCCR1A = 0x4F;
        REG_TCCR1B = 0x4E;

        REG_TIMSK = 0x57;
        OCIE1A = BIT4;
        TOIE1 = BIT2;
        TOIE0 = BIT0;

        REG_UBRRH = 0x98; // UBRR1H
        REG_UBRRL = 0x99; // UBRR1L
        REG_UCSRB = 0x9a; // UCSR1B
        REG_UCSRA = 0x9b; // UCSR1A
        REG_UDR = 0x9c;   // UDR1
                          //      REG_UCSRC   = 0x9d; // UCSR1C

        REG_SFIOR = 0x40;
        PSR2 = BIT0;

        REG_sleep = 0x55; // REG_MCUCR
        SE = BIT5;
        SM0 = BIT3;
        SM1 = BIT4;
        SM2 = BIT2;

        REG_int_sup = 0x6A;
        //      REG_EICRA   = 0x6A;
        //      REG_EICRB   = 0x5A;

        REG_int_en = 0x59; // EIMSK
        INT1 = BIT1;
        INT0 = BIT0;

        REG_int_flag = 0x58; // EIFR
        INTF1 = BIT1;
        INTF0 = BIT0;
    } else if(McuAs(" ATtiny10 ") //
    ) {
        //REG_OCR0A   = 0x49;
        REG_TCCR0A = 0x4A;
        REG_TCCR0B = 0x53;
        WGM00 = BIT0;
        WGM01 = BIT1;
        REG_TCNT0 = 0x52;

        REG_TIFR0 = 0x35;
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_ADMUX = 0x7C;
        REG_ADCSRB = 0x7B;
        REG_ADCSRA = 0x7A;
        REG_ADCH = 0x79;
        REG_ADCL = 0x78;

        REG_WDTCR = 0x60;

        REG_TIMSK = 0x6F; // TIMSK1

        REG_GTCCR = 0x43;

        REG_sleep = 0x53; // REG_SMCR
        SE = BIT0;
        SM0 = BIT1;
        SM1 = BIT2;
        SM2 = BIT3;

        REG_int_sup = 0x69;
        //      REG_EICRA   = 0x69;

        REG_int_en = 0x3D; // REG_EIMSK
        INT1 = BIT1;
        INT0 = BIT0;

        REG_int_flag = 0x3C; // REG_EIFR
        INTF1 = BIT1;
        INTF0 = BIT0;
    } else if(McuAs(" ATtiny85 ") //
    ) {
        REG_OCR0A = 0x49;
        REG_TCCR0A = 0x4A;
        REG_TCCR0B = 0x53;
        WGM00 = BIT0;
        WGM01 = BIT1;
        REG_TCNT0 = 0x52;

        REG_TIFR1 = 0x36;
        OCF1A = BIT1;
        TOV1 = BIT0;
        REG_TIFR0 = 0x35;
        OCF0A = BIT1;
        TOV0 = BIT0;

        REG_OCR2 = 0xB3; // OCR2A
        REG_TCCR2B = 0xB1;
        REG_TCCR2 = 0xB0; // TCCR2A
        WGM20 = BIT0;
        WGM21 = BIT1;
        COM21 = BIT7; // COM2A1
        COM20 = BIT6; // COM2A0

        REG_EEARH = 0x42;
        REG_EEARL = 0x41;
        REG_EEDR = 0x40;
        REG_EECR = 0x3F;

        REG_ADMUX = 0x7C;
        REG_ADCSRB = 0x7B;
        REG_ADCSRA = 0x7A;
        REG_ADCH = 0x79;
        REG_ADCL = 0x78;

        REG_WDTCR = 0x60;

        REG_OCR1AH = 0x89;
        REG_OCR1AL = 0x88;
        REG_TCCR1B = 0x81;
        REG_TCCR1A = 0x80;

        REG_TIMSK = 0x6F; // TIMSK1

        REG_UDR = 0xC6;   // UDR0
        REG_UBRRH = 0xC5; // UBRR0H
        REG_UBRRL = 0xC4; // UBRR0L
                          //      REG_UCSRC   = 0xC2; // UCSR0C
        REG_UCSRB = 0xC1; // UCSR0B
        REG_UCSRA = 0xC0; // UCSR0A

        REG_GTCCR = 0x43;

        REG_sleep = 0x53; // REG_SMCR
        SE = BIT0;
        SM0 = BIT1;
        SM1 = BIT2;
        SM2 = BIT3;

        REG_int_sup = 0x69;
        //      REG_EICRA   = 0x69;

        REG_int_en = 0x3D; // REG_EIMSK
        INT1 = BIT1;
        INT0 = BIT0;

        REG_int_flag = 0x3C; // REG_EIFR
        INTF1 = BIT1;
        INTF0 = BIT0;
        /*
    } else
    if(McuAs(" ATmega163 ")
    || McuAs(" ATmega323 ")
    || McuAs(" ATmega8535 ")
    ){
        REG_MCUCR       = 0x55;
            SE          = BIT7;
            SM0         = BIT4;

      //REG_TCCR0  = 0x45
            WGM00   = BIT;
            WGM01   = BIT;
      //REG_TCNT0  = 0x46

      //TIFR bits
        OCF1A  = BIT4;
        TOV1   = BIT2;
        TOV0   = BIT0;
      //TIMSK bits
        OCIE1A = BIT4;
        TOIE1  = BIT2;
        TOIE0  = BIT0;
    } else
    if(McuAs("Atmel AVR ATmega103 ")
    ){
      //REG_TCCR0  = 0x45
            WGM00   = BIT;
            WGM01   = BIT;
      //REG_TCNT0  = 0x46

        REG_OCR1AH  = 0x4B;
        REG_OCR1AL  = 0x4A;
        REG_TCCR1A  = 0x4F;
        REG_TCCR1B  = 0x4E;

        REG_TIMSK   = 0x57;
        REG_TIFR1   = 0x56; // TIFR
        REG_TIFR0   = 0x56; // TIFR
            OCF1A   = BIT4;
            TOV1    = BIT2;
            TOV0    = BIT0;

//      REG_UBRRH   = 0x98;
        REG_UBRRL   = 0x29; // UBRR
        REG_UCSRB   = 0x2a; // UCR
        REG_UCSRA   = 0x2b; // USR
        REG_UDR     = 0x2c;
//      REG_UCSRC   = 0x9d;
    */
    } else
        THROW_COMPILER_EXCEPTION_FMT("Don't know how to init %s.", Prog.mcu ? Prog.mcu->mcuName : "Invalid MCU");
    //***********************************************************************

    rungNow = -90;
    WipeMemory();
    MultiplyUsed = false;
    MultiplyAddress = AllocFwdAddr();
    DivideUsed = false;
    DivideAddress = AllocFwdAddr();

    MultiplyUsed8 = false;
    MultiplyAddress8 = AllocFwdAddr();
    DivideUsed8 = false;
    DivideAddress8 = AllocFwdAddr();

    MultiplyUsed24 = false;
    MultiplyAddress24 = AllocFwdAddr();
    DivideUsed24 = false;
    DivideAddress24 = AllocFwdAddr();

    rungNow = -80;
    AllocStart();

    rungNow = -70;
    if(EepromFunctionUsed()) {
        // Where we hold the high byte to program in EEPROM while the low byte
        // programs.
        // Allocate 2 bytes needed for 24 bit integers variables.
        EepromHighByte = AllocOctetRam(3); // 16-24-32 bit integer high bytes
        EepromHighBytesCounter = AllocOctetRam();
        //AllocBitRam(&EepromHighByteWaitingAddr, &EepromHighByteWaitingBit);
    }
    rungNow = -50;
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    WriteRuntime();

    Comment("CompileFromIntermediate BEGIN");
    IntPc = 0; // Ok
    CompileFromIntermediate();
    Comment("CompileFromIntermediate END");

    DWORD i;
    for(i = 0; i < MAX_RUNGS; i++)
        Prog.HexInRung[i] = 0;
    for(i = 0; i < AvrProgWriteP; i++)
        if((AvrProg[i].rung >= 0) && (AvrProg[i].rung < MAX_RUNGS))
            Prog.HexInRung[AvrProg[i].rung]++;

    if(Prog.cycleDuty) {
        Comment("ClearBit YPlcCycleDuty");
        ClearBit(addrDuty, bitDuty);
    }

    rungNow = -30;
    Comment("GOTO next PLC cycle");
    /*
    if(Prog.mcu->core >= ClassicCore8K) {
        Instruction(OP_LDI, ZL, (BeginOfPLCCycle & 0xff));
        Instruction(OP_LDI, ZH, (BeginOfPLCCycle >> 8) & 0xff);
        Instruction(OP_IJMP, BeginOfPLCCycle, 0);
    } else {
        Instruction(OP_RJMP, BeginOfPLCCycle, 0);
    }
    */
    InstructionJMP(BeginOfPLCCycle);

    rungNow = -20;

    if(MultiplyUsed)
        MultiplyRoutine();
    if(DivideUsed)
        DivideRoutine();

    if(MultiplyUsed8)
        MultiplyRoutine8();
    if(DivideUsed8)
        DivideRoutine8();

    if(MultiplyUsed24)
        MultiplyRoutine24();
    if(DivideUsed24)
        DivideRoutine24();

    Instruction(OP_RJMP, AvrProgWriteP); // as CodeVisionAVR C Compiler // for label

    rungNow = -10;
    MemCheckForErrorsPostCompile();
    AddrCheckForErrorsPostCompile();

    ProgWriteP = AvrProgWriteP;

    rungNow = -5;
    WriteHexFile(f, fAsm);
    fflush(f);
    fclose(f);

    PrintVariables(fAsm);
    fflush(fAsm);
    fclose(fAsm);

    char str[MAX_PATH + 500];
    sprintf(str,
            _("Compile successful; wrote IHEX for AVR to '%s'.\r\n\r\n"
              "Remember to set the processor configuration (fuses) correctly. "
              "This does not happen automatically."),
            outFile);

    char str2[MAX_PATH + 500];
    sprintf(str2,
            _("Used %d/%d words of program flash (chip %d%% full)."),
            AvrProgWriteP,
            Prog.mcu->flashWords,
            (100 * AvrProgWriteP) / Prog.mcu->flashWords);

    char str3[MAX_PATH + 500];
    sprintf(str3, _("Used %d/%d byte of RAM (chip %d%% full)."), UsedRAM(), McuRAM(), (100 * UsedRAM()) / McuRAM());

    char str4[MAX_PATH + 500];
    sprintf(str4, "%s\r\n\r\n%s\r\n%s", str, str2, str3);

    if(AvrProgWriteP > Prog.mcu->flashWords) {
        CompileSuccessfulMessage(str4, MB_ICONWARNING);
        CompileSuccessfulMessage(str2, MB_ICONERROR);
    } else if(UsedRAM() > McuRAM()) {
        CompileSuccessfulMessage(str4, MB_ICONWARNING);
        CompileSuccessfulMessage(str3, MB_ICONERROR);
    } else
        CompileSuccessfulMessage(str4);

    AvrProgLdLen = AvrProgWriteP - BeginOfPLCCycle;
    //dbp("%d %d %d", AvrProgLdLen, AvrProgWriteP, BeginOfPLCCycle);
}
