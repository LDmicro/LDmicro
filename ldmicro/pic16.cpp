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
// A PIC16... assembler, for our own internal use, plus routines to generate
// code from the ladder logic structure, plus routines to generate the
// runtime needed to schedule the cycles.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

// clang-format off
#define ASM_LABEL 1
//                0 - no labels
//              * 1 - only if GOTO or CALL operations need a label
//                2 - always, all line is labeled

#define AUTO_BANKING // ++ (C) LDmicro.GitHub@gmail.com
//#define ASM_COMMENT_BANK //-

//http://www.piclist.com/techref/microchip/pages.htm
#define AUTO_PAGING // ++ (C) LDmicro.GitHub@gmail.com
//#define ASM_COMMENT_PAGE // -
#define MOVE_TO_PAGE_0 // ++

//#define ASM_BANKSEL //--
//#define ASM_PAGESEL //--

#define USE_TIMER0_AS_LADDER_CYCLE // Timer1 as PLC Cycle sourse is obsolete
#ifdef USE_TIMER0_AS_LADDER_CYCLE
#endif
// clang-format on

//-----------------------------------------------------------------------------
#include "ldmicro.h"
#include "intcode.h"
#include "filetracker.hpp"

#define DEST_F 1
#define DEST_W 0

#define MAX_PROGRAM_LEN 128 * 1024
//static PicAvrInstruction PicProg[MAX_PROGRAM_LEN];
static std::vector<PicAvrInstruction> PicProg;
static DWORD             PicProgWriteP;
static DWORD             BeginOfPLCCycle;

SDWORD PicProgLdLen = 0;

static int IntPcNow = -INT_MAX; //must be static

// Scratch variables, for temporaries
static DWORD ScratchI;
static DWORD ScratchS;
static DWORD Scratch0;
static DWORD Scratch1;
static DWORD Scratch2;
static DWORD Scratch3;
static DWORD Scratch4;
static DWORD Scratch5;
static DWORD Scratch6;
static DWORD Scratch7;

static DWORD Scratch8;
static DWORD Scratch9;
static DWORD Scratch10;
static DWORD Scratch11;

// The extra byte to program, for the EEPROM (because we can only set
// up one byte to program at a time, and we will be writing two-byte
// variables, in general).
static DWORD EepromHighByte; // Allocate high bytes needed for 16-24-32 bit integers variables.
//static DWORD EepromHighByteWaitingAddr; // obsolete
//static int EepromHighByteWaitingBit;    // obsolete
static DWORD EepromHighBytesCounter;

// Subroutines to do multiply/divide
static DWORD MultiplyRoutineAddress8 = -1;
static DWORD MultiplyRoutineAddress = -1; // 16x16
static DWORD MultiplyRoutineAddress24x16 = -1;
static DWORD MultiplyRoutineAddress24 = -1;
static DWORD MultiplyRoutineAddress32 = -1;
static DWORD DivideRoutineAddress8 = -1;
static DWORD DivideRoutineAddress = -1; // 16x16
static DWORD DivideRoutineAddress24x16 = -1;
static DWORD DivideRoutineAddress32 = -1;
static bool  MultiplyNeeded; // 16x16
static bool  MultiplyNeeded8;
static bool  MultiplyNeeded24x16;
static bool  DivideNeeded; // 16x16
static bool  DivideNeeded8;
static bool  DivideNeeded24x16;

// Subroutine to do BIN2BCD
static DWORD Bin32BcdRoutineAddress;
static bool  Bin32BcdNeeded;

// For yet unresolved references in jumps
static DWORD FwdAddrCount;

// clang-format off

// As I start to support the paging; it is sometimes necessary to pick
// out the high vs. low portions of the address, so that the high portion
// goes in PCLATH, and the low portion is just used for the jump.
#define FWD(x)       ((x) | 0x80000000)
#define FWD_HI(x)    ((x) | 0x40000000)
#define FWD_LO(x)    ((x) | 0x20000000)
#define IS_FWD(x)    ((x) & (FWD(0) | FWD_HI(0) | FWD_LO(0)))

#define NOTDEF(x)       ((x) | 0x80000000)
#define MULTYDEF(x)     ((x) | 0x40000000)
#define IS_NOTDEF(x)    ((x) & NOTDEF(0))
#define IS_MULTYDEF(x)  ((x) & MULTYDEF(0))
#define IS_UNDEF(x)     ((x) & (NOTDEF(0) | MULTYDEF(0)))

//-----------------------------------------------------------------------------
// Some useful registers, which I think are mostly in the same place on
// all the PIC16... devices.

// Core Registers
#define REG_INDF      0x00
#define REG_PCL       0x02

#define REG_STATUS    0x03
// PIC10F2xx
#define  STATUS_GPWUF BIT7 // GPIO Reset bit
#define  STATUS_CWUF  BIT6 // Comparator Wake-up on Change Flag bit
// others PICS
#define  STATUS_IRP   BIT7 // Register Bank Select bit (used for indirect addressing)
#define  STATUS_RP1   BIT6 // Register Bank Select bits
#define  STATUS_RP0   BIT5 //  (used for direct addressing)
// common bits
#define  STATUS_TO    BIT4
#define  STATUS_PD    BIT3
#define  STATUS_Z     BIT2
#define  STATUS_DC    BIT1
#define  STATUS_C     BIT0

#define REG_FSR       0x04
// Bank Select Register instead REG_STATUS(STATUS_RP1,STATUS_RP0)
#define REG_BSR       0x08
#define     BSR4      BIT4
#define     BSR3      BIT3
#define     BSR2      BIT2
#define     BSR1      BIT1
#define     BSR0      BIT0

#define REG_PCLATH    0x0a
#define REG_INTCON    0x0b
#define     GIE       BIT7 // Global Interrupt Enable bit
#define     PEIE      BIT6 // Peripheral Interrupt Enable bit
#define     T0IE      BIT5 // TMR0 Overflow Interrupt Enable bit
#define     INTE      BIT4 // RB0/INT External Interrupt Enable bit // 1 = The RB0/INT external interrupt occurred (must be cleared in software)
#define     RBIE      BIT3 // RB Port Change Interrupt Enable bit
#define     T0IF      BIT2 // TMR0 Overflow Interrupt Flag bit // 1 = TMR0 register has overflowed (must be cleared in software)
#define     INTF      BIT1 // RB0/INT External Interrupt Flag bit
#define     RBIF      BIT0 // RB Port Change Interrupt Flag bit // 1 = When at least one of the RB<7:4> pins changes state (must be cleared in software)

//static DWORD REG_IOCA    = -1; // PIC12 INTERRUPT-ON-CHANGE port register

// These move around from device to device.
// 0 means not defined(error!) or not exist in MCU.
// EEPROM Registers
static DWORD REG_EECON1  = -1;
#define          EEPGD     BIT7
#define          WREN      BIT2
#define          WR        BIT1
#define          RD        BIT0
static DWORD REG_EECON2  = -1;
static DWORD REG_EEDATA  = -1;
static DWORD REG_EEADR   = -1;
static DWORD REG_EEADRH  = -1;
static DWORD REG_EEDATL  = -1;
static DWORD REG_EEDATH  = -1;

//Analog Select Register
static DWORD REG_ANSEL   = -1;
static DWORD REG_ANSELH  = -1;

static DWORD REG_ANSELA  = -1;
static DWORD REG_ANSELB  = -1;
static DWORD REG_ANSELC  = -1;
static DWORD REG_ANSELD  = -1;
static DWORD REG_ANSELE  = -1;
static DWORD REG_ANSELF  = -1;
static DWORD REG_ANSELG  = -1;

//
static DWORD REG_PIR1    = -1; // PERIPHERAL INTERRUPT REQUEST REGISTER 1
#define          RCIF      BIT5
#define          TXIF      BIT4
static int       CCP1IF  = -1; // BIT2
static DWORD REG_PIE1    = -1; // 0x8c

//
static DWORD REG_TMR1L   = -1; // 0x0e
static DWORD REG_TMR1H   = -1; // 0x0f
static DWORD REG_T1CON   = -1; // 0x10
#define          TMR1ON    BIT0
#define          T1CKPS0   BIT4

static DWORD REG_T1GCON  = -1; //
#define          TMR1GE    BIT7
static DWORD REG_CCPR1L  = -1; // 0x15
static DWORD REG_CCPR1H  = -1; // 0x16
static DWORD REG_CCP1CON = -1; // 0x17
static DWORD REG_CMCON   = -1; // 0x1f
static DWORD REG_VRCON   = -1; // 0x9f

//USART
static DWORD REG_TXSTA   = -1; // 0x98
#define          TXEN      BIT5
#define          TRMT      BIT1// 1 is TSR empty, ready; 0 is TSR full, busy
static DWORD REG_RCSTA   = -1; // 0x18
#define          SPEN      BIT7
#define          CREN      BIT4
#define          FERR      BIT2
#define          OERR      BIT1
static DWORD REG_SPBRGH  = -1; // 0x99
static DWORD REG_SPBRG   = -1; // 0x99
static DWORD REG_TXREG   = -1; // 0x19
static DWORD REG_RCREG   = -1; // 0x1a
//static DWORD REG_BAUDCON = -1; // BAUD RATE CONTROL REGISTER

//ADC
static DWORD REG_ADRESH  = -1; // 0x1e
static DWORD REG_ADRESL  = -1; // 0x9e
static DWORD REG_ADCON0  = -1; // 0x1f
static DWORD REG_ADCON1  = -1; // 0x9f

//PWM Timer2
static DWORD REG_T2CON   = -1; // 0x12
#define          TMR2ON    BIT2
static DWORD REG_CCPR2L  = -1; // 0x1b // Pulse Width
static DWORD REG_CCP2CON = -1; // 0x1d
#define          DC2B0     BIT4
#define          DC2B1     BIT5
static DWORD REG_PR2     = -1; // 0x92 // Period

//
static DWORD REG_TMR0    = -1; // 0x01
static DWORD REG_OPTION  = -1; // 0x81 or 0x181 //0x95
// PIC10F2xx
#define          _GPWU        BIT7 // Enable Wake-up on Pin Change bit (GP0, GP1, GP3)
#define          _GPPU        BIT6 // Enable Weak Pull-ups bit (GP0, GP1, GP3)
// others PICS
#define          _RBPU        BIT7 // PORTB Pull-up Enable bit
// common bits
#define          T0CS         BIT5
#define          PSA          BIT3

// static int       WDTE    = -1; //

// OSCILLATOR CONTROL REGISTER
static DWORD REG_OSCON   = -1;
#define          SCS0         BIT0 //..BIT1
#define          IRCF0        BIT3 //..BIt6
#define          SPLLEN       BIT7
// clang-format on

static DWORD CONFIG_ADDR1 = -1;
static DWORD CONFIG_ADDR2 = -1;
//-----------------------------------------------------------------------------

static uint32_t IntPc = 0;

static void CompileFromIntermediate(bool topLevel);

//-----------------------------------------------------------------------------
// A convenience function, whether we are using a particular MCU.
//-----------------------------------------------------------------------------
static bool McuIs(const char *str)
{
    if(!Prog.mcu)
        return 0;
    return strcmp(Prog.mcu->mcuName, str) == 0;
}

bool McuAs(const char *str)
{
    if(!Prog.mcu)
        return 0;
    return strstr(Prog.mcu->mcuName, str) != nullptr;
}

//-----------------------------------------------------------------------------
static char *GetName(int addrAt, char *name)
{
    name[0] = '\0';
    int   i = 0;
    char *s;
    s = strstr(PicProg[addrAt].commentAsm, "REG_");
    if(s) {
        s += 4;
        if(isalpha_(*s)) {
            while(isal_num(*s)) {
                name[i] = *s;
                s++;
                i++;
            }
            name[i] = '\0';
        }
    }
    return name;
}

//-----------------------------------------------------------------------------
static void discoverArgs(int addrAt, char *arg1s, char *arg1comm)
{
    char s[MAX_NAME_LEN];
    if(asm_discover_names <= 2) {
        sprintf(s, "0x%02X", (BYTE)PicProg[addrAt].arg1);
    } else {
        GetName(addrAt, s);
        if(strlen(s) == 0)
            sprintf(s, "0x%02X", PicProg[addrAt].arg1);
    }

    sprintf(arg1s, "%-16s", s);

    arg1comm[0] = '\0';

    if((asm_discover_names == 1) || (asm_discover_names == 2)) {
        GetName(addrAt, s);
        if(strcmp(arg1s, s) != 0)
            sprintf(arg1comm, "%s ; %s", arg1comm, s);
    }

    if(asm_discover_names >= 3)
        if(arg1s[0] != '0')
            sprintf(arg1comm, "%s ; 0x%X", arg1comm, PicProg[addrAt].arg1);

    if((asm_discover_names == 2) || (asm_discover_names == 4))
        sprintf(arg1comm, "%s ; %d", arg1comm, PicProg[addrAt].arg1);
}

//-----------------------------------------------------------------------------
// Wipe the program and set the write pointer back to the beginning.
//-----------------------------------------------------------------------------
static void WipeMemory()
{
    if(PicProg.size() < MAX_PROGRAM_LEN)
        PicProg.resize(MAX_PROGRAM_LEN);
    for(uint32_t i = 0; i < PicProg.size(); ++i)
        memset(&PicProg[i], 0, sizeof(PicAvrInstruction));
    PicProgWriteP = 0;
}

//-----------------------------------------------------------------------------
static DWORD Bank(DWORD reg)
{
    if(IS_NOTDEF(reg))
        reg &= ~(NOTDEF(0));
    if(IS_MULTYDEF(reg))
        reg &= ~(MULTYDEF(0));
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        if(reg & ~0x0FFF)
            THROW_COMPILER_EXCEPTION_FMT("0x%X", reg);
        reg &= 0x0F80;
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        if(reg & ~0x01FF)
            THROW_COMPILER_EXCEPTION_FMT("0x%X", reg);
        reg &= 0x0180;
    } else if(Prog.mcu->core == BaselineCore12bit) {
        if(reg & ~0x007F)
            THROW_COMPILER_EXCEPTION_FMT("0x%X", reg);
        reg &= 0x0000;
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    return reg;
}

//-----------------------------------------------------------------------------
static DWORD BankMask()
{
    DWORD reg;
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        reg = 0x0F80;
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        reg = 0x0180;
    } else if(Prog.mcu->core == BaselineCore12bit) {
        reg = 0x0000;
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    return reg;
}

//-----------------------------------------------------------------------------
static int IsCoreRegister(DWORD reg)
{
    reg &= ~Bank(reg); // Clear Bank
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        switch(reg) {
            case REG_INDF:
            case REG_PCL:
            case REG_STATUS:
            case REG_FSR:
            case REG_PCLATH:
            case REG_INTCON:
            case REG_BSR:
                return 1; // in all banks same, dont need to change banks
            default:
                return 0;
        }
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        switch(reg) {
            case REG_INDF:
            case REG_PCL:
            case REG_STATUS:
            case REG_FSR:
            case REG_PCLATH:
            case REG_INTCON:
                return 1; // in all banks same, dont need to change banks
            default:
                return 0;
        }
    } else if(Prog.mcu->core == BaselineCore12bit) {
        switch(reg) {
            case REG_INDF:
            case REG_PCL:
            case REG_STATUS:
            case REG_FSR:
            case REG_INTCON:
                return 1; // in all banks same, dont need to change banks
            default:
                return 0;
        }
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    return 0;
}

//-----------------------------------------------------------------------------
// clang-format off
#define IS_SKIP        2
#define IS_BANK        1
#define IS_ANY_BANK    0
#define IS_RETS       -1
#define IS_PAGE       -2
#define IS_GOTO       -2
#define IS_CALL       -3
// clang-format on

static int IsOperation(PicOp op)
{
    switch(op) {
        case OP_BTFSC:
        case OP_BTFSS:
        case OP_DECFSZ:
        case OP_INCFSZ:
            return IS_SKIP; // can need to change bank
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
        case OP_SWAPF:
        case OP_TRIS:
            return IS_BANK; // can need to change bank
        case OP_CLRWDT:
        case OP_MOVLW:
        case OP_MOVLB:
        case OP_MOVLP:
        case OP_NOP_:
        case OP_COMMENT_:
        case OP_COMMENT_INT:
            //      case OP_SUBLW:
        case OP_IORLW:
        case OP_OPTION:
        case OP_SLEEP_:
            return IS_ANY_BANK; // not need to change bank
        case OP_RETURN:
        case OP_RETFIE:
        case OP_RETLW:
            return IS_RETS; // not need to change bank
        case OP_GOTO:
            return IS_GOTO; // can need to change page
        case OP_CALL:
            return IS_CALL; // can need to change page
        default:
            THROW_COMPILER_EXCEPTION_FMT("OP_%d", op);
            return 0;
    }
}

//-----------------------------------------------------------------------------
// And use macro for bugtracking
#define Instruction(...) _Instruction(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)

// Note that all of these are single instructions on the PIC; this is not the
// case for their equivalents on the AVR!
/*
#define SetBit(reg, b)      Instruction(OP_BSF, reg, b)
#define ClearBit(reg, b)    Instruction(OP_BCF, reg, b)
#define IfBitClear(reg, b)  Instruction(OP_BTFSS, reg, b)
#define IfBitSet(reg, b)    Instruction(OP_BTFSC, reg, b)
/**/
// clang-format off
#define SetBit(...)         Instruction(OP_BSF, __VA_ARGS__)
#define ClearBit(...)       Instruction(OP_BCF, __VA_ARGS__)
#define IfBitSet(...)       Instruction(OP_BTFSC, __VA_ARGS__)
#define IfBitClear(...)     Instruction(OP_BTFSS, __VA_ARGS__)
#define SkpIfBitSet(...)    Instruction(OP_BTFSS, __VA_ARGS__)
#define SkpIfBitClear(...)  Instruction(OP_BTFSC, __VA_ARGS__)

// http://picprojects.org.uk/projects/pseudoins.htm
#define skpnc               Instruction(OP_BTFSC, REG_STATUS, STATUS_C); // Skip on No Carry
#define skpc                Instruction(OP_BTFSS, REG_STATUS, STATUS_C); // Skip on Carry
#define skpnz               Instruction(OP_BTFSC, REG_STATUS, STATUS_Z); // Skip on Non Zero // equal to IfBitSet(REG_STATUS, STATUS_Z);
#define skpz                Instruction(OP_BTFSS, REG_STATUS, STATUS_Z); // Skip on Zero     // equal to IfBitClear(REG_STATUS, STATUS_Z);
// clang-format on

//-----------------------------------------------------------------------------
// Store an instruction at the next spot in program memory.  Error condition
// if this spot is already filled. We don't actually assemble to binary yet;
// there may be references to resolve.
//-----------------------------------------------------------------------------
static void _Instruction(int l, const char *f, const char *args, PicOp op, DWORD arg1 = 0, DWORD arg2 = 0,
                         const char *comment = nullptr);
static void _Instruction(int l, const char *f, const char *args, PicOp op, DWORD arg1, DWORD arg2, const char *comment)
{
    if(IsOperation(op) >= IS_BANK) {
        if(arg1 == -1) {
            THROW_COMPILER_EXCEPTION_FMT("%d %s Not inited register!", l, f);
        }
    }

    if(PicProg[PicProgWriteP].opPic != OP_VACANT_)
        THROW_COMPILER_EXCEPTION("Internal error.");

    if(op == OP_COMMENT_INT) {
        if(comment) {
            if(strlen(PicProg[PicProgWriteP].commentInt))
                strncatn(PicProg[PicProgWriteP].commentInt, "\n    ; ", MAX_COMMENT_LEN);
            strncatn(PicProg[PicProgWriteP].commentInt, comment, MAX_COMMENT_LEN);
        }
        return;
    }

    if(PicProgWriteP)
        if((IsOperation(PicProg[PicProgWriteP - 1].opPic) == IS_SKIP) && (IsOperation(op) >= IS_BANK)) {
            if((!IsCoreRegister(PicProg[PicProgWriteP - 1].arg1)) && (!IsCoreRegister(arg1))) {
                if(Bank(PicProg[PicProgWriteP - 1].arg1) != Bank(arg1)) {
                    PicOp op_ = PicProg[PicProgWriteP - 1].opPic;
                    DWORD arg1_ = PicProg[PicProgWriteP - 1].arg1;
                    DWORD arg2_ = PicProg[PicProgWriteP - 1].arg2;
                    switch(op_) {
                        case OP_BTFSC: // IfBitSet(...)
                            SetBit(REG_STATUS, STATUS_DC, nullptr);
                            IfBitClear(arg1_, arg2_, nullptr);
                            ClearBit(REG_STATUS, STATUS_DC, nullptr);
                            IfBitSet(REG_STATUS, STATUS_DC, nullptr);
                            break;
                        case OP_BTFSS: // IfBitClear(...)
                            ClearBit(REG_STATUS, STATUS_DC, nullptr);
                            IfBitSet(arg1_, arg2_, nullptr);
                            SetBit(REG_STATUS, STATUS_DC, nullptr);
                            IfBitClear(REG_STATUS, STATUS_DC, nullptr);
                            break;
                        case OP_DECFSZ:
                        case OP_INCFSZ:
                        default:
                            THROW_COMPILER_EXCEPTION("Bank select error!");
                    }
                }
            }
        }

    PicProg[PicProgWriteP].arg1orig = arg1; // arg1 can be changed by bank or page corretion;
    //

    PicProg[PicProgWriteP].opPic = op;
    PicProg[PicProgWriteP].arg1 = arg1;
    PicProg[PicProgWriteP].arg2 = arg2;

    if(args) {
        if(strlen(PicProg[PicProgWriteP].commentAsm))
            strncatn(PicProg[PicProgWriteP].commentAsm, " ; ", MAX_COMMENT_LEN);
        strncatn(PicProg[PicProgWriteP].commentAsm, "(", MAX_COMMENT_LEN);
        strncatn(PicProg[PicProgWriteP].commentAsm, args, MAX_COMMENT_LEN);
        strncatn(PicProg[PicProgWriteP].commentAsm, ")", MAX_COMMENT_LEN);
    }
    if(comment) {
        if(strlen(PicProg[PicProgWriteP].commentAsm))
            strncatn(PicProg[PicProgWriteP].commentAsm, " ; ", MAX_COMMENT_LEN);
        strncatn(PicProg[PicProgWriteP].commentAsm, comment, MAX_COMMENT_LEN);
    }
    PicProg[PicProgWriteP].rung = rungNow;
    PicProg[PicProgWriteP].IntPc = IntPcNow;
    if(f)
        strncpy(PicProg[PicProgWriteP].f, f, MAX_PATH);
    PicProg[PicProgWriteP].l = l;
    PicProgWriteP++;
}

static void _Instruction(int l, const char *f, const char *args, PicOp op, DWORD arg1, DWORD arg2,
                         const NameArray &comment)
{
    _Instruction(l, f, args, op, arg1, arg2, comment.c_str());
}
/*
static void _Instruction(int l, const char *f, const char *args, PicOp op, DWORD arg1, DWORD arg2)
{
    _Instruction(l, f, args, op, arg1, arg2, nullptr);
}

static void _Instruction(int l, const char *f, const char *args, PicOp op, DWORD arg1)
{
    _Instruction(l, f, args, op, arg1, 0, nullptr);
}

static void _Instruction(int l, const char *f, const char *args, PicOp op)
{
    _Instruction(l, f, args, op, 0, 0, nullptr);
}
*/
//-----------------------------------------------------------------------------
static void _SetInstruction(int l, const char *f, const char *args, DWORD addr, PicOp op, DWORD arg1 = 0,
                            DWORD arg2 = 0, const char *comment = nullptr);
static void _SetInstruction(int l, const char *f, const char *args, DWORD addr, PicOp op, DWORD arg1, DWORD arg2,
                            const char *comment)
//for setiing interrupt vector, page correcting, etc
{
    DWORD savePicProgWriteP = PicProgWriteP;
    PicProgWriteP = addr;
    PicProg[PicProgWriteP].opPic = OP_VACANT_;

    if(comment) {
        if(strlen(PicProg[PicProgWriteP].commentAsm))
            strncatn(PicProg[PicProgWriteP].commentAsm, " ; ", MAX_COMMENT_LEN);
        strncatn(PicProg[PicProgWriteP].commentAsm, comment, MAX_COMMENT_LEN);
    }

    _Instruction(l, f, args, op, arg1, arg2, "");
    PicProgWriteP = savePicProgWriteP;
}
/*
static void _SetInstruction(int l, const char *f, const char *args, DWORD addr, PicOp op, DWORD arg1, DWORD arg2)
{
    _SetInstruction(l, f, args, addr, op, arg1, arg2, nullptr);
}

static void _SetInstruction(int l, const char *f, const char *args, DWORD addr, PicOp op, DWORD arg1)
{
    _SetInstruction(l, f, args, addr, op, arg1, 0, nullptr);
}

static void _SetInstruction(int l, const char *f, const char *args, DWORD addr, PicOp op, DWORD arg1,
                            const char *comment)
{
    _SetInstruction(l, f, args, addr, op, arg1, 0, comment);
}
*/
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
        Instruction(OP_COMMENT_INT, 0, 0, buf);
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

    bool  seen = false;
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        if(PicProg[i].arg1 == addr) {
            PicProg[i].arg1 = PicProgWriteP;
            seen = true;
        } else if(PicProg[i].arg1 == FWD_LO(addr)) {
            PicProg[i].arg1 = PicProgWriteP; // Full address are required for generating the proper labels in asm text.
            seen = true;
        } else if(PicProg[i].arg1 == FWD_HI(addr)) {
            PicProg[i].arg1 = (PicProgWriteP >> 8);
        }
    }
}

//-----------------------------------------------------------------------------
static void BankCorrect(DWORD addr, int nAdd, int nSkip, DWORD bankNew)
{
    DWORD i;
    for(i = addr - nSkip; i <= addr + nAdd; i++) {
        PicProg[i].BANK = bankNew;
    }
    i = addr - nSkip - 1;
    while(i > 0) {
        if(((IsOperation(PicProg[i].opPic) >= IS_BANK) && IsCoreRegister(PicProg[i].arg1))
           || (IsOperation(PicProg[i].opPic) == IS_ANY_BANK)) {
            PicProg[i].BANK = bankNew;
        } else {
            break;
        }
        i--;
    }
}

//-----------------------------------------------------------------------------
static int BankSelect(DWORD addr, int nAdd, int nSkip, DWORD bankNow, DWORD bankNew)
{
    DWORD savePicProgWriteP = PicProgWriteP;
    PicProgWriteP = addr - nSkip;
    bankNow = Bank(bankNow); // reassurance
    bankNew = Bank(bankNew);
    int n = 0;
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        if(bankNow != bankNew) {
            Instruction(OP_MOVLB, bankNew >> 7);
            n++;
        }
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        if((bankNow ^ bankNew) & 0x0080) {
            if(bankNew & 0x0080) {
                Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
            } else {
                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
            }
            n++;
        }
        if((bankNow ^ bankNew) & 0x0100) {
            if(bankNew & 0x0100) {
                Instruction(OP_BSF, REG_STATUS, STATUS_RP1);
            } else {
                Instruction(OP_BCF, REG_STATUS, STATUS_RP1);
            }
            n++;
        }
    } else if(Prog.mcu->core == BaselineCore12bit) {
        if(bankNow != bankNew) {
            THROW_COMPILER_EXCEPTION("Internal error.");
        }
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");

    if(n != nAdd)
        THROW_COMPILER_EXCEPTION_FMT("%d %d", n, nAdd);

    PicProgWriteP = savePicProgWriteP + nAdd;

    if(n)
        BankCorrect(addr, nAdd, nSkip, bankNew);

    return nAdd;
}

static DWORD CalcMaxBank()
{
    DWORD maxBank = 0;
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) >= IS_BANK)
            maxBank = std::max(maxBank, Bank(PicProg[i].arg1));
    }
    return maxBank;
}

static int BankSelectCheck(DWORD bankNow, DWORD bankNew)
{
    bankNow = Bank(bankNow); // reassurance
    bankNew = Bank(bankNew);
    int n = 0; // need OPs
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        if(bankNow != bankNew)
            n++;
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        if((bankNow ^ bankNew) & 0x0080)
            n++;
        if((bankNow ^ bankNew) & 0x0100)
            n++;
    } else if(Prog.mcu->core == BaselineCore12bit) {
        if(bankNow != bankNew) {
            THROW_COMPILER_EXCEPTION("Internal error.");
        }
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    return n;
}

static DWORD notRealocableAddr = 0; // upper range

static DWORD BankCorrection_(DWORD addr, DWORD bank, int is_call)
{
/*
    if(PicProgWriteP >= Prog.mcu->flashWords) {
        Error("Not enough memory for BANK and PAGE correction! %d %d", PicProgWriteP, Prog.mcu->flashWords);
        return 0;
    }
*/
    int   corrected = 0;
    DWORD i, j;
    int   nAdd;
    DWORD BB; // bank before
    DWORD arg1;
doBankCorrection:
    corrected = 0;
    i = addr;
    if(IS_NOTDEF(PicProg[i].BANK)) {
        PicProg[i].BANK = bank;
    } else if(PicProg[i].BANK != bank) {
        PicProg[i].BANK = MULTYDEF(0);
    }
    while((i < PicProgWriteP)/* && (PicProgWriteP < Prog.mcu->flashWords)*/) {
        if(IS_NOTDEF(PicProg[i].BANK)) {
            PicProg[i].BANK = PicProg[i - 1].BANK;
        }
        if(IsOperation(PicProg[i].opPic) >= IS_BANK) {
            nAdd = 0;
            if(!IsCoreRegister(PicProg[i].arg1)) {
                if(IS_UNDEF(PicProg[i].BANK) || IS_UNDEF(PicProg[i - 1].BANK)
                   || (PicProg[i - 1].BANK != Bank(PicProg[i].arg1))) {
                    if(IS_UNDEF(PicProg[i].BANK) || IS_UNDEF(PicProg[i - 1].BANK)) {
                        BB = PicProg[i].arg1 ^ BankMask();
                    } else {
                        BB = PicProg[i - 1].BANK;
                    }
                    arg1 = PicProg[i].arg1;
                    nAdd = BankSelectCheck(BB, arg1);
                }
            }
            if(nAdd) {
                int   nSkip = 0;
                DWORD ii = i; // address where we doing insert
                while((ii > 0) && (IsOperation(PicProg[ii - 1].opPic) == IS_SKIP)
                      || (PicProg[ii - 1].opPic == OP_MOVLW) // for asm compat
                ) {
                    // can't insert op betwen IS_SKIP and any opPic
                    ii--;
                    nSkip++;
                }
                if(ii <= notRealocableAddr) {
                    THROW_COMPILER_EXCEPTION("Internal error.");
                }
                for(j = 0; j < PicProgWriteP; j++) {
                    if(IsOperation(PicProg[j].opPic) <= IS_PAGE)
                        if(PicProg[j].arg1 > ii)
                            PicProg[j].arg1 += nAdd; // Correcting target addresses.
                }
                for(j = PicProgWriteP - 1; j >= ii; j--) {
                    // prepare a place for inserting bank correction operations
                    memcpy(&PicProg[j + nAdd], &PicProg[j], sizeof(PicProg[0]));
                }
                for(j = ii; j < (ii + nAdd); j++) {
                    memset(&PicProg[j], 0, sizeof(PicProg[j]));
                    PicProg[j].opPic = OP_VACANT_;
                    PicProg[j].arg1 = 0;
                    PicProg[j].arg2 = 0;
                    sprintf(PicProg[j].commentAsm, " BS(0x%02X,0x%02X)", BB, arg1);
                    sprintf(PicProg[j].commentInt, "");
                }
                int n = 0;
                n = BankSelect(i, nAdd, nSkip, BB, arg1);
                if(nAdd != n)
                    THROW_COMPILER_EXCEPTION_FMT("nAdd=%d n=%d", nAdd, n);
                corrected++;
                break;
            }
        } else {
            //
        }
        i++;
    }
    if(corrected && (corrected < 20))
        goto doBankCorrection;

    if(PicProgWriteP >= Prog.mcu->flashWords)
        Error("Not enough memory for BANK and PAGE correction!");

    return bank;
}

static DWORD BankPreSet(DWORD addr, DWORD bank, int is_call)
{
    DWORD i;
    i = addr;
    if(IS_NOTDEF(PicProg[i].BANK)) {
        PicProg[i].BANK = bank;
    } else if(PicProg[i].BANK != bank) {
        PicProg[i].BANK = MULTYDEF(0);
    }

    // Set target bank.
    for(i = addr; i < PicProgWriteP; i++) {
        if((IsOperation(PicProg[i].opPic) >= IS_BANK) && (!IsCoreRegister(PicProg[i].arg1))) {
            if(IS_NOTDEF(PicProg[i].BANK))
                PicProg[i].BANK = Bank(PicProg[i].arg1);
        }

        if((IsOperation(PicProg[i].opPic) == IS_RETS) && is_call) {
            break;
            //return PicProg[i].BANK;
        }
    }

    // Set target bank in flow .
    for(i = addr; i < PicProgWriteP; i++) {
        if(((IsOperation(PicProg[i].opPic) >= IS_BANK) && (IsCoreRegister(PicProg[i].arg1)))
           || (IsOperation(PicProg[i].opPic) <= IS_ANY_BANK)) {
            if(IS_NOTDEF(PicProg[i].BANK)) {
                PicProg[i].BANK = PicProg[i - 1].BANK;
            }
        }

        if((IsOperation(PicProg[i].opPic) == IS_RETS) && is_call) {
            break;
            //return PicProg[i].BANK;
        }
    }

    for(i = addr; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) == IS_CALL) {
            if(PicProg[i].arg1 >= PicProgWriteP)
                THROW_COMPILER_EXCEPTION("Internal error.");
            if(PicProg[i].arg1 < 0)
                THROW_COMPILER_EXCEPTION("Internal error.");

            BankPreSet(PicProg[i].arg1, PicProg[i].BANK, 1);
        }

        if((IsOperation(PicProg[i].opPic) == IS_RETS) && is_call) {
            break;
            //return PicProg[i].BANK;
        }
    }

    // Marking the operation as the double(multi) entry point.
    for(i = addr; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) == IS_GOTO) {
            if(PicProg[i].arg1 >= PicProgWriteP) {
                IntOp *a = &IntCode[PicProg[i].IntPc];
                THROW_COMPILER_EXCEPTION_FMT("Internal error. [%d:%s]", a->fileLine, a->fileName.c_str());
            }
            if(PicProg[i].arg1 < 0)
                THROW_COMPILER_EXCEPTION("Internal error.");

            if(IS_NOTDEF(PicProg[PicProg[i].arg1].BANK)) {
                PicProg[PicProg[i].arg1].BANK = PicProg[i].BANK;
            } else if(PicProg[PicProg[i].arg1].BANK != PicProg[i].BANK) {
                PicProg[PicProg[i].arg1].BANK = MULTYDEF(0);
            }
            //PicProg[PicProg[i].arg1].BANK = MULTYDEF(0);
        }

        if((IsOperation(PicProg[i].opPic) == IS_RETS) && is_call) {
            break;
            //return PicProg[i].BANK;
        }
    }

    for(i = addr; i < PicProgWriteP; i++) {
        if((IsOperation(PicProg[i].opPic) == IS_RETS) && is_call) {
            return PicProg[i].BANK;
        }
    }

    return PicProg[i].BANK;
}

static void BankCorrection()
{
    DWORD maxBank = CalcMaxBank();
    if(!maxBank)
        return;

    for(DWORD i = 1; i < PicProgWriteP; i++) {
        if((IsOperation(PicProg[i - 1].opPic) == IS_SKIP) && (IsOperation(PicProg[i].opPic) >= IS_BANK)) {
            if((!IsCoreRegister(PicProg[i - 1].arg1)) && (!IsCoreRegister(PicProg[i].arg1))) {
                if(Bank(PicProg[i].arg1) != Bank(PicProg[i - 1].arg1)) {
                    THROW_COMPILER_EXCEPTION("Bank select error!");
                }
            }
        }
    }

    // Marking bank as indeterminate.
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        PicProg[i].BANK = NOTDEF(0);
    }

    // Set start bank 0.
    PicProg[0].BANK = 0;

    BankPreSet(0, 0, 0);

    // Marking the interrupt vector operation as the multi entry point.
    if(Prog.mcu->core != BaselineCore12bit) {
        PicProg[4].BANK = MULTYDEF(0);
    }
    BankCorrection_(0, 0, 0);
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) >= IS_BANK)
            PicProg[i].arg1 &= ~Bank(PicProg[i].arg1);
    }
}

//---------------------------------------------------------------------------
// clang-format off
#define L_LABEL         0x01
#define I_LABEL         0x02
#define DIR_SET         0x08

