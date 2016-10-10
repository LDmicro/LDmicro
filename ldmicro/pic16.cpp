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

#define ASM_LABEL 1   //   0 - no labels
                      // * 1 - only if GOTO or CALL operations need a label
                      //   2 - always, all line is labeled

#define AUTO_BANKING  //++
#ifdef AUTO_BANKING
    //#define ASM_COMMENT_BANK //-
    //#define TEST_AUTO_BANKING //--
#endif

//http://www.piclist.com/techref/microchip/pages.htm
#define AUTO_PAGING   //++
#ifdef AUTO_PAGING
    //#define ASM_COMMENT_PAGE //-
    #define MOVE_TO_PAGE_0 //++
#endif

//#define ASM_BANKSEL //--
//#define ASM_PAGESEL //--

#define USE_TIMER0_AS_LADDER_CYCLE // Timer1 as PLC Cycle sourse is obsolete
#ifdef USE_TIMER0_AS_LADDER_CYCLE
    #ifndef AUTO_BANKING
        #error AUTO_BANKING need!
    #endif
#endif

//-----------------------------------------------------------------------------
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

#include "ldmicro.h"
#include "intcode.h"
#include "bits.h"

static FILE *f;
static FILE *fAsm;

#define fCompileError( f1, f2 ) \
            fflush(f2); \
            fclose(f2); \
            fflush(f1); \
            fclose(f1); \
            CompileError();

/* moved to ldmicro.h
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
*/
#define DEST_F 1
#define DEST_W 0

#define STATUS_IPR  7
#define STATUS_RP1  6
#define STATUS_RP0  5
#define STATUS_TO   4
#define STATUS_PD   3
#define STATUS_Z    2
#define STATUS_DC   1
#define STATUS_C    0

/* moved to ldmicro.h
typedef struct Pic16InstructionTag {
    Pic16Op     op;
    DWORD       arg1;
    DWORD       arg2;
} Pic16Instruction;
*/
#define MAX_PROGRAM_LEN 128*1024
static PicAvrInstruction PicProg[MAX_PROGRAM_LEN];
static DWORD PicProgWriteP;

static int IntPcNow = -INT_MAX; //must be static

// Scratch variables, for temporaries
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
static DWORD EepromHighByte;
static DWORD EepromHighByteWaitingAddr;
static int EepromHighByteWaitingBit;

// Subroutines to do multiply/divide
static DWORD MultiplyRoutineAddress8;
static DWORD MultiplyRoutineAddress;
static DWORD MultiplyRoutineAddress24;
static DWORD MultiplyRoutineAddress32;
static DWORD DivideRoutineAddress8;
static DWORD DivideRoutineAddress;
static DWORD DivideRoutineAddress24;
static DWORD DivideRoutineAddress32;
static BOOL MultiplyNeeded;
static BOOL DivideNeeded;

// For yet unresolved references in jumps
static DWORD FwdAddrCount;

// As I start to support the paging; it is sometimes necessary to pick
// out the high vs. low portions of the address, so that the high portion
// goes in PCLATH, and the low portion is just used for the jump.
#define FWD_LO(x)    ((x) | 0x20000000)
#define FWD_HI(x)    ((x) | 0x40000000)
#define FWD(x)       ((x) | 0x80000000)
#define IS_FWD(x)    ((x) & (FWD(0) | FWD_HI(0) | FWD_LO(0)))

#define NOTDEF(x)       ((x) | 0x80000000)
#define MULTYDEF(x)     ((x) | 0x40000000)
#define IS_NOTDEF(x)    ((x) & NOTDEF(0))
#define IS_MULTYDEF(x)  ((x) & MULTYDEF(0))
#define IS_UNDEF(x)     ((x) & (NOTDEF(0) | MULTYDEF(0)))

// Some useful registers, which I think are mostly in the same place on
// all the PIC16... devices.

// Core Registers
#define REG_INDF      0x00
#define REG_PCL       0x02
#define REG_STATUS    0x03
#define REG_FSR       0x04
#define REG_PCLATH    0x0a
#define REG_INTCON    0x0b
#define     T0IE      BIT5
#define     T0IF      BIT2

// Bank Select Register instead REG_STATUS(STATUS_RP1,STATUS_RP0)
#define REG_BSR       0x08
#define     BSR4      BIT4
#define     BSR3      BIT3
#define     BSR2      BIT2
#define     BSR1      BIT1
#define     BSR0      BIT0


// These move around from device to device.
// 0 means not defined(error!) or not exist in MCU.
// EEPROM Registers
static DWORD REG_EECON1  = 0;
static DWORD REG_EECON2  = 0;
static DWORD REG_EEDATA  = 0;
static DWORD REG_EEADR   = 0;
static DWORD REG_EEDATL  = 0;
static DWORD REG_EEDATH  = 0;
static DWORD REG_EEADRL  = 0;
static DWORD REG_EEADRH  = 0;

//Analog Select Register
static DWORD REG_ANSEL   = 0;
static DWORD REG_ANSELH  = 0;

static DWORD REG_ANSELA  = 0;
static DWORD REG_ANSELB  = 0;
static DWORD REG_ANSELC  = 0;
static DWORD REG_ANSELD  = 0;
static DWORD REG_ANSELE  = 0;
static DWORD REG_ANSELF  = 0;
static DWORD REG_ANSELG  = 0;

//
static DWORD REG_PIR1    = 0; // 0x0c
#define          RCIF      BIT5
#define          CCP1IF    BIT2
static DWORD REG_PIE1    = 0; // 0x8c

//
static DWORD REG_TMR1L   = 0; // 0x0e
static DWORD REG_TMR1H   = 0; // 0x0f
static DWORD REG_T1CON   = 0; // 0x10
#define          TMR1ON    BIT0
#define          T1CKPS0   BIT4

static DWORD REG_T1GCON  = 0; //
static DWORD REG_CCPR1L  = 0; // 0x15
static DWORD REG_CCPR1H  = 0; // 0x16
static DWORD REG_CCP1CON = 0; // 0x17
static DWORD REG_CMCON   = 0; // 0x1f
static DWORD REG_VRCON   = 0; // 0x9f

//USART
static DWORD REG_TXSTA   = 0; // 0x98
static DWORD REG_RCSTA   = 0; // 0x18
static DWORD REG_SPBRG   = 0; // 0x99
static DWORD REG_TXREG   = 0; // 0x19
static DWORD REG_RCREG   = 0; // 0x1a

//ADC
static DWORD REG_ADRESH  = 0; // 0x1e
static DWORD REG_ADRESL  = 0; // 0x9e
static DWORD REG_ADCON0  = 0; // 0x1f
static DWORD REG_ADCON1  = 0; // 0x9f

//PWM Timer2
static DWORD REG_T2CON   = 0; // 0x12
static DWORD REG_CCPR2L  = 0; // 0x1b // Pulse Width
static DWORD REG_CCP2CON = 0; // 0x1d
#define          DC2B0     BIT4
#define          DC2B1     BIT5
static DWORD REG_PR2     = 0; // 0x92 // Period

//
static DWORD REG_TMR0       = 0; //0x01
static DWORD REG_OPTION_REG = 0; //0x81 or 0x181 //0x95
#define          _RBPU        BIT7
#define          T0CS         BIT5
#define          PSA          BIT3

static int IntPc;

static void CompileFromIntermediate(BOOL topLevel);

//-----------------------------------------------------------------------------
// A convenience function, whether we are using a particular MCU.
//-----------------------------------------------------------------------------
static BOOL McuIs(char *str)
{
    return strcmp(Prog.mcu->mcuName, str) == 0;
}

BOOL McuAs(char *str)
{
    return strstr(Prog.mcu->mcuName, str) != NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static void discoverName(int addrAt, char *arg1s, char *arg1comm)
{

    arg1s[0] = '\0';

    arg1comm[0] = '\0';
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
static DWORD Bank(DWORD reg)
{
    if(IS_NOTDEF(reg)) reg &= ~(NOTDEF(0));
    if(IS_MULTYDEF(reg)) reg &= ~(MULTYDEF(0));
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        if(reg & ~0x0FFF) ooops("0x%X", reg)
        reg &= 0x0F80;
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        if(reg & ~0x01FF) ooops("0x%X", reg)
        reg &= 0x0180;
    } else if(Prog.mcu->core == BaselineCore12bit) {
        if(reg & ~0x007F) ooops("0x%X", reg)
        reg &= 0x0000;
    } else oops();
    return reg;
}

//-----------------------------------------------------------------------------
static DWORD BankMask(void)
{
    DWORD reg;
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        reg = 0x0F80;
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        reg = 0x0180;
    } else if(Prog.mcu->core == BaselineCore12bit) {
        reg = 0x0000;
    } else oops();
    return reg;
}

//-----------------------------------------------------------------------------
static int IsCoreRegister(DWORD reg)
{
    reg &= ~Bank(reg); // Clear Bank
    if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
        switch(reg) {
            case REG_INDF  :
            case REG_PCL   :
            case REG_STATUS:
            case REG_FSR   :
            case REG_PCLATH:
            case REG_INTCON:
            case REG_BSR:
                return 1; // in all banks same, dont need to change banks
            default:
                return 0;
        }
    } else if(Prog.mcu->core == MidrangeCore14bit) {
        switch(reg) {
            case REG_INDF  :
            case REG_PCL   :
            case REG_STATUS:
            case REG_FSR   :
            case REG_PCLATH:
            case REG_INTCON:
                return 1; // in all banks same, dont need to change banks
            default:
                return 0;
        }
    } else if(Prog.mcu->core == BaselineCore12bit) {
        switch(reg) {
            case REG_INDF  :
            case REG_PCL   :
            case REG_STATUS:
            case REG_FSR   :
            case REG_INTCON:
                return 1; // in all banks same, dont need to change banks
            default:
                return 0;
        }
    } else oops();
    return 0;
}

//-----------------------------------------------------------------------------
#define IS_SKIP        2
#define IS_BANK        1
#define IS_ANY_BANK    0
#define IS_RETS       -1
#define IS_PAGE       -2
#define IS_GOTO       -2
#define IS_CALL       -3

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
        case OP_TRIS:
            return IS_BANK; // can need to change bank
        case OP_CLRWDT:
        case OP_MOVLW:
        case OP_MOVLB:
        case OP_MOVLP:
        case OP_NOP_:
        case OP_COMMENT_:
        case OP_SUBLW:
        case OP_IORLW:
        case OP_OPTION:
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
            ooops("OP_%d", op);
            return 0;
    }
}

//-----------------------------------------------------------------------------
// And use macro for bugtracking
#define Instruction(...) _Instruction(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)

//-----------------------------------------------------------------------------
// Store an instruction at the next spot in program memory.  Error condition
// if this spot is already filled. We don't actually assemble to binary yet;
// there may be references to resolve.
//-----------------------------------------------------------------------------
static void _Instruction(int l, char *f, char *args, PicOp op, DWORD arg1, DWORD arg2, char *comment)
{
    if(PicProg[PicProgWriteP].opPic != OP_VACANT_) oops();

    if(op == OP_COMMENT_INT){
        if(comment) {
            if(strlen(PicProg[PicProgWriteP].commentInt))
                strncat(PicProg[PicProgWriteP].commentInt, "\n    ; ", MAX_COMMENT_LEN);
            strncat(PicProg[PicProgWriteP].commentInt, comment, MAX_COMMENT_LEN);
        }
        return;
    }
    PicProg[PicProgWriteP].arg1orig = arg1; // arg1 can be changed by bank or page corretion;
    //

    PicProg[PicProgWriteP].opPic = op;
    PicProg[PicProgWriteP].arg1 = arg1;
    PicProg[PicProgWriteP].arg2 = arg2;

    if(args) {
        if(strlen(PicProg[PicProgWriteP].commentAsm))
             strncat(PicProg[PicProgWriteP].commentAsm, " ; ", MAX_COMMENT_LEN);
        strncat(PicProg[PicProgWriteP].commentAsm, "(", MAX_COMMENT_LEN);
        strncat(PicProg[PicProgWriteP].commentAsm, args, MAX_COMMENT_LEN);
        strncat(PicProg[PicProgWriteP].commentAsm, ")", MAX_COMMENT_LEN);
    }
    if(comment) {
        if(strlen(PicProg[PicProgWriteP].commentAsm))
             strncat(PicProg[PicProgWriteP].commentAsm, " ; ", MAX_COMMENT_LEN);
        strncat(PicProg[PicProgWriteP].commentAsm, comment, MAX_COMMENT_LEN);
    }
    PicProg[PicProgWriteP].rung = rungNow;
    PicProg[PicProgWriteP].IntPc = IntPcNow;
    if(f) strncpy(PicProg[PicProgWriteP].f, f, MAX_PATH);
    PicProg[PicProgWriteP].l = l;
    PicProgWriteP++;
}

static void _Instruction(int l, char *f, char *args, PicOp op, DWORD arg1, DWORD arg2)
{
    _Instruction(l, f, args, op, arg1, arg2, NULL);
}

static void _Instruction(int l, char *f, char *args, PicOp op, DWORD arg1)
{
    _Instruction(l, f, args, op, arg1, 0, NULL);
}

static void _Instruction(int l, char *f, char *args, PicOp op)
{
    _Instruction(l, f, args, op, 0, 0, NULL);
}

//-----------------------------------------------------------------------------
static void _SetInstruction(int l, char *f, char *args, DWORD addr, PicOp op, DWORD arg1, DWORD arg2, char *comment)
//for setiing interrupt vector, page correcting, etc
{
    DWORD savePicProgWriteP = PicProgWriteP;
    PicProgWriteP = addr;
    PicProg[PicProgWriteP].opPic = OP_VACANT_;

    if(comment) {
        if(strlen(PicProg[PicProgWriteP].commentAsm))
             strncat(PicProg[PicProgWriteP].commentAsm, " ; ", MAX_COMMENT_LEN);
        strncat(PicProg[PicProgWriteP].commentAsm, comment, MAX_COMMENT_LEN);
    }

    _Instruction(l, f, args, op, arg1, arg2);
    PicProgWriteP = savePicProgWriteP;
}

