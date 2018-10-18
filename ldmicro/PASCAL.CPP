//-----------------------------------------------------------------------------
// Copyright 2015 Nehrutsa Ihor
//
// This file is part of LDmicro.
// This file was writen from ansic.cpp.
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
// Write the program as PASCAL source. This is very simple, because the
// intermediate code structure is really a lot like PASCAL. Someone else will be
// responsible for calling us with appropriate timing.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "intcode.h"
#include "filetracker.hpp"

static char  SeenVariables[MAX_IO][MAX_NAME_LEN];
static int   SeenVariablesCount;
static DWORD InCount, OutCount, TimerCount, RelayCount, VarCount;

//-----------------------------------------------------------------------------
// Have we seen a variable before? If not then no need to generate code for
// it, otherwise we will have to make a declaration, and mark it as seen.
//-----------------------------------------------------------------------------
static bool SeenVariable(const char *name)
{
    int i;
    for(i = 0; i < SeenVariablesCount; i++) {
        if(strcmp(SeenVariables[i], name) == 0) {
            return true;
        }
    }
    if(i >= MAX_IO)
        THROW_COMPILER_EXCEPTION("Internal error.");
    strcpy(SeenVariables[i], name);
    SeenVariablesCount++;
    return false;
}

//-----------------------------------------------------------------------------
// Turn an internal symbol into a PASCAL name; only trick is that internal symbols
// use $ for symbols that the int code generator needed for itself, so map
// that into something okay for PASCAL.
//-----------------------------------------------------------------------------
#define ASBIT 1
#define ASINT 2
#define ASSTR 3

//template <size_t N> static char *MapSym(const StringArray<N> &str, int how)
static const char *MapSym(const char *str, int how)
{
    if(!str)
        return nullptr;
    if(strlen(str) == 0)
        return nullptr;

    static char AllRets[16][MAX_NAME_LEN + 30];
    static int  RetCnt;

    RetCnt = (RetCnt + 1) & 15;

    char *ret = AllRets[RetCnt];

    // The namespace for bit and integer variables is distinct.
    char bit_int;
    if(how == ASBIT) {
        bit_int = 'b';
    } else if(how == ASINT) {
        bit_int = 'i';
    } else {
        THROW_COMPILER_EXCEPTION("Internal error.");
    }

    // User and internal symbols are distinguished.
    if(IsNumber(str))
        sprintf(ret, "%s", str);
    else if(str[0] == '$') {
        sprintf(ret, "I%c_%s", bit_int, str + 1);
    } else {
        sprintf(ret, "U%c_%s", bit_int, str);
    }

    char trans[1024];
    //dbp("ret   1 %s",ret);
    Transliterate(trans, ret);
    //dbp("trans 2 %s",trans);
    strcpy(ret, trans);
    //dbp("ret   2 %s",ret);

    return ret;
}