#define ENDS_RET        0x40
#define ENDS_GOTO       0x80
#define ENDS_           ((ENDS_RET) | (ENDS_GOTO))
// clang-format on
static void PagePreSet()
{
    // mark the PCLATH as not setted, not defined.
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        PicProg[i].PCLATH = NOTDEF(0);
        PicProg[i].label = 0;
    }
    // PCLATH is 0 after reset
    PicProg[0].PCLATH = 0;
    PicProg[0].label |= I_LABEL;

    // GOTO progStart
    PicProg[3].PCLATH = 0;
    PicProg[3].label |= I_LABEL;

    // Mark the interrupt vector operation as the multi entry point.
    if(Prog.mcu->core != BaselineCore12bit) {
        PicProg[4].PCLATH = MULTYDEF(0);
        PicProg[4].label |= I_LABEL;
    }
    // Mark Labels for GOTO and CALL
    for(DWORD i = 1; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) <= IS_PAGE) {
            PicProg[PicProg[i].arg1].label |= L_LABEL;
        }
        if(IsOperation(PicProg[i].opPic) == IS_GOTO) {
            if(IsOperation(PicProg[i - 1].opPic) != IS_SKIP) {
                // ended goto
                PicProg[i].label |= ENDS_GOTO;
            }
        }
        if(IsOperation(PicProg[i].opPic) == IS_RETS) {
            if(IsOperation(PicProg[i - 1].opPic) != IS_SKIP) {
                // ended return
                PicProg[i].label |= ENDS_RET;
            }
        }
    }
    // direct set the PCLATH
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        if(Prog.mcu->core == BaselineCore12bit) {
            // TODO
        } else {
            if((PicProg[i].opPic == OP_CLRF) && (PicProg[i].arg1 == REG_PCLATH)) {
                PicProg[i].PCLATH = 0;
                PicProg[i].label |= DIR_SET;
            } else if(PicProg[i].opPic == OP_MOVLP) {
                PicProg[i].PCLATH = PicProg[i].arg1;
                PicProg[i].label |= DIR_SET;
            } else if((PicProg[i].opPic == OP_BSF) && (PicProg[i].arg1 == REG_PCLATH)) {
                PicProg[i].PCLATH &= ~NOTDEF(0);
                PicProg[i].PCLATH |= 1 << PicProg[i].arg2;
                PicProg[i].label |= DIR_SET;
                if(((PicProg[i - 1].opPic == OP_BCF) || (PicProg[i - 1].opPic == OP_BSF))
                   && (PicProg[i - 1].arg1 == REG_PCLATH)) {
                    PicProg[i - 1].PCLATH &= ~NOTDEF(0);
                    PicProg[i - 1].PCLATH |= 1 << PicProg[i].arg2;
                    PicProg[i].PCLATH |= PicProg[i - 1].PCLATH;
                    PicProg[i - 1].label |= DIR_SET;
                }
            } else if((PicProg[i].opPic == OP_BCF) && (PicProg[i].arg1 == REG_PCLATH)) {
                PicProg[i].PCLATH &= ~NOTDEF(0);
                PicProg[i].PCLATH &= ~(1 << PicProg[i].arg2);
                PicProg[i].label |= DIR_SET;
                if(((PicProg[i - 1].opPic == OP_BCF) || (PicProg[i - 1].opPic == OP_BSF))
                   && (PicProg[i - 1].arg1 == REG_PCLATH)) {
                    PicProg[i - 1].PCLATH &= ~NOTDEF(0);
                    PicProg[i - 1].PCLATH &= ~(1 << PicProg[i].arg2);
                    PicProg[i].PCLATH |= PicProg[i - 1].PCLATH;
                    PicProg[i - 1].label |= DIR_SET;
                }
            } else if((PicProg[i].opPic == OP_MOVWF) && (PicProg[i].arg1 == REG_PCLATH)) {
                if(PicProg[i - 1].opPic == OP_MOVLW) {
                    PicProg[i].PCLATH = PicProg[i - 1].arg1;
                    PicProg[i].label |= DIR_SET;
                } else if((PicProg[i - 1].opPic == OP_ADDWF) && (PicProg[i - 1].arg2 == DEST_W)
                          && (PicProg[i - 2].opPic == OP_MOVLW)) {
                    // calculated PCLATH
                    // used in table
                    PicProg[i].PCLATH = MULTYDEF(0);
                    PicProg[i].label |= DIR_SET;
                } else {
                    THROW_COMPILER_EXCEPTION_FMT("PagePreSet() error at addr 0x%X", i);
                }
            } else if((IsOperation(PicProg[i].opPic) == IS_BANK) && (PicProg[i].arg2 == DEST_F)
                      && (PicProg[i].arg1 == REG_PCLATH)) {
                // calculated PCLATH
                // used in table
                PicProg[i].PCLATH = MULTYDEF(0);
                PicProg[i].label |= DIR_SET;
            }
        }
    }
    // PCLATH after CALL or GOTO will
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) <= IS_PAGE) {
            PicProg[PicProg[i].arg1].PCLATH = PicProg[i].arg1 >> 8;
        }
    }
    //
    for(DWORD i = 1; i < PicProgWriteP; i++) { // copy-paste v
        if(PicProg[i].label & L_LABEL) {
            if(PicProg[i - 1].label & ENDS_) {
                //
            } else {
                if(IS_MULTYDEF(PicProg[i - 1].PCLATH)) {
                    PicProg[i].PCLATH = MULTYDEF(0);
                } else if(!IS_NOTDEF(PicProg[i - 1].PCLATH)) {
                    if(PicProg[i - 1].PCLATH >> 3 != PicProg[i].PCLATH >> 3) {
                        PicProg[i].PCLATH = MULTYDEF(0);
                    }
                }
            }
        }
    }
    // pass thru the PCLATH
    for(DWORD i = 1; i < PicProgWriteP; i++) {
        //if(IS_NOTDEF(PicProg[i].PCLATH)) {
        if(PicProg[i - 1].label & ENDS_) {
            //
        } else {
            /*
                if(IsOperation(PicProg[i-1].opPic) == IS_GOTO) {
                    if(PicProg[i].PCLATH & MULTYDEF(0))
                        PicProg[i].PCLATH = PicProg[i-1].arg1 >> 8;
                } else
                /**/
            if(IsOperation(PicProg[i - 1].opPic) == IS_CALL) {
                PicProg[i].PCLATH = MULTYDEF(0);
            } else if(!IS_NOTDEF(PicProg[i - 1].PCLATH)) {
                if(!(PicProg[i].label & DIR_SET))
                    PicProg[i].PCLATH = PicProg[i - 1].PCLATH;
            }
        }
        //}
    }
    // double enter to Label
    for(DWORD i = 1; i < PicProgWriteP; i++) { // copy-paste ^
        if(PicProg[i].label & L_LABEL) {
            if(PicProg[i - 1].label & ENDS_) {
                //
            } else {
                if(IS_MULTYDEF(PicProg[i - 1].PCLATH)) {
                    PicProg[i].PCLATH = MULTYDEF(0);
                } else if(!IS_NOTDEF(PicProg[i - 1].PCLATH)) {
                    if(PicProg[i - 1].PCLATH >> 3 != PicProg[i].PCLATH >> 3) {
                        PicProg[i].PCLATH = MULTYDEF(0);
                    }
                }
            }
        }
    }
    // pass thru the RET's PCLATH
    for(DWORD i = 1; i < PicProgWriteP; i++) {
        if(PicProg[i].label & ENDS_RET) {
            if(!IS_NOTDEF(PicProg[i - 1].PCLATH)) {
                if(!(PicProg[i].label & DIR_SET))
                    PicProg[i].PCLATH = PicProg[i - 1].PCLATH;
            }
        }
    }
}

//-----------------------------------------------------------------------------
static int PageSelectCheck(DWORD PCLATH, DWORD PCLATHnew)
{
    if(IS_UNDEF(PCLATH)) {
        PCLATHnew &= (1 << BIT3) | (1 << BIT4);
        PCLATH = ~PCLATHnew;
        PCLATH &= (1 << BIT3) | (1 << BIT4);
    }

    int n = 0;
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        if((PCLATH >> 3) != (PCLATHnew >> 3))
            n++;
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        if((PCLATH ^ PCLATHnew) & (1 << BIT3))
            n++;
        if((PCLATH ^ PCLATHnew) & (1 << BIT4))
            n++;
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    return n;
}

//---------------------------------------------------------------------------
static int PageSelect(DWORD addr, DWORD *PCLATH, DWORD PCLATHnew)
{
    if(IS_UNDEF(*PCLATH)) {
        PCLATHnew &= (1 << BIT3) | (1 << BIT4);
        *PCLATH = ~PCLATHnew;
        *PCLATH &= (1 << BIT3) | (1 << BIT4);
    }

    int n = 0;
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        if((*PCLATH >> 3) != (PCLATHnew >> 3)) {
            SetInstruction(addr, OP_MOVLP, PCLATHnew, 0, "PageSel2");
            *PCLATH = PCLATHnew;
            n++;
        }
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        if(((*PCLATH) ^ PCLATHnew) & (1 << BIT3)) {
            if(PCLATHnew & (1 << BIT3)) {
                SetInstruction(addr + n, OP_BSF, REG_PCLATH, BIT3, "_^");
                *PCLATH |= (1 << BIT3);
                *PCLATH &= ~(NOTDEF(0) | MULTYDEF(0));
            } else {
                SetInstruction(addr + n, OP_BCF, REG_PCLATH, BIT3, "_v");
                *PCLATH &= ~(1 << BIT3);
                *PCLATH &= ~(NOTDEF(0) | MULTYDEF(0));
            }
            n++;
        }
        if(((*PCLATH) ^ PCLATHnew) & (1 << BIT4)) {
            if(PCLATHnew & (1 << BIT4)) {
                SetInstruction(addr + n, OP_BSF, REG_PCLATH, BIT4, "^_");
                *PCLATH |= (1 << BIT4);
                *PCLATH &= ~(NOTDEF(0) | MULTYDEF(0));
            } else {
                SetInstruction(addr + n, OP_BCF, REG_PCLATH, BIT4, "v_");
                *PCLATH &= ~(1 << BIT4);
                *PCLATH &= ~(NOTDEF(0) | MULTYDEF(0));
            }
            n++;
        }
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");

    if(n == 0)
        *PCLATH = PCLATHnew;

    return n;
}

//---------------------------------------------------------------------------
static void PageCorrect(DWORD addr, int n, DWORD PCLATHnew)
{
    while(n >= 0) {
        PicProg[addr].PCLATH = PCLATHnew;
        n--;
        addr++;
    }
}

//---------------------------------------------------------------------------
static void PageCorrection()
{
    static int PageSelLevel = 10;
/*
    if(PicProgWriteP >= Prog.mcu->flashWords) {
        Error("Not enough memory for PAGE correction! %d %d", PicProgWriteP, Prog.mcu->flashWords);
        return;
    }
*/
    bool  corrected;
    DWORD i, j;
doPageCorrection:
    corrected = false;
    PagePreSet();
    i = 0;
    while((i < PicProgWriteP)/* && (PicProgWriteP < Prog.mcu->flashWords)*/) {
        if(IsOperation(PicProg[i].opPic) <= IS_PAGE) {
            if(IS_UNDEF(PicProg[i].PCLATH) || ((PicProg[i].arg1 >> 11) != (PicProg[i].PCLATH >> 3))) {
                //  ^target addr^              ^current PCLATH^
                PicOp PicProgOpPic = PicProg[i].opPic;
                DWORD PicProgArg1 = PicProg[i].arg1;
                DWORD PCLATHnow = PicProg[i].PCLATH;
                if(IS_UNDEF(PicProg[i].PCLATH)) {
                    PCLATHnow = ~(PicProg[i].arg1 >> 8);
                    PCLATHnow &= (1 << BIT3) | (1 << BIT4);
                }

                corrected = true;
                // need to correct PCLATH page
                int n1, n2, n3, m3, n4; // need n opPic operations for page correcting
                n1 = PageSelectCheck(PCLATHnow, (PicProgArg1) >> 8);
                // we need n1 op's for correcting, but
                n2 = PageSelectCheck(PCLATHnow, (PicProgArg1 + n1) >> 8);
                // we can need more if after first correction we cross the page boundary
                n3 = PageSelectCheck(PCLATHnow, (PicProgArg1 + n2) >> 8);
                m3 = std::max(n1, std::max(n2, n3));

                int   nSkip = 0;
                DWORD ii = i; // address where we doing insert
                while((ii > 0) && (IsOperation(PicProg[ii - 1].opPic) == IS_SKIP)) {
                    // can't insert op betwen IS_SKIP and any opPic
                    ii--;
                    nSkip++;
                }
                if(ii <= notRealocableAddr) {
                    THROW_COMPILER_EXCEPTION("Internal error.");
                }
                for(j = 0; j < PicProgWriteP; j++) {
                    if(IsOperation(PicProg[j].opPic) <= IS_PAGE)
                        if(PicProg[j].arg1 > ii) {
                            PicProg[j].arg1 += m3;                                  // Correct all target addresses!!!
                            PicProg[PicProg[j].arg1].PCLATH = PicProg[j].arg1 >> 8; // and then Correct PCLATH
                        }
                }
                for(j = PicProgWriteP - 1; j >= ii; j--) {
                    // prepare a place for inserting page correction operations
                    memcpy(&PicProg[j + m3], &PicProg[j], sizeof(PicProg[0]));
                }
                for(j = ii; j < (ii + m3); j++) {
                    memset(&PicProg[j], 0, sizeof(PicProg[j]));
                    PicProg[j].opPic = OP_NOP_;
                    PicProg[j].arg1 = 0;
                    PicProg[j].arg2 = 0;
                    sprintf(PicProg[j].commentAsm, " PS(0x%02X,0x%02X)", PCLATHnow, PicProgArg1 >> 8);
                    sprintf(PicProg[j].commentInt, "");
                }
                // select new page
                n4 = PageSelect(ii, &PCLATHnow, PicProgArg1 >> 8);
                PageCorrect(ii, n4 + nSkip, PCLATHnow);
                //on the page bound n4 may be 0. it is ok.

                PicProgWriteP += m3; // upsize array length
                break;
            }
        }
        i++;
    }

    //  if((--PageSelLevel)>0) // // for debuging // <-------- <-------- <-------
    if(corrected)
        goto doPageCorrection;

    if(PicProgWriteP >= Prog.mcu->flashWords)
        Error("Not enough memory for PAGE correction!");
}

//-----------------------------------------------------------------------------
static void PageCheckForErrorsPostCompile()
{
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) <= IS_PAGE) {
            if((PicProg[i].arg1 >> 11) != (PicProg[i].PCLATH >> 3)) {
                //^target addr^              ^current PCLATH^
                Error("Page Error.[%d:%s] 0x%X 0x%X", PicProg[i].l, PicProg[i].f, PicProg[i].arg1>>11, PicProg[i].PCLATH>>3);
            }
        }
    }
}

//-----------------------------------------------------------------------------
static void AddrCheckForErrorsPostCompile()
{
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) <= IS_PAGE)
            if(IS_FWD(PicProg[i].arg1)) {
                Error("Every AllocFwdAddr needs FwdAddrIsNow.");
            }
    }
}

//-----------------------------------------------------------------------------
static void BankCheckForErrorsPostCompile(FileTracker& fAsm)
{
/*
    if(PicProgWriteP >= Prog.mcu->flashWords) {
        return;
    }
*/
    for(DWORD i = 1; i < PicProgWriteP; i++) {
        if((IsOperation(PicProg[i - 1].opPic) == IS_SKIP) && (IsOperation(PicProg[i].opPic) == IS_BANK)
           && (!IsCoreRegister(PicProg[i - 1].arg1orig)) && (!IsCoreRegister(PicProg[i].arg1orig))) {
            //      && (IsOperation(PicProg[i  ].opPic) <= IS_SKIP)) {
            if(Bank(PicProg[i - 1].arg1orig) ^ Bank(PicProg[i].arg1orig)) {
                fprintf(fAsm, "    ; Bank Error.\n");
                fprintf(
                    fAsm,
                    "    ; i=0x%04x op=%d arg1=%d arg2=%d bank=%x arg1orig=%d commentInt=%s commentAsm=%s arg1name=%s arg2name=%s rung=%d IntPc=%d l=%d file=%s\n",
                    i - 1,
                    PicProg[i - 1].opPic,
                    PicProg[i - 1].arg1,
                    PicProg[i - 1].arg2,
                    PicProg[i - 1].BANK,
                    PicProg[i - 1].arg1orig,
                    PicProg[i - 1].commentInt,
                    PicProg[i - 1].commentAsm,
                    PicProg[i - 1].arg1name,
                    PicProg[i - 1].arg2name,
                    PicProg[i - 1].rung,
                    PicProg[i - 1].IntPc,
                    PicProg[i - 1].l,
                    PicProg[i - 1].f);
                fprintf(
                    fAsm,
                    "    ; i=0x%04x op=%d arg1=%d arg2=%d bank=%x arg1orig=%d commentInt=%s commentAsm=%s arg1name=%s arg2name=%s rung=%d IntPc=%d l=%d file=%s\n",
                    i,
                    PicProg[i].opPic,
                    PicProg[i].arg1,
                    PicProg[i].arg2,
                    PicProg[i].BANK,
                    PicProg[i].arg1orig,
                    PicProg[i].commentInt,
                    PicProg[i].commentAsm,
                    PicProg[i].arg1name,
                    PicProg[i].arg2name,
                    PicProg[i].rung,
                    PicProg[i].IntPc,
                    PicProg[i].l,
                    PicProg[i].f);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Given an opcode and its operands, assemble the 14-bit instruction for the
// PIC. Check that the operands do not have more bits set than is meaningful;
// it is an internal error if they do.
//-----------------------------------------------------------------------------
static DWORD Assemble(DWORD addrAt, PicOp op, DWORD arg1, DWORD arg2, char *sAsm)
//14-Bit Opcode
{
    char               arg1s[1024];
    char               arg1comm[1024];
    PicAvrInstruction *PicInstr = &PicProg[addrAt];
    IntOp              intOp;
    if(PicInstr->IntPc > -1 && static_cast<uint32_t>(PicInstr->IntPc) < IntCode.size())
        intOp = IntCode[PicInstr->IntPc];
    strcpy(sAsm, "");
    sprintf(arg1s, "0x%X", arg1);
    arg1comm[0] = '\0';
#define CHECK(v, bits)                                                 \
    if((v) != ((v) & ((1 << (bits)) - 1)))                             \
    THROW_COMPILER_EXCEPTION_FMT("v=%d=0x%X ((1 << (%d))-1)=%d\nat %d in %s %s\nat %d in %s", \
          (v),                                                         \
          (v),                                                         \
          (bits),                                                      \
          ((1 << (bits)) - 1),                                         \
          PicInstr->l,                                                 \
          PicInstr->f,                                                 \
          intOp.name1.c_str(),                                         \
          intOp.fileLine,                                              \
          intOp.fileName.c_str())
#define CHECK2(v, LowerRangeInclusive, UpperRangeInclusive)              \
    if(((int)v < LowerRangeInclusive) || ((int)v > UpperRangeInclusive)) \
    THROW_COMPILER_EXCEPTION_FMT("v=%d [%d..%d]\nat %d in %s %s\nat %d in %s",                  \
          (int)v,                                                        \
          LowerRangeInclusive,                                           \
          UpperRangeInclusive,                                           \
          PicInstr->l,                                                   \
          PicInstr->f,                                                   \
          intOp.name1.c_str(),                                           \
          intOp.fileLine,                                                \
          intOp.fileName.c_str())
    switch(op) {
        case OP_ADDWF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "addwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0700 | (arg2 << 7) | arg1;

        case OP_ANDWF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "andwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0500 | (arg2 << 7) | arg1;

        case OP_BSF:
            CHECK(arg2, 3);
            CHECK(arg1, 7);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "bsf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x1400 | (arg2 << 7) | arg1;

        case OP_BCF:
            CHECK(arg2, 3);
            CHECK(arg1, 7);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "bcf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x1000 | (arg2 << 7) | arg1;

        case OP_BTFSC:
            CHECK(arg2, 3);
            CHECK(arg1, 7);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "btfsc\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x1800 | (arg2 << 7) | arg1;

        case OP_BTFSS:
            CHECK(arg2, 3);
            CHECK(arg1, 7);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "btfss\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x1c00 | (arg2 << 7) | arg1;

        case OP_CLRF:
            CHECK(arg1, 7);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "clrf\t %s\t %s", arg1s, arg1comm);
            return 0x0180 | arg1;

        case OP_CLRWDT:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "clrwdt\t \t \t ");
            return 0x0064;

        case OP_COMF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "comf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0900 | (arg2 << 7) | arg1;

        case OP_DECF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "decf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0300 | (arg2 << 7) | arg1;

        case OP_DECFSZ:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "decfsz %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0b00 | (arg2 << 7) | arg1;

        case OP_GOTO: {
            // Very special case: we will assume that the PCLATH stuff has
            // been taken care of already.
            sprintf(sAsm, "goto\t l_%06x\t \t ", arg1);
            arg1 &= 0x7ff;
            CHECK(arg1, 11);
            CHECK(arg2, 0);
            return 0x2800 | arg1;
        }
        case OP_CALL: {
            sprintf(sAsm, "call\t l_%06x\t \t ", arg1);
            arg1 &= 0x7ff;
            CHECK(arg1, 11);
            CHECK(arg2, 0);
            return 0x2000 | arg1;
        }
        case OP_INCF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "incf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0a00 | (arg2 << 7) | arg1;

        case OP_INCFSZ:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "incfsz %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0f00 | (arg2 << 7) | arg1;

        case OP_IORWF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "iorwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0400 | (arg2 << 7) | arg1;

        case OP_MOVLW:
            CHECK(arg1, 8);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "movlw\t %s \t %s", arg1s, arg1comm);
            return 0x3000 | arg1;

        case OP_MOVLB:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "movlb\t %s \t %s", arg1s, arg1comm);
            return 0x0020 | arg1;

        case OP_MOVLP:
            CHECK(arg1, 7);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "movlp\t %s \t %s", arg1s, arg1comm);
            return 0x3180 | arg1;

        case OP_MOVF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "movf\t %s, %d \t %s", arg1s, arg2, arg1comm);
            return 0x0800 | (arg2 << 7) | arg1;

        case OP_MOVWF:
            CHECK(arg1, 7);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "movwf\t %s\t %s", arg1s, arg1comm);
            return 0x0080 | arg1;

        case OP_NOP_:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "nop\t \t\t \t ");
            return 0x0000;

        case OP_COMMENT_:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            if(strlen(PicInstr->commentInt))
                sprintf(sAsm, "nop\t \t\t ; %s", PicInstr->commentInt);
            else
                sprintf(sAsm, "nop\t \t \t ");
            return 0x0000;

        case OP_RETLW:
            CHECK((BYTE)arg1, 8);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "retlw\t %s \t %s", arg1s, arg1comm);
            return 0x3400 | (BYTE)arg1;

        case OP_RETURN:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "return\t \t \t ");
            return 0x0008;

        case OP_RETFIE:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "retfie\t \t \t ");
            return 0x0009;

        case OP_RLF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "rlf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0d00 | (arg2 << 7) | arg1;

        case OP_RRF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "rrf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0c00 | (arg2 << 7) | arg1;

        case OP_IORLW:
            CHECK(arg1, 8);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "iorlw\t %s \t %s", arg1s, arg1comm);
            return 0x3800 | arg1;
        case OP_SUBWF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "subwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0200 | (arg2 << 7) | arg1;

        case OP_XORWF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "xorwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0600 | (arg2 << 7) | arg1;

        case OP_SWAPF:
            CHECK(arg1, 7);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "swapf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0E00 | (arg2 << 7) | arg1;

        case OP_SLEEP_:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "sleep\t\t\t\t ");
            return 0x0063;

        default:
            THROW_COMPILER_EXCEPTION_FMT("OP_%d", op);
            return 0;
    }
}

static DWORD Assemble12(DWORD addrAt, PicOp op, DWORD arg1, DWORD arg2, char *sAsm)
//12-Bit Opcode for PIC10, PIC12
{
    char               arg1s[1024];
    char               arg1comm[1024];
    PicAvrInstruction *PicInstr = &PicProg[addrAt];
    IntOp              intOp;
    if(PicInstr->IntPc > -1 && static_cast<uint32_t>(PicInstr->IntPc) < IntCode.size())
        intOp = IntCode[PicInstr->IntPc];
    strcpy(sAsm, "");
    sprintf(arg1s, "0x%X", arg1);
    arg1comm[0] = '\0';
#define CHECK(v, bits)                                                 \
    if((v) != ((v) & ((1 << (bits)) - 1)))                             \
    THROW_COMPILER_EXCEPTION_FMT("v=%d=0x%X ((1 << (%d))-1)=%d\nat %d in %s %s\nat %d in %s", \
          (v),                                                         \
          (v),                                                         \
          (bits),                                                      \
          ((1 << (bits)) - 1),                                         \
          PicInstr->l,                                                 \
          PicInstr->f,                                                 \
          intOp.name1.c_str(),                                         \
          intOp.fileLine,                                              \
          intOp.fileName.c_str())
    switch(op) {
        case OP_ADDWF:
            CHECK(arg2, 1);
            CHECK(arg1, 5);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "addwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x1C0 | (arg2 << 5) | arg1;

        case OP_ANDWF:
            CHECK(arg2, 1);
            CHECK(arg1, 5);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "andwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x140 | (arg2 << 5) | arg1;

        case OP_BCF:
            CHECK(arg2, 3);
            CHECK(arg1, 5);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "bcf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x400 | (arg2 << 5) | arg1;

        case OP_BSF:
            CHECK(arg2, 3);
            CHECK(arg1, 5);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "bsf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x500 | (arg2 << 5) | arg1;

        case OP_BTFSC:
            CHECK(arg2, 3);
            CHECK(arg1, 5);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "btfsc\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x600 | (arg2 << 5) | arg1;

        case OP_BTFSS:
            CHECK(arg2, 3);
            CHECK(arg1, 5);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "btfss\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x700 | (arg2 << 5) | arg1;

        case OP_CLRF:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "clrf\t %s\t %s", arg1s, arg1comm);
            return 0x060 | arg1;

        case OP_CLRWDT:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "clrwdt\t \t \t ");
            return 0x004;

        case OP_COMF:
            CHECK(arg2, 1);
            CHECK(arg1, 5);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "comf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x240 | (arg2 << 5) | arg1;

        case OP_DECF:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "decf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x0C0 | (arg2 << 5) | arg1;

        case OP_DECFSZ:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "decfsz %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x2C0 | (arg2 << 5) | arg1;

        case OP_GOTO: {
            // Very special case: we will assume that the PCLATH stuff has
            // been taken care of already.
            sprintf(sAsm, "goto\t l_%06x\t \t ", arg1);
            arg1 &= 0x1ff;
            CHECK(arg1, 9);
            CHECK(arg2, 0);
            return 0xA00 | arg1;
        }
        case OP_CALL: {
            sprintf(sAsm, "call\t l_%06x\t \t ", arg1);
            arg1 &= 0xff;
            CHECK(arg1, 8);
            CHECK(arg2, 0);
            return 0x900 | arg1;
        }
        case OP_INCF:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "incf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x280 | (arg2 << 5) | arg1;

        case OP_INCFSZ:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "incfsz %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x3C0 | (arg2 << 5) | arg1;

        case OP_IORWF:
            CHECK(arg2, 1);
            CHECK(arg1, 5);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "iorwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x100 | (arg2 << 5) | arg1;

        case OP_MOVLW:
            CHECK(arg1, 8);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "movlw\t %s \t %s", arg1s, arg1comm);
            return 0xC00 | arg1;

        case OP_MOVF:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "movf\t %s, %d \t %s", arg1s, arg2, arg1comm);
            return 0x200 | (arg2 << 5) | arg1;

        case OP_MOVWF:
            CHECK(arg1, 5);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "movwf\t %s\t %s", arg1s, arg1comm);
            return 0x020 | arg1;

        case OP_NOP_:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "nop\t \t\t \t ");
            return 0x000;

        case OP_COMMENT_:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            if(strlen(PicInstr->commentInt))
                sprintf(sAsm, "nop\t \t\t ; %s", PicInstr->commentInt);
            else
                sprintf(sAsm, "nop\t \t \t ");
            return 0x000;

        case OP_RETLW:
            CHECK(arg1, 8);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "retlw\t %s \t %s", arg1s, arg1comm);
            return 0x800 | (BYTE)arg1;

        case OP_RLF:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "rlf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x340 | (arg2 << 5) | arg1;

        case OP_RRF:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "rrf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x300 | (arg2 << 5) | arg1;

        case OP_IORLW:
            CHECK(arg1, 8);
            CHECK(arg2, 0);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "iorlw\t %s\t \t %s", arg1s, arg1comm);
            return 0xD00 | arg1;
        case OP_SUBWF:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "subwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x080 | (arg2 << 5) | arg1;

        case OP_XORWF:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "xorwf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x180 | (arg2 << 5) | arg1;

        case OP_SWAPF:
            CHECK(arg1, 5);
            CHECK(arg2, 1);
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "swapf\t %s, %d\t %s", arg1s, arg2, arg1comm);
            return 0x380 | (arg2 << 5) | arg1;

        case OP_TRIS:
            CHECK(arg1, 3);
            CHECK(arg2, 0);
            if(!((arg1 == 6) || (arg1 == 7)))
                THROW_COMPILER_EXCEPTION("Internal error.");
            discoverArgs(addrAt, arg1s, arg1comm);
            sprintf(sAsm, "tris\t %s\t %s", arg1s, arg1comm);
            return 0x000 | arg1;

        case OP_OPTION:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "option\t\t\t ");
            return 0x002;

        case OP_SLEEP_:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            sprintf(sAsm, "sleep\t\t\t\t ");
            return 0x003;

        default:
            THROW_COMPILER_EXCEPTION_FMT("OP_%d", op);
            return 0;
    }
}

static DWORD Assemble16(DWORD addrAt, PicOp op, DWORD arg1, DWORD arg2, DWORD arg3, char *sAsm)
//16-Bit Instruction Word for PIC18
{
    PicAvrInstruction *PicInstr = &PicProg[addrAt];
    IntOp              intOp;
    if(PicInstr->IntPc > -1 && static_cast<uint32_t>(PicInstr->IntPc) < IntCode.size())
        intOp = IntCode[PicInstr->IntPc];
    sAsm[0] = '\0';
    switch(op) {
        case OP_ADDWF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x2400 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_ANDWF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x1400 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_BSF:
            CHECK(arg1, 8);
            CHECK(arg2, 3);
            CHECK(arg3, 1);
            return 0x8000 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_BCF:
            CHECK(arg1, 8);
            CHECK(arg2, 3);
            CHECK(arg3, 1);
            return 0x9000 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_BTFSC:
            CHECK(arg1, 8);
            CHECK(arg2, 3);
            CHECK(arg3, 1);
            return 0xB000 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_BTFSS:
            CHECK(arg1, 8);
            CHECK(arg2, 3);
            CHECK(arg3, 1);
            return 0xA000 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_CLRF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 0);
            return 0x6A00 | (arg2 << 8) | arg1;

        case OP_CLRWDT:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            CHECK(arg3, 0);
            return 6;

        case OP_COMF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x1C00 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_DECF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x0400 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_DECFSZ:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x2C00 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_GOTO:
            // Very special case: we will assume that the PCLATH stuff has
            // been taken care of already.
            CHECK(arg1, 20);
            CHECK(arg2, 0);
            CHECK(arg3, 0);
            return 0xEF00F000 | ((arg1 & 0xff) << 16) | ((arg1 >> 8) & 0xFFF);

        case OP_CALL:
            CHECK(arg1, 20);
            CHECK(arg2, 1);
            CHECK(arg3, 0);
            return 0xEF00F000 | ((arg1 & 0xff) << 16) | ((arg1 >> 8) & 0xFFF) | (arg2 << 24);

        case OP_INCF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x2800 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_INCFSZ:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x3C00 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_IORWF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x1000 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_MOVLW:
            CHECK(arg1, 8);
            CHECK(arg2, 0);
            CHECK(arg3, 0);
            return 0x0E00 | arg1;

        case OP_MOVF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x5000 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_MOVWF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 0);
            return 0x6E00 | (arg2 << 8) | arg1;

        case OP_NOP_:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            CHECK(arg3, 0);
            return 0x0000F000;

        case OP_RETLW:
            CHECK(arg1, 8);
            CHECK(arg2, 0);
            CHECK(arg3, 0);
            return 0x0C00 | arg1;

        case OP_RETURN:
            CHECK(arg1, 1);
            CHECK(arg2, 0);
            CHECK(arg3, 0);
            return 0x0012 | arg1;

        case OP_RETFIE:
            CHECK(arg1, 1);
            CHECK(arg2, 0);
            CHECK(arg3, 0);
            return 0x0010 | arg1;

        case OP_RLF: // RLCF
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x3400 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_RRF: // RRCF
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x3000 | (arg2 << 9) | (arg3 << 8) | arg1;
            /*
        case OP_SUBLW:
            CHECK(arg1, 8); CHECK(arg2, 0); CHECK(arg3, 0);
            return 0x8000 | arg1;
*/
        case OP_SUBWF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x5C00 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_XORWF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x1800 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_SWAPF:
            CHECK(arg1, 8);
            CHECK(arg2, 1);
            CHECK(arg3, 1);
            return 0x3800 | (arg2 << 9) | (arg3 << 8) | arg1;

        case OP_SLEEP_:
            CHECK(arg1, 0);
            CHECK(arg2, 0);
            return 0x0003;

        default:
            THROW_COMPILER_EXCEPTION_FMT("OP_%d", op);
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
    char       sAsm[1024] = "";

    BYTE  soFar[16];
    int   soFarCount = 0;
    DWORD soFarStart = 0;

    // always start from address 0x0000
    fprintf(f, ":020000040000FA\n"); // generated by MPASM
                                     //fprintf(f, ":020000020000FC\n");

    DWORD ExtendedSegmentAddress = 0;
    for(DWORD i = 0; i < PicProgWriteP; i++) {
        DWORD w;
        if(Prog.mcu->core == BaselineCore12bit)
            w = Assemble12(i, PicProg[i].opPic, PicProg[i].arg1, PicProg[i].arg2, sAsm);
        else if(Prog.mcu->core == PIC18HighEndCore16bit)
            w = Assemble16(i, PicProg[i].opPic, PicProg[i].arg1, PicProg[i].arg2, 1, sAsm);
        else
            w = Assemble(i, PicProg[i].opPic, PicProg[i].arg1, PicProg[i].arg2, sAsm);

        if(ExtendedSegmentAddress != (i & ~0x7fff)) {
            ExtendedSegmentAddress = (i & ~0x7fff);
            StartIhex(f);    // ':'->Colon
            WriteIhex(f, 2); // LL->Record Length
            WriteIhex(f, 0); // AA->Address as big endian values HI()
            WriteIhex(f, 0); // AA->Address as big endian values LO()
            WriteIhex(f, 4); // TT->Record Type -> 04 is Extended Linear Address Record
            WriteIhex(f, (BYTE)(((ExtendedSegmentAddress >> 3) >> 8) & 0xff)); // AA->Address as big endian values HI()
            WriteIhex(f, (BYTE)((ExtendedSegmentAddress >> 3) & 0xff));        // AA->Address as big endian values LO()
            FinishIhex(f);                                                     // CC->Checksum
        }
        if(soFarCount == 0)
            soFarStart = i;
        soFar[soFarCount++] = (BYTE)(w & 0xff);
        soFar[soFarCount++] = (BYTE)(w >> 8);

        if(soFarCount >= 0x10 || i == (PicProgWriteP - 1)) {
            StartIhex(f);
            WriteIhex(f, soFarCount);
            WriteIhex(f, (BYTE)((soFarStart * 2) >> 8));
            WriteIhex(f, (BYTE)((soFarStart * 2) & 0xff));
            WriteIhex(f, 0x00); // RECTYP: '00' Data Record
            int j;
            for(j = 0; j < soFarCount; j++) {
                WriteIhex(f, soFar[j]);
            }
            FinishIhex(f);
            soFarCount = 0;
        }

        if(strlen(PicProg[i].commentInt)) {
            fprintf(fAsm, "    ; %s\n", PicProg[i].commentInt);
        }

        if(strlen(sAsm)) {
#ifdef ASM_BANKSEL
            if(IsOperation(PicProg[i].opPic) >= IS_BANK) {
                fprintf(fAsm, "        \t banksel 0x%X\n", PicProg[i].arg1);
            }
#endif
#ifdef ASM_PAGESEL
            if(IsOperation(PicProg[i].opPic) <= IS_PAGE) {
                fprintf(fAsm, "        \t pagesel l_%06x\n", PicProg[i].arg1);
            }
#endif

#if ASM_LABEL > 0
            if(PicProg[i].label || (ASM_LABEL == 2))
                if(PicProg[i].label & L_LABEL)
                    fprintf(fAsm, "l_%06x: %s", i, sAsm);
                else
                    fprintf(fAsm, "i_%06x: %s", i, sAsm);
            else
                fprintf(fAsm, "          %s", sAsm);
#else
            fprintf(fAsm, "          %s", sAsm);
#endif

            if(asm_comment_level >= 3) {
                fprintf(fAsm, "\t");
                if(1 || (prevRung != PicProg[i].rung)) {
                    fprintf(fAsm, " ; rung=%d", PicProg[i].rung + 1);
                    prevRung = PicProg[i].rung;
                } else {
                    fprintf(fAsm, " \t");
                }
            }

            if(asm_comment_level >= 4) {
                if(1 || (prevL != PicProg[i].l)) {
                    fprintf(fAsm, " ; line %d in %s", PicProg[i].l, PicProg[i].f);
                    prevL = PicProg[i].l;
                }
            }

            if(asm_comment_level >= 5) {
                if((PicProg[i].IntPc >= 0) && (PicProg[i].IntPc < IntCode.size())) {
                    fprintf(fAsm, "\t");
                    if(IntCode[PicProg[i].IntPc].which != INT_MAX) {
                        fprintf(fAsm, " ; ELEM_0x%X", IntCode[PicProg[i].IntPc].which);
                    }
                    if(1 || (prevIntPcL != IntCode[PicProg[i].IntPc].fileLine)) {
                        fprintf(fAsm, " ; line %d in %s", IntCode[PicProg[i].IntPc].fileLine, IntCode[PicProg[i].IntPc].fileName.c_str());
                        prevIntPcL = IntCode[PicProg[i].IntPc].fileLine;
                    }
                }
            }

#ifdef ASM_COMMENT_BANK
            fprintf(fAsm, " ;B=0x%8X", PicProg[i].BANK);
            if(IsOperation(PicProg[i].opPic) >= IS_BANK)
                fprintf(fAsm, " A=0x%8X C=%d", Bank(PicProg[i].arg1orig), IsCoreRegister(PicProg[i].arg1orig));
            else
                fprintf(fAsm, "                 ");
#endif

#ifdef ASM_COMMENT_PAGE
            fprintf(fAsm, " ;l=0x%02X PCLATH=0x%02X", PicProg[i].label, (PicProg[i].PCLATH >> 3) << 3); //ok
#endif

            if(asm_comment_level >= 2)
                if(strlen(PicProg[i].commentAsm)) {
                    fprintf(fAsm, " ; %s", PicProg[i].commentAsm);
                }

            if(asm_comment_level >= 2)
                if(strlen(PicProg[i].arg1name)) {
                    fprintf(fAsm, " ;; %s", PicProg[i].arg1name);
                }

            if(asm_comment_level >= 2)
                if(strlen(PicProg[i].arg2name)) {
                    fprintf(fAsm, " ;;; %s", PicProg[i].arg2name);
                }

            fprintf(fAsm, "\n");
        } else
            ; //;;//Error("op=%d=0x%X", PicProg[i].opPic, PicProg[i].opPic);
    }

    if(ExtendedSegmentAddress != (CONFIG_ADDR1 * 2 & ~0xffff)) {
        ExtendedSegmentAddress = (CONFIG_ADDR1 * 2 & ~0xffff);
        StartIhex(f);    // ':'->Colon
        WriteIhex(f, 2); // LL->Record Length
        WriteIhex(f, 0); // AA->Address as big endian values HI()
        WriteIhex(f, 0); // AA->Address as big endian values LO()

        WriteIhex(f, 4); // TT->Record Type -> 04 is Extended Linear Address Record
        WriteIhex(f, (BYTE)((ExtendedSegmentAddress >> (16 + 8)) & 0xff)); // AA->Address as big endian values HI()
        WriteIhex(f, (BYTE)((ExtendedSegmentAddress >> 16) & 0xff));       // AA->Address as big endian values LO()
        FinishIhex(f);                                                     // CC->Checksum
    }

    if((Prog.configurationWord & ~0xffff) && (CONFIG_ADDR2 == -1))
        THROW_COMPILER_EXCEPTION("Internal error.");

    // Configuration words start at address 0x2007 in program memory; and the
    // hex file addresses are by bytes, not words, so we start at 0x400e.
    // There may be either 16 or 32 bits of conf word, depending on the part.

    StartIhex(f);
    WriteIhex(f, 0x02); // RECLEN 2 bytes
    WriteIhex(f, (BYTE)((CONFIG_ADDR1 * 2) >> 8) & 0xff);
    WriteIhex(f, (BYTE)(CONFIG_ADDR1 * 2) & 0xff);
    WriteIhex(f, 0x00); // RECTYP: '00' Data Record
    WriteIhex(f, (BYTE)(Prog.configurationWord >> 0) & 0xff);
    WriteIhex(f, (BYTE)(Prog.configurationWord >> 8) & 0xff);
    FinishIhex(f);

    if(CONFIG_ADDR2 != -1) {
        StartIhex(f);       // ':'->Colon
        WriteIhex(f, 0x02); // RECLEN 2 bytes
        WriteIhex(f, (BYTE)((CONFIG_ADDR2 * 2) >> 8) & 0xff);
        WriteIhex(f, (BYTE)(CONFIG_ADDR2 * 2) & 0xff);
        WriteIhex(f, 0x00); // RECTYP: '00' Data Record
        WriteIhex(f, (BYTE)(Prog.configurationWord >> 16) & 0xff);
        WriteIhex(f, (BYTE)(Prog.configurationWord >> 24) & 0xff);
        FinishIhex(f);
    }
    // end of file record
    fprintf(f, ":00000001FF\n");
}