static void _SetInstruction(int l, char *f, char *args, DWORD addr, PicOp op, DWORD arg1, DWORD arg2)
{
   _SetInstruction(l, f, args, addr, op, arg1, arg2, NULL);
}

static void _SetInstruction(int l, char *f, char *args, DWORD addr, PicOp op, DWORD arg1)
{
   _SetInstruction(l, f, args, addr, op, arg1, 0, NULL);
}

static void _SetInstruction(int l, char *f, char *args, DWORD addr, PicOp op, DWORD arg1, char *comment)
{
   _SetInstruction(l, f, args, addr, op, arg1, 0, comment);
}

#define SetInstruction(...) _SetInstruction(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)

//-----------------------------------------------------------------------------
// printf-like comment function
//-----------------------------------------------------------------------------
static void Comment(char *str, ...)
{
  if(asm_comment_level) {
    if(strlen(str)>=MAX_COMMENT_LEN)
      str[MAX_COMMENT_LEN-1]='\0';
    va_list f;
    char buf[MAX_COMMENT_LEN];
    va_start(f, str);
    vsprintf(buf, str, f);
    Instruction(OP_COMMENT_INT, 0, 0, buf);
  }
}

//-----------------------------------------------------------------------------
// Allocate a unique descriptor for a forward reference. Later that forward
// reference gets assigned to an absolute address, and we can go back and
// fix up the reference.
//-----------------------------------------------------------------------------
static DWORD AllocFwdAddr(void)
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
    if(!(addr & 0x80000000)) oops();

    BOOL seen = FALSE;
    DWORD i;
    for(i = 0; i < PicProgWriteP; i++) {
        if(PicProg[i].arg1 == addr) {
            #ifndef AUTO_PAGING
            // Insist that they be in the same page, but otherwise assume
            // that PCLATH has already been set up appropriately.
            if((i >> 11) != (PicProgWriteP >> 11)) {
                Error(_("Internal error relating to PIC paging; make program "
                    "smaller or reshuffle it."));
                fCompileError(f, fAsm);
            }
            #endif
            PicProg[i].arg1 = PicProgWriteP;
            seen = TRUE;
        } else if(PicProg[i].arg1 == FWD_LO(addr)) {
            PicProg[i].arg1 = PicProgWriteP;// & 0x7ff); // Full address are required for generating the proper labels in asm text.
            seen = TRUE;
        } else if(PicProg[i].arg1 == FWD_HI(addr)) {
            PicProg[i].arg1 = (PicProgWriteP >> 8);
        }
    }
    if(!seen) {
         Error("FwdAddrIsNow not found!!! 0x%X", addr);
         strcpy(PicProg[i].commentInt, "FwdAddrIsNow not found!!!");
         ooops("0x%X=%d",addr,addr);
    }
}

//-----------------------------------------------------------------------------
static void BankCorrect(DWORD addr, int nAdd, int nSkip, DWORD bankNew)
{
    DWORD i;
    for(i = addr-nSkip; i <= addr+nAdd; i++) {
        PicProg[i].BANK = bankNew;
    }
    i = addr-nSkip-1;
    while(i>0) {
        if(((IsOperation(PicProg[i].opPic) >= IS_BANK) && IsCoreRegister(PicProg[i].arg1))
        ||  (IsOperation(PicProg[i].opPic) == IS_ANY_BANK)
        ) {
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
    PicProgWriteP = addr-nSkip;
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
            oops();
        }
    } else oops();

    if(n != nAdd)
        ooops("%d %d", n, nAdd);

    PicProgWriteP = savePicProgWriteP + nAdd;

    if(n)
        BankCorrect(addr, nAdd, nSkip, bankNew);

    return nAdd;
}

static DWORD MaxBank;

static DWORD CalcMaxBank()
{
    DWORD MaxBank = 0;
    DWORD i;
    for(i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) >= IS_BANK)
            MaxBank = max(MaxBank, Bank(PicProg[i].arg1));
    }
    return MaxBank;
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
            oops();
        }
    } else oops();
    return n;
}

#ifdef AUTO_BANKING
static DWORD BankCorrection_(DWORD addr, DWORD bank, int is_call)
{
    int corrected = 0;
    DWORD i, j;
    int nAdd;
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
    while(i < PicProgWriteP) {
        if(IS_NOTDEF(PicProg[i].BANK)) {
                PicProg[i].BANK = PicProg[i-1].BANK;
        }
        if(IsOperation(PicProg[i].opPic) >= IS_BANK) {
            nAdd = 0;
            if(!IsCoreRegister(PicProg[i].arg1)) {
                if(IS_UNDEF(PicProg[i].BANK)
                ||  IS_UNDEF(PicProg[i-1].BANK)
                ||  (PicProg[i-1].BANK != Bank(PicProg[i].arg1))) {
                    if(IS_UNDEF(PicProg[i].BANK)
                    ||  IS_UNDEF(PicProg[i-1].BANK)) {
                        BB = PicProg[i].arg1 ^ BankMask();
                    } else {
                        BB = PicProg[i-1].BANK;
                    }
                    arg1 = PicProg[i].arg1;
                    nAdd = BankSelectCheck(BB, arg1);
                }
            }
            if(nAdd) {
                int nSkip = 0;
                DWORD ii = i; // address where we doing insert
                while((ii > 0) && (IsOperation(PicProg[ii-1].opPic) == IS_SKIP)
                || (PicProg[ii-1].opPic == OP_MOVLW) // for asm compat
                ) {
                    // can't insert op betwen IS_SKIP and any opPic
                    ii--;
                    nSkip++;
                }
                for(j = 0; j < PicProgWriteP; j++) {
                    if(IsOperation(PicProg[j].opPic) <= IS_PAGE)
                        if(PicProg[j].arg1 > ii) // qqq
                            PicProg[j].arg1 += nAdd; // Correcting target addresses.
                }
                for(j = PicProgWriteP-1; j>=ii; j--) {
                    // prepare a place for inserting bank correction operations
                    memcpy(&PicProg[j+nAdd], &PicProg[j], sizeof(PicProg[0]));
                }
                for(j = ii; j<(ii+nAdd); j++) {
                    PicProg[j].opPic = OP_VACANT_;
                    sprintf(PicProg[j].commentAsm, " BS(0x%8X,0x%8X)", BB, arg1);
//                  sprintf(PicProg[j].commentInt, "");
                }
                int n = 0;
                n = BankSelect(i, nAdd, nSkip, BB, arg1);
                if(nAdd != n) ooops("nAdd=%d n=%d", nAdd, n);
                corrected++;
                break;
            }
        } else {
        //
        }
        i++;
    }
  if(corrected && (corrected<20)) goto doBankCorrection;

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
                PicProg[i].BANK= Bank(PicProg[i].arg1);
        }

        if((IsOperation(PicProg[i].opPic) == IS_RETS) && is_call) {
            break;
            //return PicProg[i].BANK;
        }
    }

    // Set target bank in flow .
    for(i = addr; i < PicProgWriteP; i++) {
        if(((IsOperation(PicProg[i].opPic) >= IS_BANK) && (IsCoreRegister(PicProg[i].arg1)))
        || (IsOperation(PicProg[i].opPic) <= IS_ANY_BANK) ) {
            if(IS_NOTDEF(PicProg[i].BANK)) {
                PicProg[i].BANK = PicProg[i-1].BANK;
            }
        }

        if((IsOperation(PicProg[i].opPic) == IS_RETS) && is_call) {
            break;
            //return PicProg[i].BANK;
        }
    }

    for(i = addr; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) == IS_CALL) {
            if(PicProg[i].arg1 >= PicProgWriteP) oops();
            if(PicProg[i].arg1 < 0) oops();

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
            if(PicProg[i].arg1 >= PicProgWriteP) oops();
            if(PicProg[i].arg1 < 0) oops();

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
    DWORD i;
    for(i = 1; i < PicProgWriteP; i++) {
        if((IsOperation(PicProg[i].opPic) >= IS_BANK)
        && (IsOperation(PicProg[i-1].opPic) == IS_SKIP)) {
            if((!IsCoreRegister(PicProg[i].arg1))
            && (!IsCoreRegister(PicProg[i-1].arg1))) {
                if(Bank(PicProg[i].arg1) != Bank(PicProg[i-1].arg1)) {
                    dbp("%3d 0x%X", i-1, PicProg[i-1].opPic, PicProg[i-1].arg1);
                    dbp("%3d 0x%X", i, PicProg[i].opPic, PicProg[i].arg1);
                    ooops("Bank select error!");
                }
            }
        }
    }

    // Marking bank as indeterminate.
    for(i = 0; i < PicProgWriteP; i++) {
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

    for(i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) >= IS_BANK)
            PicProg[i].arg1 &= ~Bank(PicProg[i].arg1);
    }
}
#endif

//---------------------------------------------------------------------------
static void PagePreSet()
{
    DWORD i;
    for(i = 0; i < PicProgWriteP; i++) {
        // mark the PCLATH as not setted
        PicProg[i].PCLATH = FWD(0);
    }
    PicProg[0].PCLATH = 0; // PCLATH is 0 after reset
    //
    for(i = 0; i < PicProgWriteP; i++) {
      // direct set the PCLATH
      if(Prog.mcu->core == BaselineCore12bit) {
        // TODO
      } else {
        if((PicProg[i].opPic == OP_CLRF)
        && (PicProg[i].arg1 == REG_PCLATH)) {
            PicProg[i].PCLATH = 0;
        } else
        if(PicProg[i].opPic == OP_MOVLP) {
            PicProg[i].PCLATH = PicProg[i].arg1;
        } else
        if((PicProg[i].opPic == OP_BSF)
        && (PicProg[i].arg1 == REG_PCLATH)) {
            PicProg[i].PCLATH &= ~FWD(0);
            PicProg[i].PCLATH |= 1 << PicProg[i].arg2;
            if( ((PicProg[i-1].opPic == OP_BCF) || (PicProg[i-1].opPic == OP_BSF))
            && (PicProg[i-1].arg1 == REG_PCLATH)) {
              PicProg[i-1].PCLATH &= ~FWD(0);
              PicProg[i-1].PCLATH |= 1 << PicProg[i].arg2;
              PicProg[i].PCLATH |= PicProg[i-1].PCLATH;
            }
        } else
        if((PicProg[i].opPic == OP_BCF)
        && (PicProg[i].arg1 == REG_PCLATH)) {
            PicProg[i].PCLATH &= ~FWD(0);
            PicProg[i].PCLATH &= ~(1 << PicProg[i].arg2);
            if( ((PicProg[i-1].opPic == OP_BCF) || (PicProg[i-1].opPic == OP_BSF))
            && (PicProg[i-1].arg1 == REG_PCLATH)) {
              PicProg[i-1].PCLATH &= ~FWD(0);
              PicProg[i-1].PCLATH &= ~(1 << PicProg[i].arg2);
              PicProg[i].PCLATH |= PicProg[i-1].PCLATH;
            }
        } else
        if((PicProg[i].opPic == OP_MOVWF)
        && (PicProg[i].arg1 == REG_PCLATH)) {
            if(PicProg[i-1].opPic == OP_MOVLW) {
                PicProg[i-1].PCLATH = PicProg[i-1].arg1;
                PicProg[i].PCLATH = PicProg[i-1].arg1;
            } else {
                Error("PagePreSet() error at addr 0x%X", i);
                oops();
            }
        }
      }
    }
    //
    for(i = 0; i < PicProgWriteP; i++) {
        // pass thru the PCLATH
        if(IS_FWD(PicProg[i].PCLATH)) {
            PicProg[i].PCLATH = PicProg[i-1].PCLATH;
        }
    }

    // Marking the interrupt vector operation as the multi entry point.
    if(Prog.mcu->core != BaselineCore12bit) {
      PicProg[4].PCLATH = FWD(0);
    }
}

#ifdef AUTO_PAGING
//-----------------------------------------------------------------------------
static int PageSelectCheck(DWORD PCLATH, DWORD PCLATHnew)
{
  int n = 0;
  if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
      if((PCLATH >> 3) != (PCLATHnew >> 3))
          n++;
  } else if(Prog.mcu->core == MidrangeCore14bit) {
      if((PCLATH ^ PCLATHnew) & (1 << BIT3))
          n++;
      if((PCLATH ^ PCLATHnew) & (1 << BIT4))
          n++;
  } else oops();
  return n;
}