static const char *MapSym(const NameArray &name, int how)
{
    return MapSym(name.c_str(), how);
}
//-----------------------------------------------------------------------------
static void ResetAndRegister(FILE *fr, FILE *fv, const char *str, char *_str)
{
    char        __str[MAX_NAME_LEN];
    char        str2[MAX_NAME_LEN];
    const char *line2 = "1234567890-=";
    const char *line3 = "QWERTYUIOP";
    const char *line4 = "ASDFGHJKL";
    const char *line5 = "ZXCVBNM";
    AnsiToOem(_str, __str);

    McuIoPinInfo *iop = PinInfoForName(_str);
    ShortPinName(iop, str2);

    int type = GetAssignedType(_str, _str);
    if(str[0] == 'U') {
        if(type == IO_TYPE_DIG_INPUT) {
            fprintf(fv, "  InArr[InCount].Name := '%s';\n", __str);
            fprintf(fv, "  InArr[InCount].Name2:= '%s';\n", str2);
            fprintf(fv, "  InArr[InCount].ReadFunc := Read_%s;\n", str);
            fprintf(fv, "  InArr[InCount].WriteFunc := Write_%s;\n", str);
            fprintf(fv, "  {$ifdef HookIn}\n");
            if(InCount < strlen(line2))
                fprintf(fv, "  InArr[InCount].KeyCode := kb_line2[InCount+1]; {'%c'}\n", line2[InCount]);
            else if(InCount < (strlen(line2) + strlen(line3)))
                fprintf(fv,
                        "  InArr[InCount].KeyCode := kb_line3[InCount-%d+1]; {'%c'}\n",
                        strlen(line2),
                        line3[InCount - strlen(line2)]);
            //          fprintf(fv, "  InArr[InCount].NPP := InCount;\n");
            fprintf(fv, "  {$endif}\n");
            fprintf(fv, "  inc(InCount);\n\n");
            InCount++;
        } else if(type == IO_TYPE_DIG_OUTPUT) {
            fprintf(fv, "  OutArr[OutCount].Name := '%s';\n", __str);
            fprintf(fv, "  OutArr[OutCount].Name2:= '%s';\n", str2);
            fprintf(fv, "  OutArr[OutCount].ReadFunc := Read_%s;\n", str);
            fprintf(fv, "  OutArr[OutCount].WriteFunc := Write_%s;\n", str);
            fprintf(fv, "  inc(OutCount);\n\n");
            OutCount++;
        } else if(str[3] == 'T') {
            fprintf(fr, "  %s := 0;\n", str);

            fprintf(fv, "  TimerArr[TimerCount].Name := '%s';\n", __str);
            fprintf(fv, "  TimerArr[TimerCount].puw:= @%s;\n", str);
            fprintf(fv, "  inc(TimerCount);\n\n");
            TimerCount++;
        } else if(str[3] == 'R') {
            fprintf(fr, "  %s := false;\n", str);

            fprintf(fv, "  RelayArr[RelayCount].Name := '%s';\n", __str);
            fprintf(fv, "  RelayArr[RelayCount].pb := @%s;\n", str);

            fprintf(fv, "  {$ifdef HookRelay}\n");
            if(RelayCount < strlen(line4))
                fprintf(fv, "  RelayArr[RelayCount].KeyCode := kb_line4[RelayCount+1]; {'%c'}\n", line4[RelayCount]);
            else if(RelayCount < (strlen(line4) + strlen(line5)))
                fprintf(fv,
                        "  RelayArr[RelayCount].KeyCode := kb_line5[RelayCount-%d+1]; {'%c'}\n",
                        strlen(line4),
                        line5[RelayCount - strlen(line4)]);
            //          fprintf(fv, "  RelayArr[RelayCount].NPP := RelayCount;\n");
            fprintf(fv, "  {$endif}\n");

            fprintf(fv, "  inc(RelayCount);\n\n");
            RelayCount++;
        } else {
            fprintf(fr, "  %s := 0;\n", str);

            fprintf(fv, "  VarArr[VarCount].Name := '%s';\n", __str);
            fprintf(fv, "  VarArr[VarCount].psw := @%s;\n", str);
            fprintf(fv, "  inc(VarCount);\n\n");
            VarCount++;
        };
    } else {
        if(str[1] == 'b')
            fprintf(fr, "  %s := false;\n", str);
        else
            fprintf(fr, "  %s := 0;\n", str);
    }
}
//-----------------------------------------------------------------------------
// Generate a declaration for an integer var; easy, a static 16-bit qty.
//-----------------------------------------------------------------------------
static void DeclareInt(FILE *f, FILE *fi, FILE *fr, FILE *fv, const char *str, char *_str)
{
    if(IsNumber(str))
        return;
    if(str[0] == 'U') {
        if(str[3] == 'T')
            fprintf(fi, "const %s : SDWORD = 0;\n", str);
        else
            fprintf(fi, "const %s : SDWORD = 0;\n", str);
    } else
        fprintf(fi, "const %s : SDWORD = 0;\n", str);
    ResetAndRegister(fr, fv, str, _str);
}
//-----------------------------------------------------------------------------
static void PortX(FILE *fi, FILE *fu, const char *str)
{
    char prt[4];
    int  portN, Reg, Mask, Addr;
    bool b = ParceVar(str, prt, &portN, &Reg, &Mask, &Addr);

    fprintf(fi, "\n");
    fprintf(fi, "function   Read_%s:boolean;\n", str);
    fprintf(fi, "procedure Write_%s(b:boolean);\n", str);
    fprintf(fi, "\n");

    fprintf(fu, "function Read_%s:boolean;\n", str);
    fprintf(fu, "begin\n");
    if(!b)
        fprintf(fu, "  {!!! not assigned to IO pin '%s' !!!}\n", str + 4);
    fprintf(fu, "  {$ifNdef FINAL}\n");
    fprintf(fu, "  if a%s[%d].Gate[%d] = nil then\n", prt, portN, Reg);
    fprintf(fu, "    RunError(204);\n");
    fprintf(fu, "  if a%s[%d].Gate[%d]^.PortAddr = 0 then\n", prt, portN, Reg);
    fprintf(fu, "    RunError(204);\n");
    fprintf(fu, "  {$endif}\n");

    fprintf(fu, "  {$ifdef HookIn}\n");
    fprintf(fu, "  if InArr[%d].IskbrdHook then begin\n", InCount - 1);
    fprintf(fu, "    Read_%s:=InArr[%d].HookValue;\n", str, InCount - 1);
    fprintf(fu, "    exit;\n");
    fprintf(fu, "  end;\n");
    fprintf(fu, "  {$endif}\n");

    fprintf(fu, "  {$ifdef PortBufferedIn}\n");
    fprintf(fu, "  Read_%s:=a%s[%d].Gate[%d]^.Buf and $%x <> 0;\n", str, prt, portN, Reg, Mask);
    fprintf(fu, "  {$endif}\n");
    fprintf(fu, "  {$ifdef PortDirectIn}\n");
    fprintf(fu,
            "  Read_%s:=(in_portb(a%s[%d].BaseAddr+%d) xor a%s[%d].Gate[%d]^.XorMask) and $%x <> 0;\n",
            str,
            prt,
            portN,
            Reg,
            prt,
            portN,
            Reg,
            Mask);
    //  fprintf(fu, "  Read_%s:=(in_portb($%x)                 xor a%s[%d].Gate[%d]^.XorMask) and $%x <> 0;\n", str, Addr, prt, portN, Reg, Mask);
    fprintf(fu, "  {$endif}\n");
    fprintf(fu, "end;\n");
    fprintf(fu, "\n");

    fprintf(fu, "procedure Write_%s(b:boolean);\n", str);
    fprintf(fu, "begin\n");
    if(!b)
        fprintf(fu, "  {!!! not assigned to IO pin '%s' !!!}\n", str + 4);
    fprintf(fu, "  {$ifNdef FINAL}\n");
    fprintf(fu, "  {$ifNdef PortBufferedIn}\n");
    fprintf(fu, "  {RunError(204);}\n");
    fprintf(fu, "  {$endif}\n");
    fprintf(fu, "  if a%s[%d].Gate[%d] = nil then\n", prt, portN, Reg);
    fprintf(fu, "    RunError(204);\n");
    fprintf(fu, "  if a%s[%d].Gate[%d]^.PortAddr = 0 then\n", prt, portN, Reg);
    fprintf(fu, "    RunError(204);\n");
    fprintf(fu, "  {$endif}\n");

    fprintf(fu, "  {$ifdef PortBufferedIn}\n");
    fprintf(fu, "  if b then begin\n");
    fprintf(
        fu, "    a%s[%d].Gate[%d]^.Buf:=a%s[%d].Gate[%d]^.Buf or      $%x;\n", prt, portN, Reg, prt, portN, Reg, Mask);
    fprintf(fu, "  end else begin\n");
    fprintf(
        fu, "    a%s[%d].Gate[%d]^.Buf:=a%s[%d].Gate[%d]^.Buf and not $%x;\n", prt, portN, Reg, prt, portN, Reg, Mask);
    fprintf(fu, "  end;\n");
    fprintf(fu, "  {$endif}\n");
    fprintf(fu, "end;\n");
    fprintf(fu, "\n");
}
//-----------------------------------------------------------------------------
static void PortY(FILE *fi, FILE *fu, const char *str)
{
    char prt[4];
    int  portN, Reg, Mask, Addr;
    bool b = ParceVar(str, prt, &portN, &Reg, &Mask, &Addr);

    fprintf(fi, "\n");
    fprintf(fi, "function   Read_%s:boolean;\n", str);
    fprintf(fi, "procedure Write_%s(b:boolean);\n", str);
    fprintf(fi, "\n");

    fprintf(fu, "function Read_%s:boolean;\n", str);
    fprintf(fu, "begin\n");
    if(!b)
        fprintf(fu, "  {!!! not assigned to IO pin '%s' !!!}\n", str + 4);
    fprintf(fu, "  {$ifNdef FINAL}\n");
    fprintf(fu, "  if a%s[%d].Gate[%d] = nil then\n", prt, portN, Reg);
    fprintf(fu, "    RunError(204);\n");
    fprintf(fu, "  if a%s[%d].Gate[%d]^.PortAddr = 0 then\n", prt, portN, Reg);
    fprintf(fu, "    RunError(204);\n");
    fprintf(fu, "  {$endif}\n");
    fprintf(fu, "  {$ifdef PortBufferedIn}\n");
    fprintf(fu, "  Read_%s:=a%s[%d].Gate[%d]^.Buf and $%x <> 0;\n", str, prt, portN, Reg, Mask);
    fprintf(fu, "  {$endif}\n");

    fprintf(fu, "  {$ifdef PortDirectIn}\n");
    fprintf(fu,
            "  Read_%s:=(in_portb(a%s[%d].BaseAddr+%d) xor a%s[%d].Gate[%d]^.XorMask) and $%x <> 0;\n",
            str,
            prt,
            portN,
            Reg,
            prt,
            portN,
            Reg,
            Mask);
    fprintf(fu, "  {$endif}\n");
    fprintf(fu, "end;\n");
    fprintf(fu, "\n");

    fprintf(fu, "procedure Write_%s(b:boolean);\n", str);
    fprintf(fu, "begin\n");
    if(!b)
        fprintf(fu, "  {!!! not assigned to IO pin '%s' !!!}\n", str + 4);
    fprintf(fu, "  {$ifNdef FINAL}\n");
    fprintf(fu, "  if a%s[%d].Gate[%d] = nil then\n", prt, portN, Reg);
    fprintf(fu, "    RunError(204);\n");
    fprintf(fu, "  if a%s[%d].Gate[%d]^.PortAddr = 0 then\n", prt, portN, Reg);
    fprintf(fu, "    RunError(204);\n");
    fprintf(fu, "  {$endif}\n");
    fprintf(fu, "  if b then begin\n");
    fprintf(
        fu, "    a%s[%d].Gate[%d]^.Buf:=a%s[%d].Gate[%d]^.Buf or      $%x;\n", prt, portN, Reg, prt, portN, Reg, Mask);
    fprintf(fu, "  end else begin\n");
    fprintf(
        fu, "    a%s[%d].Gate[%d]^.Buf:=a%s[%d].Gate[%d]^.Buf and not $%x;\n", prt, portN, Reg, prt, portN, Reg, Mask);
    fprintf(fu, "  end;\n");

    fprintf(fu, "  {$ifdef PortDirectOut}\n");
    fprintf(fu,
            "  out_portb(a%s[%d].BaseAddr+%d, a%s[%d].Gate[%d]^.Buf xor a%s[%d].Gate[%d]^.XorMask);\n",
            prt,
            portN,
            Reg,
            prt,
            portN,
            Reg,
            prt,
            portN,
            Reg);
    fprintf(fu, "  {$endif}\n");
    fprintf(fu, "end;\n");
    fprintf(fu, "\n");
}
//-----------------------------------------------------------------------------
// Generate a declaration for a bit var; three cases, input, output, and
// internal relay. An internal relay is just a bool variable, but for an
// input or an output someone else must provide read/write functions.
//-----------------------------------------------------------------------------
static void DeclareBit(FILE *f, FILE *fu, FILE *fi, FILE *fr, FILE *fv, const char *str, char *_str)
{
    // The mapped symbol has the form Ub_{X,Y,R}name, so look at character
    // four to determine if it's an input, output, internal relay.
    int type = GetAssignedType(_str, _str);

    if(type == IO_TYPE_DIG_INPUT) {
        ResetAndRegister(fr, fv, str, _str);
        PortX(fi, fu, str);
    } else if(type == IO_TYPE_DIG_OUTPUT) {
        ResetAndRegister(fr, fv, str, _str);
        PortY(fi, fu, str);
    } else if(str[0] == 'U') {
        ResetAndRegister(fr, fv, str, _str);
        fprintf(fi, "const %s : boolean = false;\n", str);
    } else {
        ResetAndRegister(fr, fv, str, _str);
        fprintf(fi, "const %s : boolean = false;\n", str);
    }
}