//-----------------------------------------------------------------------------
// Generate code to write an 8-bit value to a particular register. Takes care
// of the bank switching if necessary; assumes that code is called in bank
// 0.
//-----------------------------------------------------------------------------
static void _WriteRegister(int l, const char *f, const char *args, DWORD reg, BYTE val, char *comment = nullptr);
static void _WriteRegister(int l, const char *f, const char *args, DWORD reg, BYTE val, char *comment)
{
    // if(val) {
    char buf[MAX_COMMENT_LEN];
    if(comment && strlen(comment))
        sprintf(buf,"%s ; 0x%X", comment, val);
    else
        sprintf(buf,"0x%X", val);
    _Instruction(l, f, args, OP_MOVLW, val, 0, buf);
    if(comment && strlen(comment))
        sprintf(buf,"%s ; 0x%X", comment, reg);
    else
        sprintf(buf,"0x%X", reg);
    _Instruction(l, f, args, OP_MOVWF, reg, 0, buf);
    // } else
    //    vvv Z Status Affected !!!
    //    _Instruction(l, f, args, OP_CLRF, reg, comment);
    //    ^^^ Z Status Affected !!!
    // }
}
// And use macro for bugtracking
#define WriteRegister(...) _WriteRegister(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)
//-----------------------------------------------------------------------------
// Call a subroutine, that might be in an arbitrary page, and then put
// PCLATH back where we want it.
//-----------------------------------------------------------------------------
static void _CallWithPclath(DWORD addr, const char *comment)
{
    Instruction(OP_CALL, addr, 0, comment);
}
#define CallWithPclath(addr) _CallWithPclath(addr, #addr)

static bool IsOutputReg(DWORD addr)
{
    if((addr == -1) || (addr == 0))
        THROW_COMPILER_EXCEPTION("Internal error.");
    for(int i = 0; i < MAX_IO_PORTS; i++)
        if(Prog.mcu->outputRegs[i] == addr)
            return true;
    return false;
}

static bool IsInputReg(DWORD addr)
{
    if((addr == -1) || (addr == 0))
        THROW_COMPILER_EXCEPTION("Internal error.");
    for(int i = 0; i < MAX_IO_PORTS; i++)
        if(Prog.mcu->inputRegs[i] == addr)
            return true;
    return false;
}

static void CopyBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc, const char *nameDest, const char *nameSrc)
{
    if((addrDest == addrSrc) && (bitDest == bitSrc)) {
        return;
    }
    //  Comment("CopyBit");
    /*
    // With possibility of "jitter" on destination pin.
    // Don't use for outputs pins.
    ClearBit(addrDest, bitDest);
    IfBitSet(addrSrc, bitSrc);
    SetBit(addrDest, bitDest);
*/
    /*
    // With possibility of error due to change in value on external source pin
    IfBitSet(addrSrc, bitSrc);
    SetBit(addrDest, bitDest);
    IfBitClear(addrSrc, bitSrc);
    ClearBit(addrDest, bitDest);
*/
    // No "jitter", No "input" error
    if((!IsOutputReg(addrDest))
       && ((Bank(addrDest) == Bank(addrSrc)) || IsCoreRegister(addrDest) || IsCoreRegister(addrSrc))) {
        IfBitSet(addrSrc, bitSrc, nameSrc);
        SetBit(addrDest, bitDest, nameDest);
        IfBitClear(addrSrc, bitSrc, nameSrc);
        ClearBit(addrDest, bitDest, nameDest);
    } else {
        ClearBit(REG_STATUS, STATUS_DC, "STATUS_DC");
        IfBitSet(addrSrc, bitSrc, nameSrc);
        SetBit(REG_STATUS, STATUS_DC);

        IfBitSet(REG_STATUS, STATUS_DC, "STATUS_DC");
        SetBit(addrDest, bitDest, nameDest);
        IfBitClear(REG_STATUS, STATUS_DC, "STATUS_DC");
        ClearBit(addrDest, bitDest, nameDest);
    }
}

static void CopyBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc, const NameArray &nameDest,
                    const char *nameSrc)
{
    CopyBit(addrDest, bitDest, addrSrc, bitSrc, nameDest.c_str(), nameSrc);
}

static void CopyBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc, const char *nameDest)
{
    CopyBit(addrDest, bitDest, addrSrc, bitSrc, nameDest, nullptr);
}

static void CopyBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc)
{
    CopyBit(addrDest, bitDest, addrSrc, bitSrc, nullptr, nullptr);
}

static void CopyNotBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc, const char *nameDest,
                       const char *nameSrc)
{
    Comment("CopyNotBit");
    if(((addrDest != addrSrc) || (bitDest != bitSrc))
       && ((Bank(addrDest) == Bank(addrSrc)) || IsCoreRegister(addrDest) || IsCoreRegister(addrSrc))) {
        IfBitSet(addrSrc, bitSrc, nameSrc);
        ClearBit(addrDest, bitDest, nameDest);
        IfBitClear(addrSrc, bitSrc, nameSrc);
        SetBit(addrDest, bitDest, nameDest);
    } else {
        ClearBit(REG_STATUS, STATUS_DC, "STATUS_DC");
        IfBitSet(addrSrc, bitSrc, nameSrc);
        SetBit(REG_STATUS, STATUS_DC, "STATUS_DC");

        IfBitSet(REG_STATUS, STATUS_DC, "STATUS_DC");
        ClearBit(addrDest, bitDest, nameDest);
        IfBitClear(REG_STATUS, STATUS_DC, "STATUS_DC");
        SetBit(addrDest, bitDest, nameDest);
    }
}

static void CopyNotBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc)
{
    CopyNotBit(addrDest, bitDest, addrSrc, bitSrc, nullptr, nullptr);
}

static void AndBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc)
{
    Comment("AndBit");
    IfBitClear(addrSrc, bitSrc);
    ClearBit(addrDest, bitDest);
}

static void OrBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc)
{
    Comment("OrBit");
    IfBitSet(addrSrc, bitSrc);
    SetBit(addrDest, bitDest);
}

static void XorBit(DWORD addrDest, int bitDest, DWORD addrSrc, int bitSrc)
{
    Comment("XorBit");
    DWORD is0 = AllocFwdAddr();
    DWORD isEnd = AllocFwdAddr();
    SkpIfBitSet(addrDest, bitDest);
    Instruction(OP_GOTO, is0);
    //is1
    IfBitSet(addrSrc, bitSrc);
    ClearBit(addrDest, bitDest);
    Instruction(OP_GOTO, isEnd);

    FwdAddrIsNow(is0);
    IfBitSet(addrSrc, bitSrc);
    SetBit(addrDest, bitDest);

    FwdAddrIsNow(isEnd);
}

//-----------------------------------------------------------------------------
// Handle an IF statement. Flow continues to the first instruction generated
// by this function if the condition is true, else it jumps to the given
// address (which is an FwdAddress, so not yet assigned). Called with IntPc
// on the IF statement, returns with IntPc on the END IF.
//-----------------------------------------------------------------------------
static void CompileIfBody(DWORD condFalse, const char *s)
{
    //  Comment("CompileIfBody %s vvv", s);
    IntPc++;
    IntPcNow = IntPc;
    CompileFromIntermediate(false);
    if(IntCode[IntPc].op == INT_ELSE) {
        IntPc++;
        IntPcNow = IntPc;
        DWORD endBlock = AllocFwdAddr();
        Instruction(OP_GOTO, endBlock);

        FwdAddrIsNow(condFalse);
        CompileFromIntermediate(false);
        FwdAddrIsNow(endBlock);
    } else {
        FwdAddrIsNow(condFalse);
    }

    if(IntCode[IntPc].op != INT_END_IF)
        THROW_COMPILER_EXCEPTION("Internal error.");
    //  Comment("CompileIfBody %s ^^^", s);
}

static void CompileIfBody(DWORD condFalse)
{
    CompileIfBody(condFalse, "");
}
/*
//-----------------------------------------------------------------------------
static char *VarFromExpr(char *expr, char *tempName, DWORD addr)
{
    if(IsNumber(expr)) {
        int val = hobatoi(expr);
        int sov = SizeOfVar(expr);
        int i;
        for(i=0; i<sov; i++) {
            Instruction(OP_MOVLW, ((val >> (8*i)) && 0xFF );
            Instruction(OP_MOVWF, addr + i);
        }
        return tempName;
    } else {
        return expr;
    }
}
*/

//-----------------------------------------------------------------------------
static DWORD CopyLitToReg(DWORD addr, int sov, const char *name, SDWORD literal, const char *comment)
{
    Comment("CopyLitToReg");
    if(sov < 1)
        THROW_COMPILER_EXCEPTION(comment);
    if(sov > 4)
        THROW_COMPILER_EXCEPTION(comment);

    DWORD lNow, lPrev;
    lNow = literal & 0xff;
    lPrev = ~lNow;
    if(IsAddrInVar(name)) {
        //// sov = SizeOfVar(&name[1]); // sov == SizeOfVar(name); // It's right!
        MemForVariable(&name[1], &addr);
        Instruction(OP_MOVF, addr, DEST_W, &name[1]);
        Instruction(OP_MOVWF, REG_FSR, 0, name); // indirect address

        for(int i = 0; i < sov; i++) {
            lNow = (literal >> (8 * i)) & 0xff;
            if(lNow) {
                if(lNow != lPrev)
                    Instruction(OP_MOVLW, lNow, 0, comment);
                Instruction(OP_MOVWF, REG_INDF, 0, "indirecr");
            } else
                Instruction(OP_CLRF, REG_INDF);
            if(i < (sov - 1))
                Instruction(OP_INCF, REG_FSR, DEST_F);
            lPrev = lNow;
        }
    } else {
        for(int i = 0; i < sov; i++) {
            lNow = (literal >> (8 * i)) & 0xff;
            if(lNow) {
                if(lNow != lPrev)
                    Instruction(OP_MOVLW, lNow, 0, comment);
                Instruction(OP_MOVWF, addr + i, 0, comment);
            } else
                Instruction(OP_CLRF, addr + i, 0, comment);
            lPrev = lNow;
        }
    }
    return addr;
    /*
    DWORD l1, l2;
    l1 = (literal & 0xff);
    if(l1) {
        Instruction(OP_MOVLW, l1, 0, comment);
        Instruction(OP_MOVWF, addr, 0, comment);
    } else
        Instruction(OP_CLRF, addr, 0, comment);
    if(sov >= 2) {
        l2 = ((literal >> 8) & 0xff);
        if(l2) {
            if(l1 != l2)
                Instruction(OP_MOVLW, l2, 0, comment);
            Instruction(OP_MOVWF, addr + 1, 0, comment);
        } else
            Instruction(OP_CLRF, addr + 1, 0, comment);

        if(sov >= 3) {
            l1 = ((literal >> 16) & 0xff);
            if(l1) {
                if(l1 != l2)
                    Instruction(OP_MOVLW, l1, 0, comment);
                Instruction(OP_MOVWF, addr + 2, 0, comment);
            } else
                Instruction(OP_CLRF, addr + 2, 0, comment);

            if(sov >= 4) {
                l2 = ((literal >> 24) & 0xff);
                if(l2) {
                    if(l1 != l2)
                        Instruction(OP_MOVLW, l2, 0, comment);
                    Instruction(OP_MOVWF, addr + 3, 0, comment);
                } else
                    Instruction(OP_CLRF, addr + 3, 0, comment);
            }
        }
    }
*/
}

static DWORD CopyLitToReg(DWORD addr, int sov, const NameArray &name, SDWORD literal, const NameArray &comment)
{
    return CopyLitToReg(addr, sov, name.c_str(), literal, comment.c_str());
}

//-----------------------------------------------------------------------------
static DWORD CopyRegToReg(DWORD addr1, int sov1, DWORD addr2, int sov2, const char *name1, const char *name2,
                         bool signPropagation)
// addr1 - dest, addr2 - source
{
    Comment("CopyRegToReg");
    if(sov1 < 1)
        THROW_COMPILER_EXCEPTION(name1);
    if(sov1 > 4)
        THROW_COMPILER_EXCEPTION(name1);
    if(sov2 < 1)
        THROW_COMPILER_EXCEPTION(name2);
    if(sov2 > 4)
        THROW_COMPILER_EXCEPTION(name2);

    if(addr1 == 0)
        THROW_COMPILER_EXCEPTION(name1);
    if(addr1 == -1)
        THROW_COMPILER_EXCEPTION(name1);
    if(addr2 == 0)
        THROW_COMPILER_EXCEPTION(name2);
    if(addr2 == -1)
        THROW_COMPILER_EXCEPTION(name2);

     if(addr1 == addr2) {
        if(sov1 == sov2) {
            // Error(_(" CopyRegToReg Warning 1"));
            return addr1;
        } else {
            Error(_(" CopyRegToReg Message 2"));
            return addr1;
        }
    }

    if(IsAddrInVar(name1) && (!IsAddrInVar(name2))) {
        //// sov1 = SizeOfVar(&name1[1]); // sov1 == SizeOfVar(name1); // It's right!
        MemForVariable(&name1[1], &addr1);
        Instruction(OP_MOVF, addr1, DEST_W, &name1[1]);
        Instruction(OP_MOVWF, REG_FSR, 0, name1);

        for(int i = 0; i < sov1; i++) {
            if(i < sov2) {
                Instruction(OP_MOVF, addr2 + i, DEST_W, name2);
                Instruction(OP_MOVWF, REG_INDF, 0, name1);
            } else {
                Instruction(OP_MOVLW, 0x00); // NO BLINK
                if(signPropagation) {
                    Instruction(OP_BTFSC, addr2 + sov2 - 1, BIT7, name2); // Sign propagation
                    Instruction(OP_MOVLW, 0xFF);                          // NO BLINK
                }
                Instruction(OP_MOVWF, REG_INDF, 0, name1); // NO BLINK
            }
            if(i < (sov1 - 1))
                Instruction(OP_INCF, REG_FSR, DEST_F);
        }
    } else if((!IsAddrInVar(name1)) && IsAddrInVar(name2)) {
        //// sov2 = SizeOfVar(&name2[1]); // sov2 == SizeOfVar(name2); // It's right!
        MemForVariable(&name2[1], &addr2);
        Instruction(OP_MOVF, addr2, DEST_W, &name2[1]);
        Instruction(OP_MOVWF, REG_FSR, 0, name2);

        for(int i = 0; i < sov1; i++) {
            if(i < sov2) {
                Instruction(OP_MOVF, REG_INDF, DEST_W, name2);
            } else {
                Instruction(OP_MOVLW, 0x00); // NO BLINK
                if(signPropagation) {
                    Instruction(OP_BTFSC, addr1 + sov2 - 1, BIT7, name2); // Sign propagation // addr1 is Ok!
                    Instruction(OP_MOVLW, 0xFF);                          // NO BLINK
                }
            }
            Instruction(OP_MOVWF, addr1 + i, 0, name1);
            if(i < (sov2 - 1))
                Instruction(OP_INCF, REG_FSR, DEST_F);
        }
    } else if((IsAddrInVar(name1)) && IsAddrInVar(name2)) {
        MemForVariable(&name1[1], &addr1);
        MemForVariable(&name2[1], &addr2);

        for(int i = 0; i < sov1; i++) {
            if(i < sov2) {
                Instruction(OP_MOVF, addr2, DEST_W, &name2[1]);
                Instruction(OP_MOVWF, REG_FSR, 0, name2);
                for(int j = 0; j < i; j++)
                    Instruction(OP_INCF, REG_FSR, DEST_F);
                Instruction(OP_MOVF, REG_INDF, DEST_W, name2);
                Instruction(OP_MOVWF, ScratchI, 0, "tmp");

                Instruction(OP_MOVF, addr1, DEST_W, &name1[1]);
                Instruction(OP_MOVWF, REG_FSR, 0, name1);
                for(int j = 0; j < i; j++)
                    Instruction(OP_INCF, REG_FSR, DEST_F);
                Instruction(OP_MOVF, ScratchI, DEST_W, "tmp");
            } else {
                Instruction(OP_INCF, REG_FSR, DEST_F); // FSR is addr1
                Instruction(OP_MOVLW, 0x00);           // NO BLINK
                if(signPropagation) {
                    Instruction(OP_BTFSC, ScratchI, BIT7, "tmp"); // Sign propagation
                    Instruction(OP_MOVLW, 0xFF);                  // NO BLINK
                }
            }
            Instruction(OP_MOVWF, REG_INDF, 0, name1);
        }
    } else {
        for(int i = 0; i < sov1; i++) {
            if(i < sov2) {
                Instruction(OP_MOVF, addr2 + i, DEST_W, name2);
            } else {
                //Instruction(OP_CLRF, addr1 + i); // BLINK
                Instruction(OP_MOVLW, 0x00); // NO BLINK
                if(signPropagation) {
                    Instruction(OP_BTFSC, addr2 + sov2 - 1, BIT7, name2); // Sign propagation
                    //Instruction(OP_COMF, addr1 + i, DEST_F); // BLINK
                    Instruction(OP_MOVLW, 0xFF); // NO BLINK
                }
            }
            Instruction(OP_MOVWF, addr1 + i, 0, name1);
        }
    }
    return addr1;
    /*
    Instruction(OP_MOVF, addr2, DEST_W, name2);
    Instruction(OP_MOVWF, addr1, 0, name1);
    if(sov1 >= 2) {
        if(sov2 >= 2) {
            Instruction(OP_MOVF, addr2 + 1, DEST_W, name2);
        } else {
            Instruction(OP_MOVLW, 0x00);
            if(signPropagation) {
                Instruction(OP_BTFSC, addr2 + sov2 - 1, BIT7, name2); // Sign propagation
                Instruction(OP_MOVLW, 0xFF);
            }
        }
        Instruction(OP_MOVWF, addr1 + 1, 0, name1);

        if(sov1 >= 3) {
            if(sov2 >= 3) {
                Instruction(OP_MOVF, addr2 + 2, DEST_W, name2);
            } else {
                Instruction(OP_MOVLW, 0x00);
                if(signPropagation) {
                    Instruction(OP_BTFSC, addr2 + sov2 - 1, BIT7, name2); // Sign propagation
                    Instruction(OP_MOVLW, 0xFF);
                }
            }
            Instruction(OP_MOVWF, addr1 + 2, 0, name1);

            if(sov1 >= 4) {
                if(sov2 >= 4) {
                    Instruction(OP_MOVF, addr2 + 3, DEST_W, name2);
                } else {
                    Instruction(OP_MOVLW, 0x00);
                    if(signPropagation) {
                        Instruction(OP_BTFSC, addr2 + sov2 - 1, BIT7, name2); // Sign propagation
                        Instruction(OP_MOVLW, 0xFF);
                    }
                }
                Instruction(OP_MOVWF, addr1 + 3, 0, name1);
            }
        }
    }
    */
}

static DWORD CopyRegToReg(DWORD addr1, int sov1, DWORD addr2, int sov2, const NameArray &name1, const NameArray &name2,
                         bool Sign)
{
    return CopyRegToReg(addr1, sov1, addr2, sov2, name1.c_str(), name2.c_str(), Sign);
}

//-----------------------------------------------------------------------------
static DWORD CopyVarToReg(DWORD addr1, int sov1, const char *name2, bool Sign)
{
    DWORD addr2;
    MemForVariable(name2, &addr2);
    return CopyRegToReg(addr1, sov1, addr2, SizeOfVar(name2), "", name2, Sign);
}

static DWORD CopyVarToReg(DWORD addr1, int sov1, const char *name2)
{
    return CopyVarToReg(addr1, sov1, name2, false);
}

static DWORD CopyVarToReg(DWORD addr1, int sov1, const NameArray &name2)
{
    return CopyVarToReg(addr1, sov1, name2.c_str());
}

//-----------------------------------------------------------------------------
static DWORD CopyArgToDest(bool isFlipFlopRisk, DWORD destAddr, DWORD tmpAddr, int destSov, const char *name, bool sign)
{
    if(IsNumber(name)) {
        if(isFlipFlopRisk/* || (IsOutputReg(destAddr))*/)
            return CopyLitToReg(tmpAddr, destSov, name, hobatoi(name), name);
        else
            return CopyLitToReg(destAddr, destSov, name, hobatoi(name), name);
    } else {
        int   srcSov = SizeOfVar(name);
        DWORD srcAddr;
        if(IsAddrInVar(name))
            MemForVariable(&name[1], &srcAddr);
        else
            MemForVariable(name, &srcAddr);

        if(isFlipFlopRisk)
            return CopyRegToReg(tmpAddr, destSov, srcAddr, srcSov, "$CopyArgToReg", name, sign);
        else if(destAddr == srcAddr)// || (destSov < srcSov))
            return srcAddr; // nothing to write to the HEX
        else
            return CopyRegToReg(destAddr, destSov, srcAddr, srcSov, "$CopyArgToReg", name, sign);
    }
}

static DWORD CopyArgToDest(bool isFlipFlopRisk, DWORD destAddr, DWORD tmpAddr, int destSov, const NameArray &name, bool sign)
{
    return CopyArgToDest(isFlipFlopRisk, destAddr, tmpAddr, destSov, name.c_str(), sign);
}

//-----------------------------------------------------------------------------
static DWORD CopyArgToReg(bool isModificationRisk, DWORD destAddr, int destSov, const char *name, bool Sign)
{
    if(IsNumber(name)) {
        return CopyLitToReg(destAddr, destSov, name, hobatoi(name), name);
    } else {
        int   sov = SizeOfVar(name);
        DWORD addr;
        MemForVariable(name, &addr);
        if(isModificationRisk || (sov < destSov))
            return CopyRegToReg(destAddr, destSov, addr, sov, "$CopyArgToReg", name, Sign);
        else
            return addr;
    }
}

static DWORD CopyArgToReg(bool isModificationRisk, DWORD destAddr, int destSov, const NameArray &name, bool Sign)
{
    return CopyArgToReg(isModificationRisk, destAddr, destSov, name.c_str(), Sign);
}
//-----------------------------------------------------------------------------
// http://www.piclist.com/techref/member/BB-LTL-/index.htm
// https://wiki.nicksoft.info/mcu:pic16:32bit-math
// Filename:   QUAD_MATH_TESTS.ASM
// Author:     Brian Beard
//-----------------------------------------------------------------------------
// Binary number in ACb0 = Scratch0..Scratch3.
// BCD9 to BCD0 comprise one ten digit unpacked Binary-Coded-Decimal number.
//-----------------------------------------------------------------------------
static void WriteBin32BcdRoutine()
{
    Comment("WriteBin32BcdRoutine");
    DWORD savePicProgWriteP = PicProgWriteP;
#ifdef MOVE_TO_PAGE_0
    Bin32BcdRoutineAddress = PicProgWriteP;
#else
    FwdAddrIsNow(Bin32BcdRoutineAddress);
#endif

#define ACb0 Scratch0   // Binary value is copied to ACb0 = Scratch0..Scratch3
#define sovBin Scratch4 // Size of Binary in bytes is in Scratch4
#define BCD0 Scratch5   // Addres of Bcd is in Scratch5
#define digBcd Scratch6 // Number of digits Bcd is in Scratch6

#define bitcnt Scratch7 // bitcnt = sovBin * 8
#define digcnt Scratch8 // digcnt = digBcd

    if(BCD0 >= 0x100)
        Instruction(OP_BSF, REG_STATUS, STATUS_IRP);
    else
        Instruction(OP_BCF, REG_STATUS, STATUS_IRP);
    Instruction(OP_MOVF, BCD0, DEST_W);   // Point to address of least
    Instruction(OP_MOVWF, REG_FSR);       //  significant bcd digit
    Instruction(OP_MOVF, digBcd, DEST_W); // Load digBcd to
    Instruction(OP_MOVWF, digcnt);        //  digit counter

    DWORD b2bcd0 = PicProgWriteP;           // Clear all bcd digits
    Instruction(OP_CLRF, REG_INDF);         // Clear a digit
    Instruction(OP_INCF, REG_FSR, DEST_F);  // Point to next bcd digit
    Instruction(OP_DECFSZ, digcnt, DEST_F); // Decrement digit counter
    Instruction(OP_GOTO, b2bcd0);           //  - go up if digcnt > 0

    Instruction(OP_MOVF, sovBin, DEST_W); //
    Instruction(OP_MOVWF, bitcnt);        // bit counter = sovBin * 8
    ClearBit(REG_STATUS, STATUS_C);
    Instruction(OP_RLF, bitcnt, DEST_F);  // *2 // * 2
    Instruction(OP_RLF, bitcnt, DEST_F);  // *2 // * 4
    Instruction(OP_RLF, bitcnt, DEST_F);  // *2 // * 8

                                           // Outer loop by
    DWORD b2bcd1 = PicProgWriteP;          //  bit counter
    /*
    Instruction(OP_RLF, ACb0, DEST_F);     // Shift 32-bit accumulator
    Instruction(OP_RLF, ACb0 + 1, DEST_F); //  left to
    Instruction(OP_RLF, ACb0 + 2, DEST_F); //   put ms-bit
    Instruction(OP_RLF, ACb0 + 3, DEST_F); //    into Carry
    */
    if(ACb0 >= 0x100)
        Instruction(OP_BSF, REG_STATUS, STATUS_IRP);
    else
        Instruction(OP_BCF, REG_STATUS, STATUS_IRP);
                                           // Shift 32-bit accumulator
    Instruction(OP_MOVLW, ACb0);           //
    Instruction(OP_MOVWF, REG_FSR);        // Point to address of 32-bit accumulator
    Instruction(OP_MOVF, sovBin, DEST_W);  // Load sovBin to
    Instruction(OP_MOVWF, digcnt);         //  digit counter

    DWORD b2bcd00 = PicProgWriteP;
    Instruction(OP_RLF, REG_INDF, DEST_F);  // Shift accumulator left
    Instruction(OP_INCF, REG_FSR, DEST_F);  // Point to next byte of accumulator
    Instruction(OP_DECFSZ, digcnt, DEST_F); // Decrement digit counter
    Instruction(OP_GOTO, b2bcd00);          //  - go if digcnt > 0

    Instruction(OP_MOVF, BCD0, DEST_W);    // Point to address of least
    Instruction(OP_MOVWF, REG_FSR);        //  significant bcd digit
    Instruction(OP_MOVF, digBcd, DEST_W);  // Inner loop // D'10'
    Instruction(OP_MOVWF, digcnt);         //  digit counter

    DWORD b2bcd2 = PicProgWriteP;
    Instruction(OP_RLF, REG_INDF, DEST_F);       // Shift Carry into bcd digit
    Instruction(OP_MOVLW, 10);                   // Subtract ten from digit then
    Instruction(OP_SUBWF, REG_INDF, DEST_W);     //  check and adjust for decimal overflow
    Instruction(OP_BTFSC, REG_STATUS, STATUS_C); // If Carry = 1 (result >= 0)
    Instruction(OP_MOVWF, REG_INDF);             //  adjust for decimal overflow
    Instruction(OP_INCF, REG_FSR, DEST_F);       // Point to next bcd digit // INC for little indian
    Instruction(OP_DECFSZ, digcnt, DEST_F);      // Decrement digit counter
    Instruction(OP_GOTO, b2bcd2);                //  - go if digcnt > 0
    Instruction(OP_DECFSZ, bitcnt, DEST_F);      // Decrement bit counter
    Instruction(OP_GOTO, b2bcd1);                //  - go if bitcnt > 0
    Instruction(OP_RETLW, 0);

    if((savePicProgWriteP >> 11) != ((PicProgWriteP - 1) >> 11))
        THROW_COMPILER_EXCEPTION("Internal error.");
}

static void CallBin32BcdRoutine(const char *nameBcd, const char *nameBin)
{
    Bin32BcdNeeded = true;

    DWORD addrBin; // address is copied to ACb0
    MemForVariable(nameBin, &addrBin);

    int sizeBin; // value is copied to sovBin
    sizeBin = SizeOfVar(nameBin);

    if(IsNumber(nameBin)) {
        CopyLitToReg(ACb0, sizeBin, "", hobatoi(nameBin), nameBin);
    } else {
        CopyRegToReg(ACb0, sizeBin, addrBin, sizeBin, "", nameBin, true);
    }
    CopyLitToReg(sovBin, 1, "sovBin", sizeBin, "sizeBin");

    DWORD addrBcd; // address is copied to BCD0
    MemForVariable(nameBcd, &addrBcd);
    CopyLitToReg(BCD0, 1, "BCD0", addrBcd, "addrBcd");

    int sizeBcd; // value is copied to digBcd
    switch(sizeBin) {
        case 1:
            sizeBcd = 3;
            break;
        case 2:
            sizeBcd = 5;
            break;
        case 3:
            sizeBcd = 8;
            break;
        case 4:
            sizeBcd = 10;
            break;
        default:
            THROW_COMPILER_EXCEPTION("Internal error.");
    }
    if(sizeBcd != SizeOfVar(nameBcd)) {
        Error("sizeBcd=%d != SizeOfVar(nameBcd)==%d", sizeBcd, SizeOfVar(nameBcd));
        SetSizeOfVar(nameBcd, sizeBcd);
    }

    sizeBcd = SizeOfVar(nameBcd);
    CopyLitToReg(digBcd, 1, "digBcd", sizeBcd, "sizeBcd");

    ////DWORD BCD0, int sizeBcd, DWORD ACb0, int sizeBin
    CallWithPclath(Bin32BcdRoutineAddress);
}

static void CallBin32BcdRoutine(const NameArray& nameBcd, const NameArray& nameBin)
{
    CallBin32BcdRoutine(nameBcd.c_str(), nameBin.c_str());
}

//-----------------------------------------------------------------------------
// Alloc RAM for single bit and vars
//-----------------------------------------------------------------------------
static void AllocBitsVars()
{
    DWORD addr;
    int   bit;

    for(IntPc = 0; IntPc < IntCode.size(); IntPc++) {
        IntPcNow = IntPc;
        IntOp *a = &IntCode[IntPc];
        rungNow = a->rung;
        switch(a->op) {
            case INT_SET_BIT:
                MemForSingleBit(a->name1, false, &addr, &bit);
                break;

            case INT_CLEAR_BIT:
                MemForSingleBit(a->name1, false, &addr, &bit);
                break;

            case INT_COPY_BIT_TO_BIT:
            case INT_COPY_NOT_BIT_TO_BIT:
                MemForSingleBit(a->name1, false, &addr, &bit);
                MemForSingleBit(a->name2, true, &addr, &bit);
                break;

            case INT_SET_BIT_AND_BIT:
            case INT_SET_BIT_OR_BIT:
            case INT_SET_BIT_XOR_BIT:
                MemForSingleBit(a->name1, false, &addr, &bit);
                MemForSingleBit(a->name2, false, &addr, &bit);
                MemForSingleBit(a->name3, false, &addr, &bit);
                break;

            case INT_IF_BIT_SET:
                MemForSingleBit(a->name1, true, &addr, &bit);
                break;

            case INT_IF_BIT_CLEAR:
                MemForSingleBit(a->name1, true, &addr, &bit);
                break;

            case INT_UART_SENDn:
            case INT_UART_SEND:
                MemForSingleBit(a->name2, true, &addr, &bit);
                break;

            case INT_UART_SEND1:
            case INT_UART_RECV1:
                break;

            case INT_UART_RECV:
                MemForSingleBit(a->name2, true, &addr, &bit);
                break;

            case INT_UART_SEND_BUSY:
            case INT_UART_SEND_READY:
                MemForSingleBit(a->name1, true, &addr, &bit);
                break;

            case INT_UART_RECV_AVAIL:
                MemForSingleBit(a->name1, true, &addr, &bit);
                break;

            case INT_PWM_OFF: {
                char storeName[MAX_NAME_LEN];
                sprintf(storeName, "$pwm_init_%s", a->name1.c_str());
                MemForSingleBit(storeName, false, &addr, &bit);
                break;
            }
            case INT_SET_PWM: {
                char storeName[MAX_NAME_LEN];
                sprintf(storeName, "$pwm_init_%s", a->name3.c_str());
                MemForSingleBit(storeName, false, &addr, &bit);
                break;
            }
            case INT_EEPROM_BUSY_CHECK:
                MemForSingleBit(a->name1, false, &addr, &bit);
                break;

            default:
                break;
        }
    }
}

//-----------------------------------------------------------------------------
static void _CheckSovNames(int l, const char *f, const char *args, IntOp *a)
{
    return;
}
#define CheckSovNames(...) _CheckSovNames(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)

//-----------------------------------------------------------------------------
/*
Increment
; from David Cary: tested on PIC 16F877
; inc
    incf count0,f
    skpnz
    incf count1,f
    skpnz
    incf count2,f
    ;... {repeat the last two lines for as wide a counter as is needed}
    ; w is unchanged.
    ; Status:Z is 1 if and only if counter is now all zeros.
    ; rest of Status unchanged.
;from Dmitry Kiryashov
;inc
    movlw   1
    addwf   count0,F
    skpnc
    addwf   count1,F
    ;... {ed: repeat the last two lines for as wide a counter as is needed}
    ; w is now 1
    ; Status:Z and Status:C are 1 if and only if counter is now all zeros.
*/
static void Increment(DWORD addr, int sov, const char *name, const char *overlap, const char *overflow)
// a := a + 1
{
    if(overflow && strlen(overflow)) {
        Instruction(OP_MOVF, addr + sov - 1, DEST_W);
        Instruction(OP_MOVWF, ScratchS); // signBefore
    }
    Instruction(OP_INCF, addr, DEST_F, name);
    if(sov >= 2) {
        IfBitSet(REG_STATUS, STATUS_Z);
        Instruction(OP_INCF, addr + 1, DEST_F, name);
        if(sov >= 3) {
            IfBitSet(REG_STATUS, STATUS_Z);
            Instruction(OP_INCF, addr + 2, DEST_F, name);
            if(sov >= 4) {
                IfBitSet(REG_STATUS, STATUS_Z);
                Instruction(OP_INCF, addr + 3, DEST_F, name);
                if(sov > 4)
                    THROW_COMPILER_EXCEPTION("Internal error.");
            }
        }
    }
    DWORD addrO;
    int   bitO;
    if(overlap && strlen(overlap)) {
        MemForSingleBit(overlap, false, &addrO, &bitO);
        CopyBit(addrO, bitO, REG_STATUS, STATUS_Z); // zero as overlap flag; counter rolled over and is now all zeros.
    }
    if(overflow && strlen(overflow)) {
        /*
    if((signBefore == 0) && (signAfter != 0))
      SetSingleBit(overflow, true);
    */
        DWORD notOverflow = AllocFwdAddr();
        Instruction(OP_BTFSC, ScratchS, 7);       // signBefore
        Instruction(OP_GOTO, notOverflow);        // BIT7 == 1 if signBefore != 0
                                                  // BIT7 == 0 if signBefore == 0
        Instruction(OP_BTFSS, addr + sov - 1, 7); // signAfter
        Instruction(OP_GOTO, notOverflow);        // BIT7 == 0 if signAfter == 0
                                                  // BIT7 == 1 if signAfter != 0
        MemForSingleBit(overflow, false, &addrO, &bitO);
        SetBit(addrO, bitO);

        FwdAddrIsNow(notOverflow);
    }
}