//---------------------------------------------------------------------------
static int PageSelect(DWORD addr, DWORD *PCLATH, DWORD PCLATHnew)
{
  int n = 0;
  if(Prog.mcu->core == EnhancedMidrangeCore14bit) {
      if((*PCLATH >> 3) != (PCLATHnew >> 3)) {
          SetInstruction(addr, OP_MOVLP, PCLATHnew, "PageSel2");
          *PCLATH = PCLATHnew;
          n++;
      }
  } else if(Prog.mcu->core == MidrangeCore14bit) {
      if(((*PCLATH) ^ PCLATHnew) & (1 << BIT3)) {
          if(PCLATHnew & (1 << BIT3)) {
              SetInstruction(addr+n, OP_BSF, REG_PCLATH, BIT3, "_^ PageSel3");
              *PCLATH |= (1 << BIT3);
          } else {
              SetInstruction(addr+n, OP_BCF, REG_PCLATH, BIT3, "_v PageSel4");
              *PCLATH &= ~(1 << BIT3);
          }
          n++;
      }
      if(((*PCLATH) ^ PCLATHnew) & (1 << BIT4)) {
          if(PCLATHnew & (1 << BIT4)) {
              SetInstruction(addr+n, OP_BSF, REG_PCLATH, BIT4, "^_ PageSel5");
              *PCLATH |= (1 << BIT4);
          } else {
              SetInstruction(addr+n, OP_BCF, REG_PCLATH, BIT4, "v_ PageSel6");
              *PCLATH &= ~(1 << BIT4);
          }
          n++;
      }
  } else oops();

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
    static int PageSelLevel = 300;

    BOOL corrected;
    DWORD i, j;
  doPageCorrection:
    corrected = FALSE;
    PagePreSet();
    i = 0;
    while(i < PicProgWriteP) {
        if(IsOperation(PicProg[i].opPic) <= IS_PAGE) {
            if((PicProg[i].arg1 >> 11) != (PicProg[i].PCLATH >> 3)) {
            //  ^target addr^              ^current PCLATH^
                DWORD PCLATHnow = PicProg[i].PCLATH;
                DWORD PicProgArg1 = PicProg[i].arg1;
                corrected = TRUE;
                // need to correct PCLATH page
                int n1, n2, n3, m3, n4; // need n opPic operations for page correcting
                n1 = PageSelectCheck(PCLATHnow, (PicProgArg1   ) >> 8);
                // we need n1 op's for correcting, but
                n2 = PageSelectCheck(PCLATHnow, (PicProgArg1+n1) >> 8);
                // we can need more if after first correction we cross the page boundary
                n3 = PageSelectCheck(PCLATHnow, (PicProgArg1+n2) >> 8);
                m3 = max(n1,max(n2,n3));

                int nSkip = 0;
                DWORD ii = i; // address where we doing insert
                while((ii > 0) && (IsOperation(PicProg[ii-1].opPic) == IS_SKIP)) {
                    // can't insert op betwen IS_SKIP and any opPic
                    ii--;
                    nSkip++;
                }
                for(j = 0; j < PicProgWriteP; j++) {
                    if(IsOperation(PicProg[j].opPic) <= IS_PAGE)
                        if(PicProg[j].arg1 > ii)
                            PicProg[j].arg1 += m3; // Correcting all target addresses!!!
                }
                if((PicProgArg1 >> 8) != (PicProg[i].arg1 >> 8)) {
                    PicProgArg1 = PicProg[i].arg1;
                }
                if(IsOperation(PicProg[i].opPic) == IS_CALL) {
                    PicProg[PicProgArg1].PCLATH = PicProgArg1 >> 8;
                }
                for(j = PicProgWriteP-1; j>=ii; j--) {
                    // prepare a place for inserting page correction operations
                    memcpy(&PicProg[j+m3], &PicProg[j], sizeof(PicProg[0]));
                }
                for(j = ii; j<(ii+m3); j++) {
                    PicProg[j].opPic = OP_NOP_;
                    PicProg[j].arg1 = 0;
                    PicProg[j].arg2 = 0;
                    sprintf(PicProg[j].commentAsm, " PS(0x%02X,0x%02X)", PCLATHnow, PicProgArg1 >> 8);
                }
                // select new page
                n4 = PageSelect(ii, &PCLATHnow, PicProgArg1 >> 8);
                PageCorrect(ii, n4+nSkip, PCLATHnow);

                PicProgWriteP += m3; // upsize array length
                if(!((n1 == n2) && (n2 == n3) && (n3 == n4))) {
                    //oops()
                }
                break;
            }
        }
        i++;
    }
//  if((--PageSelLevel)>0) // for debaging
    if(corrected) goto doPageCorrection;

    for(i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) <= IS_PAGE) {
            if( ((PicProg[i].arg1 >> 11) != (PicProg[i].PCLATH >> 3)) ) {
                // ^target addr^             ^current PCLATH^
                oops();
            }
        }
    }
}

//-----------------------------------------------------------------------------
static void AddrCheckForErrorsPostCompile2()
{
    DWORD i;
    for(i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) <= IS_PAGE) {
            if((PicProg[i].arg1 >> 11) != (PicProg[i].PCLATH >> 3)) {
            //  ^target addr^              ^current PCLATH^
                Error("Page Error.");
                oops()
            }
        }
    }
}
#endif

//-----------------------------------------------------------------------------
static void AddrCheckForErrorsPostCompile()
{
    DWORD i;
    for(i = 0; i < PicProgWriteP; i++) {
        if(IS_FWD(PicProg[i].arg1)) {
            Error("Every AllocFwdAddr needs FwdAddrIsNow.");
            fCompileError(f, fAsm);
        }
    }
}

//-----------------------------------------------------------------------------
static void BankCheckForErrorsPostCompile()
{
    DWORD i;
    for(i = 1; i < PicProgWriteP; i++) {
        if((IsOperation(PicProg[i-1].opPic) == IS_SKIP)
        && (IsOperation(PicProg[i  ].opPic) == IS_BANK)) {
//      && (IsOperation(PicProg[i  ].opPic) <= IS_SKIP)) {
            if(Bank(PicProg[i-1].arg1orig) ^ Bank(PicProg[i].arg1orig)) {
                fprintf(fAsm, "    ; Bank Error.\n");
                fprintf(fAsm, "    ; i=0x%04x op=%d arg1=%d arg2=%d bank=%x arg1orig=%d commentInt=%s commentAsm=%s arg1name=%s arg2name=%s rung=%d IntPc=%d l=%d file=%s\n",
                    i-1,
                    PicProg[i-1].opPic,
                    PicProg[i-1].arg1,
                    PicProg[i-1].arg2,
                    PicProg[i-1].BANK,
                    PicProg[i-1].arg1orig,
                    PicProg[i-1].commentInt,
                    PicProg[i-1].commentAsm,
                    PicProg[i-1].arg1name,
                    PicProg[i-1].arg2name,
                    PicProg[i-1].rung,
                    PicProg[i-1].IntPc,
                    PicProg[i-1].l,
                    PicProg[i-1].f
                );
                fprintf(fAsm, "    ; i=0x%04x op=%d arg1=%d arg2=%d bank=%x arg1orig=%d commentInt=%s commentAsm=%s arg1name=%s arg2name=%s rung=%d IntPc=%d l=%d file=%s\n",
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
                    PicProg[i].f
                );
                fCompileError(f, fAsm);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Given an opcode and its operands, assemble the 14-bit instruction for the
// PIC. Check that the operands do not have more bits set than is meaningful;
// it is an internal error if they do.
//-----------------------------------------------------------------------------
static DWORD Assemble(DWORD addrAt, PicOp op, DWORD arg1, DWORD arg2)
//14-Bit Opcode
{
    char arg1s[1024];
    char arg1comm[1024];
    PicAvrInstruction *PicInstr = &PicProg[addrAt];
    IntOp *a = &IntCode[PicInstr->IntPc];
    sprintf(arg1s, "0x%X", arg1);
    arg1comm[0] = '\0';
#define CHECK(v, bits) if((v) != ((v) & ((1 << (bits))-1))) \
    ooops("v=%d=0x%X ((1 << (%d))-1)=%d\nat %d in %s %s\nat %d in %s", \
       (v), (v), (bits), ((1 << (bits))-1), \
       PicInstr->l, PicInstr->f, \
       a->name1, a->l, a->f)
    switch(op) {
        case OP_ADDWF:
            CHECK(arg2, 1); CHECK(arg1, 7);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0700 | (arg2 << 7) | arg1;

        case OP_ANDWF:
            CHECK(arg2, 1); CHECK(arg1, 7);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0500 | (arg2 << 7) | arg1;

        case OP_BSF:
            CHECK(arg2, 3); CHECK(arg1, 7);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x1400 | (arg2 << 7) | arg1;

        case OP_BCF:
            CHECK(arg2, 3); CHECK(arg1, 7);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x1000 | (arg2 << 7) | arg1;

        case OP_BTFSC:
            CHECK(arg2, 3); CHECK(arg1, 7);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x1800 | (arg2 << 7) | arg1;

        case OP_BTFSS:
            CHECK(arg2, 3); CHECK(arg1, 7);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x1c00 | (arg2 << 7) | arg1;

        case OP_CLRF:
            CHECK(arg1, 7); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0180 | arg1;

        case OP_CLRWDT:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x0064;

        case OP_COMF:
            CHECK(arg2, 1); CHECK(arg1, 7);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0900 | (arg2 << 7) | arg1;

        case OP_DECF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0300 | (arg2 << 7) | arg1;

        case OP_DECFSZ:
            CHECK(arg1, 7); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0b00 | (arg2 << 7) | arg1;

        case OP_GOTO: {
            // Very special case: we will assume that the PCLATH stuff has
            // been taken care of already.
            arg1 &= 0x7ff;
            CHECK(arg1, 11); CHECK(arg2, 0);
            return 0x2800 | arg1;
        }
        case OP_CALL: {
            arg1 &= 0x7ff;
            CHECK(arg1, 11); CHECK(arg2, 0);
            return 0x2000 | arg1;
        }
        case OP_INCF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0a00 | (arg2 << 7) | arg1;

        case OP_INCFSZ:
            CHECK(arg1, 7); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0f00 | (arg2 << 7) | arg1;

        case OP_IORWF:
            CHECK(arg2, 1); CHECK(arg1, 7);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0400 | (arg2 << 7) | arg1;

        case OP_MOVLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x3000 | arg1;

        case OP_MOVLB:
            CHECK(arg1, 5); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0020 | arg1;

        case OP_MOVLP:
            CHECK(arg1, 7); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x3180 | arg1;

        case OP_MOVF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0800 | (arg2 << 7) | arg1;

        case OP_MOVWF:
            CHECK(arg1, 7); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0080 | arg1;

        case OP_NOP_:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x0000;

        case OP_COMMENT_:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x0000;

        case OP_RETLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x3600 | arg1;

        case OP_RETURN:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x0008;

        case OP_RETFIE:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x0009;

        case OP_RLF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0d00 | (arg2 << 7) | arg1;

        case OP_RRF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0c00 | (arg2 << 7) | arg1;

        case OP_IORLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x3800 | arg1;

        case OP_SUBLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x1e00 | arg1;

        case OP_SUBWF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0200 | (arg2 << 7) | arg1;

        case OP_XORWF:
            CHECK(arg1, 7); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0600 | (arg2 << 7) | arg1;

        default:
            ooops("OP_%d", op);
            return 0;
    }
}

static DWORD Assemble12(DWORD addrAt, PicOp op, DWORD arg1, DWORD arg2)
//12-Bit Opcode for PIC10, PIC12
{
    char arg1s[1024];
    char arg1comm[1024];
    PicAvrInstruction *PicInstr = &PicProg[addrAt];
    IntOp *a = &IntCode[PicInstr->IntPc];
    sprintf(arg1s, "0x%X", arg1);
    arg1comm[0] = '\0';
#define CHECK(v, bits) if((v) != ((v) & ((1 << (bits))-1))) \
    ooops("v=%d=0x%X ((1 << (%d))-1)=%d\nat %d in %s %s\nat %d in %s", \
       (v), (v), (bits), ((1 << (bits))-1), \
       PicInstr->l, PicInstr->f, \
       a->name1, a->l, a->f)
    switch(op) {
        case OP_ADDWF:
            CHECK(arg2, 1); CHECK(arg1, 5);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x1C0 | (arg2 << 5) | arg1;

        case OP_ANDWF:
            CHECK(arg2, 1); CHECK(arg1, 5);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x140 | (arg2 << 5) | arg1;

        case OP_BCF:
            CHECK(arg2, 3); CHECK(arg1, 5);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x400 | (arg2 << 5) | arg1;

        case OP_BSF:
            CHECK(arg2, 3); CHECK(arg1, 5);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x500 | (arg2 << 5) | arg1;

        case OP_BTFSC:
            CHECK(arg2, 3); CHECK(arg1, 5);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x600 | (arg2 << 5) | arg1;

        case OP_BTFSS:
            CHECK(arg2, 3); CHECK(arg1, 5);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x700 | (arg2 << 5) | arg1;

        case OP_CLRF:
            CHECK(arg1, 5); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x060 | arg1;

        case OP_CLRWDT:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x004;

        case OP_COMF:
            CHECK(arg2, 1); CHECK(arg1, 5);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x240 | (arg2 << 5) | arg1;

        case OP_DECF:
            CHECK(arg1, 5); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x0C0 | (arg2 << 5) | arg1;

        case OP_DECFSZ:
            CHECK(arg1, 5); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x2C0 | (arg2 << 5) | arg1;

        case OP_GOTO: {
            // Very special case: we will assume that the PCLATH stuff has
            // been taken care of already.
            arg1 &= 0x1ff;
            CHECK(arg1, 9); CHECK(arg2, 0);
            return 0xA00 | arg1;
        }
        case OP_CALL: {
            arg1 &= 0xff;
            CHECK(arg1, 8); CHECK(arg2, 0);
            return 0x900 | arg1;
        }
        case OP_INCF:
            CHECK(arg1, 5); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x280 | (arg2 << 5) | arg1;

        case OP_INCFSZ:
            CHECK(arg1, 5); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x3C0 | (arg2 << 5) | arg1;

        case OP_IORWF:
            CHECK(arg2, 1); CHECK(arg1, 5);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x100 | (arg2 << 5) | arg1;

        case OP_MOVLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0xC00 | arg1;
/*
        case OP_MOVLB:
            CHECK(arg1, 5); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x020 | arg1;

        case OP_MOVLP:
            CHECK(arg1, 5); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x3180 | arg1;
*/
        case OP_MOVF:
            CHECK(arg1, 5); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x200 | (arg2 << 5) | arg1;

        case OP_MOVWF:
            CHECK(arg1, 5); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x020 | arg1;

        case OP_NOP_:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x000;

        case OP_COMMENT_:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x000;

        case OP_RETLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x800 | arg1;
/*
        case OP_RETURN:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x008;

        case OP_RETFIE:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x009;
*/
        case OP_RLF:
            CHECK(arg1, 5); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x340 | (arg2 << 5) | arg1;

        case OP_RRF:
            CHECK(arg1, 5); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x300 | (arg2 << 5) | arg1;

        case OP_IORLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0xD00 | arg1;
/*
        case OP_SUBLW:
            CHECK(arg1, 8); CHECK(arg2, 0);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x1e00 | arg1;
*/
        case OP_SUBWF:
            CHECK(arg1, 5); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x080 | (arg2 << 5) | arg1;

        case OP_XORWF:
            CHECK(arg1, 5); CHECK(arg2, 1);
            discoverName(addrAt, arg1s, arg1comm);
            return 0x180 | (arg2 << 5) | arg1;

        case OP_TRIS:
            CHECK(arg1, 3); CHECK(arg2, 0);
            if( !((arg1==6) || (arg1==7)) ) oops();
            discoverName(addrAt, arg1s, arg1comm);
            return 0x000 | arg1;

        case OP_OPTION:
            CHECK(arg1, 0); CHECK(arg2, 0);
            return 0x002;

        default:
            ooops("OP_%d", op);
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
    char sAsm[1024]="";

    BYTE soFar[16];
    int soFarCount = 0;
    DWORD soFarStart = 0;

    // always start from address 0
    fprintf(f, ":020000040000FA\n");

    DWORD i;
    for(i = 0; i < PicProgWriteP; i++) {
        PicProg[i].label = FALSE;
    }
    for(i = 0; i < PicProgWriteP; i++) {
        if(IsOperation(PicProg[i].opPic) <= IS_PAGE)
            PicProg[PicProg[i].arg1].label = TRUE;
    }

    for(i = 0; i < PicProgWriteP; i++) {
        DWORD w;
        if(Prog.mcu->core == BaselineCore12bit)
            w = Assemble12(i, PicProg[i].opPic, PicProg[i].arg1, PicProg[i].arg2);
//      else if(Prog.mcu->core == PIC18HighEndCore16bit)
//          w = Assemble16(i, PicProg[i].opPic, PicProg[i].arg1, PicProg[i].arg2, 1);
        else
            w = Assemble(i, PicProg[i].opPic, PicProg[i].arg1, PicProg[i].arg2);

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

        if(strlen(PicProg[i].commentInt)) {
            fprintf(fAsm, "    ; %s\n", PicProg[i].commentInt);
        }

        if(strlen(sAsm)) {
            if(IsOperation(PicProg[i].opPic) >= IS_BANK) {
              #ifdef ASM_BANKSEL
              fprintf(fAsm, "        \t banksel 0x%X\n", PicProg[i].arg1);
              #endif
            } else if(IsOperation(PicProg[i].opPic) <= IS_PAGE) {
              #ifdef ASM_PAGESEL
              fprintf(fAsm, "        \t pagesel l_%04x\n", PicProg[i].arg1);
              #endif
            }

            #if ASM_LABEL > 0
            if(PicProg[i].label || (ASM_LABEL == 2))
                if(PicProg[i].label)
                    fprintf(fAsm, "L_%04x: %s", i, sAsm);
                else
                    fprintf(fAsm, " l%04x: %s", i, sAsm);
            else
                fprintf(fAsm, "        %s",    sAsm);
            #else
                fprintf(fAsm, "        %s",    sAsm);
            #endif

            if(asm_comment_level >= 3) {
                fprintf(fAsm, "\t");
                if(1 || (prevRung != PicProg[i].rung)) {
                    fprintf(fAsm, " ; rung=%d", PicProg[i].rung+1);
                    prevRung = PicProg[i].rung;
                 } else {
                    fprintf(fAsm, " \t");
                 }
            }

            if(asm_comment_level >= 4) {
                if(1 || (prevL != PicProg[i].l)) {
                   fprintf(fAsm, " ; line %d in %s",
                       PicProg[i].l,
                       PicProg[i].f
                       );
                    prevL = PicProg[i].l;
                 }
            }

            if(asm_comment_level >= 5) {
              if((PicProg[i].IntPc >= 0) && (PicProg[i].IntPc < IntCodeLen)) {
                fprintf(fAsm, "\t");
                if(IntCode[PicProg[i].IntPc].which != INT_MAX) {
                    fprintf(fAsm, " ; ELEM_0x%X",
                        IntCode[PicProg[i].IntPc].which);
                }
                if(1 || (prevIntPcL != IntCode[PicProg[i].IntPc].l)) {
                    fprintf(fAsm, " ; line %d in %s",
                        IntCode[PicProg[i].IntPc].l,
                        IntCode[PicProg[i].IntPc].f);
                    prevIntPcL = IntCode[PicProg[i].IntPc].l;
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
            fprintf(fAsm, " ;PCLATH=0x%02X", PicProg[i].PCLATH); //ok
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
            ;;//;;//Error("op=%d=0x%X", PicProg[i].opPic, PicProg[i].opPic);
    }

    StartIhex(f);
    // Configuration words start at address 0x2007 in program memory; and the
    // hex file addresses are by bytes, not words, so we start at 0x400e.
    // There may be either 16 or 32 bits of conf word, depending on the part.
    if(McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    || McuAs("Microchip PIC16F88 ")
    || McuAs(" PIC16F884 ")
    || McuAs(" PIC16F883 ")
    || McuAs(" PIC16F882 ")
    ) {
        WriteIhex(f, 0x04);
        WriteIhex(f, 0x40); // 0x2007 * 2 = 0x400E
        WriteIhex(f, 0x0E);
        WriteIhex(f, 0x00);
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  0) & 0xff));
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  8) & 0xff));
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >> 16) & 0xff));
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >> 24) & 0xff));
    } else
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        WriteIhex(f, 0x04);
        WriteIhex(f, 0x01); // 0x8007 * 2 = 0x01000E
        WriteIhex(f, 0x00);
        WriteIhex(f, 0x0E);
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  0) & 0xff));
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  8) & 0xff));
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >> 16) & 0xff));
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >> 24) & 0xff));
    } else
    if(McuAs("Microchip PIC16F628 ")
    || McuAs("Microchip PIC16F819 ")
    || McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F876 ")
    ) {
        if(Prog.mcu->configurationWord & 0xffff0000) oops();
        WriteIhex(f, 0x02);
        WriteIhex(f, 0x40); // 0x2007 * 2 = 0x400E
        WriteIhex(f, 0x0E);
        WriteIhex(f, 0x00);
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  0) & 0xff));
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  8) & 0xff));
    } else
    if(McuAs("Microchip PIC10F200 ")
    || McuAs("Microchip PIC10F204 ")
    || McuAs("Microchip PIC10F220 ")
    ) {
        if(Prog.mcu->configurationWord & 0xffff0000) oops();
        WriteIhex(f, 0x02);
        WriteIhex(f, 0x03); // 0x01ff * 2 = 0x03FE
        WriteIhex(f, 0xFE);
        WriteIhex(f, 0x00);
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  0) & 0xff));
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  8) & 0xff));
    } else
    if(McuAs("Microchip PIC10F202 ")
    || McuAs("Microchip PIC10F206 ")
    || McuAs("Microchip PIC10F222 ")
    ) {
        if(Prog.mcu->configurationWord & 0xffff0000) oops();
        WriteIhex(f, 0x02);
        WriteIhex(f, 0x07); // 0x03ff * 2 = 0x07FE
        WriteIhex(f, 0xFE);
        WriteIhex(f, 0x00);
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  0) & 0xff));
        WriteIhex(f, BYTE((Prog.mcu->configurationWord >>  8) & 0xff));
    } else
        oops();
    FinishIhex(f);

    // end of file record
    fprintf(f, ":00000001FF\n");
}