//-----------------------------------------------------------------------------
// Generate declarations for all the 16-bit/single bit variables in the ladder
// program.
//-----------------------------------------------------------------------------
static void GenerateDeclarations(FILE *f, FILE *fu, FILE *fi, FILE *fr, FILE *fv)
{
    char _bitVar1[MAX_NAME_LEN], _bitVar2[MAX_NAME_LEN];
    char _intVar1[MAX_NAME_LEN], _intVar2[MAX_NAME_LEN], _intVar3[MAX_NAME_LEN];

    for(uint32_t i = 0; i < IntCode.size(); i++) {
        const char *bitVar1 = nullptr, *bitVar2 = nullptr;
        const char *intVar1 = nullptr, *intVar2 = nullptr, *intVar3 = nullptr;

        switch(IntCode[i].op) {
            case INT_SET_BIT:
            case INT_CLEAR_BIT:
                bitVar1 = IntCode[i].name1.c_str();
                break;

            case INT_COPY_BIT_TO_BIT:
                bitVar1 = IntCode[i].name1.c_str();
                bitVar2 = IntCode[i].name2.c_str();
                break;

            case INT_SET_VARIABLE_RANDOM:
            case INT_SET_VARIABLE_TO_LITERAL:
                intVar1 = IntCode[i].name1.c_str();
                break;

            case INT_SET_SEED_RANDOM:
            case INT_SET_BIN2BCD:
            case INT_SET_BCD2BIN:
            case INT_SET_OPPOSITE:
            case INT_SET_VARIABLE_NOT:
            case INT_SET_SWAP:
            case INT_SET_VARIABLE_NEG:
            case INT_SET_VARIABLE_TO_VARIABLE:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                break;

            case INT_SET_VARIABLE_SHL:
            case INT_SET_VARIABLE_SHR:
            case INT_SET_VARIABLE_SR0:
            case INT_SET_VARIABLE_ROL:
            case INT_SET_VARIABLE_ROR:
            case INT_SET_VARIABLE_AND:
            case INT_SET_VARIABLE_OR:
            case INT_SET_VARIABLE_XOR:
            case INT_SET_VARIABLE_MOD:
            case INT_SET_VARIABLE_DIVIDE:
            case INT_SET_VARIABLE_MULTIPLY:
            case INT_SET_VARIABLE_SUBTRACT:
            case INT_SET_VARIABLE_ADD:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                intVar3 = IntCode[i].name3.c_str();
                break;

            case INT_DECREMENT_VARIABLE:
            case INT_INCREMENT_VARIABLE:
            case INT_READ_ADC:
            case INT_SET_PWM:
                intVar1 = IntCode[i].name1.c_str();
                break;

            case INT_UART_RECV:
            case INT_UART_SEND:
            case INT_UART_SEND1:
            case INT_UART_SENDn:
                intVar1 = IntCode[i].name1.c_str();
                bitVar1 = IntCode[i].name2.c_str();
                break;

            case INT_UART_RECV_AVAIL:
            case INT_UART_SEND_READY:
            case INT_UART_SEND_BUSY:
                intVar1 = IntCode[i].name1.c_str();
                break;

            case INT_IF_BIT_SET:
            case INT_IF_BIT_CLEAR:
                bitVar1 = IntCode[i].name1.c_str();
                break;

#ifndef NEW_CMP
            case INT_IF_VARIABLE_LES_LITERAL:
                intVar1 = IntCode[i].name1.c_str();
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
            case INT_IF_VARIABLE_GRT_VARIABLE:
                intVar1 = IntCode[i].name1.c_str();
                intVar2 = IntCode[i].name2.c_str();
                break;
#endif

#ifdef NEW_CMP
            case INT_IF_EQU:
            case INT_IF_NEQ:
            case INT_IF_LES:
            case INT_IF_GRT:
            case INT_IF_LEQ:
            case INT_IF_GEQ:
                if(!IsNumber(IntCode[i].name1))
                    intVar1 = IntCode[i].name1.c_str();
                if(!IsNumber(IntCode[i].name2))
                    intVar2 = IntCode[i].name2.c_str();
                break;
#endif

            case INT_END_IF:
            case INT_ELSE:
            case INT_COMMENT:
            case INT_DELAY:
            case INT_SIMULATE_NODE_STATE:
            case INT_EEPROM_BUSY_CHECK:
            case INT_EEPROM_READ:
            case INT_EEPROM_WRITE:

            case INT_AllocKnownAddr:
            case INT_AllocFwdAddr:
            case INT_FwdAddrIsNow:
            case INT_GOTO:
            case INT_GOSUB:
                break;

#ifdef TABLE_IN_FLASH
            case INT_FLASH_INIT:
                break;
            case INT_RAM_READ:
            case INT_FLASH_READ:
                intVar1 = IntCode[i].name1.c_str();
                break;
#endif

            default:
                ooops("INT_%d", IntCode[i].op);
        }

        if(bitVar1)
            strcpy(_bitVar1, bitVar1);
        if(bitVar2)
            strcpy(_bitVar2, bitVar2);
        if(intVar1)
            strcpy(_intVar1, intVar1);
        if(intVar2)
            strcpy(_intVar2, intVar2);
        if(intVar3)
            strcpy(_intVar3, intVar3);

        bitVar1 = MapSym(bitVar1, ASBIT);
        bitVar2 = MapSym(bitVar2, ASBIT);

        intVar1 = MapSym(intVar1, ASINT);
        intVar2 = MapSym(intVar2, ASINT);
        intVar3 = MapSym(intVar3, ASINT);

        if(bitVar1 && !SeenVariable(bitVar1))
            DeclareBit(f, fu, fi, fr, fv, bitVar1, _bitVar1);
        if(bitVar2 && !SeenVariable(bitVar2))
            DeclareBit(f, fu, fi, fr, fv, bitVar2, _bitVar2);

        if(intVar1 && !SeenVariable(intVar1))
            DeclareInt(f, fi, fr, fv, intVar1, _intVar1);
        if(intVar2 && !SeenVariable(intVar2))
            DeclareInt(f, fi, fr, fv, intVar2, _intVar2);
        if(intVar3 && !SeenVariable(intVar3))
            DeclareInt(f, fi, fr, fv, intVar3, _intVar3);
    }
}
//-----------------------------------------------------------------------------
static int SimpleBool(const NameArray &name)
{
    if(name[0] == '$')
        return 1;
    else if(name[0] == 'X')
        return 0;
    else if(name[0] == 'Y')
        return 0;
    else
        return 1;
}
//-----------------------------------------------------------------------------
// Actually generate the C source for the datas.
//-----------------------------------------------------------------------------
static void Generate_flash_eeprom(FILE *f)
{
#ifdef TABLE_IN_FLASH
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        switch(IntCode[i].op) {
            case INT_FLASH_INIT: {
                int         sovElement = IntCode[i].literal2;
                const char *sovs;

                if(sovElement == 1) {
                    sovs = "byte";
                } else if(sovElement == 2) {
                    sovs = "word";
                } else if(sovElement == 3) {
                    sovs = "longint";
                } else if(sovElement == 4) {
                    sovs = "longint";
                } else {
                    THROW_COMPILER_EXCEPTION_FMT("sovElement=%d", sovElement);
                }

                fprintf(
                    f, "const %s: array [0..%d] of %s = (", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal-1, sovs);
                int j;
                for(j = 0; j < (IntCode[i].literal - 1); j++) {
                    fprintf(f, "%d, ", IntCode[i].data[j]);
                }
                fprintf(f, "%d);\n", IntCode[i].data[IntCode[i].literal - 1]);
                break;
            }
                //const Ui_stepZ:array [1..6] of byte = (0, 6, 3, 2, 2, 1);
#ifdef NEW_FEATURE
            case INT_EEPROM_INIT: {
                fprintf(f, "epprom datas;\n");
                break;
            }
#endif
            default: {
            }
        }
    }