static void Increment(DWORD addr, int sov, const NameArray &name, const NameArray &overlap, const NameArray &overflow)
{
    Increment(addr, sov, name.c_str(), overlap.c_str(), overflow.c_str());
}

static void Increment(DWORD addr, int sov, const char *name)
{
    Increment(addr, sov, name, nullptr, nullptr);
}

static void Increment(DWORD addr, int sov)
{
    Increment(addr, sov, nullptr, nullptr, nullptr);
}
//-----------------------------------------------------------------------------
static void UartSend(DWORD addr, int sov) // , char *name
{
    for(int i = 0; i < sov; i++) {
        IfBitClear(REG_TXSTA, TRMT); // TRMT=0 if TSR full
        Instruction(OP_GOTO, PicProgWriteP - 1);

        Instruction(OP_MOVF, addr + i, DEST_W);
        Instruction(OP_MOVWF, REG_TXREG);
    }
}
//-----------------------------------------------------------------------------
/*
Decrement
;dec
        movlw   1
        subwf   count0,F
        skpc
        subwf   count1,F
;... {ed: repeat the last two lines for as wide a counter as is needed}
    ; w is now 1
    ; Status:C is now 0 if and only if counter rolled over and is now all ones
    ; Status:Z is set if the least significant byte of the counter is zero
    ; (not necessarily all bytes !) and in certain other cases.
*/
static void Decrement(DWORD addr, int sov, const char *name, const char *overlap, const char *overflow)
// a := a - 1
{
    if(overflow && strlen(overflow)) {
        Instruction(OP_MOVF, addr + sov - 1, DEST_W);
        Instruction(OP_MOVWF, ScratchS); // signBefore
    }
    Instruction(OP_MOVLW, 1);
    Instruction(OP_SUBWF, addr, DEST_F, name);
    if(sov >= 2) {
        //IfBitSet(REG_STATUS, STATUS_C); BORROW !!!
        /* Note: For borrow, the polarity is reversed.
    A subtraction is executed by adding the two's
    complement of the second operand. */
        IfBitClear(REG_STATUS, STATUS_C);
        Instruction(OP_SUBWF, addr + 1, DEST_F, name);
        if(sov >= 3) {
            IfBitClear(REG_STATUS, STATUS_C);
            Instruction(OP_SUBWF, addr + 2, DEST_F, name);
            if(sov >= 4) {
                IfBitClear(REG_STATUS, STATUS_C);
                Instruction(OP_SUBWF, addr + 3, DEST_F, name);
                if(sov > 4)
                    THROW_COMPILER_EXCEPTION("Internal error.");
            }
        }
    }
    DWORD addrO;
    int   bitO;
    if(overlap && strlen(overlap)) {
        MemForSingleBit(overlap, false, &addrO, &bitO);
        CopyNotBit(addrO,
                   bitO,
                   REG_STATUS,
                   STATUS_C,
                   overlap,
                   "STATUS_C"); // borrow as overlap flag, counter rolled over and is now all ones.
    }
    if(overflow && strlen(overflow)) {
        /*
    if((signBefore != 0) && (signAfter == 0))
      SetSingleBit(overflow, true);
    */
        DWORD notOverflow = AllocFwdAddr();
        Instruction(OP_BTFSS, ScratchS, 7);
        Instruction(OP_GOTO, notOverflow); // BIT7 == 0 if signBefore == 0
                                           // BIT7 == 1 if signBefore != 0
        Instruction(OP_BTFSC, addr + sov - 1, 7);
        Instruction(OP_GOTO, notOverflow); // BIT7 == 1 if signAfter != 0
                                           // BIT7 == 0 if signAfter == 0
        MemForSingleBit(overflow, false, &addrO, &bitO);
        SetBit(addrO, bitO);

        FwdAddrIsNow(notOverflow);
    }
}

static void Decrement(DWORD addr, int sov, const NameArray &name, const NameArray &overlap, const NameArray &overflow)
{
    Decrement(addr, sov, name.c_str(), overlap.c_str(), overflow.c_str());
}

static void Decrement(DWORD addr, int sov, const char *name)
{
    Decrement(addr, sov, name, nullptr, nullptr);
}

static void Decrement(DWORD addr, int sov)
{
    Decrement(addr, sov, nullptr, nullptr, nullptr);
}

//-----------------------------------------------------------------------------
static void Negate(DWORD addr, int sov, const char *name)
{
    for(int i = 0; i < sov; i++)
        Instruction(OP_COMF, addr + i, DEST_F, name);
    Instruction(OP_INCF, addr, DEST_F, name);
    for(int i = 1; i < sov; i++) {
        skpnz;
        Instruction(OP_INCF, addr + i, DEST_F, name);
    }
}

//-----------------------------------------------------------------------------
static void aplusw(DWORD addr, int sov, char *name)
// a := a + W
{
    Instruction(OP_ADDWF, addr, DEST_F, name);
    if(sov >= 2) {
        IfBitSet(REG_STATUS, STATUS_C);
        Instruction(OP_INCF, addr + 1, DEST_F, name);
        if(sov >= 3) {
            IfBitSet(REG_STATUS, STATUS_Z);
            Instruction(OP_INCF, addr + 2, DEST_F, name);
            if(sov >= 4) {
                IfBitSet(REG_STATUS, STATUS_Z);
                Instruction(OP_INCF, addr + 3, DEST_F, name);
            }
        }
    }
}

static void aplusw(DWORD addr, int sov)
{
    aplusw(addr, sov, nullptr);
}

//-----------------------------------------------------------------------------
static void VariableAdd(DWORD addr1, DWORD addr2, DWORD addr3, int sov)
// addr1 = addr2 + addr3 // original
// ScratchS used !!!
{
    Instruction(OP_MOVF, addr2, DEST_W /*, a->name2*/);
    Instruction(OP_ADDWF, addr3, DEST_W /*, a->name3*/);
    Instruction(OP_MOVWF, addr1 /*, 0, a->name1*/);
    ClearBit(ScratchS, STATUS_C);
    IfBitSet(REG_STATUS, STATUS_C);
    SetBit(ScratchS, STATUS_C);

    Instruction(OP_MOVF, addr2 + 1, DEST_W);
    Instruction(OP_ADDWF, addr3 + 1, DEST_W);
    Instruction(OP_MOVWF, addr1 + 1);
    IfBitSet(ScratchS, STATUS_C);
    Instruction(OP_INCF, addr1 + 1, DEST_F);
}

//-----------------------------------------------------------------------------
/*
PIC_Mid_A_14.pdf
For example, for a 32-bit add:
    movf     a,w ; add byte 0 (LSB)
    addwf    b,f
    movf     a+1,w ; add byte 1
    btfsc    STATUS,C
    incfsz   a+1,w
    addwf    b+1,f
    movf     a+2,w ; add byte 2
    btfsc    STATUS,C
    incfsz   a+2,w
    addwf    b+2,f
    movf     a+3,w ; add byte 3 (MSB)
    btfsc    STATUS,C
    incfsz   a+3,w
    addwf    b+3,f
*/
/*
Для обнаружения переполнения разрядной сетки используют следующие способы:

1. Сравнивают знаки слагаемых со знаком суммы. Сигнал переполнения
вырабатывается тогда, когда знаки слагаемых одинаковы и не совпадают со
знаком суммы.
*/
static void add(DWORD b, DWORD a, int sov, const char *overlap, const char *overflow)
//                addrb    addra     sovb == sova
// r = b = b + a , b - is rewritten
{
    Comment("add");
    if(overflow && strlen(overflow)) {
        Instruction(OP_MOVF, b + sov - 1, DEST_W);
        Instruction(OP_XORWF, a + sov - 1, DEST_W);
        Instruction(OP_MOVWF, ScratchS); // if signa != signb then ScratchS:BIT7 is 1
    }
    Instruction(OP_MOVF, a, DEST_W);
    Instruction(OP_ADDWF, b, DEST_F);
    if(sov >= 2) {
        Instruction(OP_MOVF, a + 1, DEST_W);
        Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
        Instruction(OP_INCFSZ, a + 1, DEST_W);
        Instruction(OP_ADDWF, b + 1, DEST_F);
        if(sov >= 3) {
            Instruction(OP_MOVF, a + 2, DEST_W);
            Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
            Instruction(OP_INCFSZ, a + 2, DEST_W);
            Instruction(OP_ADDWF, b + 2, DEST_F);
            if(sov >= 4) {
                Instruction(OP_MOVF, a + 3, DEST_W);
                Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
                Instruction(OP_INCFSZ, a + 3, DEST_W);
                Instruction(OP_ADDWF, b + 3, DEST_F);
            }
        }
    }
    if(overflow && strlen(overflow)) {
    /*
    if((signb == signa)
    && (signr != signa))
        SetSingleBit(Overflow, true);
    */
        DWORD notOverflow = AllocFwdAddr();
        Instruction(OP_BTFSC, ScratchS, 7);
        Instruction(OP_GOTO, notOverflow); // BIT7 == 1 if signb != signa
                                           // BIT7 == 0 if signb == signa
        Instruction(OP_MOVF, b + sov - 1, DEST_W);
        Instruction(OP_XORWF, a + sov - 1, DEST_W);
        Instruction(OP_MOVWF, ScratchS);
        Instruction(OP_BTFSS, ScratchS, 7);
        Instruction(OP_GOTO, notOverflow); // BIT7 == 0 if signr == signa
                                           // BIT7 == 1 if signr != signa
        DWORD addr = 0;
        int   bit = -1;
        MemForSingleBit(overflow, false, &addr, &bit);
        SetBit(addr, bit, "$overflow");

        FwdAddrIsNow(notOverflow);
    }
}

static void add(DWORD b, DWORD a, int sov, const NameArray &overlap, const NameArray &overflow)
{
    add(b, a, sov, overlap.c_str(), overflow.c_str());
}

static void add(DWORD b, DWORD a, int sov, const char *overflow)
{
    add(b, a, sov, overflow, nullptr);
}

static void add(DWORD b, DWORD a, int sov)
{
    add(b, a, sov, nullptr, nullptr);
}

//-----------------------------------------------------------------------------
/*
PIC_Mid_A_14.pdf
For example, for a 32-bit subtraction:
    movf     a,w ; subtract byte 0 (LSB)
    subwf    b,f
    movf     a+1,w ; subtract byte 1
    btfss    STATUS,C
    incfsz   a+1,w
    subwf    b+1,f
    movf     a+2,w ; subtract byte 2
    btfss    STATUS,C
    incfsz   a+2,w
    subwf    b+2,f
    movf     a+3,w ; subtract byte 3 (MSB)
    btfss    STATUS,C
    incfsz   a+3,w
    subwf    b+3,f
*/
static void sub_(DWORD b, DWORD a, int sov, BYTE DEST_W_F, const char *overlap, const char *overflow)
//                  addrb    addra  sovb == sova
// r = b = b - a , b - is rewritten
{
    if(overflow && strlen(overflow)) {
        Comment("save sign");
        Instruction(OP_MOVF, b + sov - 1, DEST_W);
        Instruction(OP_XORWF, a + sov - 1, DEST_W);
        Instruction(OP_MOVWF, ScratchS); // if signa != signb then ScratchS:BIT7 is 1
    }
    Comment("sub_");
    Instruction(OP_MOVF, a, DEST_W); // subtract byte 0 (LSB)
    Instruction(OP_SUBWF, b, DEST_W_F);
    if(sov >= 2) {
        Instruction(OP_MOVF, a + 1, DEST_W); // subtract byte 1
        Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
        Instruction(OP_INCFSZ, a + 1, DEST_W);
        Instruction(OP_SUBWF, b + 1, DEST_W_F);
        if(sov >= 3) {
            Instruction(OP_MOVF, a + 2, DEST_W); // subtract byte 2
            Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
            Instruction(OP_INCFSZ, a + 2, DEST_W);
            Instruction(OP_SUBWF, b + 2, DEST_W_F);
            if(sov >= 4) {
                Instruction(OP_MOVF, a + 3, DEST_W); // subtract byte 3 (MSB)
                Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
                Instruction(OP_INCFSZ, a + 3, DEST_W);
                Instruction(OP_SUBWF, b + 3, DEST_W_F);
            }
        }
    }
    if(overflow && strlen(overflow)) {
    /*
    if((signb != signa)
    && (signr == signa))
          SetSingleBit(Overflow, true);
    */
        Comment("check sign");
        DWORD notOverflow = AllocFwdAddr();
        Instruction(OP_BTFSS, ScratchS, 7);
        Instruction(OP_GOTO, notOverflow); // BIT7 == 0 if sign2 == sign3
                                           // BIT7 == 1 if sign2 != sign3
        if(DEST_W_F == DEST_F)
            Instruction(OP_MOVF, b + sov - 1, DEST_W);
        else
            Instruction(OP_NOP_);
        Instruction(OP_XORWF, a + sov - 1, DEST_W);
        Instruction(OP_MOVWF, ScratchS);
        Instruction(OP_BTFSC, ScratchS, 7);
        Instruction(OP_GOTO, notOverflow); // BIT7 == 1 if sign2 != sign3
                                           // BIT7 == 0 if sign2 == sign3
        DWORD addr = 0;
        int   bit = -1;
        MemForSingleBit(overflow, false, &addr, &bit);
        SetBit(addr, bit, "$overflow");

        FwdAddrIsNow(notOverflow);
    }
}

static void sub(DWORD b, DWORD a, int sov, const char *overlap, const char *overflow)
{
    sub_(b, a, sov, DEST_F, overlap, overflow);
}

static void sub(DWORD b, DWORD a, int sov, const NameArray &overlap, const NameArray &overflow)
{
    sub(b, a, sov, overlap.c_str(), overflow.c_str());
}

static void sub(DWORD b, DWORD a, int sov)
{
    sub_(b, a, sov, DEST_F, nullptr, nullptr);
}
/*
static void cmp(DWORD b, DWORD a, int sov, char *overlap, char *overflow)
{
    sub_(b, a, sov, DEST_W, overlap, overflow);
}
/*
static void cmp(DWORD b, DWORD a, int sov)
{
    sub_(b, a, sov, DEST_W, nullptr, nullptr);
}
*/
//-----------------------------------------------------------------------------
static void AndReg(DWORD addr, int sov, DWORD reg2)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error.");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error.");
    for(int i = 0; i < sov; i++) {
        Instruction(OP_MOVF, reg2 + i, DEST_W);
        Instruction(OP_ANDWF, addr + i, DEST_F);
    }
}

static void OrReg(DWORD addr, int sov, DWORD reg2)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error.");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error.");
    for(int i = 0; i < sov; i++) {
        Instruction(OP_MOVF, reg2 + i, DEST_W);
        Instruction(OP_IORWF, addr + i, DEST_F);
    }
}

static void XorReg(DWORD addr, int sov, DWORD reg2)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error.");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error.");
    for(int i = 0; i < sov; i++) {
        Instruction(OP_MOVF, reg2 + i, DEST_W);
        Instruction(OP_XORWF, addr + i, DEST_F);
    }
}

//-----------------------------------------------------------------------------
static void shl(DWORD addr, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error.");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error.");
    Instruction(OP_BCF, REG_STATUS, STATUS_C); // 0 to Carry
    for(int i = 0; i < sov; i++)
        Instruction(OP_RLF, addr + i, DEST_F);
}

static void rol(DWORD addr, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error.");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error.");
    Instruction(OP_RLF, addr + sov - 1, DEST_W); // copy MSB bit 7 to Carry
    for(int i = 0; i < sov; i++)
        Instruction(OP_RLF, addr + i, DEST_F);
}

static void sr0(DWORD addr, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error.");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error.");
    Instruction(OP_BCF, REG_STATUS, STATUS_C); // 0 to Carry
    for(int i = sov - 1; i >= 0; i--)
        Instruction(OP_RRF, addr + i, DEST_F);
}

static void shr(DWORD addr, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error.");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error.");
    Instruction(OP_RLF, addr + sov - 1, DEST_W); // copy MSB bit 7 to Carry
    for(int i = sov - 1; i >= 0; i--)
        Instruction(OP_RRF, addr + i, DEST_F);
}

static void ror(DWORD addr, int sov)
{
    if(sov < 1)
        THROW_COMPILER_EXCEPTION("Internal error.");
    if(sov > 4)
        THROW_COMPILER_EXCEPTION("Internal error.");
    Instruction(OP_RRF, addr, DEST_W); // copy LSB bit 0 to Carry
    for(int i = sov - 1; i >= 0; i--)
        Instruction(OP_RRF, addr + i, DEST_F);
}

//-----------------------------------------------------------------------------
static void Delay(DWORD addr, int sov)
{
    //; Status:C is now 0 if and only if counter rolled over and is now all ones
    Instruction(OP_MOVLW, 1);
    DWORD Loop = PicProgWriteP;
    Instruction(OP_SUBWF, addr, DEST_F);
    if(sov >= 2) {
        //IfBitSet(REG_STATUS, STATUS_C); BORROW !!!
        /* Note: For borrow, the polarity is reversed.
    A subtraction is executed by adding the two's
    complement of the second operand. */
        IfBitClear(REG_STATUS, STATUS_C);
        Instruction(OP_SUBWF, addr + 1, DEST_F);
        if(sov >= 3) {
            IfBitClear(REG_STATUS, STATUS_C);
            Instruction(OP_SUBWF, addr + 2, DEST_F);
            if(sov >= 4) {
                IfBitClear(REG_STATUS, STATUS_C);
                Instruction(OP_SUBWF, addr + 3, DEST_F);
                if(sov > 4)
                    THROW_COMPILER_EXCEPTION("Internal error.");
            }
        }
    }
    IfBitSet(REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, Loop);
}