//-----------------------------------------------------------------------------
// Generate code to write an 8-bit value to a particular register. Takes care
// of the bank switching if necessary; assumes that code is called in bank
// 0.
//-----------------------------------------------------------------------------
static void _WriteRegister(int l, char *f, char *args, DWORD reg, BYTE val, char *comment)
{
    #ifdef AUTO_BANKING
        _Instruction(l, f, args, OP_MOVLW, val, 0, comment);
        _Instruction(l, f, args, OP_MOVWF, reg, 0, comment);
    #else
    if(reg & 0x080) Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
    if(reg & 0x100) Instruction(OP_BSF, REG_STATUS, STATUS_RP1);

        _Instruction(l, f, args, OP_MOVLW, val, 0, comment);
        _Instruction(l, f, args, OP_MOVWF, (reg & 0x7f), 0, comment);

    if(reg & 0x080) Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
    if(reg & 0x100) Instruction(OP_BCF, REG_STATUS, STATUS_RP1);
    #endif
}

static void _WriteRegister(int l, char *f, char *args, DWORD reg, BYTE val)
{
    _WriteRegister(l, f, args, reg, val, NULL);
}

// And use macro for bugtracking
#define WriteRegister(...) _WriteRegister(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)
//-----------------------------------------------------------------------------
// Call a subroutine, that might be in an arbitrary page, and then put
// PCLATH back where we want it.
//-----------------------------------------------------------------------------
static void CallWithPclath(DWORD addr)
{
    #ifdef AUTO_PAGING
    Instruction(OP_CALL, addr);
    #else
    // Set up PCLATH for the jump, and then do it.
    #ifdef MOVE_TO_PAGE_0
    Instruction(OP_MOVLW, addr >> 8);
    Instruction(OP_MOVWF, REG_PCLATH);
    Instruction(OP_CALL, addr);
    #else
    Instruction(OP_MOVLW, FWD_HI(addr), 0);
    Instruction(OP_MOVWF, REG_PCLATH, 0);
    Instruction(OP_CALL, FWD_LO(addr), 0);
    #endif

    // Restore PCLATH to something appropriate for our page. (We have
    // already made fairly sure that we will never try to compile across
    // a page boundary.)
    Instruction(OP_MOVLW, (PicProgWriteP >> 8), 0);
    Instruction(OP_MOVWF, REG_PCLATH, 0);
    #endif
}