#endif
}
//-----------------------------------------------------------------------------
static int  indent = 1;
static void doIndent(FILE *f, int i)
{
    if((IntCode[i].op != INT_SIMULATE_NODE_STATE) //
       && (IntCode[i].op != INT_AllocKnownAddr) //
       && (IntCode[i].op != INT_FwdAddrIsNow) //
       && (IntCode[i].op != INT_AllocFwdAddr))
        for(int j = 0; j < indent; j++)
#if 1
            fprintf(f, "  ");
#else
            fprintf(f, " %d ", IntCode[i].op);
#endif
}
//-----------------------------------------------------------------------------
// Actually generate the PASCAL source for the program.
//-----------------------------------------------------------------------------
static void GenerateSourceCode(FILE *f, int begin, int end)
{
    fprintf(f,
            "{  Call this function once per PLC cycle. You are responsible for calling\n"
            "   it at the interval that you specified in the MCU configuration when you\n"
            "   generated this code. }\n"
            "procedure PlcCycle;\n");
    fprintf(f, "  label Rung0label;\n");
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        switch(IntCode[i].op) {
            //case INT_AllocKnownAddr:
            case INT_AllocFwdAddr:
                fprintf(f, "  label LabelRung%d;\n", IntCode[i].literal + 1);
                break;
            default:;
        }
    }
    fprintf(f,
            "begin\n"
            "  Rung0label:;\n");
    indent = 1;
    for(uint32_t i = 0; i < IntCode.size(); i++) {

        if(IntCode[i].op == INT_END_IF)
            indent--;
        if(IntCode[i].op == INT_ELSE)
            indent--;

        doIndent(f, i);

        switch(IntCode[i].op) {
            case INT_SET_BIT:
                if(SimpleBool(IntCode[i].name1))
                    fprintf(f, "%s:=true;\n", MapSym(IntCode[i].name1, ASBIT));
                else
                    fprintf(f, "Write_%s(true);\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_CLEAR_BIT:
                if(SimpleBool(IntCode[i].name1))
                    fprintf(f, "%s:=false;\n", MapSym(IntCode[i].name1, ASBIT));
                else
                    fprintf(f, "Write_%s(false);\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_COPY_BIT_TO_BIT:
                if(SimpleBool(IntCode[i].name1))
                    fprintf(f,
                            "%s:=%s%s;\n",
                            MapSym(IntCode[i].name1, ASBIT),
                            SimpleBool(IntCode[i].name2) ? "" : "Read_",
                            MapSym(IntCode[i].name2, ASBIT));
                else
                    fprintf(f,
                            "Write_%s(%s%s);\n",
                            MapSym(IntCode[i].name1, ASBIT),
                            SimpleBool(IntCode[i].name2) ? "" : "Read_",
                            MapSym(IntCode[i].name2, ASBIT));
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                fprintf(f, "%s := %d;\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal);
                break;

            case INT_SET_VARIABLE_RANDOM:
                fprintf(f, "%s := random();\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_SET_SEED_RANDOM:
                fprintf(f, "seed_%s := %s;\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                fprintf(f, "%s := %s;\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_BIN2BCD:
                fprintf(f, "%s := bin2bcd(%s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_BCD2BIN:
                fprintf(f, "%s := bcd2bin(%s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_OPPOSITE:
                fprintf(f, "%s := opposite(%s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_VARIABLE_NOT:
                fprintf(f, "%s := not %s;\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_SWAP:
                fprintf(f, "%s := swap(%s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_VARIABLE_NEG:
                fprintf(f, "%s := - %s;\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;

                {
                    const char *op;
                    case INT_SET_VARIABLE_SHL:
                        op = "shl";
                        goto arith;
                    case INT_SET_VARIABLE_SHR:
                        op = "shr";
                        goto arith;
                    case INT_SET_VARIABLE_SR0:
                        op = "sr0";
                        goto arith;
                    case INT_SET_VARIABLE_ROL:
                        op = "rol";
                        goto arith;
                    case INT_SET_VARIABLE_ROR:
                        op = "ror";
                        goto arith;
                    case INT_SET_VARIABLE_AND:
                        op = "and";
                        goto arith;
                    case INT_SET_VARIABLE_OR:
                        op = "or";
                        goto arith;
                    case INT_SET_VARIABLE_XOR:
                        op = "xor";
                        goto arith;
                    case INT_SET_VARIABLE_ADD:
                        op = "+";
                        goto arith;
                    case INT_SET_VARIABLE_SUBTRACT:
                        op = "-";
                        goto arith;
                    case INT_SET_VARIABLE_MULTIPLY:
                        op = "*";
                        goto arith;
                    case INT_SET_VARIABLE_DIVIDE:
                        op = "/";
                        goto arith;
                    case INT_SET_VARIABLE_MOD:
                        op = "mod";
                        goto arith;
                    arith:
                        fprintf(f,
                                "%s := %s %s %s;\n",
                                MapSym(IntCode[i].name1, ASINT),
                                MapSym(IntCode[i].name2, ASINT),
                                op,
                                MapSym(IntCode[i].name3, ASINT));
                        break;
                }

            case INT_INCREMENT_VARIABLE:
                fprintf(f, "inc(%s);\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_DECREMENT_VARIABLE:
                fprintf(f, "dec(%s);\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_IF_BIT_SET:
                fprintf(f,
                        "if %s%s then begin\n",
                        SimpleBool(IntCode[i].name1) ? "" : "Read_",
                        MapSym(IntCode[i].name1, ASBIT));
                indent++;
                break;

            case INT_IF_BIT_CLEAR:
                fprintf(f,
                        "if not %s%s then begin\n",
                        SimpleBool(IntCode[i].name1) ? "" : "Read_",
                        MapSym(IntCode[i].name1, ASBIT));
                indent++;
                break;

#ifdef NEW_CMP
            case INT_IF_NEQ:
                fprintf(
                    f, "if %s <> %s then begin\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;
#endif

#ifdef NEW_CMP
            case INT_IF_LES:
                fprintf(f, "if %s < %s then begin\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].name2.c_str());
#else
            case INT_IF_VARIABLE_LES_LITERAL:
                fprintf(f, "if %s < %d then begin\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal);
#endif
                indent++;
                break;

#ifdef NEW_CMP
            case INT_IF_EQU:
#else
            case INT_IF_VARIABLE_EQUALS_VARIABLE:
#endif
                fprintf(f, "if %s = %s then begin\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

#ifdef NEW_CMP
            case INT_IF_GRT:
#else
            case INT_IF_VARIABLE_GRT_VARIABLE:
#endif
                fprintf(f, "if %s > %s then begin\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

#ifdef NEW_CMP
            case INT_IF_LEQ:
                fprintf(
                    f, "if %s <= %s then begin\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_GEQ:
                fprintf(
                    f, "if %s >= %s then begin\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;
#endif

            case INT_END_IF:
                fprintf(f, "end;\n");
                break;

            case INT_ELSE:
                fprintf(f, "end else begin\n");
                indent++;
                break;

            case INT_SIMULATE_NODE_STATE:
                // simulation-only
                //fprintf(f, "\n");
                break;

            case INT_COMMENT:
                if(IntCode[i].name1.size()) {
                    fprintf(f, "{ %s }\n", IntCode[i].name1.c_str());
                } else {
                    fprintf(f, "\n");
                }
                break;

            case INT_DELAY:
                fprintf(f, "DelayUs(%d)//us;", IntCode[i].literal);
                break;

            case INT_EEPROM_BUSY_CHECK:
                fprintf(f, "{INT_EEPROM_BUSY_CHECK %s }\n", IntCode[i].name1.c_str());
                break;
            case INT_EEPROM_READ:
                fprintf(f, "{INT_EEPROM_READ %s }\n", IntCode[i].name1.c_str());
                break;
            case INT_EEPROM_WRITE:
                fprintf(f, "{INT_EEPROM_WRITE %s }\n", IntCode[i].name1.c_str());
                break;

            case INT_AllocKnownAddr:
                fprintf(f, "Label%s:;\n", IntCode[i].name1.c_str());
                break;
            case INT_AllocFwdAddr:
            case INT_FwdAddrIsNow:
                break;
            case INT_RETURN:
                fprintf(f, "exit;\n");
                break;
            case INT_GOSUB:
                fprintf(f, "CallLabel%s;\n", IntCode[i].name1.c_str());
                break;
            case INT_GOTO:
                fprintf(f, "goto Label%s;\n", IntCode[i].name1.c_str());
                break;
            case INT_READ_ADC:
            case INT_SET_PWM:
            case INT_UART_SEND_BUSY:
            case INT_UART_SEND_READY:
            case INT_UART_RECV_AVAIL:
            case INT_UART_RECV:
            case INT_UART_SEND:
            case INT_UART_SEND1:
            case INT_UART_SENDn:
                Error(_(
                    " PASCAL target does not support peripherals (UART, PWM, ADC, EEPROM). Skipping that instruction."));
                break;

#ifdef TABLE_IN_FLASH
            case INT_FLASH_INIT: {
                break;
            }
            case INT_FLASH_READ: {
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f,
                            "%s := %d; { %s[%s] }\n",
                            MapSym(IntCode[i].name1, ASINT),
                            IntCode[i].data[CheckMakeNumber(IntCode[i].name3.c_str())],
                            MapSym(IntCode[i].name2, ASINT),
                            IntCode[i].name3.c_str());
                } else {
                    fprintf(f,
                            "%s := %s[%s];\n",
                            MapSym(IntCode[i].name1, ASINT),
                            MapSym(IntCode[i].name2, ASINT),
                            MapSym(IntCode[i].name3, ASINT));
                }
                break;
            }
            case INT_RAM_READ: {
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f,
                            "%s := %s[%d]\n",
                            MapSym(IntCode[i].name1, ASINT),
                            MapSym(IntCode[i].name2, ASSTR),
                            CheckMakeNumber(IntCode[i].name3.c_str()));
                } else {
                    fprintf(f,
                            "%s := %s[%s];\n",
                            MapSym(IntCode[i].name1, ASINT),
                            MapSym(IntCode[i].name2, ASSTR),
                            MapSym(IntCode[i].name3, ASINT));
                }
                break;
            }
#endif

            default:
                THROW_COMPILER_EXCEPTION_FMT("INT_%d", IntCode[i].op);
        }
    }
    fprintf(f, "end;\n");
}

//-----------------------------------------------------------------------------
static void GenerateSUBPROG(FILE *f)
{
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        switch(IntCode[i].op) {
            case INT_GOSUB: {
                fprintf(f, "\n");
                fprintf(f,
                        "procedure Call_SUBPROG_%s() { // LabelRung%d\n",
                        IntCode[i].name1.c_str(),
                        IntCode[i].literal + 1);
                int indentSave = indent;
                indent = 1;
                GenerateSourceCode(f,
                                   FindOpName(INT_AllocKnownAddr, IntCode[i].name1, "SUBPROG") + 1,
                                   FindOpNameLast(INT_RETURN, IntCode[i].name1));
                indent = indentSave;
                fprintf(f, "}\n");
            }
        }
    }
}

//-----------------------------------------------------------------------------
static char *_GetUnitName(char *dest)
{
    char *c;
    if(strlen(dest)) {
        c = strrchr(dest, '.');
        c[0] = '\0';
    };
    if(strlen(dest)) {
        c = strrchr(dest, '\\');
        dest = ++c;
    };
    return dest;
}
//-----------------------------------------------------------------------------
static char *GetUnitName(char *unitn, char *dest)
{
    strcpy(unitn, dest);
    return _GetUnitName(unitn);
}
//-----------------------------------------------------------------------------
void CompilePascal(const char *dest)
{
    SeenVariablesCount = 0;

    char destn[MAX_PATH];
    char destu[MAX_PATH];
    char destr[MAX_PATH];
    char destv[MAX_PATH];
    char desti[MAX_PATH];

    char _destn[MAX_PATH];
    char _destu[MAX_PATH];
    char _destr[MAX_PATH];
    char _destv[MAX_PATH];

    char *unitn;
    char *unitu;
    char *unitr;
    char *unitv;

    strcpy(destn, dest);
    SetExt(destu, dest, "U.pas");
    SetExt(destr, dest, "R.pas");
    SetExt(destv, dest, "V.pas");
    SetExt(desti, dest, "U.inc");

    FileTracker f(dest, "w");
    if(!f) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), dest);
        return;
    }
    FileTracker fu(destu, "w");
    if(!fu) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), destu);
        return;
    }
    FileTracker fr(destr, "w");
    if(!fr) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), destr);
        return;
    }
    FileTracker fv(destv, "w");
    if(!fv) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), destv);
        return;
    }
    FileTracker fi(desti, "w");
    if(!fi) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't open file '%s'"), desti);
        return;
    }

    unitn = GetUnitName(_destn, destn);
    unitu = GetUnitName(_destu, destu);
    unitr = GetUnitName(_destr, destr);
    unitv = GetUnitName(_destv, destv);

    fprintf(fi,
            "type SDWORD = longint;{signed}\n"
            "type UWORD = word;{unsigned}\n"
            "{ This is INTERFACE section of '%s.pas' unit. }\n"
            "{ You provide all these functions and procedures. }\n",
            unitu);

    fprintf(fu,
            "{$I machine.inc}\n"
            "{-$define PortBufferedIn}\n"
            "{-$define PortDirectIn}\n"
            "{$ifdef PortBufferedIn} {$UNDEF PortDirectIn}   {$endif}\n"
            "{$ifdef PortDirectIn}   {$UNDEF PortBufferedIn} {$endif}\n"
            "\n"
            "{-$define PortBufferedOut}\n"
            "{-$define PortDirectOut}\n"
            "{$ifdef PortBufferedOut} {$UNDEF PortDirectOut}   {$endif}\n"
            "{$ifdef PortDirectOut}   {$UNDEF PortBufferedOut} {$endif}\n"
            "{$E-,N-}\n"
            "unit %s;\n"
            "{ You provide all these functions and procedures. }\n"
            "interface\n"
            "uses PcPorts, ports_, portIO;\n",
            unitu);

    fprintf(fr,
            "{$E-,N-}\n"
            "unit %s;\n"
            "interface\n"
            "uses %s, %s, %s;\n",
            unitr,
            unitn,
            unitu,
            unitv);
    fprintf(fr,
            "procedure ResetPlcVars; { Resets all user vars. }\n"
            "\n"
            "implementation\n"
            "procedure ResetPlcVars;\n"
            "begin\n");
    fprintf(fv,
            "{$I machine.inc}\n"
            "{$E-,N-}\n"
            "unit %s;\n"
            "interface\n"
            "uses ld, %s, %s, keybrd{, win_vk};\n",
            unitv,
            unitn,
            unitu);
    fprintf(fv,
            "{procedure RegisterPlcVars; { Register all user vars. }\n"
            "implementation\n"
            "const\n"
            "  InCount:word=0;\n"
            "  RelayCount:word=0;\n"
            "  TimerCount:word=0;\n"
            "  VarCount:word=0;\n"
            "  OutCount:word=0;\n"
            "procedure RegisterPlcVars;\n"
            "begin\n");
    fprintf(f,
            "{$I machine.inc}\n"
            "{$E-,N-}\n"
            "unit %s;\n"
            "interface\n",
            unitn);
    fprintf(f,
            "uses %s, ports_{, crt, serv_crt, cpuspeed, utypes};\n"
            "{  This is auto-generated code from LDmicro. Do not edit this file! Go\n"
            "   back to the ladder diagram source for changes in the logic, and make\n"
            "   any PASCAL additions either in ladder.pas unit or in additional .pas files linked\n"
            "   against this one. }\n"
            "\n"
            "{  You must provide ladder.pas unit; there you must provide:\n"
            "      * a typedef for SWORD and bool, signed 16 bit and boolean types\n"
            "        (probably typedef signed short SWORD; typedef unsigned char bool;)\n"
            "\n"
            "   You must also provide implementations of all the I/O read/write\n"
            "   either as inlines in the header file or in another source file. (The\n"
            "   I/O functions are all declared extern.)\n"
            "\n"
            "   See the generated source code (below) for function names. }\n",
            unitu);
    fprintf(f,
            "\n"
            "{  Define EXTERN_EVERYTHING in ladder.pas if you want all symbols extern.\n"
            "   This could be useful to implement `magic variables,' so that for\n"
            "   example when you write to the ladder variable duty_cycle, your PLC\n"
            "   runtime can look at the PASCAL variable U_duty_cycle and use that to set\n"
            "   the PWM duty cycle on the micro. That way you can add support for\n"
            "   peripherals that LDmicro doesn't know about. }\n"
            "{\n"
            "#ifdef EXTERN_EVERYTHING\n"
            "#define STATIC \n"
            "#else\n"
            "#define STATIC static\n"
            "#endif\n"
            "}\n"
            "{  Define NO_PROTOTYPES if you don't want LDmicro to provide prototypes for\n"
            "   all the I/O procedures (Read_Ux_xxx, Write_Ux_xxx) that you must provide.\n"
            "   If you define this then you must provide your own prototypes for these\n"
            "   functions in ladder.pas, or provide definitions (e.g. as inlines or macros)\n"
            "   for them in ladder.pas. }\n"
            "{\n"
            "#ifdef NO_PROTOTYPES\n"
            "#define PROTO(x)\n"
            "#else\n"
            "#define PROTO(x) x\n"
            "#endif\n"
            "}\n"
            "{  Ux_xxx symbols correspond to user-defined names. There is such a symbol\n"
            "   for every internal relay, variable, timer, and so on in the ladder\n"
            "   program. Ix_xxx symbols are internally generated. }\n");

    fprintf(f,
            "procedure PlcCycle;\n"
            "\n"
            "const PlcCycleTimeUs:comp=%d; {us} {from %s.ld CYCLE=}\n"
            "\n"
            "implementation\n\n",
            Prog.cycleTime,
            unitn);
    fprintf(fu, "{$I %s.inc}\n", unitu);
    fprintf(fu,
            "implementation\n"
            "uses ld;\n"
            "\n");

    InCount = 0;
    OutCount = 0;
    TimerCount = 0;
    RelayCount = 0;
    VarCount = 0;
    // now generate declarations for all variables
    GenerateDeclarations(f, fu, fi, fr, fv);
    Generate_flash_eeprom(f);
    GenerateSUBPROG(f);

    fprintf(f,
            "(*\n"
            "const CounterNext:comp=-9.2e18;\n"
            "const Counter_:comp=-9.2e18;\n"
            "const Counter0:comp=0;\n"
            "const Counter:comp=0;\n"
            "\n"
            "procedure InitPlcCycle;\n"
            "begin\n"
            "  SetT64(PlcCycleTimeTsc,PlcCycleTimeUs);\n"
            "  UsAsTsc(PlcCycleTimeTsc);\n"
            "  SetT64(CounterNext,Counter_);\n"
            "end;\n\n"
            "\n"
            "var n:double; i:integer;\n"
            "procedure CallPlcCycle;\n"
            "begin\n"
            "  getTsc(Counter);{not use x87}\n"
            "  if Counter>CounterNext then begin\n"
            "   {if CounterNext<0 then\n"
            "      CounterNext:=Counter;}\n"
            "\n"
            "    n:=(double(Counter)-double(CounterNext))/double(PlcCycleTimeTsc);\n"
            "    if n>10.0 then begin\n"
            "      n:=11.0;\n"
            "    end;\n"
            "    {$ifndef FINAL}\n"
            "    gotoXY(1,MaxY-1);\n"
            "    write('tsc=',PlcCycleTimeTsc:0:0,' n=',n:0:3,' ',\n"
            "      Counter-CounterNext:10:0,' ',\n"
            "      '         ');\n"
            "    {$endif}\n"
            "    if n>=1.0 then\n"
            "      CounterNext:=Counter;\n"
            "\n"
            "    CounterNext:=CounterNext + PlcCycleTimeTsc;\n"
            "   {if CounterNext<0 then\n"
            "      CounterNext:=Counter;}\n"
            "    for i:=trunc(n) downto 0 do begin\n"
            "      {$ifdef PortBufferedIn}\n"
            "      {InBufs;{}\n"
            "      {$endif}\n"
            "      PlcCycle;\n"
            "      {$ifdef PortBufferedOut}\n"
            "      {OutBufs;{}\n"
            "      {$endif}\n"
            "    end;\n"
            "  end;\n"
            "end;\n"
            "(*\n"
            "var Sub,n:comp; i:integer;\n"
            "procedure CallPlcCycle;\n"
            "begin\n"
            "  getTsc(Counter);{not use x87}\n"
            "  if CmpT64(Counter,CounterNext)>=0 then begin\n"
            "    {if CmpT64(CounterNext,Counter0)<0 then\n"
            "      SetT64(CounterNext,Counter);}\n"
            "\n"
            "    SubT64(Sub,Counter,CounterNext);\n"
            "    DivT64(n,Sub,PlcCycleTimeTsc);\n"
            "    {}\n"
            "    n:=(double(Counter)-double(CounterNext))/double(PlcCycleTimeTsc);\n"
            "    if n>10 then begin\n"
            "      n:=11;\n"
            "    end;\n"
            "    {$ifndef FINAL}\n"
            "    gotoXY(1,MaxY-1);\n"
            "    write('tsc=',PlcCycleTimeTsc:0:0,' n=',n:0:3,' ',\n"
            "      Counter-CounterNext:10:0,' ',\n"
            "      '         ');\n"
            "    {$endif}\n"
            "    if n>1 then\n"
            "      SetT64(CounterNext,Counter);\n"
            "    {}\n"
            "    AddT64(CounterNext,CounterNext,PlcCycleTimeTsc);\n"
            "    for i:=trunc(n) downto 0 do begin\n"
            "      {$ifdef PortBufferedIn}\n"
            "      {InBufs;{}\n"
            "      {$endif}\n"
            "      PlcCycle;\n"
            "      {$ifdef PortBufferedOut}\n"
            "      {OutBufs;{}\n"
            "      {$endif}\n"
            "    end;\n"
            "  end;\n"
            "end;\n"
            "*)\n");

    GenerateSourceCode(f, 0, IntCode.size() - 1);

    fprintf(f,
            "\n"
            "{begin}\n"
            "end.\n");
    fprintf(fu,
            "\n"
            "begin\n"
            "  {SetPcPortsDir;}\n"
            "end.\n");
    fprintf(fr,
            "end;\n"
            "\n"
            "begin\n"
            "  ResetPlcVars;\n"
            "end.\n");
    fprintf(fv,
            "end;\n"
            "\n"
            "begin\n"
            "  RegisterPlcVars;\n"
            "end.\n");
    fprintf(fi,
            "const\n"
            "  InCountConst    =%d;\n"
            "  OutCountConst   =%d;\n"
            "  TimerCountConst =%d;\n"
            "  RelayCountConst =%d;\n"
            "  VarCountConst   =%d;\n",
            InCount,
            OutCount,
            TimerCount,
            RelayCount,
            VarCount);

    f.close();
    fu.close();
    fr.close();
    fv.close();
    fi.close();
    /*
    TranslitFile(dest );
    TranslitFile(destu);
    TranslitFile(destr);
    TranslitFile(destv);
    TranslitFile(desti);
    */
    char str[MAX_PATH + 500];
    sprintf(str,
            _("Compile successful; wrote PASCAL source code to '%s'.\r\n\r\n"
              "This is not a complete PASCAL program. You have to provide the runtime "
              "and all the I/O routines. See the comments in the source code for "
              "information about how to do this."),
            dest);
    CompileSuccessfulMessage(str);
}

/*
bool SetConsoleCP(UINT wCodePageID);
bool SetConsoleOutputCP(UINT wCodePageID);
Кому интересно, вот решение. Для того, чтобы SetConsoleOutPutCP работала, необходимо:
1. Операционная система Win NT (2000, XP). 95, 98, Me - не поддерживаются.
2. В настройках консоли стоял не точечный шрифт, а Lucida Console.
*/