#ifdef TABLE_IN_FLASH
//-----------------------------------------------------------------------------
static void InitTable(IntOp *a)
{
    DWORD savePicProgWriteP = PicProgWriteP;
    DWORD addrOfTableRoutine = 0;
    MemOfVar(a->name1, &addrOfTableRoutine);

    if(addrOfTableRoutine == 0) {
        Comment("TABLE %s[%d]", a->name1.c_str(), a->literal);
        addrOfTableRoutine = PicProgWriteP;

        SetMemForVariable(a->name1, addrOfTableRoutine, a->literal);

#define TABLE_CALC 8
        //This code is unrealocable.
        Instruction(OP_MOVLW, ((addrOfTableRoutine + TABLE_CALC) >> 8) & 0xFF);
        Instruction(OP_ADDWF, Scratch1, DEST_W); // index hi
        Instruction(OP_MOVWF, REG_PCLATH);

        Instruction(OP_MOVLW, (addrOfTableRoutine + TABLE_CALC) & 0xFF);
        Instruction(OP_ADDWF, Scratch0, DEST_W); // index lo
        skpnc;
        Instruction(OP_INCF, REG_PCLATH, DEST_F);
        Instruction(OP_MOVWF, REG_PCL); // jump

        if((addrOfTableRoutine + TABLE_CALC) != PicProgWriteP)
            THROW_COMPILER_EXCEPTION_FMT("TABLE_CALC=%d", PicProgWriteP - addrOfTableRoutine);

        int sovElement = a->literal2;
        Comment("DATA's size is %d", sovElement);
        for(int i = 0; i < a->literal; i++) {
            if(sovElement == 1) {
                Instruction(OP_RETLW, a->data[i]);
            } else if(sovElement == 2) {
                Instruction(OP_RETLW, a->data[i] & 0xFF);
                Instruction(OP_RETLW, a->data[i] >> 8);
            } else if(sovElement == 3) {
                Instruction(OP_RETLW, a->data[i] & 0xFF);
                Instruction(OP_RETLW, (a->data[i] >> 8) & 0xFF);
                Instruction(OP_RETLW, a->data[i] >> 16);
            } else if(sovElement == 4) {
                Instruction(OP_RETLW, a->data[i] & 0xFF);
                Instruction(OP_RETLW, (a->data[i] >> 8) & 0xFF);
                Instruction(OP_RETLW, (a->data[i] >> 16) & 0xFF);
                Instruction(OP_RETLW, a->data[i] >> 24);
            } else
                THROW_COMPILER_EXCEPTION("Internal error.");
        }
        Comment("TABLE %s END", a->name1.c_str());
    }

    if((savePicProgWriteP >> 11) != ((PicProgWriteP - 1) >> 11))
        THROW_COMPILER_EXCEPTION("Internal error.");

    notRealocableAddr = PicProgWriteP - 1; // Index calculation can't be moved
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
                //Comment("INT_FLASH_INIT %dbyte %s[%d]", a->literal2, a->name1, a->literal);
                InitTable(a);
                break;
            default:
                break;
        }
    }
}
#endif
//-----------------------------------------------------------------------------
// Compile the intermediate code to PIC16 native code.
//-----------------------------------------------------------------------------
static void CompileFromIntermediate(bool topLevel)
{
    DWORD addr1 = -1, addr2 = -1, addr3 = -1, addr4 = -1;
    int   bit1 = -1, bit2 = -1, bit3 = -1, bit4 = -1;
    int   bit = -1;
    int   sov = -1, sov1 = -1, sov2 = -1, sov3 = -1;
    char  comment[MAX_NAME_LEN] = "";

    // Keep track of which 2k section we are using. When it looks like we
    // are about to run out, fill with nops and move on to the next one.
    DWORD section = 0;

    for(; IntPc < IntCode.size(); IntPc++) {
        IntPcNow = IntPc;
        IntOp *a = &IntCode[IntPc];
        rungNow = a->rung;
        switch(a->op) {
            case INT_SET_BIT:
                Comment("INT_SET_BIT %s", a->name1.c_str());
                MemForSingleBit(a->name1, false, &addr1, &bit1);
                SetBit(addr1, bit1, a->name1);
                break;

            case INT_CLEAR_BIT:
                Comment("INT_CLEAR_BIT %s", a->name1.c_str());
                MemForSingleBit(a->name1, false, &addr1, &bit1);
                ClearBit(addr1, bit1, a->name1);
                break;

            case INT_COPY_BIT_TO_BIT:
                Comment("INT_COPY_BIT_TO_BIT %s:=%s", a->name1.c_str(), a->name2.c_str());
                MemForSingleBit(a->name1, false, &addr1, &bit1);
                MemForSingleBit(a->name2, true, &addr2, &bit2);
                CopyBit(addr1, bit1, addr2, bit2);
                break;

            case INT_COPY_NOT_BIT_TO_BIT:
                Comment("INT_COPY_NOT_BIT_TO_BIT %s:=!%s", a->name1.c_str(), a->name2.c_str());
                MemForSingleBit(a->name1, false, &addr1, &bit1);
                MemForSingleBit(a->name2, true, &addr2, &bit2);
                CopyNotBit(addr1, bit1, addr2, bit2);
                break;

            case INT_SET_BIT_XOR_BIT:
                Comment("INT_SET_BIT_XOR_BIT %s:=%s^%s", a->name1.c_str(), a->name2.c_str(), a->name3.c_str());
                MemForSingleBit(a->name1, false, &addr1, &bit1);
                MemForSingleBit(a->name2, false, &addr2, &bit2);
                MemForSingleBit(a->name3, false, &addr3, &bit3);
                XorBit(addr1, bit1, addr2, bit2/*, addr3, bit3*/);
                break;
            //
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
                        THROW_COMPILER_EXCEPTION("Internal error.");
                } else {
                    CopyVarToReg(ScratchS, 1, a->name2);
                    CopyLitToReg(Scratch0, sov1, "", -2, ""); // 0xF..FE
                    DWORD Skip = AllocFwdAddr();
                    DWORD Loop = PicProgWriteP;
                    Instruction(OP_MOVF, ScratchS, DEST_F);
                    skpnz;
                    Instruction(OP_GOTO, Skip);
                    Instruction(OP_DECF, ScratchS, DEST_F);
                    rol(Scratch0, sov1);
                    Instruction(OP_GOTO, Loop);
                    FwdAddrIsNow(Skip);

                    AndReg(addr1, sov1, Scratch0);
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
                        THROW_COMPILER_EXCEPTION("Internal error.");
                } else {
                    CopyVarToReg(ScratchS, 1, a->name2);
                    CopyLitToReg(Scratch0, sov1, "", 0x01, "");
                    DWORD Skip = AllocFwdAddr();
                    DWORD Loop = PicProgWriteP;
                    Instruction(OP_MOVF, ScratchS, DEST_F);
                    skpnz;
                    Instruction(OP_GOTO, Skip);
                    Instruction(OP_DECF, ScratchS, DEST_F);
                    shl(Scratch0, sov1);
                    Instruction(OP_GOTO, Loop);
                    FwdAddrIsNow(Skip);

                    OrReg(addr1, sov1, Scratch0);
                }
                break;
            }
            case INT_IF_BIT_SET_IN_VAR: {
                Comment("INT_IF_BIT_SET_IN_VAR %s %s", a->name1.c_str(), a->name2.c_str());
                DWORD endifAddr = AllocFwdAddr();
                DWORD ifAddr = AllocFwdAddr();
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);
                if(IsNumber(a->name2)) {
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
                        THROW_COMPILER_EXCEPTION("Internal error.");
                    Instruction(OP_GOTO, endifAddr); // here bit is CLR
                } else {
                    CopyVarToReg(ScratchS, 1, a->name2);
                    CopyLitToReg(Scratch0, sov1, "", 0x01, "");
                    DWORD Skip = AllocFwdAddr();
                    DWORD Loop = PicProgWriteP;
                    Instruction(OP_MOVF, ScratchS, DEST_F);
                    skpnz;
                    Instruction(OP_GOTO, Skip);
                    Instruction(OP_DECF, ScratchS, DEST_F);
                    shl(Scratch0, sov1);
                    Instruction(OP_GOTO, Loop);
                    FwdAddrIsNow(Skip);

                    Instruction(OP_MOVF, Scratch0, DEST_W);
                    Instruction(OP_ANDWF, addr1, DEST_W);
                    Instruction(OP_SUBWF, Scratch0, DEST_W);
                    skpz;
                    Instruction(OP_GOTO, endifAddr); // here bit is CLR
                    if(sov1 >= 2) {
                        Instruction(OP_MOVF, Scratch0 + 1, DEST_W);
                        Instruction(OP_ANDWF, addr1 + 1, DEST_W);
                        Instruction(OP_SUBWF, Scratch0 + 1, DEST_W);
                        skpz;
                        Instruction(OP_GOTO, endifAddr); // here bit is CLR
                    }
                    if(sov1 >= 3) {
                        Instruction(OP_MOVF, Scratch0 + 2, DEST_W);
                        Instruction(OP_ANDWF, addr1 + 2, DEST_W);
                        Instruction(OP_SUBWF, Scratch0 + 2, DEST_W);
                        skpz;
                        Instruction(OP_GOTO, endifAddr); // here bit is CLR
                    }
                    if(sov1 >= 4) {
                        Instruction(OP_MOVF, Scratch0 + 3, DEST_W);
                        Instruction(OP_ANDWF, addr1 + 3, DEST_W);
                        Instruction(OP_SUBWF, Scratch0 + 3, DEST_W);
                        skpz;
                        Instruction(OP_GOTO, endifAddr); // here bit is CLR
                    }
                    //Instruction(OP_GOTO, ifAddr);      // here bit is SET
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
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);
                if(IsNumber(a->name2)) {
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
                        THROW_COMPILER_EXCEPTION("Internal error.");
                    Instruction(OP_GOTO, endifAddr); // here bit is SET
                } else {
                    CopyVarToReg(ScratchS, 1, a->name2);
                    CopyLitToReg(Scratch0, sov1, "", 0x01, "");
                    DWORD Skip = AllocFwdAddr();
                    DWORD Loop = PicProgWriteP;
                    Instruction(OP_MOVF, ScratchS, DEST_F);
                    skpnz;
                    Instruction(OP_GOTO, Skip);
                    Instruction(OP_DECF, ScratchS, DEST_F);
                    shl(Scratch0, sov1);
                    Instruction(OP_GOTO, Loop);
                    FwdAddrIsNow(Skip);

                    Instruction(OP_MOVF, Scratch0, DEST_W);
                    Instruction(OP_ANDWF, addr1, DEST_W);
                    Instruction(OP_SUBWF, Scratch0, DEST_W);
                    skpz;
                    Instruction(OP_GOTO, ifAddr); // here bit is CLR
                    if(sov1 >= 2) {
                        Instruction(OP_MOVF, Scratch0 + 1, DEST_W);
                        Instruction(OP_ANDWF, addr1 + 1, DEST_W);
                        Instruction(OP_SUBWF, Scratch0 + 1, DEST_W);
                        skpz;
                        Instruction(OP_GOTO, ifAddr); // here bit is CLR
                    }
                    if(sov1 >= 3) {
                        Instruction(OP_MOVF, Scratch0 + 2, DEST_W);
                        Instruction(OP_ANDWF, addr1 + 2, DEST_W);
                        Instruction(OP_SUBWF, Scratch0 + 2, DEST_W);
                        skpz;
                        Instruction(OP_GOTO, ifAddr); // here bit is CLR
                    }
                    if(sov1 >= 4) {
                        Instruction(OP_MOVF, Scratch0 + 3, DEST_W);
                        Instruction(OP_ANDWF, addr1 + 3, DEST_W);
                        Instruction(OP_SUBWF, Scratch0 + 3, DEST_W);
                        skpz;
                        Instruction(OP_GOTO, ifAddr); // here bit is CLR
                    }
                    Instruction(OP_GOTO, endifAddr); // here bit is SET
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
            case INT_SET_VARIABLE_TO_LITERAL:
                Comment("INT_SET_VARIABLE_TO_LITERAL %s:=0x%X(%d)", a->name1.c_str(), a->literal, a->literal);
                CheckSovNames(a);
                MemForVariable(a->name1, &addr1);
                sprintf(comment, "%s(0x%X):=%d(0x%X)", a->name1.c_str(), addr1, a->literal, a->literal);
                sov1 = SizeOfVar(a->name1);
                //sov2 = byteNeeded(a->literal);
                CopyLitToReg(addr1, sov1, a->name1.c_str(), a->literal, comment);
                break;

            case INT_INCREMENT_VARIABLE: {
                Comment("INT_INCREMENT_VARIABLE %s overlap to %s overflow to %s",
                        a->name1.c_str(),
                        a->name2.c_str(),
                        a->name3.c_str());
                CheckSovNames(a);
                sov1 = SizeOfVar(a->name1);
                MemForVariable(a->name1, &addr1);
                Increment(addr1, sov1, a->name1, a->name2, a->name3);
                break;
            }
            case INT_DECREMENT_VARIABLE: {
                Comment("INT_DECREMENT_VARIABLE %s overlap to %s overflow to %s",
                        a->name1.c_str(),
                        a->name2.c_str(),
                        a->name3.c_str());
                CheckSovNames(a);
                sov1 = SizeOfVar(a->name1);
                MemForVariable(a->name1, &addr1);
                Decrement(addr1, sov1, a->name1, a->name2, a->name3);
                break;
            }
            case INT_IF_BIT_EQU_BIT: {
                Comment("INT_IF_BIT_EQU_BIT %s %s", a->name1.c_str(), a->name2.c_str());
                DWORD condFalse = AllocFwdAddr();
                DWORD condTrue = AllocFwdAddr();
                DWORD now1Set = AllocFwdAddr();
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                MemForSingleBit(a->name2, true, &addr2, &bit2);
                IfBitSet(addr1, bit1, a->name1);
                Instruction(OP_GOTO, now1Set);
                //now1Clear
                IfBitSet(addr2, bit2, a->name2);
                Instruction(OP_GOTO, condFalse);
                Instruction(OP_GOTO, condTrue);

                FwdAddrIsNow(now1Set);
                IfBitClear(addr2, bit2, a->name2);
                Instruction(OP_GOTO, condFalse);

                FwdAddrIsNow(condTrue);
                CompileIfBody(condFalse);
                break;
            }
            case INT_IF_BIT_NEQ_BIT: {
                Comment("INT_IF_BIT_NEQ_BIT %s %s", a->name1.c_str(), a->name2.c_str());
                DWORD condFalse = AllocFwdAddr();
                DWORD condTrue = AllocFwdAddr();
                DWORD now1Set = AllocFwdAddr();
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                MemForSingleBit(a->name2, true, &addr2, &bit2);
                IfBitSet(addr1, bit1, a->name1);
                Instruction(OP_GOTO, now1Set);
                //now1Clear
                IfBitClear(addr2, bit2, a->name2);
                Instruction(OP_GOTO, condFalse);
                Instruction(OP_GOTO, condTrue);

                FwdAddrIsNow(now1Set);
                IfBitSet(addr2, bit2, a->name2);
                Instruction(OP_GOTO, condFalse);

                FwdAddrIsNow(condTrue);
                CompileIfBody(condFalse);
                break;
            }
            case INT_IF_BIT_SET: {
                Comment("INT_IF_BIT_SET %s", a->name1.c_str());
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                IfBitClear(addr1, bit1, a->name1);
                Instruction(OP_GOTO, condFalse);
                CompileIfBody(condFalse);
                break;
            }
            case INT_IF_BIT_CLEAR: {
                Comment("INT_IF_BIT_CLEAR %s", a->name1.c_str());
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                IfBitSet(addr1, bit1, a->name1);
                Instruction(OP_GOTO, condFalse);
                CompileIfBody(condFalse);
                break;
            }

#ifdef NEW_CMP
            case INT_IF_NEQ:
                Comment("INT_IF_NEQ");
                goto cmp1;
            case INT_IF_EQU:
                Comment("INT_IF_EQU");
                goto cmp1;
            cmp1 : {
                CheckSovNames(a);
                DWORD ifThen = AllocFwdAddr(); // isTrue
                DWORD ifEnd = AllocFwdAddr();  // notTrue

                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                sov = std::max(sov1, sov2);

                DWORD addrA = CopyArgToReg(false, Scratch0, sov, a->name1, false);
                DWORD addrB = CopyArgToReg(false, Scratch4, sov, a->name2, false);

                for(int i = 0; i < sov; i++) {
                    Instruction(OP_MOVF, addrA + i, DEST_W);
                    Instruction(OP_SUBWF, addrB + i, DEST_W);
                    switch(a->op) {
                        case INT_IF_NEQ: // ELEM_EQU
                            IfBitClear(REG_STATUS, STATUS_Z);
                            Instruction(OP_GOTO, ifThen); // Z=0, A!=B
                            break;
                        case INT_IF_EQU: // ELEM_NEQ
                            IfBitClear(REG_STATUS, STATUS_Z);
                            Instruction(OP_GOTO, ifEnd); // Z=0, A!=B
                            break;
                        default:
                            THROW_COMPILER_EXCEPTION("Internal error.");
                    }
                }
                switch(a->op) {
                    case INT_IF_NEQ:                 // ELEM_EQU
                        Instruction(OP_GOTO, ifEnd); // Z=1, A==B
                        break;
                }
                //ifThen:
                FwdAddrIsNow(ifThen);
                CompileIfBody(ifEnd);
                //ifEnd:
                break;
            }

            case INT_IF_GEQ:
                Comment("INT_IF_GEQ %s %s", a->name1.c_str(), a->name2.c_str());
                goto cmp2;
            // (A>=B) equal to (A-B), C=0
            case INT_IF_LEQ:
                Comment("INT_IF_LEQ %s %s", a->name1.c_str(), a->name2.c_str());
                goto cmp2;
            // (A<=B) equal to (B-A), C=0
            case INT_IF_LES:
                Comment("INT_IF_LES %s %s", a->name1.c_str(), a->name2.c_str());
                goto cmp2;
            // (A<B) equal to !(A>=B) equal to (A-B), C=1
            case INT_IF_GRT:
                Comment("INT_IF_GRT %s %s", a->name1.c_str(), a->name2.c_str());
                goto cmp2;
            // (A>B) equal to !(A<=B) equal to (B-A), C=1
            cmp2 : {
                CheckSovNames(a);
                DWORD ifThen = AllocFwdAddr(); // isTrue
                DWORD ifEnd = AllocFwdAddr();  // notTrue

                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                sov = std::max(sov1, sov2);

                DWORD addrA;
                DWORD addrB;
                switch(a->op) {
                    case INT_IF_GEQ: // op1 - op2
                    case INT_IF_LES:
                        // don't change argument a->name1 !!!
                        addrB = CopyArgToReg(true, Scratch0, sov, a->name1, true);
                        addrA = CopyArgToReg(false, Scratch4, sov, a->name2, true);
                        break;
                    case INT_IF_LEQ: // op2 - op1
                    case INT_IF_GRT:
                        // don't change argument a->name2 !!!
                        addrB = CopyArgToReg(true, Scratch0, sov, a->name2, true);
                        addrA = CopyArgToReg(false, Scratch4, sov, a->name1, true);
                        break;
                    default:
                        THROW_COMPILER_EXCEPTION("Internal error.");
                }

                DWORD addrO;
                int   bitO;
                MemForSingleBit("$overflow", &addrO, &bitO);
                ClearBit(addrO, bitO, "$overflow");

                // sub used, we need $overflow
                sub(addrB, addrA, sov, nullptr, "$overflow"); // b = b - a , b - is rewritten
/*
If the exclusive-or of the sign and overflow flags is 1,
the subtraction result was less than zero,
otherwise the result was zero or greater.
*/
                XorBit(addrO, bitO, addrB + sov - 1, 7);
                switch(a->op) {
                    case INT_IF_GEQ:
                    case INT_IF_LEQ:
                        IfBitClear(addrO, bitO);
                        break;
                    case INT_IF_LES:
                    case INT_IF_GRT:
                        IfBitSet(addrO, bitO);
                        break;
                    default:
                        THROW_COMPILER_EXCEPTION("Internal error.");
                }
                Instruction(OP_GOTO, ifThen);
                Instruction(OP_GOTO, ifEnd);
                //ifThen:
                FwdAddrIsNow(ifThen);
                CompileIfBody(ifEnd);
                //ifEnd:
                break;
            }
#else
            case INT_IF_VARIABLE_LES_LITERAL: {
                Comment("INT_IF_VARIABLE_LES_LITERAL %s < 0x%X(%d)", a->name1.c_str(), a->literal, a->literal);
                DWORD notTrue = AllocFwdAddr();
                DWORD isTrue = AllocFwdAddr();
                DWORD lsbDecides = AllocFwdAddr();

                // V = Rd7*(Rr7')*(R7') + (Rd7')*Rr7*R7 ; but only one of the
                // product terms can be true, and we know which at compile
                // time
                BYTE litH = (BYTE)((a->literal >> 8) & 0xff);
                BYTE litL = (BYTE)(a->literal & 0xff);

                MemForVariable(a->name1, &addr1);

                // var - lit
                Instruction(OP_MOVLW, litH);
                Instruction(OP_SUBWF, addr1 + 1, DEST_W, a->name1);
                IfBitSet(REG_STATUS, STATUS_Z);
                Instruction(OP_GOTO, lsbDecides);
                Instruction(OP_MOVWF, Scratch0);
                if(litH & 0x80) {
                    Instruction(OP_COMF, addr1 + 1, DEST_W);
                    Instruction(OP_ANDWF, Scratch0, DEST_W);
                    Instruction(OP_XORWF, Scratch0, DEST_F);
                } else {
                    Instruction(OP_COMF, Scratch0, DEST_W);
                    Instruction(OP_ANDWF, addr1 + 1, DEST_W);
                    Instruction(OP_XORWF, Scratch0, DEST_F);
                }
                IfBitSet(Scratch0, 7); // var - lit < 0, var < lit
                Instruction(OP_GOTO, isTrue);
                Instruction(OP_GOTO, notTrue);

                FwdAddrIsNow(lsbDecides);

                // var - lit < 0
                // var < lit
                Instruction(OP_MOVLW, litL);
                Instruction(OP_SUBWF, addr1, DEST_W, a->name1);
                IfBitClear(REG_STATUS, STATUS_C);
                Instruction(OP_GOTO, isTrue);
                Instruction(OP_GOTO, notTrue);

                FwdAddrIsNow(isTrue);
                CompileIfBody(notTrue);
                break;
            }
            case INT_IF_VARIABLE_EQUALS_VARIABLE: {
                DWORD notEqual = AllocFwdAddr();

                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                Instruction(OP_MOVF, addr1, DEST_W, a->name1);
                Instruction(OP_SUBWF, addr2, DEST_W, a->name2);
                IfBitClear(REG_STATUS, STATUS_Z);
                Instruction(OP_GOTO, notEqual);

                Instruction(OP_MOVF, addr1 + 1, DEST_W);
                Instruction(OP_SUBWF, addr2 + 1, DEST_W);
                IfBitClear(REG_STATUS, STATUS_Z);
                Instruction(OP_GOTO, notEqual);

                CompileIfBody(notEqual);
                break;
            }
            case INT_IF_VARIABLE_GRT_VARIABLE: {
                DWORD notTrue = AllocFwdAddr();
                DWORD isTrue = AllocFwdAddr();
                DWORD lsbDecides = AllocFwdAddr();

                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);

                // first, a signed comparison of the high octets, which is
                // a huge pain on the PIC16
                DWORD iu = addr2 + 1, ju = addr1 + 1;
                DWORD signa = Scratch0;
                DWORD signb = Scratch1;

                Instruction(OP_COMF, ju, DEST_W);
                Instruction(OP_MOVWF, signb);

                Instruction(OP_ANDWF, iu, DEST_W);
                Instruction(OP_MOVWF, signa);

                Instruction(OP_MOVF, iu, DEST_W);
                Instruction(OP_IORWF, signb, DEST_F);
                Instruction(OP_COMF, signb, DEST_F);

                Instruction(OP_MOVF, ju, DEST_W);
                Instruction(OP_SUBWF, iu, DEST_W);
                IfBitSet(REG_STATUS, STATUS_Z);
                Instruction(OP_GOTO, lsbDecides);

                Instruction(OP_ANDWF, signb, DEST_F);
                Instruction(OP_MOVWF, Scratch2);
                Instruction(OP_COMF, Scratch2, DEST_W);
                Instruction(OP_ANDWF, signa, DEST_W);
                Instruction(OP_IORWF, signb, DEST_W);
                Instruction(OP_XORWF, Scratch2, DEST_F);
                IfBitSet(Scratch2, 7);
                Instruction(OP_GOTO, isTrue);

                Instruction(OP_GOTO, notTrue);

                FwdAddrIsNow(lsbDecides);
                Instruction(OP_MOVF, addr1, DEST_W);
                Instruction(OP_SUBWF, addr2, DEST_W);
                IfBitClear(REG_STATUS, STATUS_C);
                Instruction(OP_GOTO, isTrue);

                Instruction(OP_GOTO, notTrue);

                FwdAddrIsNow(isTrue);
                CompileIfBody(notTrue);
                break;
            }
#endif
            case INT_SET_VARIABLE_TO_VARIABLE:
                Comment("INT_SET_VARIABLE_TO_VARIABLE %s := %s", a->name1.c_str(), a->name2.c_str());
                CheckSovNames(a);
                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);

                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);

                CopyRegToReg(addr1, sov1, addr2, sov2, a->name1, a->name2, true);
                break;

            case INT_SET_SWAP: {
                Comment("INT_SET_SWAP %s := SWAP(%s)", a->name1.c_str(), a->name2.c_str());
                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                CopyArgToReg(true, Scratch0, sov2, a->name2, false);
                if(sov2 == 1) {
                    Instruction(OP_SWAPF, Scratch0, DEST_F);
                } else if(sov2 == 2) {
                    Instruction(OP_MOVF, Scratch0, DEST_W);
                    Instruction(OP_MOVWF, ScratchS);
                    Instruction(OP_MOVF, Scratch0 + 1, DEST_W);
                    Instruction(OP_MOVWF, Scratch0);
                    Instruction(OP_MOVF, ScratchS, DEST_W);
                    Instruction(OP_MOVWF, Scratch0 + 1);
                } else if(sov2 == 3) {
                    Instruction(OP_SWAPF, Scratch0 + 1, DEST_F);

                    Instruction(OP_MOVF, Scratch0, DEST_W);
                    Instruction(OP_MOVWF, ScratchS);
                    Instruction(OP_MOVF, Scratch0 + 2, DEST_W);
                    Instruction(OP_MOVWF, Scratch0);
                    Instruction(OP_MOVF, ScratchS, DEST_W);
                    Instruction(OP_MOVWF, Scratch0 + 2);
                } else if(sov2 == 4) {
                    Instruction(OP_MOVF, Scratch0, DEST_W);
                    Instruction(OP_MOVWF, ScratchS);
                    Instruction(OP_MOVF, Scratch0 + 3, DEST_W);
                    Instruction(OP_MOVWF, Scratch0);
                    Instruction(OP_MOVF, ScratchS, DEST_W);
                    Instruction(OP_MOVWF, Scratch0 + 3);

                    Instruction(OP_MOVF, Scratch0 + 1, DEST_W);
                    Instruction(OP_MOVWF, ScratchS);
                    Instruction(OP_MOVF, Scratch0 + 2, DEST_W);
                    Instruction(OP_MOVWF, Scratch0 + 1);
                    Instruction(OP_MOVF, ScratchS, DEST_W);
                    Instruction(OP_MOVWF, Scratch0 + 2);
                } else
                    THROW_COMPILER_EXCEPTION("Internal error.");

                        MemForVariable(a->name1, &addr1);
                CopyRegToReg(addr1, sov1, Scratch0, sov2, a->name1, "$Scratch0", false);
                break;
            }
            case INT_SET_OPPOSITE: {
                Comment("INT_SET_OPPOSITE %s := OPPOSITE(%s)", a->name1.c_str(), a->name2.c_str());
                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                CopyArgToReg(true, Scratch0, sov2, a->name2, false);
                CopyLitToReg(Scratch4, sov2, a->name2.c_str(), 0x0, "$OPPOSITE");
                for(int j = 0; j < 8 * sov2; j++) {
                    for(int i = 0; i < sov2; i++) {
                        Instruction(OP_RLF, Scratch0 + i, DEST_F);
                    }
                    for(int i = sov2 - 1; i >= 0; i--) {
                        Instruction(OP_RRF, Scratch4 + i, DEST_F);
                    }
                }
                MemForVariable(a->name1, &addr1);
                CopyRegToReg(addr1, sov1, Scratch4, sov2, a->name1, "$Scratch4", false);
                break;
            }
            case INT_SET_VARIABLE_ROR:
                Comment("INT_SET_VARIABLE_ROR");
                goto ror;
            case INT_SET_VARIABLE_ROL:
                Comment("INT_SET_VARIABLE_ROL");
                goto ror;
            case INT_SET_VARIABLE_SHL:
                Comment("INT_SET_VARIABLE_SHL");
                goto ror;
            case INT_SET_VARIABLE_SHR:
                Comment("INT_SET_VARIABLE_SHR");
                goto ror;
            case INT_SET_VARIABLE_SR0:
                Comment("INT_SET_VARIABLE_SR0");
                goto ror;
            ror : {
                CheckSovNames(a);

                sov1 = SizeOfVar(a->name1);
                if(sov1 < 1)
                    THROW_COMPILER_EXCEPTION("Internal error.");
                MemForVariable(a->name1, &addr1);

                sov2 = SizeOfVar(a->name2);
                if(sov2 < 1)
                    THROW_COMPILER_EXCEPTION("Internal error.");
                // full source copy to shadow
                // all operation execute in shadow
                // and then copy to dest
                CopyArgToReg(true, Scratch0, sov2, a->name2, false);

                DWORD addrA = Scratch0;
                if((a->op == INT_SET_VARIABLE_SR0) || //
                   (a->op == INT_SET_VARIABLE_ROR) || //
                   (a->op == INT_SET_VARIABLE_SHR)) {
                    addrA += sov2 - 1; // start at MSB
                } else {
                    // start at LSB
                }

                sov3 = SizeOfVar(a->name3);
                //if(sov3 != 1) oops();
                sov3 = 1;
                CopyArgToReg(true, ScratchS, sov3, a->name3, false);
                DWORD loop = PicProgWriteP;

                if(a->op == INT_SET_VARIABLE_SR0) {
                    ClearBit(REG_STATUS, STATUS_C);
                } else if(a->op == INT_SET_VARIABLE_ROL) {
                    Instruction(OP_RLF, addrA + sov1 - 1, DEST_W); // copy MSB bit 7 to Carry
                } else if(a->op == INT_SET_VARIABLE_SHL) {
                    ClearBit(REG_STATUS, STATUS_C);
                } else if(a->op == INT_SET_VARIABLE_ROR) {
                    Instruction(OP_RRF, addrA - sov1 + 1, DEST_W); // copy LSB bit 0 to Carry
                } else if(a->op == INT_SET_VARIABLE_SHR) {
                    Instruction(OP_RLF, addrA + sov1 - 1, DEST_W); // copy MSB bit 7 to Carry
                } else
                    THROW_COMPILER_EXCEPTION("Internal error.");

                for(int i = 0; i < sov2; i++) {
                    if(a->op == INT_SET_VARIABLE_SR0) {
                        Instruction(OP_RRF, addrA - i, DEST_F);
                    } else if(a->op == INT_SET_VARIABLE_ROL) {
                        Instruction(OP_RLF, addrA + i, DEST_F);
                    } else if(a->op == INT_SET_VARIABLE_SHL) {
                        Instruction(OP_RLF, addrA + i, DEST_F);
                    } else if(a->op == INT_SET_VARIABLE_ROR) {
                        Instruction(OP_RRF, addrA - i, DEST_F);
                    } else if(a->op == INT_SET_VARIABLE_SHR) {
                        Instruction(OP_RRF, addrA - i, DEST_F);
                    } else
                        THROW_COMPILER_EXCEPTION("Internal error.");
                }

                Instruction(OP_DECFSZ, ScratchS, DEST_F);
                Instruction(OP_GOTO, loop);

                if(a->name4.size()) {
                    MemForSingleBit(a->name4, true, &addr4, &bit4);
                    CopyBit(addr4, bit4, REG_STATUS, STATUS_C, a->name4, "REG_STATUS_C");
                }
                CopyRegToReg(addr1, sov1, Scratch0, sov2, a->name1, "$Scratch0", false);
                break;
            }
            case INT_SET_VARIABLE_AND:
                Comment("INT_SET_VARIABLE_AND");
                goto andlabel;
            case INT_SET_VARIABLE_OR:
                Comment("INT_SET_VARIABLE_OR ");
                goto andlabel;
                break;
            case INT_SET_VARIABLE_XOR:
                Comment("INT_SET_VARIABLE_XOR");
                goto andlabel;
            andlabel : {
                CheckSovNames(a);
                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                sov3 = SizeOfVar(a->name3);
                //sov = std::max(sov1,std::max(sov2,sov3));
                sov = sov1;

                MemForVariable(a->name1, &addr1);

                // no sign extention !
                //   -1==0xff..ff
                // 0xff==0xff

                DWORD addrA = CopyArgToReg(false, Scratch0, sov, a->name2, false);
                DWORD addrB = CopyArgToReg(false, Scratch4, sov, a->name3, false);

                if((addr1 != addrB) && (addr1 == addrA)) {
                    DWORD a = addrA;
                    addrA = addrB;
                    addrB = a;
                }

                for(int i = 0; i < sov; i++) {
                    Instruction(OP_MOVF, addrA + i, DEST_W);
                    if(a->op == INT_SET_VARIABLE_AND)
                        Instruction(OP_ANDWF, addrB + i, addr1 == addrB ? DEST_F : DEST_W);
                    else if(a->op == INT_SET_VARIABLE_OR)
                        Instruction(OP_IORWF, addrB + i, addr1 == addrB ? DEST_F : DEST_W);
                    else if(a->op == INT_SET_VARIABLE_XOR)
                        Instruction(OP_XORWF, addrB + i, addr1 == addrB ? DEST_F : DEST_W);
                    else
                        THROW_COMPILER_EXCEPTION("Internal error.");
                    if(addr1 != addrB)
                        Instruction(OP_MOVWF, addr1 + i);
                }
                break;
            }
            case INT_SET_VARIABLE_NOT: {
                Comment("INT_SET_VARIABLE_NOT %s := ~%s", a->name1.c_str(), a->name2.c_str());
                CheckSovNames(a);
                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);

                DWORD addrA = CopyArgToReg(false, Scratch0, sov1, a->name2, false);
                MemForVariable(a->name1, &addr1);

                for(int i = 0; i < sov1; i++) {
                    if(addr1 == addrA) {
                        Instruction(OP_COMF, addr1 + i, DEST_F);
                    } else {
                        Instruction(OP_COMF, addrA + i, DEST_W);
                        Instruction(OP_MOVWF, addr1 + i);
                    }
                }
                break;
            }
            case INT_SET_VARIABLE_NEG: {
                Comment("INT_SET_VARIABLE_NEG %s := - %s", a->name1.c_str(), a->name2.c_str());
                CheckSovNames(a);
                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                MemForVariable(a->name1, &addr1);

                CopyArgToReg(true, Scratch0, sov1, a->name2, true);

                DWORD neg = AllocFwdAddr();
                for(int i = 0; i < sov1; i++)
                    Instruction(OP_COMF, Scratch0 + i, DEST_F);
                for(int i = 0; i < (sov1 - 1); i++) {
                    Instruction(OP_INCFSZ, Scratch0 + i, DEST_F);
                    Instruction(OP_GOTO, neg);
                }
                Instruction(OP_INCF, Scratch0 + sov1 - 1, DEST_F);
                FwdAddrIsNow(neg);
                CopyRegToReg(addr1, sov1, Scratch0, sov1, a->name1, "$Scratch0", false);
                break;
            }
                // The add and subtract routines must be written to return correct
                // results if the destination and one of the operands happen to
                // be the same registers (e.g. for A := A - C).

            case INT_SET_VARIABLE_ADD: {
                Comment("INT_SET_VARIABLE_ADD %s := %s + %s; '%s'; '%s'",
                        a->name1.c_str(),
                        a->name2.c_str(),
                        a->name3.c_str(),
                        a->name4.c_str(),
                        a->name5.c_str());
                // a->name1 = a->name2 + a->name3
                MemForVariable(a->name1, &addr1);
                // MemForVariable(a->name2, &addr2);
                // MemForVariable(a->name3, &addr3);
                //MemForSingleBit(a->name4, &addr4, &bit4); // Set Carry to stateInOut // Overflow

                sov1 = SizeOfVar(a->name1);
                // sov2 = SizeOfVar(a->name2);
                // sov3 = SizeOfVar(a->name3);

                // DWORD addrB = CopyArgToReg(true, Scratch0, sov, a->name2, true); // v1
                // isModificationRisk = addr1 != addr2;
                // DWORD addrB = CopyArgToReg(addr1 != addr2, Scratch0, sov1, a->name2, true); // v2
                DWORD addrB = CopyArgToDest(IsOutputReg(addr1) && (sov1 > 1), addr1, Scratch0, sov1, a->name2, true);
                DWORD addrA = CopyArgToReg(false, Scratch4, sov1, a->name3, true);
                add(addrB, addrA, sov1, a->name4, a->name5); // b = b + a , b - is rewritten
                CopyRegToReg(addr1, sov1, addrB, sov1, a->name1, "addrB", true);
                break;
            }

            case -INT_SET_VARIABLE_ADD:
                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                MemForVariable(a->name3, &addr3);

                VariableAdd(addr1, addr2, addr3, 2);
                break;

            case -INT_SET_VARIABLE_SUBTRACT:
                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                MemForVariable(a->name3, &addr3);

                Instruction(OP_MOVF, addr3, DEST_W, a->name3);
                Instruction(OP_SUBWF, addr2, DEST_W, a->name2);
                Instruction(OP_MOVWF, addr1, 0, a->name1);
                ClearBit(Scratch0, 0);
                IfBitSet(REG_STATUS, STATUS_C);
                SetBit(Scratch0, 0);

                Instruction(OP_MOVF, addr3 + 1, DEST_W);
                Instruction(OP_SUBWF, addr2 + 1, DEST_W);
                Instruction(OP_MOVWF, addr1 + 1);
                IfBitClear(Scratch0, 0); // bit is carry / (not borrow)
                Instruction(OP_DECF, addr1 + 1, DEST_F);
                break;

            case INT_SET_VARIABLE_SUBTRACT: {
                Comment("INT_SET_VARIABLE_SUBTRACT %s := %s - %s; '%s'; '%s'",
                        a->name1.c_str(),
                        a->name2.c_str(),
                        a->name3.c_str(),
                        a->name4.c_str(),
                        a->name5.c_str());
                // a->name1 = a->name2 - a->name3
                MemForVariable(a->name1, &addr1);

                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                sov3 = SizeOfVar(a->name3);
                sov = std::max(sov2, sov3);
                if(sov1 < sov) {
                    Error(" Size of result '%s' less then an argument(s) '%s' or '%s'",
                          a->name1.c_str(),
                          a->name2.c_str(),
                          a->name3.c_str());
                }

                // DWORD addrB = CopyArgToReg(true, Scratch0, sov, a->name2, true);  // v1
                // isModificationRisk = addr1 != addr2;
                // DWORD addrB = CopyArgToReg(addr1 != addr2, Scratch0, sov, a->name2, true); // v2
                DWORD addrB = CopyArgToDest(IsOutputReg(addr1) && (sov1 > 1), addr1, Scratch0, sov1, a->name2, true);
                DWORD addrA = CopyArgToReg(false, Scratch4, sov, a->name3, true);
                sub(addrB, addrA, sov, a->name4, a->name5); // b = b - a , b - is rewritten
                CopyRegToReg(addr1, sov1, addrB, sov, a->name1, "addrB", true);
                break;
            }
            case -INT_SET_VARIABLE_MULTIPLY:
                MultiplyNeeded = true;

                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                MemForVariable(a->name3, &addr3);

                Instruction(OP_MOVF, addr2, DEST_W, a->name2);
                Instruction(OP_MOVWF, Scratch0);
                Instruction(OP_MOVF, addr2 + 1, DEST_W);
                Instruction(OP_MOVWF, Scratch1);

                Instruction(OP_MOVF, addr3, DEST_W, a->name3);
                Instruction(OP_MOVWF, Scratch2);
                Instruction(OP_MOVF, addr3 + 1, DEST_W);
                Instruction(OP_MOVWF, Scratch3);

                CallWithPclath(MultiplyRoutineAddress);

                Instruction(OP_MOVF, Scratch2, DEST_W);
                Instruction(OP_MOVWF, addr1, 0, a->name1);
                Instruction(OP_MOVF, Scratch3, DEST_W);
                Instruction(OP_MOVWF, addr1 + 1);
                break;

            case INT_SET_VARIABLE_MULTIPLY: {
                Comment("INT_SET_VARIABLE_MULTIPLY %s := %s * %s",
                        a->name1.c_str(),
                        a->name2.c_str(),
                        a->name3.c_str());
                MemForVariable(a->name1, &addr1);

                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                sov3 = SizeOfVar(a->name3);
                if(sov2 < 2)
                    sov2 = 2;
                if(sov3 < 2)
                    sov3 = 2;
                int sov12 = std::max(sov1, sov2);

                CopyArgToReg(true, Scratch2, sov12, a->name2, true);
                CopyArgToReg(true, Scratch0, sov3, a->name3, true);

                if((sov12 == 3) && (sov3 <= 2)) {
                    MultiplyNeeded24x16 = true;
                    CallWithPclath(MultiplyRoutineAddress24x16);
                } else if((sov12 <= 2) && (sov3 <= 2)) {
                    MultiplyNeeded = true;
                    CallWithPclath(MultiplyRoutineAddress);
                } else
                     oops();

                CopyRegToReg(addr1, sov1, Scratch2, sov1, a->name1, "Scratch2", true);
                break;
            }
            case INT_SET_VARIABLE_MOD:
                Comment("INT_SET_VARIABLE_DIVIDE %s := %s / %s",
                        a->name1.c_str(),
                        a->name2.c_str(),
                        a->name3.c_str());
                goto div;
            case INT_SET_VARIABLE_DIVIDE:
                Comment("INT_SET_VARIABLE_DIVIDE %s := %s / %s",
                        a->name1.c_str(),
                        a->name2.c_str(),
                        a->name3.c_str());
                div :
                sov1 = SizeOfVar(a->name1);
                sov2 = SizeOfVar(a->name2);
                sov3 = SizeOfVar(a->name3);
                if(sov2 < 2)
                    sov2 = 2;
                if(sov3 < 2)
                    sov3 = 2;

                MemForVariable(a->name1, &addr1);
                MemForVariable(a->name2, &addr2);
                MemForVariable(a->name3, &addr3);

                CopyArgToReg(true, Scratch0, sov2, a->name2, true);
                CopyArgToReg(true, Scratch4, sov3, a->name3, true);

                if((sov2 == 3) && (sov3 <= 2)) {
                    DivideNeeded24x16 = true;
                    CallWithPclath(DivideRoutineAddress24x16);
                } else if((sov2 <= 2) && (sov3 <= 2)) {
                    DivideNeeded = true;
                    CallWithPclath(DivideRoutineAddress);
                } else
                    oops()

                if(a->op == INT_SET_VARIABLE_DIVIDE) {
                    CopyRegToReg(addr1, sov1, Scratch0, sov2, a->name1, "Scratch0", true);
                } else {
                    CopyRegToReg(addr1, sov1, Scratch6, sov3, a->name1, "Scratch6", true);
                }
                break;

            case INT_UART_SEND_READY: {
                Comment("INT_UART_SEND_READY");
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                CopyBit(addr1, bit1, REG_TXSTA, TRMT); // TRMT=1 is TSR empty, ready; TRMT=0 is TSR full
                break;
            }
            case INT_UART_SEND_BUSY: {
                Comment("INT_UART_SEND_BUSY");
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                CopyNotBit(addr1, bit1, REG_TXSTA, TRMT); // TRMT=1 is TSR empty, ready; TRMT=0 is TSR full
                break;
            }
            case INT_UART_SEND1: {
                // Attention! Busy flag is not checked!!!
                // Caller should check the busy flag!!!
                Comment("INT_UART_SEND1");
                MemForVariable(a->name1, &addr1);
                addr1 += a->literal;

                DWORD isBusy = PicProgWriteP;
                IfBitClear(REG_TXSTA, TRMT); // TRMT=0 if TSR full
                Instruction(OP_GOTO, isBusy);

                if(strlen(a->name2.c_str())) {
                    MemForVariable(a->name2, &addr2);
                    Instruction(OP_MOVLW, addr1); // Point to address
                    Instruction(OP_ADDWF, addr2, DEST_W);
                    Instruction(OP_MOVWF, REG_FSR);
                    Instruction(OP_MOVF, REG_INDF, DEST_W); // addr1 +=[a->name2]
                } else {
                    Instruction(OP_MOVF, addr1, DEST_W);
                }
                Instruction(OP_MOVWF, REG_TXREG);
                break;
            }
            case INT_UART_SEND: {
                Comment("INT_UART_SEND");
                MemForVariable(a->name1, &addr1);
                MemForSingleBit(a->name2, true, &addr2, &bit2); // stateInOut returns BUSY flag

                DWORD noSend = AllocFwdAddr();
                IfBitClear(addr2, bit2);
                Instruction(OP_GOTO, noSend);

                DWORD isBusy = AllocFwdAddr();

                IfBitClear(REG_TXSTA, TRMT);  // TRMT=0 if TSR full
                Instruction(OP_GOTO, isBusy); // output stay HI level

                Instruction(OP_MOVF, addr1, DEST_W);
                Instruction(OP_MOVWF, REG_TXREG);

                FwdAddrIsNow(noSend);
                CopyNotBit(
                    addr2, bit2, REG_TXSTA, TRMT); // return as busy // TRMT=1 if TSR empty, ready; TRMT=0 if TSR full
                FwdAddrIsNow(isBusy);
                break;
            }
            case INT_UART_RECV_AVAIL: {
                MemForSingleBit(a->name1, true, &addr1, &bit1);
                CopyBit(addr1, bit1, REG_PIR1, RCIF);
                break;
            }
            case INT_UART_RECV: {
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);
                MemForSingleBit(a->name2, true, &addr2, &bit2);

                ClearBit(addr2, bit2);

                // If RCIF is still clear, then there's nothing to do; in that
                // case jump to the end, and leave the rung-out clear.
                DWORD done = AllocFwdAddr();
                IfBitClear(REG_PIR1, RCIF);
                Instruction(OP_GOTO, done);

                // RCIF is set, so we have a character. Read it now.
                Instruction(OP_MOVF, REG_RCREG, DEST_W);
                Instruction(OP_MOVWF, addr1);
                for(int i = 1; i < sov1; i++)
                    Instruction(OP_CLRF, addr1 + i);
                // and set rung-out true
                SetBit(addr2, bit2);

                // And check for errors; need to reset the UART if yes.
                DWORD yesError = AllocFwdAddr();
                IfBitSet(REG_RCSTA, OERR); // overrun error
                Instruction(OP_GOTO, yesError);
                IfBitSet(REG_RCSTA, FERR); // framing error
                Instruction(OP_GOTO, yesError);

                // Neither FERR nor OERR is set, so we're good.
                Instruction(OP_GOTO, done);

                FwdAddrIsNow(yesError);
                // An error did occur, so flush the FIFO.
                Instruction(OP_MOVF, REG_RCREG, DEST_W);
                Instruction(OP_MOVF, REG_RCREG, DEST_W);
                // And clear and then set CREN, to clear the error flags.
                ClearBit(REG_RCSTA, CREN);
                SetBit(REG_RCSTA, CREN);

                FwdAddrIsNow(done);
                break;
            }
            case INT_UART_RECV1: {
                //Receive one char/byte in a single PLC cycle.
                //Skip if no char.
                Comment("INT_UART_RECV1");
                MemForVariable(a->name1, &addr1);
                addr1 += a->literal;
//              sov1 = SizeOfVar(a->name1);
//              MemForSingleBit(a->name2, true, &addr2, &bit2);

//              ClearBit(addr2, bit2);

                // If RCIF is still clear, then there's nothing to do; in that
                // case jump to the end, and leave the rung-out clear.
                DWORD done = AllocFwdAddr();
                IfBitClear(REG_PIR1, RCIF);
                Instruction(OP_GOTO, done);

                if(strlen(a->name2.c_str())) {
                    MemForVariable(a->name2, &addr2);
                    Instruction(OP_MOVLW, addr1); // Point to address
                    Instruction(OP_ADDWF, addr2, DEST_W);
                    Instruction(OP_MOVWF, REG_FSR);

                    // RCIF is set, so we have a character. Read it now.
                    Instruction(OP_MOVF, REG_RCREG, DEST_W);
                    Instruction(OP_MOVWF, REG_INDF); // addr1 +=[a->name2]
                } else {
                    // RCIF is set, so we have a character. Read it now.
                    Instruction(OP_MOVF, REG_RCREG, DEST_W);
                    Instruction(OP_MOVWF, addr1);
                }

                // And check for errors; need to reset the UART if yes.
                DWORD yesError = AllocFwdAddr();
                IfBitSet(REG_RCSTA, OERR); // overrun error
                Instruction(OP_GOTO, yesError);
                IfBitSet(REG_RCSTA, FERR); // framing error
                Instruction(OP_GOTO, yesError);

                // Neither FERR nor OERR is set, so we're good.
                Instruction(OP_GOTO, done);

                FwdAddrIsNow(yesError);
                // An error did occur, so flush the FIFO.
                Instruction(OP_MOVF, REG_RCREG, DEST_W);
                Instruction(OP_MOVF, REG_RCREG, DEST_W);
                // And clear and then set CREN, to clear the error flags.
                ClearBit(REG_RCSTA, CREN);
                SetBit(REG_RCSTA, CREN);

                FwdAddrIsNow(done);
                break;
            }
            case INT_SPI: {
                break;
            }
            case INT_PWM_OFF: {
                McuPwmPinInfo *ioPWM;
                ioPWM = PwmPinInfoForName(a->name1.c_str(), Prog.cycleTimer);
                if(!ioPWM) {
                    THROW_COMPILER_EXCEPTION_FMT(_("Pin '%s': PWM output not available!"), a->name1.c_str());
                }
                int timer = ioPWM->timer;
                if(timer == 1)
                    WriteRegister(REG_CCP1CON, 0);
                else if(timer == 2)
                    WriteRegister(REG_CCP2CON, 0);
                else
                    THROW_COMPILER_EXCEPTION("Internal error.");

                DWORD addr;
                int   bit;
                MemForSingleBit(a->name1, false, &addr, &bit);
                ClearBit(addr, bit, a->name1);

                char storeName[MAX_NAME_LEN];
                sprintf(storeName, "$pwm_init_%s", a->name1.c_str());
                MemForSingleBit(storeName, false, &addr, &bit);
                ClearBit(addr, bit, storeName);
                break;
            }
            case INT_SET_PWM: {
                // Op(INT_SET_PWM, l->d.setPwm.duty_cycle, l->d.setPwm.targetFreq, l->d.setPwm.name, l->d.setPwm.resolution);
                Comment(
                    "INT_SET_PWM %s %s %s %s", a->name1.c_str(), a->name2.c_str(), a->name3.c_str(), a->name4.c_str());
                int resol, TOP;
                getResolution(a->name4.c_str(), &resol, &TOP);
                McuPwmPinInfo *ioPWM;
                ioPWM = PwmPinInfoForName(a->name3.c_str(), Prog.cycleTimer);
                if(!ioPWM) {
                    THROW_COMPILER_EXCEPTION_FMT(_("Pin '%s': PWM output not available!"), a->name3.c_str());
                }

                int timer = ioPWM->timer;
                int target = hobatoi(a->name2.c_str());

                // Timer2
                // So the PWM frequency is given by
                //    target = xtal/(4*prescale*pr2)
                //    xtal/target = 4*prescale*pr2
                // and pr2 should be made as large as possible to keep
                // resolution, so prescale should be as small as possible

                // Software programmable prescaler (1:1, 1:4, 1:16)
                // PWM Period = (PR2 + 1) * 4 * Tosc * (TMR2 Prescale Value)
                // PWM Period = 1 / targetFreq
                // Tosc = 1 / Fosc
                // Fosc = Prog.mcuClock
                // PR2 = 0..255 available
                // Duty Cycle Ratio = CCPR2L:CCP2CON<5:4> / (4 * (PR2 + 1))

                // PWM freq = Fosc / (PR2 + 1) * 4 * (TMR2 Prescale Value)
                // PR2 = Fosc / (4 * (TMR2 Prescale Value) * targetFreq) - 1

                // Timer1
                // Software programmable prescaler (1:1, 1:2, 1:4, 1:8) not used.

                char   str0[1024];
                char   str1[1024];
                char   str3[1024];
                char   minSI[5];
                char   maxSI[5];
                double minFreq;
                double maxFreq;

                double McuClock;
                double minMcuClock;
                double maxMcuClock;
                char   McuClockSI[5];
                char   minMcuClockSI[5];
                char   maxMcuClockSI[5];
                McuClock = SIprefix(Prog.mcuClock, McuClockSI);
                sprintf(str0, _("When the MCU crystal frequency is %.3g %sHz."), McuClock, McuClockSI);

                if((timer == 2) || (timer == 1)) {
                    minFreq = SIprefix(Prog.mcuClock / ((255 + 1) * 4 * 16), minSI);
                    maxFreq = SIprefix(Prog.mcuClock / ((3 + 1) * 4 * 1), maxSI);
                    minMcuClock = SIprefix(target * ((3 + 1) * 4 * 1), minMcuClockSI);
                    maxMcuClock = SIprefix(target * ((255 + 1) * 4 * 16), maxMcuClockSI);
                    /*
                } else if(timer == 1) {
                    minFreq = SIprefix(Prog.mcuClock / ((255+1)*4*8), minSI);
                    maxFreq = SIprefix(Prog.mcuClock / ((  3+1)*4*1), maxSI);
                    minMcuClock = SIprefix(target * ((  3+1)*4*1), minMcuClockSI);
                    maxMcuClock = SIprefix(target * ((255+1)*4*8), maxMcuClockSI);
                */
                } else
                    THROW_COMPILER_EXCEPTION("Internal error.");
                sprintf(
                    str1, _("Available PWM frequency from %.3f %sHz up to %.3f %sHz"), minFreq, minSI, maxFreq, maxSI);
                sprintf(str3,
                        _("Required MCU crystal frequency from %.3g %sHz up to %.3g %sHz"),
                        minMcuClock,
                        minMcuClockSI,
                        maxMcuClock,
                        maxMcuClockSI);

                int pr2plus1;
                int prescale;
                for(prescale = 1;;) {
                    int dv = 4 * prescale * target;
                    pr2plus1 = (Prog.mcuClock + (dv / 2)) / dv;
                    //pr2plus1 = Prog.mcuClock / (4 * prescale * target);
                    if(pr2plus1 < 3) {
                        THROW_COMPILER_EXCEPTION_FMT("'%s' %s\n\n%s", a->name3.c_str(), _("PWM frequency too fast."), str1);
                    }
                    if(pr2plus1 > 256) {
                        if((timer == 2) || (timer == 1)) {
                            if(prescale == 1) {
                                prescale = 4;
                            } else if(prescale == 4) {
                                prescale = 16;
                            } else {
                                THROW_COMPILER_EXCEPTION_FMT("SET '%s': %s %s\n\n%s\n\n\t\tOR\n\n%s",
                                                             a->name3.c_str(),
                                                             _("PWM frequency too slow."),
                                                             str0,
                                                             str1,
                                                             str3);
                            }
                        } else
                            THROW_COMPILER_EXCEPTION("Internal error.");
                    } else {
                        break;
                    }
                }
                double targetFreq = 1.0 * Prog.mcuClock / (pr2plus1 * 4 * prescale);
                /*
                dbps(a->name1.c_str())
                dbpd(timer)
                dbpd(target)
                dbpd(pr2plus1)
                dbpd(prescale)
                dbpf(targetFreq)
                dbp("%f %s", minFreq, minSI);
                dbp("%f %s\n\n", maxFreq, maxSI);

/**/
                // Copy l->d.setPwm.duty_cycle into Scratch0:1
                if(IsNumber(a->name1)) {
                    CopyLitToReg(Scratch0, 2, "Scratch0:1", hobatoi(a->name1.c_str()), a->name1);
                } else {
                    MemForVariable(a->name1, &addr1);
                    CopyRegToReg(Scratch0, 2, addr1, 2, "Scratch0:1", a->name1, false);
                }

                DWORD REG_CCPRxL = -1;
                DWORD REG_CCPxCON = -1;

                if((timer == 1) || McuAs(" PIC16F72 ")) {
                    REG_CCPRxL = REG_CCPR1L;
                    REG_CCPxCON = REG_CCP1CON;
                } else if(timer == 2) {
                    REG_CCPRxL = REG_CCPR2L;
                    REG_CCPxCON = REG_CCP2CON;
                } else
                    THROW_COMPILER_EXCEPTION("Internal error.");

                if(resol == 8) {
                    // First scale the input variable from percent to timer units,
                    // with a multiply and then a divide.
                    MultiplyNeeded = true;
                    CopyLitToReg(Scratch2, 2, "", pr2plus1, "pr2plus1");
                    CallWithPclath(MultiplyRoutineAddress);

                    Instruction(OP_MOVF, Scratch3, DEST_W); //  divide by 256
                    Instruction(OP_MOVWF, REG_CCPRxL);

                    Instruction(OP_RRF, Scratch2, DEST_F);   //LSbs
                    Instruction(OP_RRF, Scratch2, DEST_F);   //LSbs
                    Instruction(OP_MOVLW, 0x30);             //LSbs
                    Instruction(OP_ANDWF, Scratch2, DEST_W); //LSbs
                    Instruction(OP_IORLW, 0x0c);
                    Instruction(OP_MOVWF, REG_CCPxCON); // PWM mode, use LSbs
                } else if(resol == 9) {
                    // First scale the input variable from percent to timer units,
                    // with a multiply and then a divide.
                    MultiplyNeeded24x16 = true;
                    CopyLitToReg(Scratch2, 3, "", pr2plus1, "pr2plus1");
                    CallWithPclath(MultiplyRoutineAddress24x16);

                    sr0(Scratch2, 3);                       //  divide by 2
                    Instruction(OP_MOVF, Scratch3, DEST_W); //  divide by 256
                    Instruction(OP_MOVWF, REG_CCPRxL);

                    Instruction(OP_RRF, Scratch2, DEST_F);   //LSbs
                    Instruction(OP_RRF, Scratch2, DEST_F);   //LSbs
                    Instruction(OP_MOVLW, 0x30);             //LSbs
                    Instruction(OP_ANDWF, Scratch2, DEST_W); //LSbs
                    Instruction(OP_IORLW, 0x0c);
                    Instruction(OP_MOVWF, REG_CCPxCON); // PWM mode, use LSbs
                } else if(resol == 10) {
                    // First scale the input variable from percent to timer units,
                    // with a multiply and then a divide.
                    MultiplyNeeded24x16 = true;
                    CopyLitToReg(Scratch2, 3, "", pr2plus1, "pr2plus1");
                    CallWithPclath(MultiplyRoutineAddress24x16);

                    sr0(Scratch2, 3);                       // divide by 2
                    sr0(Scratch2, 3);                       // divide by 2
                    Instruction(OP_MOVF, Scratch3, DEST_W); // divide by 256
                    Instruction(OP_MOVWF, REG_CCPRxL);

                    Instruction(OP_RRF, Scratch2, DEST_F);   //LSbs
                    Instruction(OP_RRF, Scratch2, DEST_F);   //LSbs
                    Instruction(OP_MOVLW, 0x30);             //LSbs
                    Instruction(OP_ANDWF, Scratch2, DEST_W); //LSbs
                    Instruction(OP_IORLW, 0x0c);
                    Instruction(OP_MOVWF, REG_CCPxCON); // PWM mode, use LSbs
                } else if(resol == 7) {
                    // First scale the input variable from percent to timer units,
                    // with a multiply and then a divide.
                    MultiplyNeeded = true;
                    DivideNeeded = true;
#if 0
                    Instruction(OP_MOVLW, pr2plus1 & 0xff);
                    Instruction(OP_MOVWF, Scratch2);
                    if(pr2plus1 >> 8) {
                        Instruction(OP_MOVLW, pr2plus1 >> 8);
                        Instruction(OP_MOVWF, Scratch3);
                    } else
                        Instruction(OP_CLRF, Scratch3);
#else
                    CopyLitToReg(Scratch2, 2, "", pr2plus1, "pr2plus1");
#endif
                    CallWithPclath(MultiplyRoutineAddress);

#if 0
                    Instruction(OP_MOVF, Scratch3, DEST_W);
                    Instruction(OP_MOVWF, Scratch1);
                    Instruction(OP_MOVF, Scratch2, DEST_W);
                    Instruction(OP_MOVWF, Scratch0);
#else
                    CopyRegToReg(Scratch0, 2, Scratch2, 2, "Scratch0:1", "Scratch2:3", false);
#endif

#if 0
                    Instruction(OP_MOVLW, 100);
                    Instruction(OP_MOVWF, Scratch4);
                    Instruction(OP_CLRF, Scratch5);
#else
                    CopyLitToReg(Scratch4, 2, "", 100, "100");
#endif
                    CallWithPclath(DivideRoutineAddress);

                    Instruction(OP_MOVF, Scratch0, DEST_W);
                    Instruction(OP_MOVWF, REG_CCPRxL);
                    WriteRegister(REG_CCPxCON, 0x0c); // PWM mode, ignore LSbs
                } else
                    THROW_COMPILER_EXCEPTION("Internal error.");

                // Only need to do the setup stuff once
                Comment("PWM init");
                DWORD addr;
                int   bit;
                char  storeName[MAX_NAME_LEN];
                sprintf(storeName, "$pwm_init_%s", a->name3.c_str());
                MemForSingleBit(storeName, false, &addr, &bit);

                DWORD skip = AllocFwdAddr();
                IfBitSet(addr, bit);
                Instruction(OP_GOTO, skip);
                SetBit(addr, bit);

                // Set up the CCP2 and TMR2 peripherals.
                WriteRegister(REG_PR2, pr2plus1 - 1); // - 1 is Ok
                                                      ////            WriteRegister(REG_PR2, 3);
                                                      // 1-3-3-50%  62.5kHz
                                                      // 1-2-0-     83kHz
                                                      // 1-1-0-50%-122kHz
                                                      // 0-0-2-50%-250kHz

                //TODO: if(Prog.mcu->core == EnhancedMidrangeCore14bit)
                if(McuAs(" PIC16F72 ")) {
                    BYTE t2con = _BV(TMR2ON); // timer 2 on
                    if(prescale == 1)
                        t2con |= 0;
                    else if(prescale == 4)
                        t2con |= 1;
                    else if(prescale == 16)
                        t2con |= 2;
                    else
                        THROW_COMPILER_EXCEPTION("Internal error.");

                    WriteRegister(REG_T2CON, t2con);
                } else if(timer == 1) {
                    BYTE t2con = _BV(TMR2ON); // timer 2 on
                    if(prescale == 1)
                        t2con |= 0;
                    else if(prescale == 4)
                        t2con |= 1;
                    else if(prescale == 16)
                        t2con |= 2;
                    else
                        THROW_COMPILER_EXCEPTION("Internal error.");

                    WriteRegister(REG_T2CON, t2con);
                } else if(timer == 2) {
                    BYTE t2con = _BV(TMR2ON); // timer 2 on
                    if(prescale == 1)
                        t2con |= 0;
                    else if(prescale == 4)
                        t2con |= 1;
                    else if(prescale == 16)
                        t2con |= 2;
                    else
                        THROW_COMPILER_EXCEPTION("Internal error.");

                    WriteRegister(REG_T2CON, t2con);
                } else
                    THROW_COMPILER_EXCEPTION("Internal error.");

                FwdAddrIsNow(skip);
                break;
            }

            case INT_EEPROM_BUSY_CHECK: {
                Comment("INT_EEPROM_BUSY_CHECK");
                DWORD isBusy = AllocFwdAddr();
                DWORD done = AllocFwdAddr();
                MemForSingleBit(a->name1, false, &addr1, &bit1);

                IfBitSet(REG_EECON1, 1);
                Instruction(OP_GOTO, isBusy);

                //IfBitClear(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
                Instruction(OP_MOVF, EepromHighBytesCounter, DEST_W);
                skpnz Instruction(OP_GOTO, done);

                Instruction(OP_DECF, EepromHighBytesCounter, DEST_F);

                // So there is not a write pending, but we have another
                // character to transmit queued up.

                Instruction(OP_INCF, REG_EEADR, DEST_F);
                //Instruction(OP_MOVF, EepromHighByte, DEST_W);
                Instruction(OP_MOVLW, EepromHighByte); //Point to address
                Instruction(OP_ADDWF, EepromHighBytesCounter, DEST_W);
                Instruction(OP_MOVWF, REG_FSR);
                Instruction(OP_MOVF, REG_INDF, DEST_W);

                Instruction(OP_MOVWF, REG_EEDATA);
                Instruction(OP_BCF, REG_EECON1, 7);
                Instruction(OP_BSF, REG_EECON1, 2);
                Instruction(OP_MOVLW, 0x55);
                Instruction(OP_MOVWF, REG_EECON2);
                Instruction(OP_MOVLW, 0xaa);
                Instruction(OP_MOVWF, REG_EECON2);
                Instruction(OP_BSF, REG_EECON1, 1);
                Instruction(OP_BCF, REG_EECON1, 2);

                //ClearBit(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);

                FwdAddrIsNow(isBusy);
                SetBit(addr1, bit1);

                FwdAddrIsNow(done);
                break;
            }
            case INT_EEPROM_WRITE: {
                Comment("INT_EEPROM_WRITE");
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);

                //SetBit(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
                WriteRegister(EepromHighBytesCounter, sov1 - 1);
                if(sov1 > 1) {
                    Instruction(OP_MOVF, addr1 + 1, DEST_W);
                    Instruction(OP_MOVWF, EepromHighByte);
                    if(sov1 > 2) {
                        Instruction(OP_MOVF, addr1 + 2, DEST_W);
                        Instruction(OP_MOVWF, EepromHighByte + 1);
                        if(sov1 > 3) {
                            Instruction(OP_MOVF, addr1 + 3, DEST_W);
                            Instruction(OP_MOVWF, EepromHighByte + 2);
                        }
                    }
                }
                Instruction(OP_MOVLW, a->literal);
                Instruction(OP_MOVWF, REG_EEADR);
                Instruction(OP_MOVF, addr1, DEST_W);

                Instruction(OP_MOVWF, REG_EEDATA);
                Instruction(OP_BCF, REG_EECON1, 7);
                Instruction(OP_BSF, REG_EECON1, 2);
                Instruction(OP_MOVLW, 0x55);
                Instruction(OP_MOVWF, REG_EECON2);
                Instruction(OP_MOVLW, 0xaa);
                Instruction(OP_MOVWF, REG_EECON2);
                Instruction(OP_BSF, REG_EECON1, 1);
                Instruction(OP_BCF, REG_EECON1, 2);
                break;
            }
            case INT_EEPROM_READ: {
                Comment("INT_EEPROM_READ");
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);
                for(int i = 0; i < sov1; i++) {
                    Instruction(OP_MOVLW, a->literal + i);
                    Instruction(OP_MOVWF, REG_EEADR);
                    Instruction(OP_BCF, REG_EECON1, 7);
                    Instruction(OP_BSF, REG_EECON1, 0);
                    Instruction(OP_MOVF, REG_EEDATA, DEST_W);
                    Instruction(OP_MOVWF, addr1 + i);
                }
                break;
            }
            case INT_SET_VARIABLE_RANDOM: {
                MemForVariable(a->name1, &addr1);
                sov1 = SizeOfVar(a->name1);

                char seedName[MAX_NAME_LEN];
                sprintf(seedName, "$seed_%s", a->name1.c_str());
                /*
//https://en.m.wikipedia.org/wiki/Linear_congruential_generator
// X[n+1] = (a * X[n] + c) mod m
//VMS's MTH$RANDOM, old versions of glibc
// a = 69069 ( 0x10DCD ) (1 00001101 11001101b)
//     bits              16        8 7      0
// (Multipliers: 3 * 7 * 11 * 13 * 23) (Dividers: 1, 3, 7, 11, 13, 21, 23, 33, 39, 69, 77, 91, 143, 161, 231, 253, 273, 299, 429, 483, 759, 897, 1001, 1771, 2093, 3003, 3289, 5313, 6279, 9867, 23023, 69069)
// c = 1
// m = 2^32
// X = (X * 0x10DCD + 1) % 0x100000000
*/
                SetSizeOfVar(seedName, 4);
                MemForVariable(seedName, &addr2);
                CopyRegToReg(Scratch1, 4, addr2, 4, "", "", false);

                //add(addr2, Scratch1, 4);     // * bit0 already in result
                add(addr2 + 1, Scratch1, 3); // * bit8
                add(addr2 + 2, Scratch1, 2); // * bit16

                shl(Scratch1, 4);        //1
                shl(Scratch1, 4);        //2
                add(addr2, Scratch1, 4); // * bit2
                shl(Scratch1, 4);        //3
                add(addr2, Scratch1, 4); // * bit3
                shl(Scratch1, 4);        //4
                shl(Scratch1, 4);        //5
                shl(Scratch1, 4);        //6
                add(addr2, Scratch1, 4); // * bit6
                shl(Scratch1, 4);        //7
                add(addr2, Scratch1, 4); // * bit7
                shl(Scratch1, 4);        //8
                                         //add(addr2, Scratch1, 4);   // * bit8
                shl(Scratch1, 4);        //9
                shl(Scratch1, 4);        //10
                add(addr2, Scratch1, 4); // * bit10
                shl(Scratch1, 4);        //11
                add(addr2, Scratch1, 4); // * bit11
                                         //shl(Scratch1, 4); //12
                                         //shl(Scratch1, 4); //13
                                         //shl(Scratch1, 4); //14
                                         //shl(Scratch1, 4); //15
                                         //shl(Scratch1, 4); //16
                                         //add(addr2, Scratch1, 4);   // * bit16

                Increment(addr2, 4, seedName);
                CopyRegToReg(addr1, sov1, addr2 + 4 - sov1, 4, "", "", false);
                break;
            }
            case INT_READ_ADC: {
                Comment("INT_READ_ADC");
                BYTE adcs;

                MemForVariable(a->name1, &addr1);
                BYTE refs = a->literal & 0xF;
                //
                int goPos, chsPos;
                if(McuAs("Microchip PIC16F887 ") || //
                   McuAs("Microchip PIC16F886 ") || //
                   McuAs(" PIC16F882 ")          || //
                   McuAs(" PIC16F883 ")          || //
                   McuAs(" PIC16F884 ")          || //
                   McuAs(" PIC16F1512 ")         || //
                   McuAs(" PIC16F1513 ")         || //
                   McuAs(" PIC16F1516 ")         || //
                   McuAs(" PIC16F1517 ")         || //
                   McuAs(" PIC16F1518 ")         || //
                   McuAs(" PIC16F1519 ")         || //
                   McuAs(" PIC16F1526 ")         || //
                   McuAs(" PIC16F1527 ")         || //
                   McuAs(" PIC16F1933 ")         || //
                   McuAs(" PIC16F1947 ")         || //
                   McuAs(" PIC12F675 ")          || //
                   McuAs(" PIC12F683 ")          || //
                   McuAs(" PIC12F752 ")          || //
                   McuAs(" PIC16F1824 ")         || //
                   McuAs(" PIC16F1827 ")         //
                ) {
                    goPos = 1;
                    chsPos = 2;
                } else if(McuAs(" PIC16F819 ") || //
                          McuAs(" PIC16F873 ") || //
                          McuAs(" PIC16F874 ") || //
                          McuAs(" PIC16F876 ") || //
                          McuAs(" PIC16F877 ") || //
                          McuAs(" PIC16F88 ")  || //
                          McuAs(" PIC16F72 ")  //
                ) {
                    goPos = 2;
                    chsPos = 3;
                } else
                    THROW_COMPILER_EXCEPTION("Internal error.");

                if((Prog.mcu->core == EnhancedMidrangeCore14bit) || //
                     McuAs(" PIC12F683 ") || //
                     McuAs(" PIC12F675 ") || //
                     McuAs(" PIC12F752 ") //
                ) {
                    if(Prog.mcuClock > 8000000) {        // 20 MHz
                        adcs = 6;                        // Fosc/64
                    } else if(Prog.mcuClock > 4000000) { // 8 MHz
                        adcs = 2;                        // Fosc/32
                    } else if(Prog.mcuClock > 1000000) { // 4 MHz
                        adcs = 5;                        // Fosc/16
                    } else if(Prog.mcuClock >  500000) { // 1 MHz
                        adcs = 4;                        // Fosc/4
                    } else {                             // 0.5 MHz
                        adcs = 0;                        // Fosc/2
                    }
                } else if(McuAs(" PIC16F88 ") //
                ) {
                    if(Prog.mcuClock > 10000000) {       // 20 MHz
                        adcs = 6;                        // Fosc/64
                    } else if(Prog.mcuClock > 5000000) { // 10 MHz
                        adcs = 5;                        // Fosc/16
                    } else if(Prog.mcuClock > 2500000) { // 5 MHz
                        adcs = 1;                        // Fosc/8
                    } else if(Prog.mcuClock > 1250000) { // 2.5 MHz
                        adcs = 4;                        // Fosc/4
                    } else {                             // 1.25 MHz
                        adcs = 0;                        // Fosc/2
                    }
                } else {
                    if(Prog.mcuClock > 5000000) {        // 20 MHz
                        adcs = 2;                        // 32*Tosc
                    } else if(Prog.mcuClock > 1250000) { // 5 MHz
                        adcs = 1;                        // 8*Tosc
                    } else {                             // 1.25 MHz
                        adcs = 0;                        // 2*Tosc
                    }
                }

                int adcsPos;
                if(McuAs(" PIC16F1512 ") || //
                   McuAs(" PIC16F1513 ") || //
                   McuAs(" PIC16F1516 ") || //
                   McuAs(" PIC16F1517 ") || //
                   McuAs(" PIC16F1518 ") || //
                   McuAs(" PIC16F1519 ") || //
                   McuAs(" PIC16F1526 ") || //
                   McuAs(" PIC16F1527 ") || //
                   McuAs(" PIC16F1933 ") || //
                   McuAs(" PIC16F1947 ") || //
                   McuAs(" PIC16F1824 ") || //
                   McuAs(" PIC16F1827 ") //
                ) {
                    adcsPos = 4;                                            // in REG_ADCON1
                    WriteRegister(REG_ADCON0,                               //
                                  (MuxForAdcVariable(a->name1) << chsPos) | //
                                      (0 << goPos) |                        // don't start yet
                                                                            // bit 1 unimplemented
                                      (1 << 0)                              // A/D peripheral on
                    );

                    WriteRegister(REG_ADCON1,             //
                                  (1 << 7) |              // right-justified
                                  (adcs << adcsPos) | //
                                  ((refs & 3) << 0)            // 00 = VREF is connected to VDD
                    );
                } else if(McuAs(" PIC16F819 ") || //
                          McuAs(" PIC16F873 ") || //
                          McuAs(" PIC16F874 ") || //
                          McuAs(" PIC16F876 ") || //
                          McuAs(" PIC16F877 ") || //
                          McuAs(" PIC16F72 ")     //
                ) {
                    adcsPos = 6;                                                // in REG_ADCON0
                    WriteRegister(REG_ADCON0,                                   //
                                  (adcs << adcsPos) |                           //
                                  (MuxForAdcVariable(a->name1) << chsPos) | //
                                  (0 << goPos) |                            // don't start yet
                                                                            // bit 1 unimplemented
                                  (1 << 0)                                  // A/D peripheral on
                    );

                    WriteRegister(REG_ADCON1,  //
                                  (1 << 7) |   // right-justified
                                  //(0 << 0)   // for now, all analog inputs
                                  (refs << 0)  //

                    );
                } else if(McuAs(" PIC16F882 ") || //
                          McuAs(" PIC16F883 ") || //
                          McuAs(" PIC16F884 ") || //
                          McuAs(" PIC16F886 ") || //
                          McuAs(" PIC16F887 ") //
                ) {
                    adcsPos = 6;                                                // in REG_ADCON0
                    WriteRegister(REG_ADCON0,                                   //
                                  (adcs << adcsPos) |                           //
                                  (MuxForAdcVariable(a->name1) << chsPos) | //
                                  (0 << goPos) |                            // don't start yet
                                                                            // bit 1 unimplemented
                                  (1 << 0)                                  // A/D peripheral on
                    );

                    WriteRegister(REG_ADCON1,  //
                                  (1 << 7) |   // right-justified
                                  //(0 << 0)   // for now, all analog inputs
                                  ((refs & 3) << 4)  //

                    );
                } else if(McuAs(" PIC16F88 ") //
                ) {
                    adcsPos = 6;                                                // in REG_ADCON0
                    WriteRegister(REG_ADCON0,                                   //
                                  (adcs << adcsPos) |                           //
                                  (MuxForAdcVariable(a->name1) << chsPos) | //
                                  (0 << goPos) |                            // don't start yet
                                                                            // bit 1 unimplemented
                                  (1 << 0)                                  // A/D peripheral on
                    );

                    WriteRegister(REG_ADCON1,  //
                                  (1 << 7) |   // right-justified
                                  ((adcs & _BV(BIT2)) << 4) | // ADCS2 is BIT6
                                  ((refs & 3) << 4)  //

                    );
                } else if(McuAs(" PIC12F683 ") || //
                          McuAs(" PIC12F675 ") || //
                          McuAs(" PIC12F752 ") //
                ) {
                    adcsPos = 4; // in REG_ANSEL
                    WriteRegister(REG_ANSEL, (adcs << adcsPos) | (1 << MuxForAdcVariable(a->name1)));

                    WriteRegister(REG_ADCON0,                               //
                                  (1 << 7) |                                // right-justified
                                  ((refs & 1) << 6) |                     // VDD Voltage Reference
                                  (MuxForAdcVariable(a->name1) << chsPos) | // Analog Channel Select bits
                                  (0 << goPos) |                            // don't start yet
                                                                            // bit 1 unimplemented
                                  (1 << 0)                                  // A/D peripheral on
                    );
                } else
                    THROW_COMPILER_EXCEPTION("Internal error.");

                if(McuAs("Microchip PIC16F88 ")) {
                    WriteRegister(REG_ANSEL, 0x7f);
                } else if(McuAs("Microchip PIC16F887 ") || //
                          McuAs("Microchip PIC16F886 ")) {
                    WriteRegister(REG_ANSEL, 0xff);
                    WriteRegister(REG_ANSELH, 0x3f);
                }

                // need to wait Tacq (about 20 us) for mux, S/H etc. to settle
                int cyclesToWait = ((Prog.mcuClock / 4) * 20) / 1000000;
                cyclesToWait /= 3; // division by 3 = (DEC + GOTO) = 3 cycles time
                if(cyclesToWait < 1)
                    cyclesToWait = 1;

                Instruction(OP_MOVLW, cyclesToWait);
                Instruction(OP_MOVWF, Scratch1);
                DWORD wait = PicProgWriteP;
                Instruction(OP_DECFSZ, Scratch1, DEST_F);
                Instruction(OP_GOTO, wait);

                SetBit(REG_ADCON0, goPos); // starting a A/D conversion
                DWORD spin = PicProgWriteP;
                IfBitSet(REG_ADCON0, goPos);
                Instruction(OP_GOTO, spin);

                if(REG_ADRESH != -1) {
                    Instruction(OP_MOVF, REG_ADRESH, DEST_W);
                    Instruction(OP_MOVWF, addr1 + 1);
                }

                Instruction(OP_MOVF, REG_ADRESL, DEST_W);
                Instruction(OP_MOVWF, addr1);

                // hook those pins back up to the digital inputs in case
                // some of them are used that way
                if(REG_ADCON1 != -1)
                    WriteRegister(REG_ADCON1, //
                                  (1 << 7) |  // right-justify A/D result
                                  // (6 << 0)    // all digital inputs
                                  (refs << 4)  //
                    );

                if(McuAs("Microchip PIC16F88 ") // || //
                    // McuAs(" PIC12F675 ") || //
                    // McuAs(" PIC12F683 ") //
                ) {
                    Instruction(OP_CLRF, REG_ANSEL);
                } else if(McuAs("Microchip PIC16F887 ") || //
                          McuAs("Microchip PIC16F886 ") //
                ) {
                    Instruction(OP_CLRF, REG_ANSEL);
                    Instruction(OP_CLRF, REG_ANSELH);
                }
                break;
            }
            case INT_END_IF:
            case INT_ELSE:
                return;

            case INT_WRITE_STRING:
                THROW_COMPILER_EXCEPTION(_("Unsupported operation for target, skipped."));
            case INT_SIMULATE_NODE_STATE:
                break;

            case INT_COMMENT:
                Comment("%s", a->name1.c_str());
                break;

            case INT_AllocKnownAddr: {
                LabelAddr * l = GetLabelAddr(a->name1.c_str());
                l->KnownAddr = PicProgWriteP;
                break;
            }
            case INT_AllocFwdAddr: {
                LabelAddr * l = GetLabelAddr(a->name1.c_str());
                l->FwdAddr = AllocFwdAddr();
                break;
            }
            case INT_FwdAddrIsNow: {
                LabelAddr * l = GetLabelAddr(a->name1.c_str());
                FwdAddrIsNow(l->FwdAddr);
                break;
            }
            case INT_RETURN:
                Instruction(OP_RETURN);
                break;

            case INT_GOTO: {
                Comment("INT_GOTO %s // %s %d",
                        a->name1.c_str(),
                        a->name2.c_str(),
                        a->literal);
                LabelAddr * l = GetLabelAddr(a->name1.c_str());
                if(a->literal) {
                    Instruction(OP_GOTO, l->KnownAddr);
                } else {
                    Instruction(OP_GOTO, l->FwdAddr);
                }
                break;
            }
            case INT_GOSUB: {
                Comment("INT_GOSUB %s // %s %d",
                        a->name1.c_str(),
                        a->name2.c_str(),
                        a->literal);
                LabelAddr * l = GetLabelAddr(a->name1.c_str());
                if(a->literal) {
                    Instruction(OP_CALL, l->KnownAddr);
                } else {
                    Instruction(OP_CALL, l->FwdAddr);
                }
                break;
            }
            case INT_SET_BIN2BCD: {
                Comment("INT_SET_BIN2BCD: %s = Bin2Bcd(%s)", a->name1.c_str(), a->name2.c_str());
                CallBin32BcdRoutine(a->name1, a->name2);
                break;
            }