// Note that all of these are single instructions on the PIC; this is not the
// case for their equivalents on the AVR!
/*
#define SetBit(reg, b)      Instruction(OP_BSF, reg, b)
#define ClearBit(reg, b)    Instruction(OP_BCF, reg, b)
#define IfBitClear(reg, b)  Instruction(OP_BTFSS, reg, b)
#define IfBitSet(reg, b)    Instruction(OP_BTFSC, reg, b)
/**/
#define SetBit(...)         Instruction(OP_BSF, __VA_ARGS__)
#define ClearBit(...)       Instruction(OP_BCF, __VA_ARGS__)
#define IfBitClear(...)     Instruction(OP_BTFSS, __VA_ARGS__)
#define IfBitSet(...)       Instruction(OP_BTFSC, __VA_ARGS__)
/**/
// http://picprojects.org.uk/projects/pseudoins.htm
#define skpnc               Instruction(OP_BTFSC,  REG_STATUS, STATUS_C); // Skip on No Carry

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
    IntPcNow = IntPc;
    CompileFromIntermediate(FALSE);
    if(IntCode[IntPc].op == INT_ELSE) {
        IntPc++;
        IntPcNow = IntPc;
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
static void CopyLiteralToRegs(int reg, int literal, int sov)
{
    if(sov >= 1) {
      Instruction(OP_MOVLW, (literal & 0xff));
      Instruction(OP_MOVWF, reg);
    } else
      oops();
    if(sov >= 2) {
      Instruction(OP_MOVLW, (literal >> 8) & 0xff);
      Instruction(OP_MOVWF, reg+1);
    }
    if(sov >= 3) {
      Instruction(OP_MOVLW, (literal >> 16) & 0xff);
      Instruction(OP_MOVWF, reg+2);
    }
    if(sov >= 4) {
      Instruction(OP_MOVLW, (literal >> 24) & 0xff);
      Instruction(OP_MOVWF, reg+3);
    }
}
//-----------------------------------------------------------------------------
static void CopyVarToRegs(int reg, char *var, int sovRegs)
{
    int sov = SizeOfVar(var);
    if(sov == 0) oops();
    if(sovRegs == 0) oops();
    if(sov != sovRegs)
      dbp("reg=%d sovRegs=%d <- var=%s sov=%d",reg,sovRegs,var,sov);

    DWORD addrl, addrh;
    MemForVariable(var, &addrl, &addrh);

    Instruction(OP_MOVF, addrl, DEST_W);
    Instruction(OP_MOVWF, reg);
    if(sovRegs >= 2) {
        if(sov >= 2)
            Instruction(OP_MOVF, addrl+1, DEST_W);
        else {
            Instruction(OP_MOVLW, 0x00);
            Instruction(OP_BTFSC, reg, BIT7);
            Instruction(OP_MOVLW, 0xff);
        }
        Instruction(OP_MOVWF, reg+1);
    }
    if(sovRegs >= 3) {
        if(sov >= 3)
            Instruction(OP_MOVF, addrl+2, DEST_W);
        else {
            Instruction(OP_MOVLW, 0x00, 0);
            Instruction(OP_BTFSC, reg+1, BIT7);
            Instruction(OP_MOVLW, 0xff);
        }
        Instruction(OP_MOVWF, reg+2);
    }
    if(sovRegs >= 4) {
        if(sov >= 4)
            Instruction(OP_MOVF, addrl+3, DEST_W);
        else {
            Instruction(OP_MOVLW, 0x00, 0);
            Instruction(OP_BTFSC, reg+2, BIT7);
            Instruction(OP_MOVLW, 0xff);
        }
        Instruction(OP_MOVWF, reg+3);
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Alloc RAM for single bit and vars
//-----------------------------------------------------------------------------
void AllocBitsVars()
{
    DWORD addr;
    int   bit;

    for(IntPc=0; IntPc < IntCodeLen; IntPc++) {
        IntPcNow = IntPc;
        IntOp *a = &IntCode[IntPc];
        rungNow = a->rung;
        switch(a->op) {
            case INT_SET_BIT:
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                break;

            case INT_CLEAR_BIT:
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                break;

            case INT_COPY_BIT_TO_BIT:
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                MemForSingleBit(a->name2, FALSE, &addr, &bit);
                break;

            case INT_IF_BIT_SET:
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                break;

            case INT_IF_BIT_CLEAR:
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                break;

            case INT_UART_SEND_BUSY:
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                break;

            case INT_UART_SEND:
                MemForSingleBit(a->name2, TRUE, &addr, &bit);
                break;

            case INT_UART_RECV_AVAIL:
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                break;

            case INT_UART_RECV:
                MemForSingleBit(a->name2, TRUE, &addr, &bit);
                break;

            case INT_SET_PWM:
                char storeName[MAX_NAME_LEN];
                GenSymOneShot(storeName, "ONE_SHOT_RISING", "pwm_init");
                MemForSingleBit(storeName, FALSE, &addr, &bit);
                break;

            case INT_EEPROM_BUSY_CHECK:
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                break;

            default:
                break;
        }
    }
}

//-----------------------------------------------------------------------------
// Compile the intermediate code to PIC16 native code.
//-----------------------------------------------------------------------------
static void CompileFromIntermediate(BOOL topLevel)
{
    DWORD addr, addr1, addr2, addr3;
    int bit, bit2;
    DWORD addrl, addrh;
    DWORD addrl2, addrh2;
    DWORD addrl3, addrh3;
    int sov, sov1, sov2, sov3;
    char comment[MAX_NAME_LEN];

    // Keep track of which 2k section we are using. When it looks like we
    // are about to run out, fill with nops and move on to the next one.
    DWORD section = 0;

    for(; IntPc < IntCodeLen; IntPc++) {
        IntPcNow = IntPc;
        #ifndef AUTO_PAGING
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
        #endif
        IntOp *a = &IntCode[IntPc];
        rungNow = a->rung;
        switch(a->op) {
            case INT_SET_BIT:
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                SetBit(addr, bit, a->name1);
                break;

            case INT_CLEAR_BIT:
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                ClearBit(addr, bit, a->name1);
                break;

            case INT_COPY_BIT_TO_BIT:
                MemForSingleBit(a->name1, FALSE, &addr, &bit);
                MemForSingleBit(a->name2, FALSE, &addr2, &bit2);
                CopyBit(addr, bit, addr2, bit2);
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                MemForVariable(a->name1, &addr);
                sprintf(comment, "%s=%d==0x%X", a->name1, a->literal, a->literal);
                sov = SizeOfVar(a->name1);
                if(sov >= 1) {
                  WriteRegister(addr, BYTE(a->literal & 0xff), comment);
                  if(sov >= 2) {
                    WriteRegister(addr+1, BYTE((a->literal >> 8) & 0xff), comment);
                    if(sov >= 3) {
                      WriteRegister(addr+2, BYTE((a->literal >> 16) & 0xff), comment);
                      if(sov == 4) {
                        WriteRegister(addr+3, BYTE((a->literal >> 24) & 0xff), comment);
                      } else if(sov > 4) oops();
                    }
                  }
                } else oops();
                break;

            case INT_INCREMENT_VARIABLE: {
                MemForVariable(a->name1, &addr);
                sov = SizeOfVar(a->name1);
                if(sov >= 1) {
                  Instruction(OP_INCF, addr, DEST_F, a->name1);
                  if(sov >= 2) {
                    IfBitSet(REG_STATUS, STATUS_Z);
                    Instruction(OP_INCF, addr+1, DEST_F, a->name1);
                    if(sov >= 3) {
                      IfBitSet(REG_STATUS, STATUS_Z);
                      Instruction(OP_INCF, addr+2, DEST_F, a->name1);
                      if(sov == 4) {
                        IfBitSet(REG_STATUS, STATUS_Z);
                        Instruction(OP_INCF, addr+3, DEST_F, a->name1);
                      } else if(sov > 4) oops();
                    }
                  }
                } else oops();
                break;
            }
            case INT_DECREMENT_VARIABLE: {
                MemForVariable(a->name1, &addr);
                sov = SizeOfVar(a->name1);
                if(sov >= 1) {
                  Instruction(OP_MOVLW, 1);
                  Instruction(OP_SUBWF, addr, DEST_F, a->name1);
                  if(sov >= 2) {
                    //IfBitSet(REG_STATUS, STATUS_C); BORROW !!!
                    /* Note: For borrow, the polarity is reversed.
                    A subtraction is executed by adding the two's
                    complement of the second operand. */
                    IfBitClear(REG_STATUS, STATUS_C);
                    Instruction(OP_SUBWF, addr+1, DEST_F);
                    if(sov >= 3) {
                      IfBitClear(REG_STATUS, STATUS_C);
                      Instruction(OP_SUBWF, addr+2, DEST_F);
                      if(sov == 4) {
                        IfBitClear(REG_STATUS, STATUS_C);
                        Instruction(OP_SUBWF, addr+3, DEST_F);
                      } else if(sov > 4) oops();
                    }
                  }
                } else oops();
                break;
            }
            case INT_IF_BIT_SET: {
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                IfBitClear(addr, bit, a->name1);
                Instruction(OP_GOTO, condFalse, 0);
                CompileIfBody(condFalse);
                break;
            }
            case INT_IF_BIT_CLEAR: {
                DWORD condFalse = AllocFwdAddr();
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                IfBitSet(addr, bit, a->name1);
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
                BYTE litH = BYTE(a->literal >> 8);
                BYTE litL = BYTE(a->literal & 0xff);

                MemForVariable(a->name1, &addrl, &addrh);

                // var - lit
                Instruction(OP_MOVLW, litH, 0);
                Instruction(OP_SUBWF, addrh, DEST_W, a->name1);
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
                Instruction(OP_SUBWF, addrl, DEST_W, a->name1);
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
                Instruction(OP_MOVF, addrl, DEST_W, a->name1);
                Instruction(OP_SUBWF, addrl2, DEST_W, a->name2);
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

                Instruction(OP_MOVF, addrl2, DEST_W, a->name2);
                Instruction(OP_MOVWF, addrl, 0, a->name1);

                Instruction(OP_MOVF, addrh2, DEST_W, a->name2);
                Instruction(OP_MOVWF, addrh, 0, a->name1);
                break;


            // The add and subtract routines must be written to return correct
            // results if the destination and one of the operands happen to
            // be the same registers (e.g. for B = A - B).

            case INT_SET_VARIABLE_ADD:
                MemForVariable(a->name1, &addrl, &addrh);
                MemForVariable(a->name2, &addrl2, &addrh2);
                MemForVariable(a->name3, &addrl3, &addrh3);

                Instruction(OP_MOVF, addrl2, DEST_W, a->name2);
                Instruction(OP_ADDWF, addrl3, DEST_W, a->name3);
                Instruction(OP_MOVWF, addrl, 0, a->name1);
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

                Instruction(OP_MOVF, addrl3, DEST_W, a->name3);
                Instruction(OP_SUBWF, addrl2, DEST_W, a->name2);
                Instruction(OP_MOVWF, addrl, 0, a->name1);
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

                Instruction(OP_MOVF, addrl2, DEST_W, a->name2);
                Instruction(OP_MOVWF, Scratch0, 0);
                Instruction(OP_MOVF, addrh2, DEST_W);
                Instruction(OP_MOVWF, Scratch1, 0);

                Instruction(OP_MOVF, addrl3, DEST_W, a->name3);
                Instruction(OP_MOVWF, Scratch2, 0);
                Instruction(OP_MOVF, addrh3, DEST_W);
                Instruction(OP_MOVWF, Scratch3, 0);

                CallWithPclath(MultiplyRoutineAddress);

                Instruction(OP_MOVF, Scratch2, DEST_W);
                Instruction(OP_MOVWF, addrl, 0, a->name1);
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
                if(a->op == INT_SET_VARIABLE_DIVIDE) {
                Instruction(OP_MOVF, Scratch0, DEST_W);
                Instruction(OP_MOVWF, addrl, 0);
                Instruction(OP_MOVF, Scratch1, DEST_W);
                Instruction(OP_MOVWF, addrh, 0);
                } else {
                ooops("TODO");
                }
                break;

            case INT_UART_SEND_BUSY: {
                MemForSingleBit(a->name1, TRUE, &addr, &bit);

                ClearBit(addr, bit);

                DWORD notBusy = AllocFwdAddr();
                #ifdef AUTO_BANKING
                Instruction(OP_BTFSC, REG_TXSTA, 1);
                Instruction(OP_GOTO, notBusy, 0);
                #else
                Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
                Instruction(OP_BTFSC, REG_TXSTA ^ 0x80, 1);
                Instruction(OP_GOTO, notBusy, 0);

                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
                #endif
                SetBit(addr, bit);

                FwdAddrIsNow(notBusy);
                #ifdef AUTO_BANKING
                #else
                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
                #endif
                break;
            }
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
                #ifdef AUTO_BANKING
                Instruction(OP_BTFSC, REG_TXSTA, 1);
                Instruction(OP_GOTO, notBusy, 0);
                #else
                Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
                Instruction(OP_BTFSC, REG_TXSTA ^ 0x80, 1);
                Instruction(OP_GOTO, notBusy, 0);

                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
                #endif
                SetBit(addr, bit);

                FwdAddrIsNow(notBusy);
                #ifdef AUTO_BANKING
                #else
                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
                #endif

                break;
            }
            case INT_UART_RECV_AVAIL: {
                MemForSingleBit(a->name1, TRUE, &addr, &bit);
                SetBit(addr, bit); // Set // TODO
                break;
            }
            case INT_UART_RECV: {
                MemForVariable(a->name1, &addrl, &addrh);
                MemForSingleBit(a->name2, TRUE, &addr, &bit);

                ClearBit(addr, bit);

                // If RCIF is still clear, then there's nothing to do; in that
                // case jump to the end, and leave the rung-out clear.
                DWORD done = AllocFwdAddr();
                IfBitClear(REG_PIR1, RCIF);
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
                int timer = -1;
                McuIoPinInfo *iop = PinInfoForName(a->name3);
                if(iop) {
                    McuPwmPinInfo *ioPWM = PwmPinInfo(iop->pin);
                    if(ioPWM)
                        timer = ioPWM->timer;
                }
                if(timer < 0) oops();
                int target = hobatoi(a->name2);

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
                // Duty Cycle Ratio =ÉCCPR2L:CCP2CON<5:4>É/ (4 * (PR2 + 1))

                // PWM freq = Fosc / (PR2 + 1) * 4 * (TMR2 Prescale Value)
                // PR2 = Fosc / (4 * (TMR2 Prescale Value) * targetFreq) - 1

                // Timer1
                // Software programmable prescaler (1:1, 1:2, 1:4, 1:8)

                char str1[1024];
                char str2[1024];
                char minSI[5];
                char maxSI[5];
                double minFreq;
                double maxFreq;
                if(timer == 2) {
                    minFreq = SIprefix(Prog.mcuClock / ((255+1)*4*16), minSI);
                    maxFreq = SIprefix(Prog.mcuClock / ((  3+1)*4* 1), maxSI);
                } else if(timer == 1) {
                    minFreq = SIprefix(Prog.mcuClock / ((255+1)*4*8), minSI);
                    maxFreq = SIprefix(Prog.mcuClock / ((  3+1)*4*1), maxSI);
                } else oops();
                sprintf(str1,_("Available PWM frequency from %.3f %sHz up to %.3f %sHz"), minFreq, minSI, maxFreq, maxSI);

                int pr2plus1;
                int prescale;
                for(prescale = 1;;) {
                    int dv = 4*prescale*target;
                    pr2plus1 = (Prog.mcuClock + (dv/2))/dv;
                    if(pr2plus1 < 3) {
                        sprintf(str2,"'%s' %s\n\n%s",
                            a->name3,
                            _("PWM frequency too fast."),
                            str1);
                        Error(str2);
                        fCompileError(f, fAsm);
                    }
                    if(pr2plus1 >= 256) {
                      if(timer == 2) {
                        if(prescale == 1) {
                            prescale = 4;
                        } else if(prescale == 4) {
                            prescale = 16;
                        } else {
                            sprintf(str2,"'%s' %s\n\n%s",
                                a->name3,
                                _("PWM frequency too slow."),
                                str1);
                            Error(str2);
                            fCompileError(f, fAsm);
                        }
                      } else if(timer == 1) {
                        prescale *= 2;
                        if(prescale == 16) {
                            sprintf(str2,"'%s' %s\n\n%s",
                                a->name3,
                                _("PWM frequency too slow."),
                                str1);
                            Error(str2);
                            fCompileError(f, fAsm);
                        }
                      } else oops();
                    } else {
                        break;
                    }
                }

                double targetFreq = 1.0 * Prog.mcuClock / (pr2plus1 * 4 * prescale);
                // First scale the input variable from percent to timer units,
                // with a multiply and then a divide.
                MultiplyNeeded = TRUE; DivideNeeded = TRUE;
                MemForVariable(a->name1, &addrl, &addrh);
                Instruction(OP_MOVF, addrl, DEST_W);
                Instruction(OP_MOVWF, Scratch0, 0);
                Instruction(OP_CLRF, Scratch1, 0);

                Instruction(OP_MOVLW, (pr2plus1 * 1) & 0xff);
                Instruction(OP_MOVWF, Scratch2);
                if((pr2plus1 * 1) >> 8) {
                    Instruction(OP_MOVLW, (pr2plus1 * 1) >> 8);
                    Instruction(OP_MOVWF, Scratch3);
                } else
                    Instruction(OP_CLRF, Scratch3);

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

                if(timer == 2)
                    Instruction(OP_MOVWF, REG_CCPR2L, 0);
                else if(timer == 1)
                    Instruction(OP_MOVWF, REG_CCPR1L, 0);
                else oops();


                // Only need to do the setup stuff once
                //MemForSingleBit("$pwm_init", FALSE, &addr, &bit);

                char storeName[MAX_NAME_LEN];
                GenSymOneShot(storeName, "ONE_SHOT_RISING", "pwm_init");
                MemForSingleBit(storeName, FALSE, &addr, &bit);

                DWORD skip = AllocFwdAddr();
                IfBitSet(addr, bit);
                Instruction(OP_GOTO, skip, 0);
                SetBit(addr, bit);

                // Set up the CCP2 and TMR2 peripherals.
                WriteRegister(REG_PR2, pr2plus1 - 1);
                //                              - 1 // Ok
                if(timer == 2) {
                    WriteRegister(REG_CCP2CON, 0x0c); // PWM mode, ignore LSbs

                    BYTE t2con = (1 << 2); // timer 2 on
                    if(prescale == 1)
                        t2con |= 0;
                    else if(prescale == 4)
                        t2con |= 1;
                    else if(prescale == 16)
                        t2con |= 2;
                    else oops();
                    WriteRegister(REG_T2CON, t2con);
                } else if(timer == 1) {
                    WriteRegister(REG_CCP1CON, 0x0c); // PWM mode, ignore LSbs

                    BYTE t1con;
                    if(prescale == 1)
                        t1con = 0;
                    else if(prescale == 2)
                        t1con = 1;
                    else if(prescale == 4)
                        t1con = 2;
                    else if(prescale == 8)
                        t1con = 3;
                    else oops();

                    t1con = (t1con<<T1CKPS0) | (1 << TMR1ON); // timer 1 on
                    WriteRegister(REG_T1CON, t1con);
                } else oops();

                FwdAddrIsNow(skip);
                break;
            }

#ifndef AUTO_BANKING
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
#endif

            case INT_EEPROM_BUSY_CHECK: {
                DWORD isBusy = AllocFwdAddr();
                DWORD done = AllocFwdAddr();
                MemForSingleBit(a->name1, FALSE, &addr, &bit);

                #ifdef AUTO_BANKING
                IfBitSet(REG_EECON1, 1);
                Instruction(OP_GOTO, isBusy, 0);

                IfBitClear(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
                Instruction(OP_GOTO, done, 0);

                // So there is not a write pending, but we have another
                // character to transmit queued up.

                Instruction(OP_INCF, REG_EEADR, DEST_F);
                Instruction(OP_MOVF, EepromHighByte, DEST_W);
                Instruction(OP_MOVWF, REG_EEDATA, 0);
                Instruction(OP_BCF, REG_EECON1, 7);
                Instruction(OP_BSF, REG_EECON1, 2);
                Instruction(OP_MOVLW, 0x55, 0);
                Instruction(OP_MOVWF, REG_EECON2, 0);
                Instruction(OP_MOVLW, 0xaa, 0);
                Instruction(OP_MOVWF, REG_EECON2, 0);
                Instruction(OP_BSF, REG_EECON1, 1);

                ClearBit(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);

                FwdAddrIsNow(isBusy);
                #else
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
                #endif
                SetBit(addr, bit);

                FwdAddrIsNow(done);
                break;
            }
            case INT_EEPROM_WRITE: {
                MemForVariable(a->name1, &addrl, &addrh);

                #ifdef AUTO_BANKING
                SetBit(EepromHighByteWaitingAddr, EepromHighByteWaitingBit);
                Instruction(OP_MOVF, addrh, DEST_W);
                Instruction(OP_MOVWF, EepromHighByte, 0);

                Instruction(OP_MOVLW, a->literal, 0);
                Instruction(OP_MOVWF, REG_EEADR, 0);
                Instruction(OP_MOVF, addrl, DEST_W);
                Instruction(OP_MOVWF, REG_EEDATA, 0);
                Instruction(OP_BCF, REG_EECON1, 7);
                Instruction(OP_BSF, REG_EECON1, 2);
                Instruction(OP_MOVLW, 0x55, 0);
                Instruction(OP_MOVWF, REG_EECON2, 0);
                Instruction(OP_MOVLW, 0xaa, 0);
                Instruction(OP_MOVWF, REG_EECON2, 0);
                Instruction(OP_BSF, REG_EECON1, 1);
                #else
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
                #endif
                break;
            }
            case INT_EEPROM_READ: {
                int i;
                MemForVariable(a->name1, &addrl, &addrh);
                #ifdef AUTO_BANKING
                for(i = 0; i < 2; i++) {
                    Instruction(OP_MOVLW, a->literal+i, 0);
                    Instruction(OP_MOVWF, REG_EEADR, 0);
                    Instruction(OP_BCF, REG_EECON1, 7);
                    Instruction(OP_BSF, REG_EECON1, 0);
                    Instruction(OP_MOVF, REG_EEDATA, DEST_W);
                    Instruction(OP_MOVWF, addrl+i, 0);
                }
                #else
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
                #endif
                break;
            }
            case INT_READ_ADC: {
                BYTE adcs;

                MemForVariable(a->name1, &addrl, &addrh);
                //
                int goPos, chsPos;
                if(McuAs("Microchip PIC16F887 ")
                || McuAs("Microchip PIC16F886 ")
                || McuAs(" PIC16F882 ")
                || McuAs(" PIC16F883 ")
                || McuAs(" PIC16F884 ")
                || McuAs(" PIC16F1512 ")
                || McuAs(" PIC16F1513 ")
                || McuAs(" PIC16F1516 ")
                || McuAs(" PIC16F1517 ")
                || McuAs(" PIC16F1518 ")
                || McuAs(" PIC16F1519 ")
                || McuAs(" PIC16F1526 ")
                || McuAs(" PIC16F1527 ")
                || McuAs(" PIC16F1933 ")
                || McuAs(" PIC16F1947 ")
                ) {
                     goPos = 1;
                    chsPos = 2;
                } else
                if(McuAs(" PIC16F819 ")
                || McuAs(" PIC16F873 ")
                || McuAs(" PIC16F874 ")
                || McuAs(" PIC16F876 ")
                || McuAs(" PIC16F877 ")
                || McuAs(" PIC16F88 ")
                ) {
                     goPos = 2;
                    chsPos = 3;
                } else oops();
                //
                if(Prog.mcuClock > 5000000) {
                    adcs = 2; // 32*Tosc
                } else if(Prog.mcuClock > 1250000) {
                    adcs = 1; // 8*Tosc
                } else {
                    adcs = 0; // 2*Tosc
                }
                //
                int adcsPos;
                if(McuAs(" PIC16F1512 ")
                || McuAs(" PIC16F1513 ")
                || McuAs(" PIC16F1516 ")
                || McuAs(" PIC16F1517 ")
                || McuAs(" PIC16F1518 ")
                || McuAs(" PIC16F1519 ")
                || McuAs(" PIC16F1526 ")
                || McuAs(" PIC16F1527 ")
                || McuAs(" PIC16F1933 ")
                || McuAs(" PIC16F1947 ")
                ) {
                    adcsPos = 4; // in REG_ADCON1
                    WriteRegister(REG_ADCON0,
                         (MuxForAdcVariable(a->name1) << chsPos) |
                         (0 << goPos) |  // don't start yet
                         (1 << 0)        // A/D peripheral on
                    );

                    WriteRegister(REG_ADCON1,
                        (1 << 7) |      // right-justified
                        (adcs << adcsPos) |
                        (0 << 0)        // 00 = VREF is connected to VDD
                    );
                } else
                if(McuAs(" PIC16F819 ")
                || McuAs(" PIC16F873 ")
                || McuAs(" PIC16F874 ")
                || McuAs(" PIC16F876 ")
                || McuAs(" PIC16F877 ")
                || McuAs(" PIC16F88 ")
                || McuAs(" PIC16F882 ")
                || McuAs(" PIC16F883 ")
                || McuAs(" PIC16F884 ")
                || McuAs(" PIC16F886 ")
                || McuAs(" PIC16F887 ")
                ) {
                    adcsPos = 6; // in REG_ADCON0
                    WriteRegister(REG_ADCON0,
                         (adcs << adcsPos) |
                         (MuxForAdcVariable(a->name1) << chsPos) |
                         (0 << goPos) |  // don't start yet
                                         // bit 1 unimplemented
                         (1 << 0)        // A/D peripheral on
                    );

                    WriteRegister(REG_ADCON1,
                        (1 << 7) |      // right-justified
                        (0 << 0)        // for now, all analog inputs
                    );
                } else oops();
                if(McuAs("Microchip PIC16F88 "))
                {
                    WriteRegister(REG_ANSEL, 0x7f);
                }
                if(McuAs("Microchip PIC16F887 ") ||
                   McuAs("Microchip PIC16F886 "))
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

                #ifdef AUTO_BANKING
                Instruction(OP_MOVF, REG_ADRESL, DEST_W);
                #else
                Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
                Instruction(OP_MOVF, REG_ADRESL ^ 0x80, DEST_W);
                Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
                #endif
                Instruction(OP_MOVWF, addrl, 0);

                // hook those pins back up to the digital inputs in case
                // some of them are used that way
                WriteRegister(REG_ADCON1,
                    (1 << 7) |      // right-justify A/D result
                    (6 << 0)        // all digital inputs
                );
                if(McuAs("Microchip PIC16F88 "))
                {
                    WriteRegister(REG_ANSEL, 0x00);
                }
                if(McuAs("Microchip PIC16F887 ") ||
                   McuAs("Microchip PIC16F886 "))
                {
                    WriteRegister(REG_ANSEL, 0x00);
                    WriteRegister(REG_ANSELH, 0x00);
                }
                break;
            }
            case INT_END_IF:
            case INT_ELSE:
                return;

            case INT_WRITE_STRING:
                Error(_("Unsupported operation for target, skipped."));
            case INT_SIMULATE_NODE_STATE:
                break;

            case INT_COMMENT:
                Comment(a->name1);
                break;

            default:
                ooops("INT_%d", a->op);
                break;
        }
        #ifndef AUTO_PAGING
        if(((PicProgWriteP >> 11) != section) && topLevel) {
            // This is particularly prone to happening in the last section,
            // if the program doesn't fit (since we won't have attempted
            // to add padding).
            Error(_("Internal error relating to PIC paging; make program "
                    "smaller or reshuffle it."));
            fCompileError(f, fAsm);
        }
        #endif
    }
}

//-----------------------------------------------------------------------------
// Configure Timer1 and Ccp1 to generate the periodic `cycle' interrupt
// that triggers all the ladder logic processing. We will always use 16-bit
// Timer1, with the prescaler configured appropriately.
//-----------------------------------------------------------------------------
static void ConfigureTimer1(int cycleTimeMicroseconds)
{
    Comment("Configure Timer1");
    int prescaler = 1;
    int countsPerCycle;

    while(prescaler < 16) {
        int timerRate = (Prog.mcuClock / (4*prescaler)); // hertz
        double timerPeriod = 1e6 / timerRate; // timer period, us
        countsPerCycle = (int)(cycleTimeMicroseconds / timerPeriod);

        if(countsPerCycle < PLC_CLOCK_MIN * 2) {
            Error(_("Cycle time too fast; increase cycle time, or use faster "
                "crystal."));
            fCompileError(f, fAsm);
        } else if(countsPerCycle > 0xffff) {
            if(prescaler >= 8) {
                Error(
                    _("Cycle time too slow; decrease cycle time, or use slower "
                    "crystal."));
                fCompileError(f, fAsm);
            }
        } else {
            break;
        }
        prescaler *= 2;
    }

    WriteRegister(REG_CCPR1L, countsPerCycle & 0xff);
    WriteRegister(REG_CCPR1H, countsPerCycle >> 8);

    WriteRegister(REG_TMR1L, 0);
    WriteRegister(REG_TMR1H, 0);

    BYTE t1con = 0;
    // set up prescaler
    if(prescaler == 1)        t1con |= 0x00;
    else if(prescaler == 2)   t1con |= 0x10;
    else if(prescaler == 4)   t1con |= 0x20;
    else if(prescaler == 8)   t1con |= 0x30;
    else oops();
    // enable clock, internal source
    t1con |= 0x01;
    WriteRegister(REG_T1CON, t1con);

    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        Instruction(OP_BCF, REG_T1GCON, 7);
    }

    BYTE ccp1con;
    // compare mode, reset TMR1 on trigger
    ccp1con = 0x0b;
    WriteRegister(REG_CCP1CON, ccp1con);
}

//-----------------------------------------------------------------------------
static int softDivisor; // 1..0xFFFF
static DWORD addrDivisor;
static int tmr0;
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

//  countsPerCycle = (Fosc/4) * (cycleTimeMicroseconds/1e6)
//  countsPerCycle = prescaler*softDivisor*TMR0
    softDivisor = 1;
    int prescaler = 1;
    long long int countsPerCycle;
    countsPerCycle =(1.0 * Prog.mcuClock / 4.0) * cycleTimeMicroseconds / 1000000.0;

    while((prescaler <= 256) && (softDivisor < 0x10000)) {
        tmr0 = (int)(countsPerCycle / (prescaler*softDivisor));
        if(tmr0 > 255) {
            if(prescaler < 256) {
                prescaler *= 2;
            } else {
                softDivisor *= 2;
            }
        } else {
            break;
        }
    }
    double Fcycle=1.0*Prog.mcuClock/(4.0*prescaler*softDivisor*tmr0);

    countsPerCycle = prescaler*softDivisor*tmr0;

    if(countsPerCycle < PLC_CLOCK_MIN * 2) {
        int min_cycleTimeMicroseconds = PLC_CLOCK_MIN * 2 *4 *1000000 / Prog.mcuClock;
        char str[1024];
        sprintf(str, _("\n\nMinimum PLC cycle time is %d us."),
            min_cycleTimeMicroseconds);
        Error("%s%s",
            _("Cycle time too fast; increase cycle time, or use faster "
            "crystal."),str);
        fCompileError(f, fAsm);
    }
    if(softDivisor > 0x10000) {
        double max_cycleTimeSeconds = 1.0 * 0x100 * 0x10000 * 0x100 * 4 /* * 1000000*/ / Prog.mcuClock;
        char str[1024];
        sprintf(str, _("\n\nMaximum PLC cycle time is %.3f s."),
            max_cycleTimeSeconds);
        Error("%s%s",
            _("Cycle time too slow; decrease cycle time, or use slower "
            "crystal."),str);
        fCompileError(f, fAsm);
    }

    BYTE PS = 0;
    // set up prescaler
    if     (prescaler ==   1)   PS = 0x08;
    else if(prescaler ==   2)   PS = 0x00;
    else if(prescaler ==   4)   PS = 0x01;
    else if(prescaler ==   8)   PS = 0x02;
    else if(prescaler ==  16)   PS = 0x03;
    else if(prescaler ==  32)   PS = 0x04;
    else if(prescaler ==  64)   PS = 0x05;
    else if(prescaler == 128)   PS = 0x06;
    else if(prescaler == 256)   PS = 0x07;
    else oops();

    if(Prog.mcu->core == BaselineCore12bit) {
        // enable clock, internal source
        //Instruction(OP_BCF, REG_OPTION_REG, T0CS);

        if(prescaler == 1) {
            //CHANGING PRESCALER(TIMER0 -> WDT)
            Instruction(OP_CLRWDT);                   // Clear WDT
            Instruction(OP_CLRF, REG_TMR0);           // Clear TMR0 and prescaler
            Instruction(OP_MOVLW, 0x08);              // Select WDT // 18 ms up to 2.3 seconds
            Instruction(OP_OPTION);
            /*
            Instruction(OP_CLRWDT);                   //PS<2:0> are 000 or 001
            Instruction(OP_MOVLW, 0x0F);              //Set Postscaler to
            Instruction(OP_OPTION);                   // desired WDT rate
            */
        } else {
            //CHANGING PRESCALER(WDT -> TIMER0)
            //The WDT has a nominal time-out period of 18 ms, (with no prescaler).
            Instruction(OP_CLRWDT);                        // Clear WDT and prescaler
            // Select TMR0, new prescale value and clock source
            Instruction(OP_MOVLW, PS);                     // Set prescale to PS
            Instruction(OP_OPTION);
        }

        Instruction(OP_CLRF, REG_TMR0);
    } else {
        // disable interrupt
        Instruction(OP_BCF, REG_INTCON ,T0IE);

        // enable clock, internal source
        Instruction(OP_BCF, REG_OPTION_REG, T0CS);

        if(prescaler == 1) {
            Instruction(OP_CLRWDT);                   // Clear WDT
            Instruction(OP_CLRF, REG_TMR0);           // Clear TMR0 and prescaler
            Instruction(OP_BSF, REG_OPTION_REG, PSA); // Select WDT
            Instruction(OP_CLRWDT);
            Instruction(OP_MOVLW, 0xF8);              // Mask prescaler bits
            Instruction(OP_ANDWF, REG_OPTION_REG, DEST_W);
            Instruction(OP_MOVWF, REG_OPTION_REG);
        } else {
            Instruction(OP_CLRWDT);                        // Clear WDT and prescaler
            Instruction(OP_MOVLW, 0xF0);                   // Mask TMR0 select and
            Instruction(OP_ANDWF, REG_OPTION_REG, DEST_W); // prescaler bits
            Instruction(OP_IORLW, PS);                     // Set prescale to PS
            Instruction(OP_MOVWF, REG_OPTION_REG);
        }

        Instruction(OP_MOVLW, 256 - tmr0);
        Instruction(OP_MOVWF, REG_TMR0);
    }

    if(softDivisor > 1) {
        MemForVariable("$softDivisor", &addrDivisor);
        //dbpx(addrDivisor)
        //dbpx(addrDivisor+1)
        WriteRegister(addrDivisor,   BYTE(softDivisor & 0xff));
        WriteRegister(addrDivisor+1, BYTE(softDivisor >> 8));
    }
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 16x16 signed multiply. One operand in
// Scratch1:Scratch0, other in Scratch3:Scratch2, result in Scratch3:Scratch2.
//-----------------------------------------------------------------------------
static void WriteMultiplyRoutine(void)
{
    Comment("MultiplyRoutine16");
    DWORD savePicProgWriteP = PicProgWriteP;
    #ifdef MOVE_TO_PAGE_0
    MultiplyRoutineAddress = PicProgWriteP;
    #else
    FwdAddrIsNow(MultiplyRoutineAddress);
    #endif

    DWORD result3 = Scratch5;
    DWORD result2 = Scratch4;
    DWORD result1 = Scratch3;
    DWORD result0 = Scratch2;

    DWORD multiplicand0 = Scratch0;
    DWORD multiplicand1 = Scratch1;

    DWORD counter = Scratch6;

    DWORD dontAdd = AllocFwdAddr();
    DWORD top;

    Instruction(OP_CLRF, result3, 0);
    Instruction(OP_CLRF, result2, 0);
    Instruction(OP_BCF,  REG_STATUS, STATUS_C);
    Instruction(OP_RRF,  result1, DEST_F);
    Instruction(OP_RRF,  result0, DEST_F);

    Instruction(OP_MOVLW, 16, 0);
    Instruction(OP_MOVWF, counter, 0);

    top = PicProgWriteP;
    Instruction(OP_BTFSS, REG_STATUS, STATUS_C);
    Instruction(OP_GOTO,  dontAdd, 0);
    Instruction(OP_MOVF,  multiplicand0, DEST_W);
    Instruction(OP_ADDWF, result2, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_C);
    Instruction(OP_INCF,  result3, DEST_F);
    Instruction(OP_MOVF,  multiplicand1, DEST_W);
    Instruction(OP_ADDWF, result3, DEST_F);
    FwdAddrIsNow(dontAdd);


    Instruction(OP_BCF, REG_STATUS, STATUS_C);
    Instruction(OP_RRF, result3, DEST_F);
    Instruction(OP_RRF, result2, DEST_F);
    Instruction(OP_RRF, result1, DEST_F);
    Instruction(OP_RRF, result0, DEST_F);

    Instruction(OP_DECFSZ, counter, DEST_F);
    Instruction(OP_GOTO, top, 0);

    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW, 0);
    else
        Instruction(OP_RETURN, 0, 0);

    if((savePicProgWriteP >> 11) != (PicProgWriteP >> 11)) oops();
}

//-----------------------------------------------------------------------------
// Write a subroutine to do a 16/16 signed divide. Call with dividend in
// Scratch1:0, divisor in Scratch3:2, and get the result in Scratch1:0.
//-----------------------------------------------------------------------------
static void WriteDivideRoutine(void)
{
    Comment("DivideRoutine16");
    DWORD savePicProgWriteP = PicProgWriteP;

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

    #ifdef MOVE_TO_PAGE_0
    DivideRoutineAddress = PicProgWriteP;
    #else
    FwdAddrIsNow(DivideRoutineAddress);
    #endif

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
    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW, 0);
    else
        Instruction(OP_RETURN, 0, 0);

    Instruction(OP_COMF, dividend0, DEST_F);
    Instruction(OP_COMF, dividend1, DEST_F);
    Instruction(OP_INCF, dividend0, DEST_F);
    Instruction(OP_BTFSC, REG_STATUS, STATUS_Z);
    Instruction(OP_INCF, dividend1, DEST_F);
    if(Prog.mcu->core == BaselineCore12bit)
        Instruction(OP_RETLW, 0);
    else
        Instruction(OP_RETURN, 0, 0);

    if((savePicProgWriteP >> 11) != (PicProgWriteP >> 11)) oops();
}

//-----------------------------------------------------------------------------
// Compile the program to PIC16 code for the currently selected processor
// and write it to the given file. Produce an error message if we cannot
// write to the file, or if there is something inconsistent about the
// program.
//-----------------------------------------------------------------------------
void CompilePic16(char *outFile)
{
    if(McuAs("Microchip PIC16F628 ")
    || McuAs("Microchip PIC16F88 " )
    || McuAs("Microchip PIC16F819 ")
    || McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F876 ")
    || McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    ) {
        REG_PIR1    = 0x0c;
        REG_TMR1L   = 0x0e;
        REG_TMR1H   = 0x0f;
        REG_T1CON   = 0x10;
        REG_CCPR1L  = 0x15;
        REG_CCPR1H  = 0x16;
        REG_CCP1CON = 0x17;
    } else
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_PIR1    = 0x0011;
        REG_TMR1L   = 0x0016;
        REG_TMR1H   = 0x0017;
        REG_T1CON   = 0x0018;
        REG_T1GCON  = 0x0019;
        REG_CCPR1L  = 0x0291;
        REG_CCPR1H  = 0x0292;
        REG_CCP1CON = 0x0293;
    } else
    if(McuAs(" PIC10F")
    || McuAs(" PIC12F")
    ) {
        // has not
    } else
        oops();

    if(McuAs("Microchip PIC16F628 ")
    ) {
        REG_CMCON   = 0x1f;
    } else
    if(McuAs("Microchip PIC16F88 ")
    ) {
        REG_CMCON   = 0x009C;
    } else
    if(McuAs("Microchip PIC16F819 ")
    || McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F876 ")
    || McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    || McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        // has not
    } else
    if(McuAs(" PIC10F")
    || McuAs(" PIC12F")
    ) {
        // has not
    } else
        oops();

    if(McuAs("Microchip PIC16F628 ")
    || McuAs("Microchip PIC16F88 " )
    || McuAs("Microchip PIC16F819 ")
    || McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F876 ")
    || McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    ) {
        REG_TXSTA    = 0x98;
        REG_RCSTA    = 0x18;
        REG_SPBRG    = 0x99;
        REG_TXREG    = 0x19;
        REG_RCREG    = 0x1a;
    } else
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_TXSTA    = 0x019E;
        REG_RCSTA    = 0x019D;
        REG_SPBRG    = 0x019B;
        REG_TXREG    = 0x019A;
        REG_RCREG    = 0x0199;
    } else
    if(McuAs(" PIC10F")
    || McuAs(" PIC12F")
    ) {
        // has not
    } else
        oops();

    if(McuAs("Microchip PIC16F88 " )
    || McuAs("Microchip PIC16F819 ")
    || McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F876 ")
    || McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    ) {
        REG_ADRESH   = 0x1e;
        REG_ADRESL   = 0x9e;
        REG_ADCON0   = 0x1f;
        REG_ADCON1   = 0x9f;
    } else
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_ADRESH   = 0x009C;
        REG_ADRESL   = 0x009B;
        REG_ADCON0   = 0x009D;
        REG_ADCON1   = 0x009E;
    } else
    if(McuAs("Microchip PIC16F628 ")
        // has not
    ) {
    } else
    if(McuAs(" PIC10F")
    || McuAs(" PIC12F")
    ) {
        // has not
    } else
        oops();

    if(McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F876 ")
    || McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    ) {
        REG_CCPR2L  = 0x1b;
        REG_CCP2CON = 0x1d;
    } else
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_CCPR2L  = 0x0298;
        REG_CCP2CON = 0x029A;
    } else
    if(McuAs("Microchip PIC16F628 ")
    || McuAs("Microchip PIC16F88 " )
    || McuAs("Microchip PIC16F819 ")
    ) {
        // has not
    } else
    if(McuAs(" PIC10F")
    || McuAs(" PIC12F")
    ) {
        // has not
    } else
        oops();

    if(McuAs("Microchip PIC16F628 ")
    || McuAs("Microchip PIC16F88 " )
    || McuAs("Microchip PIC16F819 ")
    || McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F876 ")
    || McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    ) {
        REG_T2CON   = 0x12;
        REG_PR2     = 0x92;
    } else
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_T2CON   = 0x001C;
        REG_PR2     = 0x001B;
    } else
    if(McuAs(" PIC10F")
    || McuAs(" PIC12F")
    ) {
        // has not
    } else
        oops();

    if(McuAs("Microchip PIC16F628 ")
    || McuAs("Microchip PIC16F88 " )
    || McuAs("Microchip PIC16F819 ")
    || McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F876 ")
    || McuAs("Microchip PIC16F874 ")
    || McuAs("Microchip PIC16F873 ")
    || McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    || McuAs("Microchip PIC16F884 ")
    || McuAs("Microchip PIC16F883 ")
    || McuAs("Microchip PIC16F882 ")
    ) {
        REG_TMR0       = 0x01;
        REG_OPTION_REG = 0x81;
    } else
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_TMR0       = 0x15;
        REG_OPTION_REG = 0x95;
    } else
    if(McuAs(" PIC10F")
    || McuAs(" PIC12F")
    ) {
        REG_TMR0       = 0x01;
      //REG_OPTION_REG not available for read. Write able via OP_OPTION operation.
    } else
        oops();

    if(McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F819 ")
    || McuAs("Microchip PIC16F88 " )
    || McuAs("Microchip PIC16F876 ")
    || McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    ) {
        REG_EECON1  = 0x18c;
        REG_EECON2  = 0x18d;
        REG_EEDATA  = 0x10c;
        REG_EEADR   = 0x10d;
    } else
    if(McuAs("Microchip PIC16F628 ")) {
        REG_EECON1  = 0x9c;
        REG_EECON2  = 0x9d;
        REG_EEDATA  = 0x9a;
        REG_EEADR   = 0x9b;
    } else
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    ) {
       // has not EEPROM, use PFM
    } else
    if(McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_EECON1  = 0x195;
        REG_EECON2  = 0x196;
        REG_EEDATL  = 0x193;
        REG_EEDATH  = 0x194;
        REG_EEADRL  = 0x191;
        REG_EEADRH  = 0x192;
    } else
    if(McuAs(" PIC10F")
    || McuAs(" PIC12F")
    ) {
        // has not
    } else
        oops();

    if(McuAs("Microchip PIC16F887 ")
    || McuAs("Microchip PIC16F886 ")
    ) {
        REG_ANSEL  = 0x0188;
        REG_ANSELH = 0x0189;
    } else
    if(McuAs("Microchip PIC16F88 ")
    ) {
        REG_ANSEL  = 0x009B;
    } else
    if(McuAs("Microchip PIC16F628 ")
    || McuAs("Microchip PIC16F819 ")
    || McuAs("Microchip PIC16F877 ")
    || McuAs("Microchip PIC16F876 ")
    ) {
        // has not
    } else {
//      oops();
    }

    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_ANSELA  = 0x18C;
    }
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1933 ")
    ) {
        REG_ANSELB  = 0x18D;
    }
    if(McuAs(" PIC16F1512 ")
    || McuAs(" PIC16F1513 ")
    || McuAs(" PIC16F1516 ")
    || McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1518 ")
    || McuAs(" PIC16F1519 ")
    ) {
        REG_ANSELC  = 0x18E;
    }
    if(McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    ) {
        REG_ANSELD  = 0x18F;
    }
    if(McuAs(" PIC16F1517 ")
    || McuAs(" PIC16F1519 ")
    || McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_ANSELE  = 0x190;
    }
    if(McuAs(" PIC16F1526 ")
    || McuAs(" PIC16F1527 ")
    || McuAs(" PIC16F1947 ")
    ) {
        REG_ANSELF  = 0x40C;
        REG_ANSELG  = 0x40D;
    }

    f = fopen(outFile, "w");
    if(!f) {
        Error(_("Couldn't open file '%s'"), outFile);
        return;
    }

    char outFileAsm[MAX_PATH];
    SetExt(outFileAsm, outFile, ".asm");
    fAsm = fopen(outFileAsm, "w");
    if(!fAsm) {
        Error(_("Couldn't open file '%s'"), outFileAsm);
        fclose(f);
        return;
    }

    if(setjmp(CompileErrorBuf) != 0) {
        fclose(f);
        fclose(fAsm);
        return;
    }

    rungNow = -100;
    fprintf(fAsm,
";/* This is auto-generated ASM code from LDmicro. Do not edit this file!\n"
";   Go back to the LDmicro ladder diagram source for changes in the ladder logic. */\n"
    );
    fprintf(fAsm, "; %s is the LDmicro target processor.\n", Prog.mcu->mcuList);
    fprintf(fAsm, "\tLIST    p=%s\n", Prog.mcu->mcuList);
    fprintf(fAsm, "#include ""%s.inc""\n", Prog.mcu->mcuInc);
    fprintf(fAsm, ".CONFIG = 0x%X\n", Prog.mcu->configurationWord);
    fprintf(fAsm, ";CONFIG1 WORD= 0x%04X\n", Prog.mcu->configurationWord & 0xffff);
    fprintf(fAsm, ";CONFIG2 WORD= 0x%04X\n", Prog.mcu->configurationWord >> 16);
    fprintf(fAsm, "\tradix dec\n");
    fprintf(fAsm, "\torg 0\n");
    fprintf(fAsm, ";TABSIZE = 8\n");
    fprintf(fAsm, ";\tCODE\n");

    WipeMemory();

    AllocStart();

    AllocBitsVars();

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
    Scratch10= AllocOctetRam();
    Scratch11= AllocOctetRam();
    }

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
    if(Prog.mcu->core != BaselineCore12bit) {
      Instruction(OP_CLRF, REG_PCLATH);  //0
    } else {
      Instruction(OP_NOP_, 0, 0);        //0
    }
    Instruction(OP_CLRF, REG_STATUS);    //1
    Comment("GOTO, progStart");
    Instruction(OP_GOTO, progStart, 0);  //2
    if(Prog.mcu->core != BaselineCore12bit) {
      Instruction(OP_NOP_, 0, 0);        //3
      Instruction(OP_RETFIE, 0, 0);      //4 OR
  //  Instruction(OP_NOP_, 0, 0);        //4 OR
      Instruction(OP_NOP_, 0, 0);        //5
      Instruction(OP_NOP_, 0, 0);        //6
      Instruction(OP_NOP_, 0, 0);        //7
    }

    #ifdef MOVE_TO_PAGE_0
    if(MultiplyRoutineUsed()) WriteMultiplyRoutine();
    if(DivideRoutineUsed()) WriteDivideRoutine();
    #endif

    FwdAddrIsNow(progStart);

    Comment("Now zero out the RAM");
    int i;
    for(i = 0; i <= RamSection; i++) {
        Instruction(OP_MOVLW, Prog.mcu->ram[i].start + 8, 0);
        Instruction(OP_MOVWF, REG_FSR, 0);
        Instruction(OP_MOVLW, Prog.mcu->ram[i].len - 8, 0);
        Instruction(OP_MOVWF, Scratch0, 0);

        DWORD zeroMem = PicProgWriteP;
        Instruction(OP_CLRF, REG_INDF, 0);
        Instruction(OP_INCF, REG_FSR, DEST_F);
        Instruction(OP_DECFSZ, Scratch0, DEST_F);
        Instruction(OP_GOTO, zeroMem, 0);
    }

    #ifndef MOVE_TO_PAGE_0
    DivideRoutineAddress = AllocFwdAddr();
    #endif
    DivideNeeded = FALSE;
    #ifndef MOVE_TO_PAGE_0
    MultiplyRoutineAddress = AllocFwdAddr();
    #endif
    MultiplyNeeded = FALSE;

    // Configure PLC Timer near the progStart
    if(Prog.cycleTimer==0)
        ConfigureTimer0(Prog.cycleTime);
    else
        ConfigureTimer1(Prog.cycleTime);

    Comment("Set up the TRISx registers (direction). 1 means tri-stated (input).");
    BYTE isInput[MAX_IO_PORTS], isOutput[MAX_IO_PORTS];
    BuildDirectionRegisters(isInput, isOutput);

    if(McuAs("Microchip PIC16F877 ")) {
        // This is a nasty special case; one of the extra bits in TRISE
        // enables the PSP, and must be kept clear (set here as will be
        // inverted).
        isOutput[4] |= 0xf8;
    }

    if(McuAs("Microchip PIC16F877 ") ||
       McuAs("Microchip PIC16F819 ") ||
       McuAs("Microchip PIC16F876 "))
    {
        // The GPIOs that can also be A/D inputs default to being A/D
        // inputs, so turn that around
        WriteRegister(REG_ADCON1,
            (1 << 7) |      // right-justify A/D result
            (6 << 0)        // all digital inputs
        );
    }

    if(McuAs("Microchip PIC16F88 ")) {
        WriteRegister(REG_ANSEL, 0x00); // all digital inputs
    } else
    if(McuAs("Microchip PIC16F887 ") ||
       McuAs("Microchip PIC16F886 "))
    {
        WriteRegister(REG_ANSEL,  0x00);    // all digital inputs
        WriteRegister(REG_ANSELH, 0x00);    // all digital inputs
    }
    if(REG_ANSELA)
        Instruction(OP_CLRF, REG_ANSELA);
    if(REG_ANSELB)
        Instruction(OP_CLRF, REG_ANSELB);
    if(REG_ANSELC)
        Instruction(OP_CLRF, REG_ANSELC);
    if(REG_ANSELD)
        Instruction(OP_CLRF, REG_ANSELD);
    if(REG_ANSELE)
        Instruction(OP_CLRF, REG_ANSELE);
    if(REG_ANSELF)
        Instruction(OP_CLRF, REG_ANSELF);
    if(REG_ANSELG)
        Instruction(OP_CLRF, REG_ANSELG);

    if(McuAs("Microchip PIC16F628 ")) {
        // This is also a nasty special case; the comparators on the
        // PIC16F628 are enabled by default and need to be disabled, or
        // else the PORTA GPIOs don't work.
        WriteRegister(REG_CMCON, 0x07);
    }

    if(PwmFunctionUsed()) {
        Comment("PwmFunctionUsed");
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

    if(Prog.mcu->core == BaselineCore12bit) {
    } else {
      Comment("Clear Bit 7 - PORTs pull-ups are enabled by individual port latch values");
      #ifdef AUTO_BANKING
      Instruction(OP_BCF, REG_OPTION_REG, _RBPU);
      #else
      Instruction(OP_BSF, REG_STATUS, STATUS_RP0);
      Instruction(OP_BCF, REG_OPTION_REG & 7, _RBPU);
      Instruction(OP_BCF, REG_STATUS, STATUS_RP0);
      #endif
    }

    Comment("Turn on the pull-ups, and drive the outputs low to start");
    for(i = 0; Prog.mcu->dirRegs[i] != 0; i++) {
        WriteRegister(Prog.mcu->outputRegs[i], 0x00);
    }
    for(i = 0; Prog.mcu->dirRegs[i] != 0; i++) {
        if(Prog.mcu->core == BaselineCore12bit) {
            Instruction(OP_MOVLW, BYTE(~isOutput[i]));
            Instruction(OP_TRIS, Prog.mcu->outputRegs[i]);
            break;
        } else {
            WriteRegister(Prog.mcu->dirRegs[i], ~isOutput[i]);
        }
    }

    if(UartFunctionUsed()) {
        if(Prog.baudRate == 0) {
            Error(_("Zero baud rate not possible."));
            fclose(f);
            return;
        }

        Comment("UartFunctionUsed. UART setup");
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

    Comment("Begin Of PLC Cycle");
    DWORD top = PicProgWriteP;
    if(Prog.cycleTimer == 0) {
        if(Prog.mcu->core == BaselineCore12bit) {
            Instruction(OP_MOVLW, tmr0 - 1 - 3); // tested in Proteus (... - 1 - 3 ) == 1.999-2.002 kHz}
//          Instruction(OP_MOVLW, tmr0 - 1); // tested in Proteus - 1) 1kHz}
            Instruction(OP_SUBWF, REG_TMR0, DEST_W);
            Instruction(OP_BTFSS,REG_STATUS, STATUS_C);
            Instruction(OP_GOTO, top);
            Instruction(OP_CLRF, REG_TMR0);
        } else {
            Instruction(OP_MOVLW, 256 - tmr0 + 1); // tested in Proteus {+1} {1ms=1kHz} {0.250ms=4kHz}
            IfBitClear(REG_INTCON ,T0IF);
            Instruction(OP_GOTO, PicProgWriteP - 1, 0);
            Instruction(OP_MOVWF, REG_TMR0);
            Instruction(OP_BCF, REG_INTCON ,T0IF);
        }

        if(softDivisor > 1) {
            DWORD yesZero;
            if(softDivisor > 0xff) {
                yesZero = AllocFwdAddr();
            }
            Instruction(OP_DECFSZ, addrDivisor, DEST_F); // Skip if zero
            Instruction(OP_GOTO, top);

            if(softDivisor > 0xff) {
                Instruction(OP_MOVF, addrDivisor+1, DEST_F);
                Instruction(OP_BTFSC,REG_STATUS, STATUS_Z); // Skip if not zero
                Instruction(OP_GOTO, yesZero);
                Instruction(OP_DECF, addrDivisor+1, DEST_F);
                Instruction(OP_GOTO, top);
                FwdAddrIsNow(yesZero);

                WriteRegister(addrDivisor+1, BYTE(softDivisor >> 8));
            }

            WriteRegister(addrDivisor,   BYTE(softDivisor & 0xff));
        }
    } else {
        if(Prog.mcu->core == BaselineCore12bit) {
            Error("Select Timer0 in menu 'Settings'-'MCU parameters'!");
            fCompileError(f, fAsm);
        }
        IfBitClear(REG_PIR1, CCP1IF);
        Instruction(OP_GOTO, PicProgWriteP - 1, 0);
        Instruction(OP_BCF, REG_PIR1, CCP1IF);
    }

    DWORD addrDuty;
    int   bitDuty;
    if(Prog.cycleDuty) {
        MemForSingleBit(YPlcCycleDuty, FALSE, &addrDuty, &bitDuty);
        Comment("SetBit YPlcCycleDuty");
        SetBit(addrDuty, bitDuty, "YPlcCycleDuty");
    }

    Comment("Watchdog reset");
    Instruction(OP_CLRWDT, 0, 0);
    IntPc = 0;
    //Comment("CompileFromIntermediate BEGIN");
    CompileFromIntermediate(TRUE);
    //Comment("CompileFromIntermediate END");

    for(i = 0; i < MAX_RUNGS; i++)
        Prog.HexInRung[i] = 0;
    for(i = 0; i < PicProgWriteP; i++)
        if((PicProg[i].rung >= 0)
        && (PicProg[i].rung < MAX_RUNGS))
            Prog.HexInRung[PicProg[i].rung]++;

    if(Prog.cycleDuty) {
        Comment("ClearBit YPlcCycleDuty");
        ClearBit(addrDuty, bitDuty, "YPlcCycleDuty");
    }

    Comment("GOTO next PLC cycle");
    #ifndef AUTO_PAGING
    // This is probably a big jump, so give it PCLATH.
    Instruction(OP_CLRF, REG_PCLATH, 0);
    #endif
    Instruction(OP_GOTO, top, 0);

    rungNow = -50;

    #ifndef AUTO_PAGING
    // Once again, let us make sure not to put stuff on a page boundary
    if((PicProgWriteP >> 11) != ((PicProgWriteP + 150) >> 11)) {
        DWORD section = (PicProgWriteP >> 11);
        // Just burn the last of this section with NOPs.
        while((PicProgWriteP >> 11) == section) {
            Instruction(OP_MOVLW, 0xab, 0);
        }
    }
    #endif

    #ifndef MOVE_TO_PAGE_0
    if(MultiplyNeeded) WriteMultiplyRoutine();
    if(DivideNeeded) WriteDivideRoutine();
    #endif

    Instruction(OP_GOTO, PicProgWriteP); // for last label in asm

    MemCheckForErrorsPostCompile();
    AddrCheckForErrorsPostCompile();
    #ifdef AUTO_BANKING
    MaxBank = CalcMaxBank();
    if(MaxBank)
        BankCorrection();
    #endif
    BankCheckForErrorsPostCompile();

    #ifdef AUTO_PAGING
    PageCorrection();
    AddrCheckForErrorsPostCompile2();
    #else
    PagePreSet();
    #endif

    ProgWriteP = PicProgWriteP;

    WriteHexFile(f, fAsm);
    fflush(f);
    fclose(f);

    fprintf(fAsm, "\tEND\n");
    PrintVariables(fAsm);
    fflush(fAsm);
    fclose(fAsm);

    char str[MAX_PATH+500];
    sprintf(str, _("Compile successful; wrote IHEX for PIC16 to '%s'.\r\n\r\n"
        "Configuration word (fuses) has been set for crystal oscillator, BOD "
        "enabled, LVP disabled, PWRT enabled, all code protection off."),
            outFile, PicProgWriteP, Prog.mcu->flashWords,
            (100*PicProgWriteP)/Prog.mcu->flashWords);

    char str2[MAX_PATH+500];
    sprintf(str2, _("Used %d/%d words of program flash (chip %d%% full)."),
        PicProgWriteP, Prog.mcu->flashWords,
        (100*PicProgWriteP)/Prog.mcu->flashWords);

    char str3[MAX_PATH+500];
    sprintf(str3, _("Used %d/%d byte of RAM (chip %d%% full)."),
        UsedRAM(), McuRAM(),
         (100*UsedRAM())/McuRAM());

    char str4[MAX_PATH+500];
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