#ifdef TABLE_IN_FLASH
            case INT_FLASH_INIT: {
                // InitTable(a); // Inited by InitTables()
                break;
            }
            case INT_FLASH_READ: {
                sov1 = SizeOfVar(a->name1);
                MemForVariable(a->name1, &addr1); // dest

                Comment("Scratch0:1 := Index '%s'", a->name3.c_str());
                if(IsNumber(a->name3)) {
                    CopyLitToReg(Scratch0, 2, "", hobatoi(a->name3.c_str()), a->name3);
                } else {
                    MemForVariable(a->name3, &addr3);
                    sov3 = SizeOfVar(a->name3);
                    CopyRegToReg(Scratch0, 2, addr3, sov3, "$Scratch0", a->name3, false);
                }

                int sovElement = a->literal2;
                Comment("Index := Index * sovElement '%d'", sovElement);
                if(sovElement == 1) {
                    // nothing
                } else if(sovElement == 2) {
                    VariableAdd(Scratch0, Scratch0, Scratch0, 2); // * 2
                } else if(sovElement == 3) {
                    CopyRegToReg(Scratch2, 2, addr3, 2, "$Scratch2", a->name3, false);
                    VariableAdd(Scratch0, Scratch0, Scratch0, 2); // * 2
                    VariableAdd(Scratch0, Scratch0, Scratch2, 2); // * 3
                } else if(sovElement == 4) {
                    VariableAdd(Scratch0, Scratch0, Scratch0, 2); // * 2
                    VariableAdd(Scratch0, Scratch0, Scratch0, 2); // * 4
                } else
                    THROW_COMPILER_EXCEPTION("Internal error.");

                Comment("CALL Table '%s' address in flash", a->name2.c_str());
                MemOfVar(a->name2, &addr2);

                if(sovElement < 1)
                    THROW_COMPILER_EXCEPTION("Internal error.");
                if(sovElement > 4)
                    THROW_COMPILER_EXCEPTION("Internal error.");

                if((sovElement >= 1) && (sov1 >= 1)) {
                    Instruction(OP_CALL, addr2);
                    Instruction(OP_MOVWF, addr1);
                    if((sovElement >= 2) && (sov1 >= 2)) {
                        Increment(Scratch0, 2, "Index");
                        Instruction(OP_CALL, addr2);
                        Instruction(OP_MOVWF, addr1 + 1);
                        if((sovElement >= 3) && (sov1 >= 3)) {
                            Increment(Scratch0, 2, "Index");
                            Instruction(OP_CALL, addr2);
                            Instruction(OP_MOVWF, addr1 + 2);
                            if((sovElement == 4) && (sov1 >= 4)) {
                                Increment(Scratch0, 2, "Index");
                                Instruction(OP_CALL, addr2);
                                Instruction(OP_MOVWF, addr1 + 3);
                            }
                        }
                    }
                }
                if(sovElement < sov1) {
                    Comment("Clear upper bytes of dest");
                    for(int i = 0; i < (sov1 - sovElement); i++)
                        Instruction(OP_CLRF, addr1 + sovElement + i);
                }
                Comment("END CALLs");
                break;
            }
#endif

            case INT_DELAY: {
// #define DELAY_TEST
#ifdef DELAY_TEST
                SetBit(0x06, 6);   // 1 clocks
                ClearBit(0x06, 6); // 1 clocks
                SetBit(0x06, 6);   // 1 clocks
                ClearBit(0x06, 6); // 1 clocks
#endif
                if(IsNumber(a->name1)) {
                    long long clocks = CalcDelayClock(hobatoi(a->name1.c_str()));
                    long long clocksSave = clocks;
                    Comment("INT_DELAY %s us = %lld clocks", a->name1.c_str(), clocks);

                    clocks = (clocks - 10) / 6;
                    if(clocks > 0xffff) {
                        clocks = 0xffff;
                        clocksSave = clocks * 6;
                        Error(_(" The delay is too long!\n"
                                "The maximum possible delay is %lld us."),
                              (clocks * 6 + 10) * 4000000 / Prog.mcuClock);
                    }
                    if(clocks < 0)
                        clocks = 0;
                    if(clocks > 0) {
                        Instruction(OP_MOVLW, (BYTE)clocks & 0xff);
                        Instruction(OP_MOVWF, Scratch0);
                        Instruction(OP_MOVLW, (BYTE)(clocks >> 8) & 0xff);
                        Instruction(OP_MOVWF, Scratch0 + 1);
                        Delay(Scratch0, 2);
                        clocksSave -= clocks * 6 + 10;
                    }
                    for(int i = 0; i < clocksSave; i++)
                        Instruction(OP_NOP_);
                } else {
                    Comment("INT_DELAY %s us", a->name1.c_str());
                    CopyVarToReg(Scratch0, 2, a->name1);
                    Delay(Scratch0, 2);
                }
#ifdef DELAY_TEST
                SetBit(0x06, 6);   // 1 clocks
                ClearBit(0x06, 6); // 1 clocks
                SetBit(0x06, 6);   // 1 clocks
                                   //ClearBit(0x06,6); // 1 clocks
#endif
                break;
            }
            case INT_CLRWDT:
                Instruction(OP_CLRWDT);
                break;
            case INT_LOCK:
                Instruction(OP_GOTO, PicProgWriteP); // dead loop
                break;
            case INT_SLEEP: {
                if(Prog.WDTPSA) {
                }

                DWORD loopIfWdtWakeUp = PicProgWriteP;
                if(Prog.mcu->core == BaselineCore12bit) {
                    Instruction(OP_BCF, REG_STATUS, STATUS_GPWUF);
                    Instruction(OP_BCF, REG_STATUS, STATUS_CWUF);
                    Instruction(OP_MOVF, 0x06, DEST_W);
                } else {
                    /* If the GIE bit is clear (disabled), the device continues execution at the
instruction after the SLEEP instruction. If the GIE bit is set (enabled),
the device executes the instruction after the SLEEP instruction and then
branches to the interrupt address (0004h). */
                    Instruction(OP_BCF, REG_INTCON, GIE); // Prevent branches to the interrupt address (0004h).
                    /* If the global interrupts are disabled (GIE is cleared), but any interrupt
source has both its interrupt enable bit and the corresponding interrupt
flag bits set, the device will not enter Sleep. The SLEEP instruction is
executed as a NOP instruction. */
                    Instruction(OP_BCF, REG_INTCON, INTF); // Clear previous state (must be cleared in software)
                    Instruction(OP_BSF, REG_INTCON, INTE); // Enables the RB0/INT external interrupt for wake-up
                }
                Instruction(OP_SLEEP_); // stop here
                // Wake-up on PIC10,12 cause a RESET ! See SLEEP trap.
                // Others PIC executes next OP_xxxx
                Instruction(OP_NOP_);                         // can GOTO 0x0004 from here
                Instruction(OP_BTFSS, REG_STATUS, STATUS_TO); // The TO bit is cleared if WDT wake-up occurred.
                Instruction(OP_GOTO, loopIfWdtWakeUp);
                break;
            }
            default:
                THROW_COMPILER_EXCEPTION_FMT("INT_%d", a->op);
                break;
        }
    }
}

//-----------------------------------------------------------------------------
PlcTimerData plcTmr;
//-----------------------------------------------------------------------------
// Configure Timer1 and Ccp1 to generate the periodic `cycle' interrupt
// that triggers all the ladder logic processing. We will always use 16-bit
// Timer1, with the prescaler configured appropriately.
//-----------------------------------------------------------------------------
static void ConfigureTimer1(long long int cycleTimeMicroseconds)
{
    Comment("Configure Timer1");
    Instruction(OP_CLRWDT); // Clear WDT and prescaler
    WriteRegister(REG_CCPR1L, (BYTE)(plcTmr.tmr & 0xff));
    WriteRegister(REG_CCPR1H, (BYTE)(plcTmr.tmr >> 8));

    WriteRegister(REG_TMR1L, 0);
    WriteRegister(REG_TMR1H, 0);

    WriteRegister(REG_T1CON, plcTmr.PS);

    if(McuAs(" PIC16F1512 ") || //
       McuAs(" PIC16F1513 ") || //
       McuAs(" PIC16F1516 ") || //
       McuAs(" PIC16F1517 ") || //
       McuAs(" PIC16F1518 ") || //
       McuAs(" PIC16F1519 ") || //
       McuAs(" PIC16F1526 ") || //
       McuAs(" PIC16F1527 ") || //
       McuAs(" PIC16F1933 ") || //
       McuAs(" PIC16F1947 ") //
    ) {
        Instruction(OP_BCF, REG_T1GCON, TMR1GE);
    }

    BYTE ccp1con;
    // compare mode, reset TMR1 on trigger
    ccp1con = 0x0b;
    WriteRegister(REG_CCP1CON, ccp1con);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void ConfigureTimer0(long long int cycleTimeMicroseconds)
{
    Comment("Configure Timer0");
    //  Note: The value written to the TMR0 register can be adjusted,
    //  in order to account for the two instruction
    //  cycle delay when TMR0 is written.
    //  Prescaler Rate Select bits (1:2, 1:4, ... 1:256)
    //
    //  Fcycle=Fosc/4/prescaler/softDivisor/TMR0
    //  Fcycle=Fosc/(4*prescaler*softDivisor*TMR0)
    //  Fcycle=1e6/cycleTimeMicroseconds
    //  cycleTimeMicroseconds=(4*prescaler*softDivisor*TMR0)*1e6/Fcycle

    //  ticksPerCycle = (Fosc/4) * (cycleTimeMicroseconds/1e6)
    //  ticksPerCycle = prescaler*softDivisor*TMR0
    /*
    int err = PicTimer0Prescaler(cycleTimeMicroseconds);
    if(err < 0) {
        fCompileError(f, fAsm);
    }
    */
    if(Prog.mcu->core == BaselineCore12bit) {
        if(plcTmr.prescaler == 1) {
            Prog.WDTPSA = 1;
            //CHANGING PRESCALER(TIMER0 -> WDT)
            Instruction(OP_CLRWDT);         // Clear WDT, not a prescaler !
            Instruction(OP_CLRF, REG_TMR0); // Clear TMR0 and prescaler
            Prog.OPTION |= 1 << PSA;
            Instruction(OP_MOVLW, Prog.OPTION); // Select WDT
            Instruction(OP_OPTION);             //

            Instruction(OP_CLRWDT); //PS<2:0> are 000 or 001
            // 18 ms up to 2.3 seconds
            //Prog.OPTION &=~0x07;                // 18 ms
            Prog.OPTION |= 0x07;                // 2.3 seconds
            Instruction(OP_MOVLW, Prog.OPTION); //Set Postscaler to
            Instruction(OP_OPTION);             // desired WDT rate
        } else {
            Prog.WDTPSA = 0;
            //CHANGING PRESCALER(WDT -> TIMER0)
            //The WDT has a nominal time-out period of 18 ms, (with no prescaler).
            Instruction(OP_CLRWDT); // Clear WDT and prescaler
            // Select TMR0, new prescale value and clock source
            Prog.OPTION &= ~(1 << PSA);
            Prog.OPTION &= ~0x07;
            Prog.OPTION |= plcTmr.PS;
            Instruction(OP_MOVLW, Prog.OPTION); // Set prescale to PS
            Instruction(OP_OPTION);
        }

        Instruction(OP_MOVLW, 256 - plcTmr.tmr + 22); // init first PLC cycle
        Instruction(OP_MOVWF, REG_TMR0);
    } else {
        // redisable interrupt
        Instruction(OP_BCF, REG_INTCON, T0IE);

        // enable clock, internal source
        Instruction(OP_BCF, REG_OPTION, T0CS);

        if(plcTmr.prescaler == 1) {
            //CHANGING PRESCALER(TIMER0 -> WDT)
            Instruction(OP_CLRWDT);               // Clear WDT
            Instruction(OP_CLRF, REG_TMR0);       // Clear TMR0 and prescaler
            Instruction(OP_BSF, REG_OPTION, PSA); // Set prescaler to WDT
            Instruction(OP_CLRWDT);
            Instruction(OP_MOVLW, 0xF8); // Mask prescaler bits
            Instruction(OP_ANDWF, REG_OPTION, DEST_W);
            Instruction(OP_MOVWF, REG_OPTION);
        } else {
            Prog.WDTPSA = 0;
            //CHANGING PRESCALER(WDT -> TIMER0)
            Instruction(OP_CLRWDT);                    // Clear WDT and prescaler
            Instruction(OP_MOVLW, 0xF0);               // Mask TMR0 select and
            Instruction(OP_ANDWF, REG_OPTION, DEST_W); // prescaler bits
            Instruction(OP_IORLW, plcTmr.PS);          // Set prescale to PS
            Instruction(OP_MOVWF, REG_OPTION);
            //          Instruction(OP_BCF, REG_OPTION, PSA);      // Set prescaler to TMR0
        }

        Instruction(OP_MOVLW, 256 - plcTmr.tmr);
        Instruction(OP_MOVWF, REG_TMR0);
    }
}

//-----------------------------------------------------------------------------
static void SetPrescaler(int tmr)
{
    if(tmr == 0) {
        // set up prescaler
        if(plcTmr.prescaler == 1)
            plcTmr.PS = 0x07; // wdt 2.3s
        else if(plcTmr.prescaler == 2)
            plcTmr.PS = 0x00;
        else if(plcTmr.prescaler == 4)
            plcTmr.PS = 0x01;
        else if(plcTmr.prescaler == 8)
            plcTmr.PS = 0x02;
        else if(plcTmr.prescaler == 16)
            plcTmr.PS = 0x03;
        else if(plcTmr.prescaler == 32)
            plcTmr.PS = 0x04;
        else if(plcTmr.prescaler == 64)
            plcTmr.PS = 0x05;
        else if(plcTmr.prescaler == 128)
            plcTmr.PS = 0x06;
        else if(plcTmr.prescaler == 256)
            plcTmr.PS = 0x07;
        else
            THROW_COMPILER_EXCEPTION_FMT("%d", plcTmr.prescaler);
    } else if(tmr == 1) {
        plcTmr.PS = 0x00;
        // set up prescaler
        if(plcTmr.prescaler == 1)
            plcTmr.PS |= 0x00;
        else if(plcTmr.prescaler == 2)
            plcTmr.PS |= 0x10;
        else if(plcTmr.prescaler == 4)
            plcTmr.PS |= 0x20;
        else if(plcTmr.prescaler == 8)
            plcTmr.PS |= 0x30;
        else
            THROW_COMPILER_EXCEPTION_FMT("%d", plcTmr.prescaler);
        // enable clock, internal source
        plcTmr.PS |= 0x01; // TMR1ON
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
}
//-----------------------------------------------------------------------------
// Calc PIC 16-bit Timer1 or 8-bit Timer0  to do the timing of PLC cycle.
bool CalcPicPlcCycle(long long int cycleTimeMicroseconds, SDWORD PicProgLdLen)
{
    //memset(plcTmr, 0, sizeof(plcTmr));
    plcTmr.ticksPerCycle = (long long int)floor(1.0 * Prog.mcuClock / 4 * cycleTimeMicroseconds / 1000000 + 0.5);
    plcTmr.softDivisor = 1;
    plcTmr.prescaler = 1;
    plcTmr.PS = 0;
    plcTmr.cycleTimeMin = (int)floor(1e6 * (PLC_CLOCK_MIN * 2) * 1 * 4 / Prog.mcuClock + 0.5);
    //                                      ^min_divider         ^min_prescaler
    long int max_tmr, max_prescaler, max_softDivisor;
    if(Prog.cycleTimer == 0) {
        max_tmr = 0x100;
        max_prescaler = 256;
        max_softDivisor = 0xFF; // 1..0xFFFF
    } else {
        max_tmr = 0x10000;
        max_prescaler = 8;
        max_softDivisor = 0xFF; // 1..0xFF
    }
    plcTmr.cycleTimeMax =
        (long long int)floor(1.0e6 * max_tmr * max_prescaler * max_softDivisor * 4 / Prog.mcuClock + 0.5);

    long int      bestTmr = LONG_MIN / 4;
    long int      bestPrescaler = LONG_MAX / 4;
    long int      bestSoftDivisor;
    long long int bestErr = LLONG_MAX / 4;
    long long int err;
    while(plcTmr.softDivisor <= max_softDivisor) {
        plcTmr.prescaler = max_prescaler;
        while(plcTmr.prescaler >= 1) {
            for(plcTmr.tmr = 1; plcTmr.tmr <= max_tmr; plcTmr.tmr++) {
                err = plcTmr.ticksPerCycle - (long long int)plcTmr.tmr * plcTmr.prescaler * plcTmr.softDivisor;
                if(err < 0)
                    err = -err;

                if((PicProgLdLen <= 0) || (plcTmr.prescaler * plcTmr.tmr >= PicProgLdLen / 3))
                    if((bestErr > err) || ((bestErr == err) && (bestPrescaler < plcTmr.prescaler))) {
                        bestErr = err;
                        bestSoftDivisor = plcTmr.softDivisor;
                        bestPrescaler = plcTmr.prescaler;
                        bestTmr = plcTmr.tmr;
                        if(err == 0)
                            goto err0;
                    }
            }
            plcTmr.prescaler /= 2;
        }
        if(plcTmr.softDivisor == max_softDivisor)
            break;
        plcTmr.softDivisor++;
    }
err0:
    plcTmr.softDivisor = bestSoftDivisor;
    plcTmr.prescaler = bestPrescaler;
    plcTmr.tmr = bestTmr;
    plcTmr.Fcycle = 1.0 * Prog.mcuClock / (4.0 * plcTmr.softDivisor * plcTmr.prescaler * plcTmr.tmr);
    plcTmr.TCycle = 4.0 * plcTmr.prescaler * plcTmr.softDivisor * plcTmr.tmr / (1.0 * Prog.mcuClock);
    SetPrescaler(Prog.cycleTimer);
    if(cycleTimeMicroseconds > plcTmr.cycleTimeMax) {
        Error(_("PLC cycle time more then %.3f ms not valid."), 0.001 * plcTmr.cycleTimeMax);
        return false;
    } else if(cycleTimeMicroseconds < plcTmr.cycleTimeMin) {
        Error(_("PLC cycle time less then %.3f ms not valid."), 0.001 * plcTmr.cycleTimeMin);
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 8x8=16 signed multiply. One operand in
// addr1->mem[1], other operand in addr2->mem[1], write result to addr3->mem[2].
// Result may be a byte size addr3->mem[1].
// http://www.piclist.com/techref/microchip/math/32bmath-ph.htm
//-----------------------------------------------------------------------------
static void WriteMultiplyRoutine8(DWORD addr3, DWORD addr1, DWORD addr2, int sov3) // addr3 = addr1 * addr2;
{
    Comment("MultiplyRoutine8");
    DWORD savePicProgWriteP = PicProgWriteP;
#ifdef MOVE_TO_PAGE_0
    MultiplyRoutineAddress8 = PicProgWriteP;
#else
    FwdAddrIsNow(MultiplyRoutineAddress8);
#endif

    DWORD multiplicand0;
    DWORD multiplier0;
    DWORD result0;
    DWORD result1;

    // Multiplicand not changed in routine, we can use original address of data.
    // Multiplier changed in routine (RRF), we use copy of data in Scratch2.
    multiplier0 = Scratch2;
    if(addr3 != addr1) {
        multiplicand0 = addr1;
        Instruction(OP_MOVF, addr2, DEST_W);
    } else {
        multiplicand0 = addr2;
        Instruction(OP_MOVF, addr1, DEST_W);
    }
    Instruction(OP_MOVWF, multiplier0); // Copy data to Scratch2

    bool needCopyResult = false;
    if(addr3 != addr1) { // a=b*c or a=b*a
        result0 = addr3;
        if(sov3 >= 2)
            result1 = addr3 + 1;
        else
            result1 = Scratch1;
    } else if(addr3 != addr2) { // a=a*b
        result0 = addr3;
        if(sov3 >= 2)
            result1 = addr3 + 1;
        else
            result1 = Scratch1;
    } else { // a=a*a // ((addr3 == addr1) && (addr3 == addr2))
        result0 = Scratch0;
        result1 = Scratch1;
        needCopyResult = true;
    }

    DWORD counter = Scratch6;

    Instruction(OP_CLRF, result0); // Clear result
    Instruction(OP_CLRF, result1);
    Instruction(OP_MOVLW, 0x08); // Bit
    Instruction(OP_MOVWF, counter);
    Instruction(OP_MOVF, multiplicand0, DEST_W); // Multiplicand in W reg

    DWORD loop = PicProgWriteP;
    Instruction(OP_RRF, multiplier0, DEST_F);   // LSB into Carry
    skpnc                                       // Test Carry
        Instruction(OP_ADDWF, result1, DEST_F); // Add multiplicand to result
    Instruction(OP_RRF, result1, DEST_F);       // Align result
    Instruction(OP_RRF, result0, DEST_F);
    Instruction(OP_DECFSZ, counter, DEST_F); // Next bit
    Instruction(OP_GOTO, loop);

    if(needCopyResult) {
        Instruction(OP_MOVF, result0, DEST_W);
        Instruction(OP_MOVWF, addr3);

        if(sov3 >= 2) {
            Instruction(OP_MOVF, result1, DEST_W);
            Instruction(OP_MOVWF, addr3 + 1);
        }
    }
    if(sov3 >= 3) {
        Instruction(OP_CLRF, addr3 + 2);   // Clear result
        Instruction(OP_BTFSC, result1, 7); // MSB
        Instruction(OP_COMF, addr3 + 2);   // Negate result
        if(sov3 == 4) {
            Instruction(OP_CLRF, addr3 + 3);   // Clear result
            Instruction(OP_BTFSC, result1, 7); // MSB
            Instruction(OP_COMF, addr3 + 3);   // Negate result
        } else
            THROW_COMPILER_EXCEPTION("Internal error.");
    }

    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW, 0);
    else
        Instruction(OP_RETURN);

    if((savePicProgWriteP >> 11) != ((PicProgWriteP - 1) >> 11))
        THROW_COMPILER_EXCEPTION("Internal error.");
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 16x16 signed multiply.
// One operand in Scratch1:Scratch0,
// other in Scratch3:Scratch2,
// result in Scratch5:Scratch4:Scratch3:Scratch2.
//-----------------------------------------------------------------------------
static void WriteMultiplyRoutine()
{
    Comment("MultiplyRoutine16x16=32 (2x2=4)");
    DWORD savePicProgWriteP = PicProgWriteP;
#ifdef MOVE_TO_PAGE_0
    MultiplyRoutineAddress = PicProgWriteP;
#else
    FwdAddrIsNow(MultiplyRoutineAddress);
#endif

    DWORD result3 = Scratch5;             // result
    DWORD result2 = Scratch4;             // result
    DWORD result1 = Scratch3; // operand2 // result
    DWORD result0 = Scratch2; // operand2 // result

    DWORD multiplicand0 = Scratch0; // operand1
    DWORD multiplicand1 = Scratch1; // operand1

    DWORD counter = ScratchI;

    DWORD dontAdd = AllocFwdAddr();
    DWORD top;

    Instruction(OP_CLRF, result3);
    Instruction(OP_CLRF, result2);
    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_RRF, result1, DEST_F);
    Instruction(OP_RRF, result0, DEST_F);

    Instruction(OP_MOVLW, 16);
    Instruction(OP_MOVWF, counter);

    top = PicProgWriteP;
    Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, dontAdd);

    Instruction(OP_MOVF, multiplicand0, DEST_W);
    Instruction(OP_ADDWF, result2, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
    Instruction(OP_INCF, result3, DEST_F);

    Instruction(OP_MOVF, multiplicand1, DEST_W);
    Instruction(OP_ADDWF, result3, DEST_F); // never overflow occurs here

    FwdAddrIsNow(dontAdd);

    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_RRF, result3, DEST_F);
    Instruction(OP_RRF, result2, DEST_F);
    Instruction(OP_RRF, result1, DEST_F);
    Instruction(OP_RRF, result0, DEST_F);

    Instruction(OP_DECFSZ, counter, DEST_F);
    Instruction(OP_GOTO, top);

    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW);
    else
        Instruction(OP_RETURN);

    if((savePicProgWriteP >> 11) != ((PicProgWriteP - 1) >> 11))
        THROW_COMPILER_EXCEPTION("Internal error.");
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 8x8 signed multiply.
// One operand in     Scratch0,
// other in           Scratch2,
// result in Scratch3:Scratch2.
//-----------------------------------------------------------------------------
static void WriteMultiplyRoutine8x8()
{
    Comment("MultiplyRoutine8x8=16 (1x1=2)");
    DWORD savePicProgWriteP = PicProgWriteP;
#ifdef MOVE_TO_PAGE_0
    MultiplyRoutineAddress = PicProgWriteP;
#else
    FwdAddrIsNow(MultiplyRoutineAddress);
#endif

    DWORD result1 = Scratch3; //          // result
    DWORD result0 = Scratch2; // operand2 // result

    DWORD multiplicand0 = Scratch0; // operand1

    DWORD counter = ScratchI;

    DWORD dontAdd = AllocFwdAddr();
    DWORD top;

    Instruction(OP_CLRF, result1);
    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_RRF, result0, DEST_F);

    Instruction(OP_MOVLW, 8);
    Instruction(OP_MOVWF, counter);

    top = PicProgWriteP;
    Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, dontAdd);

    Instruction(OP_MOVF, multiplicand0, DEST_W);
    Instruction(OP_ADDWF, result1, DEST_F); // never overflow occurs here

    FwdAddrIsNow(dontAdd);

    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_RRF, result1, DEST_F);
    Instruction(OP_RRF, result0, DEST_F);

    Instruction(OP_DECFSZ, counter, DEST_F);
    Instruction(OP_GOTO, top);

    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW);
    else
        Instruction(OP_RETURN);

    if((savePicProgWriteP >> 11) != ((PicProgWriteP - 1) >> 11))
        THROW_COMPILER_EXCEPTION("Internal error.");
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 24x16 signed multiply.
// One operand in                       Scratch1:Scratch0,
// other in                    Scratch4:Scratch3:Scratch2,
// result in Scratch6:Scratch5:Scratch4:Scratch3:Scratch2.
//-----------------------------------------------------------------------------
static void WriteMultiplyRoutine24x16()
{
    Comment("MultiplyRoutine24x16=40 (3x2=5)");
    DWORD savePicProgWriteP = PicProgWriteP;
#ifdef MOVE_TO_PAGE_0
    MultiplyRoutineAddress24x16 = PicProgWriteP;
#else
    FwdAddrIsNow(MultiplyRoutineAddress24x16);
#endif

    DWORD result4 = Scratch6;             // result
    DWORD result3 = Scratch5;             // result
    DWORD result2 = Scratch4; // operand2 // result
    DWORD result1 = Scratch3; // operand2 // result
    DWORD result0 = Scratch2; // operand2 // result

    DWORD multiplicand0 = Scratch0; // operand1
    DWORD multiplicand1 = Scratch1; // operand1
//  DWORD multiplicand2 = Scratch7; // always 0
//  Instruction(OP_CLRF, multiplicand2);

    DWORD counter = ScratchI;

    DWORD dontAdd = AllocFwdAddr();
    DWORD top;

    Instruction(OP_CLRF, result4);
    Instruction(OP_CLRF, result3);
    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_RRF, result2, DEST_F);
    Instruction(OP_RRF, result1, DEST_F);
    Instruction(OP_RRF, result0, DEST_F);

    Instruction(OP_MOVLW, 24);
    Instruction(OP_MOVWF, counter);

    top = PicProgWriteP;
    Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, dontAdd);
#if 0
    Instruction(OP_MOVF, multiplicand0, DEST_W);
    Instruction(OP_ADDWF, result3, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
    Instruction(OP_INCF, result4, DEST_F);

    Instruction(OP_MOVF, multiplicand1, DEST_W);
    Instruction(OP_ADDWF, result4, DEST_F); // never overflow occurs here
#else
    Instruction(OP_MOVF, multiplicand0, DEST_W);
    Instruction(OP_ADDWF, result3, DEST_F);

    Instruction(OP_MOVF, multiplicand1, DEST_W);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
    Instruction(OP_INCFSZ, multiplicand1, DEST_W);
    Instruction(OP_ADDWF, result4, DEST_F); // never overflow occurs here
#endif
    FwdAddrIsNow(dontAdd);

    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_RRF, result4, DEST_F);
    Instruction(OP_RRF, result3, DEST_F);
    Instruction(OP_RRF, result2, DEST_F);
    Instruction(OP_RRF, result1, DEST_F);
    Instruction(OP_RRF, result0, DEST_F);

    Instruction(OP_DECFSZ, counter, DEST_F);
    Instruction(OP_GOTO, top);

    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW);
    else
        Instruction(OP_RETURN);

    if((savePicProgWriteP >> 11) != ((PicProgWriteP - 1) >> 11))
        THROW_COMPILER_EXCEPTION("Internal error.");
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 16/16 signed divide.
// Call with dividend in Scratch1:0,
// divisor in             Scratch5:Scratch4,
// and get the result in Scratch1:0.
//-----------------------------------------------------------------------------
static void WriteDivideRoutine()
{
    Comment("DivideRoutine16");
    DWORD savePicProgWriteP = PicProgWriteP;

    DWORD dividend0 = Scratch0;
    DWORD dividend1 = Scratch1;

    DWORD divisor0 = Scratch4;
    DWORD divisor1 = Scratch5;

    DWORD remainder0 = Scratch6;
    DWORD remainder1 = Scratch7;

    DWORD counter = ScratchI;
    DWORD sign = ScratchS;

    DWORD dontNegateDivisor = AllocFwdAddr();
    DWORD dontNegateDividend = AllocFwdAddr();
    DWORD done = AllocFwdAddr();
    DWORD notNegative = AllocFwdAddr();
    DWORD loop;

#ifdef MOVE_TO_PAGE_0
    DivideRoutineAddress = PicProgWriteP;
#else
    FwdAddrIsNow(DivideRoutineAddress);
#endif

    Instruction(OP_MOVF, dividend1, DEST_W);
    Instruction(OP_XORWF, divisor1, DEST_W);
    Instruction(OP_MOVWF, sign);

    Instruction(OP_BTFSS, divisor1, 7);
    Instruction(OP_GOTO, dontNegateDivisor);
    Negate(divisor0, 2, "divisor");
    FwdAddrIsNow(dontNegateDivisor);

    Instruction(OP_BTFSS, dividend1, 7);
    Instruction(OP_GOTO, dontNegateDividend);
    Negate(dividend0, 2, "dividend");
    FwdAddrIsNow(dontNegateDividend);

    Instruction(OP_CLRF, remainder1);
    Instruction(OP_CLRF, remainder0);

    Instruction(OP_BCF, REG_STATUS, STATUS_C);

    Instruction(OP_MOVLW, 16 + 1);
    Instruction(OP_MOVWF, counter);

    loop = PicProgWriteP;
    Instruction(OP_RLF, dividend0, DEST_F);
    Instruction(OP_RLF, dividend1, DEST_F);

    Instruction(OP_DECF, counter, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_Z);
    Instruction(OP_GOTO, done);

    Instruction(OP_RLF, remainder0, DEST_F);
    Instruction(OP_RLF, remainder1, DEST_F);

    Instruction(OP_MOVF, divisor0, DEST_W);
    Instruction(OP_SUBWF, remainder0, DEST_F);
    Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
    Instruction(OP_DECF, remainder1, DEST_F);
    Instruction(OP_MOVF, divisor1, DEST_W);
    Instruction(OP_SUBWF, remainder1, DEST_F);

    Instruction(OP_BTFSS, remainder1, 7);
    Instruction(OP_GOTO, notNegative);

    Instruction(OP_MOVF, divisor0, DEST_W);
    Instruction(OP_ADDWF, remainder0, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
    Instruction(OP_INCF, remainder1, DEST_F);
    Instruction(OP_MOVF, divisor1, DEST_W);
    Instruction(OP_ADDWF, remainder1, DEST_F);

    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, loop);

    FwdAddrIsNow(notNegative);
    Instruction(OP_BSF, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, loop);

    FwdAddrIsNow(done);
    Instruction(OP_BTFSS, sign, 7);
    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW, 0);
    else
        Instruction(OP_RETURN, 0, 0);

    Negate(dividend0, 2, "result");

    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW, 0);
    else
        Instruction(OP_RETURN, 0, 0);

    if((savePicProgWriteP >> 11) != ((PicProgWriteP - 1) >> 11))
        THROW_COMPILER_EXCEPTION("Internal error.");
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 24/16 signed divide.
// Call with dividend in  Scratch2:Scratch1:Scratch0,
// divisor in             Scratch5:Scratch4,
// and get the result in  Scratch2:Scratch1:Scratch0.
//-----------------------------------------------------------------------------
static void WriteDivideRoutine24x16()
{
    Comment("DivideRoutine24/16=24");
    DWORD savePicProgWriteP = PicProgWriteP;

    DWORD dividend0 = Scratch0;
    DWORD dividend1 = Scratch1;
    DWORD dividend2 = Scratch2;

    DWORD divisor0 = Scratch4;
    DWORD divisor1 = Scratch5;

    DWORD remainder0 = Scratch6;
    DWORD remainder1 = Scratch7;

    DWORD counter = ScratchI;
    DWORD sign = ScratchS;

    DWORD dontNegateDivisor = AllocFwdAddr();
    DWORD dontNegateDividend = AllocFwdAddr();
    DWORD done = AllocFwdAddr();
    DWORD notNegative = AllocFwdAddr();
    DWORD loop;

#ifdef MOVE_TO_PAGE_0
    DivideRoutineAddress24x16 = PicProgWriteP;
#else
    FwdAddrIsNow(DivideRoutineAddress24x16);
#endif

    Instruction(OP_MOVF, dividend2, DEST_W);
    Instruction(OP_XORWF, divisor1, DEST_W);
    Instruction(OP_MOVWF, sign);

    Instruction(OP_BTFSS, divisor1, 7);
    Instruction(OP_GOTO, dontNegateDivisor);
    Negate(divisor0, 2, "divisor");
    FwdAddrIsNow(dontNegateDivisor);

    Instruction(OP_BTFSS, dividend2, 7);
    Instruction(OP_GOTO, dontNegateDividend);
    Negate(dividend0, 3, "dividend");
    FwdAddrIsNow(dontNegateDividend);

    Instruction(OP_CLRF, remainder1);
    Instruction(OP_CLRF, remainder0);

    Instruction(OP_BCF, REG_STATUS, STATUS_C);

    Instruction(OP_MOVLW, 24 + 1);
    Instruction(OP_MOVWF, counter);

    loop = PicProgWriteP;
    Instruction(OP_RLF, dividend0, DEST_F);
    Instruction(OP_RLF, dividend1, DEST_F);
    Instruction(OP_RLF, dividend2, DEST_F);

    Instruction(OP_DECF, counter, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_Z);
    Instruction(OP_GOTO, done);

    Instruction(OP_RLF, remainder0, DEST_F);
    Instruction(OP_RLF, remainder1, DEST_F);

    Instruction(OP_MOVF, divisor0, DEST_W);
    Instruction(OP_SUBWF, remainder0, DEST_F);
    Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
    Instruction(OP_DECF, remainder1, DEST_F);
    Instruction(OP_MOVF, divisor1, DEST_W);
    Instruction(OP_SUBWF, remainder1, DEST_F);

    Instruction(OP_BTFSS, remainder1, 7);
    Instruction(OP_GOTO, notNegative);

    Instruction(OP_MOVF, divisor0, DEST_W);
    Instruction(OP_ADDWF, remainder0, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
    Instruction(OP_INCF, remainder1, DEST_F);
    Instruction(OP_MOVF, divisor1, DEST_W);
    Instruction(OP_ADDWF, remainder1, DEST_F);

    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, loop);

    FwdAddrIsNow(notNegative);
    Instruction(OP_BSF, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO, loop);

    FwdAddrIsNow(done);
    Instruction(OP_BTFSS, sign, 7);
    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW, 0);
    else
        Instruction(OP_RETURN, 0, 0);

    Negate(dividend0, 3, "result");
    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW, 0);
    else
        Instruction(OP_RETURN, 0, 0);

    if((savePicProgWriteP >> 11) != ((PicProgWriteP - 1) >> 11))
        THROW_COMPILER_EXCEPTION("Internal error.");
}

//-----------------------------------------------------------------------------
// Compile the program to PIC16 code for the currently selected processor
// and write it to the given file. Produce an error message if we cannot
// write to the file, or if there is something inconsistent about the
// program.
//-----------------------------------------------------------------------------
static bool _CompilePic16(const char *outFile, int ShowMessage)
{
    if(McuAs("Microchip PIC16F628 ")    //
       || McuAs(" PIC16F72 ")           //
       || McuAs("Microchip PIC16F819 ") //
       || McuAs(" PIC16F873 ")          //
       || McuAs(" PIC16F874 ")          //
       || McuAs("Microchip PIC16F876 ") //
       || McuAs("Microchip PIC16F877 ") //
       || McuAs("Microchip PIC16F88 ")  //
    ) {
        REG_PIR1 = 0x0c;
        CCP1IF = BIT2;
        REG_TMR1L = 0x0e;
        REG_TMR1H = 0x0f;
        REG_T1CON = 0x10;
        REG_CCPR1L = 0x15;
        REG_CCPR1H = 0x16;
        REG_CCP1CON = 0x17;
        // WDTE = BIT2;
    } else if(McuAs(" PIC12F629 ") || //
              McuAs(" PIC12F675 ") //
    ) {
        REG_PIR1 = 0x0c;
        CCP1IF = BIT2;
        REG_TMR1L = 0x0e;
        REG_TMR1H = 0x0f;
        REG_T1CON = 0x10;
    } else if(McuAs(" PIC12F683 ") //
    ) {
        REG_PIR1 = 0x0c;
        CCP1IF = BIT5;
        REG_TMR1L = 0x0e;
        REG_TMR1H = 0x0f;
        REG_T1CON = 0x10;
        REG_CCPR1L = 0x13;
        REG_CCPR1H = 0x14;
        REG_CCP1CON = 0x15;
        // WDTE = BIT3;
    } else if(McuAs(" PIC12F752 ") //
    ) {
        REG_PIR1 = 0x0c;
        CCP1IF = BIT0;
        REG_TMR1L = 0x0f;
        REG_TMR1H = 0x10;
        REG_T1CON = 0x11;
        REG_CCPR1L = 0x13;
        REG_CCPR1H = 0x14;
        REG_CCP1CON = 0x15;
        // WDTE = BIT3;
    } else if(McuAs(" PIC16F882 ")             //
              || McuAs(" PIC16F883 ")          //
              || McuAs(" PIC16F884 ")          //
              || McuAs("Microchip PIC16F886 ") //
              || McuAs("Microchip PIC16F887 ") //
    ) {
        REG_PIR1 = 0x0c;
        CCP1IF = BIT2;
        REG_TMR1L = 0x0e;
        REG_TMR1H = 0x0f;
        REG_T1CON = 0x10;
        REG_CCPR1L = 0x15;
        REG_CCPR1H = 0x16;
        REG_CCP1CON = 0x17;
        // WDTE = BIT3;
    } else if(McuAs(" PIC16F1512 ")    //
              || McuAs(" PIC16F1513 ") //
              || McuAs(" PIC16F1516 ") //
              || McuAs(" PIC16F1517 ") //
              || McuAs(" PIC16F1518 ") //
              || McuAs(" PIC16F1519 ") //
              || McuAs(" PIC16F1526 ") //
              || McuAs(" PIC16F1527 ") //
              || McuAs(" PIC16F1933 ") //
              || McuAs(" PIC16F1947 ") //
              || McuAs(" PIC16F1824 ") //
              || McuAs(" PIC16F1827 ") //
    ) {
        REG_PIR1 = 0x0011;
        CCP1IF = BIT2;
        REG_TMR1L = 0x0016;
        REG_TMR1H = 0x0017;
        REG_T1CON = 0x0018;
        REG_T1GCON = 0x0019;
        REG_CCPR1L = 0x0291;
        REG_CCPR1H = 0x0292;
        REG_CCP1CON = 0x0293;
        // WDTE = BIT4;           // WDT enabled while running and disabled in Sleep
    } else if(McuAs(" PIC10F") //
    ) {
        // has not
        // WDTE = BIT2;
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    //------------------------------------------------------------
    if(McuAs("Microchip PIC16F628 ") //
    ) {
        REG_CMCON = 0x1f;
    } else if(McuAs("Microchip PIC16F88 ") //
    ) {
        REG_CMCON = 0x009C;
    } else if(McuAs(" PIC12F675 ") || //
              McuAs(" PIC12F683 ") //
    ) {
        REG_CMCON = 0x19;
        // REG_IOCA    = 0x96;
    } else if(McuAs("Microchip PIC16F819 ")    //
              || McuAs("Microchip PIC16F876 ") //
              || McuAs("Microchip PIC16F877 ") //
              || McuAs("Microchip PIC16F887 ") //
              || McuAs("Microchip PIC16F886 ") //
              || McuAs(" PIC16F72 ")           //
              || McuAs(" PIC12F629 ")          //
              || McuAs(" PIC16F1512 ")         //
              || McuAs(" PIC16F1513 ")         //
              || McuAs(" PIC16F1516 ")         //
              || McuAs(" PIC16F1517 ")         //
              || McuAs(" PIC16F1518 ")         //
              || McuAs(" PIC16F1519 ")         //
              || McuAs(" PIC16F1526 ")         //
              || McuAs(" PIC16F1527 ")         //
              || McuAs(" PIC16F1933 ")         //
              || McuAs(" PIC16F1947 ")         //
              || McuAs(" PIC16F1824 ")         //
              || McuAs(" PIC16F1827 ")         //
              || McuAs(" PIC12F752 ")         //
    ) {
        // has not
    } else if(McuAs(" PIC10F") //
    ) {
        // has not
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    //------------------------------------------------------------
    if(McuAs("Microchip PIC16F628 ")    //
       || McuAs("Microchip PIC16F873 ") //
       || McuAs("Microchip PIC16F874 ") //
       || McuAs("Microchip PIC16F876 ") //
       || McuAs("Microchip PIC16F877 ") //
       || McuAs("Microchip PIC16F88 ")  //
    ) {
        REG_TXSTA = 0x98;
        REG_RCSTA = 0x18;
        REG_SPBRG = 0x99;
        REG_TXREG = 0x19;
        REG_RCREG = 0x1a;
    } else if(McuAs(" PIC16F882 ")    //
              || McuAs(" PIC16F883 ") //
              || McuAs(" PIC16F884 ") //
              || McuAs(" PIC16F886 ") //
              || McuAs(" PIC16F887 ") //
    ) {
        REG_TXSTA = 0x98;
        REG_RCSTA = 0x18;
        REG_SPBRGH = 0x9A;
        REG_SPBRG = 0x99;
        REG_TXREG = 0x19;
        REG_RCREG = 0x1a;
    } else if(McuAs(" PIC16F1512 ")    //
              || McuAs(" PIC16F1513 ") //
              || McuAs(" PIC16F1516 ") //
              || McuAs(" PIC16F1517 ") //
              || McuAs(" PIC16F1518 ") //
              || McuAs(" PIC16F1519 ") //
              || McuAs(" PIC16F1526 ") //
              || McuAs(" PIC16F1527 ") //
              || McuAs(" PIC16F1933 ") //
              || McuAs(" PIC16F1947 ") //
              || McuAs(" PIC16F1824 ") //
              || McuAs(" PIC16F1827 ") //
    ) {
        REG_TXSTA = 0x019E;
        REG_RCSTA = 0x019D;
        REG_SPBRGH = 0x019C;
        REG_SPBRG = 0x019B;
        REG_TXREG = 0x019A;
        REG_RCREG = 0x0199;
    } else if(McuAs(" PIC10F")                 //
              || McuAs(" PIC12F")              //
              || McuAs(" PIC16F72 ")           //
              || McuAs("Microchip PIC16F819 ") //
    ) {
        // has not
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    //------------------------------------------------------------
    if(McuAs("Microchip PIC16F88 ")     //
       || McuAs("Microchip PIC16F819 ") //
       || McuAs("Microchip PIC16F876 ") //
       || McuAs("Microchip PIC16F877 ") //
       || McuAs("Microchip PIC16F886 ") //
       || McuAs("Microchip PIC16F887 ") //
    ) {
        REG_ADRESH = 0x1e;
        REG_ADRESL = 0x9e;
        REG_ADCON0 = 0x1f;
        REG_ADCON1 = 0x9f;
    } else if(McuAs(" PIC16F72 ") //
    ) {
        REG_ADRESL = 0x1e;
        REG_ADCON0 = 0x1f;
        REG_ADCON1 = 0x9f;
    } else if(McuAs(" PIC12F752 ") //
    ) {
        REG_ADRESL = 0x1c;
        REG_ADRESH = 0x1d;
        REG_ADCON0 = 0x1e;
        REG_ADCON1 = 0x1f;
    } else if(McuAs(" PIC12F675 ") || //
              McuAs(" PIC12F683 ") //
    ) {
        REG_ADRESH = 0x1e;
        REG_ADRESL = 0x9e;
        REG_ADCON0 = 0x1f;
    } else if(McuAs(" PIC16F1512 ")    //
              || McuAs(" PIC16F1513 ") //
              || McuAs(" PIC16F1516 ") //
              || McuAs(" PIC16F1517 ") //
              || McuAs(" PIC16F1518 ") //
              || McuAs(" PIC16F1519 ") //
              || McuAs(" PIC16F1526 ") //
              || McuAs(" PIC16F1527 ") //
              || McuAs(" PIC16F1933 ") //
              || McuAs(" PIC16F1947 ") //
              || McuAs(" PIC16F1824 ") //
              || McuAs(" PIC16F1827 ") //
    ) {
        REG_ADRESH = 0x009C;
        REG_ADRESL = 0x009B;
        REG_ADCON0 = 0x009D;
        REG_ADCON1 = 0x009E;
    } else if(McuAs("Microchip PIC16F628 ")
              // has not
    ) {
    } else if(McuAs(" PIC10F")    //
              || McuAs(" PIC12F") //
    ) {
        // has not
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    //------------------------------------------------------------
    if(McuAs("Microchip PIC16F876 ")    //
       || McuAs("Microchip PIC16F877 ") //
       || McuAs("Microchip PIC16F886 ") //
       || McuAs("Microchip PIC16F887 ") //
    ) {
        REG_CCPR2L = 0x1b;
        REG_CCP2CON = 0x1d;
    } else if(McuAs(" PIC16F1512 ")    //
              || McuAs(" PIC16F1513 ") //
              || McuAs(" PIC16F1516 ") //
              || McuAs(" PIC16F1517 ") //
              || McuAs(" PIC16F1518 ") //
              || McuAs(" PIC16F1519 ") //
              || McuAs(" PIC16F1526 ") //
              || McuAs(" PIC16F1527 ") //
              || McuAs(" PIC16F1933 ") //
              || McuAs(" PIC16F1947 ") //
              || McuAs(" PIC16F1824 ") //
              || McuAs(" PIC16F1827 ") //
    ) {
        REG_CCPR2L = 0x0298;
        REG_CCP2CON = 0x029A;
    } else if(McuAs("Microchip PIC16F628 ")    //
              || McuAs("Microchip PIC16F819 ") //
              || McuAs("Microchip PIC16F88 ")  //
              || McuAs(" PIC16F72 ")           //
    ) {
        // has not
    } else if(McuAs(" PIC10F")    //
              || McuAs(" PIC12F") //
    ) {
        // has not
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    //------------------------------------------------------------
    if(McuAs("Microchip PIC16F628 ") || //
       McuAs(" PIC16F72 ")           || //
       McuAs("Microchip PIC16F819 ") || //
       McuAs("Microchip PIC16F876 ") || //
       McuAs("Microchip PIC16F877 ") || //
       McuAs("Microchip PIC16F88 ")  || //
       McuAs("Microchip PIC16F886 ") || //
       McuAs("Microchip PIC16F887 ") //
    ) {
        REG_T2CON = 0x12;
        REG_PR2 = 0x92;
    } else if(McuAs(" PIC16F1512 ") || //
              McuAs(" PIC16F1513 ") || //
              McuAs(" PIC16F1516 ") || //
              McuAs(" PIC16F1517 ") || //
              McuAs(" PIC16F1518 ") || //
              McuAs(" PIC16F1519 ") || //
              McuAs(" PIC16F1526 ") || //
              McuAs(" PIC16F1527 ") || //
              McuAs(" PIC16F1933 ") || //
              McuAs(" PIC16F1947 ") || //
              McuAs(" PIC16F1824 ") || //
              McuAs(" PIC16F1827 ") //
    ) {
        REG_T2CON = 0x001C;
        REG_PR2 = 0x001B;
    } else if(McuAs(" PIC12F683 ") //
    ) {
        REG_T2CON = 0x12;
        REG_PR2 = 0x92;
    } else if(McuAs(" PIC12F752 ") //
    ) {
        REG_T2CON = 0x112;
        REG_PR2 = 0x111;
    } else if(McuAs(" PIC10F") ||    //
              McuAs(" PIC12F") //
    ) {
        // has not
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    //------------------------------------------------------------
    if(McuAs("Microchip PIC16F628 ")    //
       || McuAs("Microchip PIC16F88 ")  //
       || McuAs("Microchip PIC16F819 ") //
       || McuAs("Microchip PIC16F877 ") //
       || McuAs("Microchip PIC16F876 ") //
       || McuAs("Microchip PIC16F874 ") //
       || McuAs("Microchip PIC16F873 ") //
       || McuAs("Microchip PIC16F887 ") //
       || McuAs("Microchip PIC16F886 ") //
       || McuAs("Microchip PIC16F884 ") //
       || McuAs("Microchip PIC16F883 ") //
       || McuAs("Microchip PIC16F882 ") //
       || McuAs(" PIC16F72 ")           //
       || McuAs(" PIC12F629 ")          //
       || McuAs(" PIC12F675 ")          //
       || McuAs(" PIC12F683 ")          //
       || McuAs(" PIC12F752 ")          //
    ) {
        REG_TMR0 = 0x01;
        REG_OPTION = 0x81;
    } else if(McuAs(" PIC16F1512 ")    //
              || McuAs(" PIC16F1513 ") //
              || McuAs(" PIC16F1516 ") //
              || McuAs(" PIC16F1517 ") //
              || McuAs(" PIC16F1518 ") //
              || McuAs(" PIC16F1519 ") //
              || McuAs(" PIC16F1526 ") //
              || McuAs(" PIC16F1527 ") //
              || McuAs(" PIC16F1933 ") //
              || McuAs(" PIC16F1947 ") //
              || McuAs(" PIC16F1824 ") //
              || McuAs(" PIC16F1827 ") //
    ) {
        REG_TMR0 = 0x15;
        REG_OPTION = 0x95;
    } else if(McuAs(" PIC10F")    //
              || McuAs(" PIC12F") //
    ) {
        REG_TMR0 = 0x01;
        //REG_OPTION not available for read. Write able via OP_OPTION operation.
        // WDTE = BIT2;
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    //------------------------------------------------------------
    if(McuAs("Microchip PIC16F877 ")    //
       || McuAs("Microchip PIC16F819 ") //
       || McuAs("Microchip PIC16F88 ")  //
       || McuAs("Microchip PIC16F876 ") //
       || McuAs("Microchip PIC16F887 ") //
       || McuAs("Microchip PIC16F886 ") //
    ) {
        REG_EECON1 = 0x18c;
        REG_EECON2 = 0x18d;
        REG_EEDATA = 0x10c;
        REG_EEADR = 0x10d;
    } else if(McuAs("Microchip PIC16F628 ") || //
              McuAs(" PIC12F629 ") ||          //
              McuAs(" PIC12F675 ") ||          //
              McuAs(" PIC12F683 ")             //
    ) {
        REG_EECON1 = 0x9c;
        REG_EECON2 = 0x9d;
        REG_EEDATA = 0x9a;
        REG_EEADR = 0x9b;
    } else if(McuAs(" PIC16F1512 ")    //
              || McuAs(" PIC16F1513 ") //
              || McuAs(" PIC16F1516 ") //
              || McuAs(" PIC16F1517 ") //
              || McuAs(" PIC16F1518 ") //
              || McuAs(" PIC16F1519 ") //
              || McuAs(" PIC16F1526 ") //
              || McuAs(" PIC16F1527 ") //
    ) {
        // has not EEPROM, use PFM
    } else if(McuAs(" PIC16F1933 ")    //
              || McuAs(" PIC16F1947 ") //
              || McuAs(" PIC16F1824 ") //
              || McuAs(" PIC16F1827 ") //
    ) {
        REG_EECON1 = 0x195;
        REG_EECON2 = 0x196;
        REG_EEDATA = 0x193;
        REG_EEDATH = 0x194;
        REG_EEADR = 0x191;
        REG_EEADRH = 0x192;
    } else if(McuAs(" PIC10F") ||  //
              McuAs(" PIC12F752 ") || //
              McuAs(" PIC16F72 ") //
    ) {
        // has not
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    //------------------------------------------------------------
    if(McuAs("Microchip PIC16F887 ")    //
       || McuAs("Microchip PIC16F886 ") //
    ) {
        REG_ANSEL = 0x0188;
        REG_ANSELH = 0x0189;
    } else if(McuAs("Microchip PIC16F88 ") //
    ) {
        REG_ANSEL = 0x009B;
    } else if(McuAs(" PIC12F675 ") || //
              McuAs(" PIC12F683 ") //
    ) {
        REG_ANSEL = 0x009F;
    } else if(McuAs(" PIC12F752 ") //
    ) {
        REG_ANSEL = 0x0185; // ANSELA
    } else if(McuAs("Microchip PIC16F628 ")    //
              || McuAs("Microchip PIC16F819 ") //
              || McuAs("Microchip PIC16F877 ") //
              || McuAs("Microchip PIC16F876 ") //
    ) {
        // has not
    } else {
        //      oops();
    }
    //----------continue------------------------------------------
    if(McuAs(" PIC16F1512 ")    //
       || McuAs(" PIC16F1513 ") //
       || McuAs(" PIC16F1516 ") //
       || McuAs(" PIC16F1517 ") //
       || McuAs(" PIC16F1518 ") //
       || McuAs(" PIC16F1519 ") //
       || McuAs(" PIC16F1526 ") //
       || McuAs(" PIC16F1527 ") //
       || McuAs(" PIC16F1933 ") //
       || McuAs(" PIC16F1947 ") //
       || McuAs(" PIC16F1824 ") //
       || McuAs(" PIC16F1827 ") //
    ) {
        REG_ANSELA = 0x18C;
    }
    if(McuAs(" PIC16F1512 ")    //
       || McuAs(" PIC16F1513 ") //
       || McuAs(" PIC16F1516 ") //
       || McuAs(" PIC16F1517 ") //
       || McuAs(" PIC16F1518 ") //
       || McuAs(" PIC16F1519 ") //
       || McuAs(" PIC16F1526 ") //
       || McuAs(" PIC16F1527 ") //
       || McuAs(" PIC16F1933 ") //
       || McuAs(" PIC16F1827 ") //
    ) {
        REG_ANSELB = 0x18D;
    }
    if(McuAs(" PIC16F1512 ")    //
       || McuAs(" PIC16F1513 ") //
       || McuAs(" PIC16F1516 ") //
       || McuAs(" PIC16F1517 ") //
       || McuAs(" PIC16F1518 ") //
       || McuAs(" PIC16F1519 ") //
       || McuAs(" PIC16F1824 ") //
    ) {
        REG_ANSELC = 0x18E;
    }
    if(McuAs(" PIC16F1517 ")    //
       || McuAs(" PIC16F1519 ") //
       || McuAs(" PIC16F1526 ") //
       || McuAs(" PIC16F1527 ") //
    ) {
        REG_ANSELD = 0x18F;
    }
    if(McuAs(" PIC16F1517 ")    //
       || McuAs(" PIC16F1519 ") //
       || McuAs(" PIC16F1526 ") //
       || McuAs(" PIC16F1527 ") //
       || McuAs(" PIC16F1947 ") //
    ) {
        REG_ANSELE = 0x190;
    }
    if(McuAs(" PIC16F1526 ")    //
       || McuAs(" PIC16F1527 ") //
       || McuAs(" PIC16F1947 ") //
    ) {
        REG_ANSELF = 0x40C;
        REG_ANSELG = 0x40D;
    }
    //------------------------------------------------------------
    if(McuAs(" PIC16F1824 ")    //
       || McuAs(" PIC16F1827 ") //
    ) {
        REG_OSCON = 0x099;
    }
    //------------------------------------------------------------
    if(McuAs("Microchip PIC16F887 ")    //
       || McuAs("Microchip PIC16F886 ") //
       || McuAs("Microchip PIC16F88 ")  //
       || McuAs(" PIC16F87 ")           //
       || McuAs(" PIC16F884 ")          //
       || McuAs(" PIC16F883 ")          //
       || McuAs(" PIC16F882 ")          //
    ) {
        CONFIG_ADDR1 = 0x2007;
        CONFIG_ADDR2 = 0x2008;
    } else if(McuAs(" PIC16F1512 ")    //
              || McuAs(" PIC16F1513 ") //
              || McuAs(" PIC16F1516 ") //
              || McuAs(" PIC16F1517 ") //
              || McuAs(" PIC16F1518 ") //
              || McuAs(" PIC16F1519 ") //
              || McuAs(" PIC16F1526 ") //
              || McuAs(" PIC16F1527 ") //
              || McuAs(" PIC16F1933 ") //
              || McuAs(" PIC16F1947 ") //
              || McuAs(" PIC16F1824 ") //
              || McuAs(" PIC16F1827 ") //
    ) {
        CONFIG_ADDR1 = 0x8007;
        CONFIG_ADDR2 = 0x8008;
    } else if(McuAs("Microchip PIC16F628 ") || //
              McuAs("Microchip PIC16F819 ") || //
              McuAs("Microchip PIC16F876 ") || //
              McuAs("Microchip PIC16F877 ") || //
              McuAs(" PIC16F873 ")          || //
              McuAs(" PIC16F874 ")          || //
              McuAs(" PIC16F72 ")           || //
              McuAs(" PIC12F629 ")          || //
              McuAs(" PIC12F675 ")          || //
              McuAs(" PIC12F683 ")          || //
              McuAs(" PIC12F752 ")          //
    ) {
        CONFIG_ADDR1 = 0x2007;
    } else if(McuAs(" PIC10F200 ")    //
              || McuAs(" PIC10F204 ") //
              || McuAs(" PIC10F220 ") //
    ) {
        CONFIG_ADDR1 = 0x01ff;
    } else if(McuAs(" PIC10F202 ")    //
              || McuAs(" PIC10F206 ") //
              || McuAs(" PIC10F222 ") //
    ) {
        CONFIG_ADDR1 = 0x03ff;
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    //------------------------------------------------------------
    FileTracker f(outFile, "w");
    if(!f) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), outFile);
        return false;
    }

    char outFileAsm[MAX_PATH];
    SetExt(outFileAsm, outFile, ".asm");
    FileTracker fAsm(outFileAsm, "w");
    if(!fAsm) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), outFileAsm);
        return false;
    }

    rungNow = -100;
    fprintf(fAsm,
            ";/* This is auto-generated ASM code from LDmicro. Do not edit this file!\n"
            ";   Go back to the LDmicro ladder diagram source for changes in the ladder logic. */\n");
    fprintf(fAsm, "; %s is the LDmicro target processor.\n", Prog.mcu->mcuList);
    fprintf(fAsm, "\tLIST    p=%s\n", Prog.mcu->mcuList);
    fprintf(fAsm,
            "#include "
            "%s.inc"
            "\n",
            Prog.mcu->mcuInc);

    if(!Prog.configurationWord)
        Prog.configurationWord = Prog.mcu->configurationWord;
    if(CONFIG_ADDR2 != -1) {
        fprintf(fAsm, "\t__CONFIG 0x%X, 0x%X\n", CONFIG_ADDR1, (WORD)Prog.configurationWord & 0xFFFF);
        fprintf(fAsm, "\t__CONFIG 0x%X, 0x%X\n", CONFIG_ADDR2, (WORD)(Prog.configurationWord >> 16) & 0xFFFF);
    } else {
        fprintf(fAsm, "\t__CONFIG 0x%X\n", (WORD)Prog.configurationWord & 0xFFFF);
    }
    fprintf(fAsm, "\tradix dec\n");
    fprintf(fAsm, "\torg 0\n");
    fprintf(fAsm, ";TABSIZE = 8\n");
    fprintf(fAsm, ";\tCODE\n");

    WipeMemory();

    AllocStart();

    AllocBitsVars(); // first

    ScratchS = AllocOctetRam(); // REG_STATUS or sign
    Scratch0 = AllocOctetRam();
    Scratch1 = AllocOctetRam();
    Scratch2 = AllocOctetRam();
    Scratch3 = AllocOctetRam();
    Scratch4 = AllocOctetRam();
    Scratch5 = AllocOctetRam();
    Scratch6 = AllocOctetRam();
    Scratch7 = AllocOctetRam();
    if(Prog.mcu->core != BaselineCore12bit) {
        Scratch8 = AllocOctetRam();
        Scratch9 = AllocOctetRam();
        Scratch10 = AllocOctetRam();
        Scratch11 = AllocOctetRam();
    }
    ScratchI = AllocOctetRam(); // tmp indirect addressing or Counter

    if(REG_EEDATA != -1) {
        // Allocate the register used to hold the high byte of the EEPROM word
        // that's queued up to program, plus the bit to indicate that it is
        // valid.
        EepromHighByte = AllocOctetRam(3);
        EepromHighBytesCounter = AllocOctetRam();
    }

    DWORD progStart = AllocFwdAddr();
    // Our boot vectors; not necessary to do it like this, but it lets
    // bootloaders rewrite the beginning of the program to do their magic.
    // PCLATH is init to 0, but apparently some bootloaders want to see us
    // initialize it again.
    Comment("Reset vector"); // may be relocated with botloders
    if(Prog.mcu->core == BaselineCore12bit) {
        Instruction(OP_CLRF, REG_STATUS); //0 // Select Indirect Bank 0
        Instruction(OP_NOP_);             //1
        Instruction(OP_NOP_, 0, 0);       //2
    } else if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        Instruction(OP_CLRF, REG_STATUS); //0 // Select Indirect Bank 0
        Instruction(OP_MOVLB, 0);         //1 // Select Bank 0
        Instruction(OP_MOVLP, 0);         //2 // Select Page 0
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        Instruction(OP_CLRF, REG_STATUS); //0 // Select Bank 0 and Indirect Bank 0
        Instruction(OP_CLRF, REG_PCLATH); //1 // Select Page 0
        Instruction(OP_NOP_, 0, 0);       //2
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");
    Comment("GOTO progStart");
    Instruction(OP_GOTO, progStart); //3
    if(Prog.mcu->core != BaselineCore12bit) {
        Comment("Interrupt Vector");
        Instruction(OP_RETFIE, 0, 0); //4 or this
                                      //  Instruction(OP_NOP_, 0, 0);         //4 or this
        Instruction(OP_NOP_, 0, 0);   //5 ???
        Instruction(OP_NOP_, 0, 0);   //6 ???
        Instruction(OP_NOP_, 0, 0);   //7 ??? may be not needed
    }

#ifdef TABLE_IN_FLASH
    InitTables();
#endif

#ifdef MOVE_TO_PAGE_0
    if(MultiplyRoutineUsed())
        WriteMultiplyRoutine();
    if(MultiplyRoutineUsed())
        WriteMultiplyRoutine24x16();
    if(DivideRoutineUsed())
        WriteDivideRoutine();
    if(DivideRoutineUsed())
        WriteDivideRoutine24x16();
    if(Bin32BcdRoutineUsed())
        WriteBin32BcdRoutine();
#endif

#ifndef MOVE_TO_PAGE_0
    DivideRoutineAddress = AllocFwdAddr();
#endif
    DivideNeeded = false;
#ifndef MOVE_TO_PAGE_0
    MultiplyRoutineAddress = AllocFwdAddr();
#endif
    MultiplyNeeded = false;
    MultiplyNeeded8 = false;
    MultiplyNeeded24x16 = false;
#ifndef MOVE_TO_PAGE_0
    Bin32BcdRoutineAddress = AllocFwdAddr();
#endif
    Bin32BcdNeeded = false;

    FwdAddrIsNow(progStart);
    Comment("Program Start");

    if(McuAs(" PIC16F1824 ") || McuAs(" PIC16F1827 ")) {
        Comment("Selects 16MHz for the Internal Oscillator when it used, ignored otherwise.");
        WriteRegister(REG_OSCON, 0xF << IRCF0);
    }
    if(Prog.mcu->core == BaselineCore12bit) {
        Prog.WDTPSA = 1;
        Prog.OPTION = 0xFF; // default; only for PIC10Fxxx
    }
    if(Prog.cycleTimer >= 0) { // 1
        // Configure PLC Timer near the progStart
        CalcPicPlcCycle(Prog.cycleTime, PicProgLdLen);
        if(Prog.cycleTimer == 0)
            ConfigureTimer0(Prog.cycleTime);
        else
            ConfigureTimer1(Prog.cycleTime);
    }
    Comment("Now zero out the RAM"); // 2
    DWORD progStartBank = 0;
    // for(DWORD i = 0; i < MAX_RAM_SECTIONS; i++) {
    for(DWORD i = 0; i <= RamSection; i++) {
        if(Prog.mcu->ram[i].len) {
            if(Bank(Prog.mcu->ram[i].start) != Bank(Prog.mcu->ram[i].start + Prog.mcu->ram[i].len - 1))
                THROW_COMPILER_EXCEPTION_FMT("%d", i);
            if(Prog.mcu->ram[i].len > 256)
                THROW_COMPILER_EXCEPTION_FMT("%d", i);

            // Select bank N
            if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
                Instruction(OP_MOVLB, progStartBank >> 7);
            } else {
                if((progStartBank ^ Bank(Prog.mcu->ram[i].start) & 0x0080)) {
                    if(Prog.mcu->ram[i].start & 0x0080)
                        Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
                    else
                        Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
                }
                if((progStartBank ^ Bank(Prog.mcu->ram[i].start) & 0x0100)) {
                    if(Prog.mcu->ram[i].start & 0x0100)
                        Instruction(OP_BSF, REG_STATUS, STATUS_RP1);
                    else
                        Instruction(OP_BCF, REG_STATUS, STATUS_RP1);
                }
            }
            if((progStartBank ^ Bank(Prog.mcu->ram[i].start) & 0x0100)) {
                if(Prog.mcu->ram[i].start & 0x0100)
                    Instruction(OP_BSF, REG_STATUS, STATUS_IRP);
                else
                    Instruction(OP_BCF, REG_STATUS, STATUS_IRP);
            }
            progStartBank = Bank(Prog.mcu->ram[i].start);
            Instruction(OP_MOVLW, Prog.mcu->ram[i].len - 1);
            Instruction(OP_MOVWF, Prog.mcu->ram[i].start & ~BankMask());
            Instruction(OP_MOVLW, (Prog.mcu->ram[i].start + 1) & ~BankMask());
            Instruction(OP_MOVWF, REG_FSR);

            DWORD zeroMem = PicProgWriteP;
            Instruction(OP_CLRF, REG_INDF);
            Instruction(OP_INCF, REG_FSR, DEST_F);
            Instruction(OP_DECFSZ, Prog.mcu->ram[i].start & ~BankMask(), DEST_F); //  <<<<<<<<
            Instruction(OP_GOTO, zeroMem);                                        //                                ^
            //Instruction(OP_CLRF, Prog.mcu->ram[i].start & ~BankMask()); // not need, self cleared here >>>^
        }
    }
    if(Bank(Prog.mcu->ram[RamSection].start)) { // 3
        Instruction(OP_CLRF, REG_STATUS);       // Select Bank 0 and Indirect Bank 0
        if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
            Instruction(OP_MOVLB, 0); // Select Bank 0
        }
    }
    if(Prog.cycleTimer >= 0) { // 4
        // Configure PLC Timer near the progStart after zero out RAM
        if(plcTmr.softDivisor > 1) { // RAM neded
            Comment("Configure PLC Timer softDivisor");
            SetSizeOfVar("$softDivisor", byteNeeded(plcTmr.softDivisor));
            MemForVariable("$softDivisor", &plcTmr.softDivisorAddr);
            WriteRegister(plcTmr.softDivisorAddr, (BYTE)(plcTmr.softDivisor & 0xff));
            if(plcTmr.softDivisor > 0xff)
                WriteRegister(plcTmr.softDivisorAddr + 1, (BYTE)(plcTmr.softDivisor >> 8));
        }
    }
    BYTE isInput[MAX_IO_PORTS], isAnsel[MAX_IO_PORTS], isOutput[MAX_IO_PORTS];
    BuildDirectionRegisters(isInput, isAnsel, isOutput);

    if(Prog.mcu->core == BaselineCore12bit) {
        Comment("Set up the TRISx registers (direction). 1-tri-stated (input), 0-output.");

        Prog.OPTION &=
            ~(1 << T0CS); // Timer0 Clock Source Select bit, 0 = Transition on internal instruction cycle clock, FOSC/4
        Prog.OPTION &= ~(1 << _GPWU); // Enable Wake-up on Pin Change bit (GP0, GP1, GP3)

        for(DWORD i = 0; i < MAX_IO_PORTS; i++) {
            if(IS_MCU_REG(i)) {
                WriteRegister(Prog.mcu->outputRegs[i], 0x00);
                Instruction(OP_MOVLW, (BYTE)(~isOutput[i]));
                Instruction(OP_TRIS, Prog.mcu->outputRegs[i]);
                break;
            }
        }

        // Pull-ups are enabled after direction settings !
        Comment("Clear Bit 6 - Enable Weak Pull-ups bit (GP0, GP1, GP3)");
        Prog.OPTION &= ~(1 << _GPPU);
        Instruction(OP_MOVLW, Prog.OPTION);
        Instruction(OP_OPTION);
    }

    if(SleepFunctionUsed()) {
        if(Prog.mcu->core == BaselineCore12bit) {
            Comment("SLEEP trap");
            DWORD notWdtWakeUp = AllocFwdAddr();
            Instruction(OP_BTFSC, REG_STATUS, STATUS_TO); // The TO bit is cleared if WDT wake-up occurred.
            Instruction(OP_GOTO, notWdtWakeUp);
            Instruction(OP_BTFSC, REG_STATUS, STATUS_PD); // 0 = By execution of the SLEEP instruction.
            Instruction(OP_GOTO, notWdtWakeUp);

            Instruction(OP_BCF, REG_STATUS, STATUS_GPWUF);
            Instruction(OP_BCF, REG_STATUS, STATUS_CWUF);
            Instruction(OP_MOVF, 0x06, DEST_W);

            Instruction(OP_SLEEP_); // stop here
            // Wake-up on PIC10,12? cause a RESET !

            FwdAddrIsNow(notWdtWakeUp);
        }
    }

    if(McuAs("Microchip PIC16F628 ") || //
       McuAs(" PIC12F683 ") ||          //
       McuAs(" PIC12F675 ")             //
    ) {
        // This is also a nasty special case; the comparators on the
        // PIC16F628 are enabled by default and need to be disabled, or
        // else the PORTA GPIOs don't work.
        Comment("Comparator Off. Normal port I/O.");
        WriteRegister(REG_CMCON, 0x07);
    }

    if(McuAs("Microchip PIC16F877 ") || //
       McuAs("Microchip PIC16F819 ") || //
       McuAs("Microchip PIC16F876 ")    //
    ) {
        // The GPIOs that can also be A/D inputs default to being A/D
        // inputs, so turn that around
        WriteRegister(REG_ADCON1,  //
                      (1 << 7) |   // right-justify A/D result
                      (7 << 0)     // all digital inputs
        );
    } else if(McuAs(" PIC16F72 ")) {
        WriteRegister(REG_ADCON1, 0x7); // all digital inputs
    }

    Comment("Set up the ANSELx registers. 1-analog input, 0-digital I/O.");
    if(McuAs("Microchip PIC16F88 ") || //
       McuAs(" PIC12F683 ") ||         //
       McuAs(" PIC12F675 ")            //
    ) {
        Instruction(OP_CLRF, REG_ANSEL);       // all digital inputs
    } else if(McuAs("Microchip PIC16F887 ") || //
              McuAs("Microchip PIC16F886 ")    //
    ) {
        Instruction(OP_CLRF, REG_ANSEL);  // all digital inputs
        Instruction(OP_CLRF, REG_ANSELH); // all digital inputs
    }
    /*
    if(REG_ANSEL != -1)
        Instruction(OP_CLRF, REG_ANSEL);
    if(REG_ANSELH != -1)
        Instruction(OP_CLRF, REG_ANSELH);
    /**/
    if(REG_ANSELA != -1) {
        Instruction(OP_CLRF, REG_ANSELA);
        if(isAnsel[0])
            WriteRegister(REG_ANSELA, isAnsel[0]);
    }
    if(REG_ANSELB != -1) {
        Instruction(OP_CLRF, REG_ANSELB);
        if(isAnsel[1])
            WriteRegister(REG_ANSELB, isAnsel[1]);
    }
    if(REG_ANSELC != -1) {
        Instruction(OP_CLRF, REG_ANSELC);
        if(isAnsel[2])
            WriteRegister(REG_ANSELC, isAnsel[2]);
    }
    if(REG_ANSELD != -1) {
        Instruction(OP_CLRF, REG_ANSELD);
        if(isAnsel[3])
            WriteRegister(REG_ANSELD, isAnsel[3]);
    }
    if(REG_ANSELE != -1) {
        Instruction(OP_CLRF, REG_ANSELE);
        if(isAnsel[4])
            WriteRegister(REG_ANSELE, isAnsel[4]);
    }
    if(REG_ANSELF != -1) {
        Instruction(OP_CLRF, REG_ANSELF);
        if(isAnsel[5])
            WriteRegister(REG_ANSELF, isAnsel[5]);
    }
    if(REG_ANSELG != -1) {
        Instruction(OP_CLRF, REG_ANSELG);
        if(isAnsel[6])
            WriteRegister(REG_ANSELG, isAnsel[6]);
    }

    if(Prog.mcu->core == BaselineCore12bit) {
        ; //
    } else {
        Comment(
            "Set up the TRISx registers (direction). 1-tri-stated (input), 0-output and drive the outputs low to start");
        for(DWORD i = 0; i < MAX_IO_PORTS; i++) {
            if(IS_MCU_REG(i))
                WriteRegister(Prog.mcu->outputRegs[i], 0x00);
        }
        for(DWORD i = 0; i < MAX_IO_PORTS; i++) {
            if(IS_MCU_REG(i)) {
                WriteRegister(Prog.mcu->dirRegs[i], ~isOutput[i]);
            }
        }

        // Pull-ups are enabled after direction settings !
        Comment("Clear Bit 7 - PORTs pull-ups are enabled by individual port latch values");
        Instruction(OP_BCF, REG_OPTION, _RBPU);
    }

    if(UartFunctionUsed()) {
        if(Prog.baudRate == 0) {
            THROW_COMPILER_EXCEPTION(_("Zero baud rate not possible."));
            return false;
        }

        Comment("UART setup");
        // So now we should set up the UART. First let us calculate the
        // baud rate; there is so little point in the fast baud rates that
        // I won't even bother, so
        // bps = Fosc/(64*(X+1))
        // bps*64*(X + 1) = Fosc
        // X = Fosc/(bps*64)-1
        // and round, don't truncate
        int divisor = (Prog.mcuClock + Prog.baudRate * 32) / (Prog.baudRate * 64) - 1;
        if(divisor > 255) {
            ComplainAboutBaudRateOverflow();
            divisor = 255;
        }
        double actual = Prog.mcuClock / (64.0 * (divisor + 1));
        double percentErr = 100 * (actual - Prog.baudRate) / Prog.baudRate;

        if(fabs(percentErr) > 2) {
            ComplainAboutBaudRateError(divisor, actual, percentErr);
        }
        WriteRegister(REG_SPBRG, divisor & 0xFF);
        WriteRegister(REG_TXSTA, 1 << TXEN);                 // only TXEN set, SYNC=0
        WriteRegister(REG_RCSTA, (1 << SPEN) | (1 << CREN)); // only SPEN, CREN set
    }

    //  Comment("Select Bank 0");
    //  BankSelect(0xFF, 0);
    DWORD addrDuty;
    int   bitDuty;
    if(Prog.cycleDuty) {
        MemForSingleBit(YPlcCycleDuty, false, &addrDuty, &bitDuty);
    }
    DWORD addrINTCON_T0IF;
    int   bitINTCON_T0IF;
    DWORD BeginOfPLCCycle0;

    //Instruction(OP_BCF,   REG_INTCON ,T0IF); // must be cleared in software

    Comment("Begin Of PLC Cycle");
    BeginOfPLCCycle = PicProgWriteP;
    BeginOfPLCCycle0 = PicProgWriteP;
    if(Prog.cycleTimer == 0) {
        if(Prog.mcu->core == BaselineCore12bit) {
            Instruction(OP_MOVF, REG_TMR0, DEST_W);
            Instruction(OP_BTFSS, REG_STATUS, STATUS_Z);
            Instruction(OP_GOTO, BeginOfPLCCycle);
            Instruction(OP_MOVLW, 256 - plcTmr.tmr + 1); // tested in Proteus +1 = 1001Hz = 1987Hz
            //Instruction(OP_MOVLW, 256 - plcTmr.tmr + 2); // tested in Proteus +2 = 985Hz = 2002Hz
            Instruction(OP_MOVWF, REG_TMR0);
        } else {
            if(plcTmr.softDivisor > 1) {
                if(Prog.cycleDuty) {
                    CopyBit(addrDuty, bitDuty, REG_INTCON, T0IF, YPlcCycleDuty);
                    addrINTCON_T0IF = addrDuty;
                    bitINTCON_T0IF = bitDuty;
                } else {
                    MemForSingleBit("$Y_INTCON_T0IF", false, &addrINTCON_T0IF, &bitINTCON_T0IF);
                    CopyBit(addrINTCON_T0IF, bitINTCON_T0IF, REG_INTCON, T0IF, "$Y_INTCON_T0IF");
                }
            }
            BeginOfPLCCycle0 = PicProgWriteP;
            Instruction(OP_MOVLW, 256 - plcTmr.tmr + 0); // tested in Proteus {DONE +0} {1ms=1kHz} {0.250ms=4kHz}
            IfBitClear(REG_INTCON, T0IF);
            Instruction(OP_GOTO, PicProgWriteP - 1);
            //Instruction(OP_MOVWF,  REG_TMR0);         // 3999 //7920 = 8kHz = 0.125ms
            Instruction(OP_ADDWF, REG_TMR0, DEST_F); // 4012 //7967 = 8kHz = 0.125ms
            Instruction(OP_BCF, REG_INTCON, T0IF);   // must be cleared in software
        }
    } else if(Prog.cycleTimer == 1) {
        if(Prog.mcu->core == BaselineCore12bit) {
            THROW_COMPILER_EXCEPTION("Select Timer0 in menu 'Settings -> MCU parameters'!");
        }
        if(Prog.cycleDuty) {
            CopyBit(addrDuty, bitDuty, REG_PIR1, CCP1IF, YPlcCycleDuty);
            addrINTCON_T0IF = addrDuty;
            bitINTCON_T0IF = bitDuty;
        } else {
            MemForSingleBit("$Y_INTCON_T0IF", false, &addrINTCON_T0IF, &bitINTCON_T0IF);
            CopyBit(addrINTCON_T0IF, bitINTCON_T0IF, REG_PIR1, CCP1IF, "$Y_INTCON_T0IF");
        }

        BeginOfPLCCycle0 = PicProgWriteP;
        IfBitClear(REG_PIR1, CCP1IF);
        Instruction(OP_GOTO, PicProgWriteP - 1);
        Instruction(OP_BCF, REG_PIR1, CCP1IF);
    } else if(Prog.cycleTimer < 0) { // (Prog.cycleTime == 0)
        Comment("Watchdog reset");
        Instruction(OP_CLRWDT);
    } else
        THROW_COMPILER_EXCEPTION("Internal error.");

    if(Prog.cycleTimer >= 0) {
        Comment("Watchdog reset");
        Instruction(OP_CLRWDT);
        if(plcTmr.softDivisor > 1) {
            if(Prog.mcu->core == BaselineCore12bit) {
                Instruction(OP_DECFSZ, plcTmr.softDivisorAddr, DEST_F); // Skip if zero
                Instruction(OP_GOTO, BeginOfPLCCycle);
                if(plcTmr.softDivisor > 0xff) {
                    Instruction(OP_DECFSZ, plcTmr.softDivisorAddr + 1, DEST_F); // Skip if zero
                    Instruction(OP_GOTO, BeginOfPLCCycle);
                }
                CopyLitToReg(plcTmr.softDivisorAddr,
                             byteNeeded(plcTmr.softDivisor),
                             "",
                             plcTmr.softDivisor,
                             "plcTmr.softDivisor");
            } else {
                DWORD setLiteral = AllocFwdAddr();
                /*
          DWORD yesZero;
          if(plcTmr.softDivisor > 0xff) {
              yesZero = AllocFwdAddr();
          }
          */
                DWORD Decrement2 = AllocFwdAddr();
                /*
          Instruction(OP_MOVF, plcTmr.softDivisorAddr, DEST_F);
          Instruction(OP_BTFSC,REG_STATUS, STATUS_Z); // Skip if not zero
          Instruction(OP_GOTO, yesZero);
          */

                /**/
                IfBitClear(addrINTCON_T0IF, bitINTCON_T0IF);
                Instruction(OP_GOTO, Decrement2);
                /*
              Instruction(OP_MOVLW, 256 - plcTmr.tmr + 0); // tested in Proteus {DONE +0} {1ms=1kHz} {0.250ms=4kHz}
              Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
              Instruction(OP_SUBWF, REG_TMR0, DEST_F); // 4012 //7967 = 8kHz = 0.125ms
*/
                Instruction(OP_DECF, plcTmr.softDivisorAddr, DEST_F); // Skip if zero

                Instruction(OP_BTFSC, REG_STATUS, STATUS_Z); // Skip if not zero
                Instruction(OP_GOTO, setLiteral);

                FwdAddrIsNow(Decrement2);
                /**/

                Instruction(OP_DECFSZ, plcTmr.softDivisorAddr, DEST_F); // Skip if zero
                Instruction(OP_GOTO, BeginOfPLCCycle0);
                /*
          if(plcTmr.softDivisor > 0xff) {
              Instruction(OP_MOVF, plcTmr.softDivisorAddr+1, DEST_F);
              Instruction(OP_BTFSC,REG_STATUS, STATUS_Z); // Skip if not zero
              Instruction(OP_GOTO, yesZero);
              Instruction(OP_DECF, plcTmr.softDivisorAddr+1, DEST_F);
              Instruction(OP_GOTO, BeginOfPLCCycle0);
              FwdAddrIsNow(yesZero);
          }
          */
                FwdAddrIsNow(setLiteral);
                CopyLitToReg(plcTmr.softDivisorAddr,
                             byteNeeded(plcTmr.softDivisor),
                             "",
                             plcTmr.softDivisor,
                             "plcTmr.softDivisor");
            }
        }
    }
    if(Prog.cycleDuty) {
        Comment("SetBit YPlcCycleDuty");
        SetBit(addrDuty, bitDuty, "YPlcCycleDuty");
    }

    IntPc = 0;
    //Comment("CompileFromIntermediate BEGIN");
    CompileFromIntermediate(true);
    //Comment("CompileFromIntermediate END");

    for(DWORD i = 0; i < MAX_RUNGS; i++)
        Prog.HexInRung[i] = 0;
    for(DWORD i = 0; i < PicProgWriteP; i++)
        if((PicProg[i].rung >= 0) && (PicProg[i].rung < MAX_RUNGS))
            Prog.HexInRung[PicProg[i].rung]++;

    if(Prog.cycleDuty) {
        Comment("ClearBit YPlcCycleDuty");
        ClearBit(addrDuty, bitDuty, "YPlcCycleDuty");
    }

    Comment("GOTO next PLC cycle");
    Instruction(OP_GOTO, BeginOfPLCCycle);

    rungNow = -50;

#ifndef MOVE_TO_PAGE_0
    if(MultiplyNeeded)
        WriteMultiplyRoutine();
    if(MultiplyNeeded8)
        WriteMultiplyRoutine8();
    if(MultiplyNeeded24x16)
        WriteMultiplyRoutine24x16();
    if(DivideNeeded)
        WriteDivideRoutine();
    if(Bin32BcdNeeded)
        WriteBin32BcdRoutine();
#endif

    Instruction(OP_GOTO, PicProgWriteP); // for last label in asm

    MemCheckForErrorsPostCompile();
    AddrCheckForErrorsPostCompile();

    BankCorrection();
    BankCheckForErrorsPostCompile(fAsm);

    PageCorrection();
    PageCheckForErrorsPostCompile();

    ProgWriteP = PicProgWriteP;

    WriteHexFile(f, fAsm);
    fflush(f);

    fprintf(fAsm, "\tEND\n");
    PrintVariables(fAsm);
    fflush(fAsm);

    PicProgLdLen = PicProgWriteP - BeginOfPLCCycle;
    //dbp("%ld - %ld = %ld", PicProgWriteP, BeginOfPLCCycle, PicProgLdLen);

    if(ShowMessage) {
        char str[MAX_PATH + 500];

        if(Prog.cycleTime) {
            double CycleDeviation = 1e2 * (1e6 * plcTmr.TCycle - Prog.cycleTime) / Prog.cycleTime;
            if(CycleDeviation > 1.0) {
                THROW_COMPILER_EXCEPTION_FMT(_("%sPLC cycle deviation is %.3f %%%% !"), (CycleDeviation > 5.0) ? "" : " ", CycleDeviation);
            }
        }

        sprintf(str,
                _("Compile successful; wrote IHEX for PIC16 to '%s'.\r\n\r\n"
                  "Configuration word (fuses) has been set for crystal oscillator, BOD "
                  "enabled, LVP disabled, PWRT enabled, all code protection off."),
                outFile,
                PicProgWriteP,
                Prog.mcu->flashWords,
                (100 * PicProgWriteP) / Prog.mcu->flashWords);

        char str2[MAX_PATH + 500];
        sprintf(str2,
                _("Used %d/%d words of program flash (chip %d%% full)."),
                PicProgWriteP,
                Prog.mcu->flashWords,
                (100 * PicProgWriteP) / Prog.mcu->flashWords);

        char str3[MAX_PATH + 500];
        sprintf(str3, _("Used %d/%d byte of RAM (chip %d%% full)."), UsedRAM(), McuRAM(), (100 * UsedRAM()) / McuRAM());

        char str4[MAX_PATH + 500];
        sprintf(str4, "%s\r\n\r\n%s\r\n%s", str, str2, str3);

        if(PicProgWriteP > Prog.mcu->flashWords) {
            CompileSuccessfulMessage(str4, MB_ICONWARNING);
            CompileSuccessfulMessage(str2, MB_ICONERROR);
        } else if(UsedRAM() > McuRAM()) {
            CompileSuccessfulMessage(str4, MB_ICONWARNING);
            CompileSuccessfulMessage(str3, MB_ICONERROR);
        } else
            CompileSuccessfulMessage(str4);
    }
    return true;
}

void CompilePic16(const char *outFile)
{
    if((Prog.mcu->core == BaselineCore12bit) || //
        McuAs(" PIC12F629 ")          || //
        McuAs(" PIC12F675 ")          //
    ) {
        if(Prog.cycleTimer > 0) {
            THROW_COMPILER_EXCEPTION("Select Timer 0 in menu 'Settings -> MCU parameters'!");
            return;
        }
    }
    bool b = _CompilePic16(outFile, 0); // 1) calc LD length approximately
    if(b)
        _CompilePic16(outFile, 1); // 2) recalc timer prescaler and value
}
