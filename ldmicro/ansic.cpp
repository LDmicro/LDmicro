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
// Write the program as ANSI C source. This is very simple, because the
// intermediate code structure is really a lot like C. Someone else will be
// responsible for calling us with appropriate timing.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

#include "ldmicro.h"
#include "intcode.h"

static char SeenVariables[MAX_IO][MAX_NAME_LEN];
int SeenVariablesCount;

static FILE *fh;
static FILE *flh;

static int _compile_ISA;

//-----------------------------------------------------------------------------
// Have we seen a variable before? If not then no need to generate code for
// it, otherwise we will have to make a declaration, and mark it as seen.
//-----------------------------------------------------------------------------
static BOOL SeenVariable(char *name)
{
    int i;
    for(i = 0; i < SeenVariablesCount; i++) {
        if(strcmp(SeenVariables[i], name)==0) {
            return TRUE;
        }
    }
    if(i >= MAX_IO) oops();
    strcpy(SeenVariables[i], name);
    SeenVariablesCount++;
    return FALSE;
}

//-----------------------------------------------------------------------------
// Turn an internal symbol into a C name; only trick is that internal symbols
// use $ for symbols that the int code generator needed for itself, so map
// that into something okay for C.
//-----------------------------------------------------------------------------
#define ASBIT 1
#define ASINT 2
#define ASSTR 3
static char *MapSym(char *str, int how)
{
    if(!str) return NULL;

    static char AllRets[16][MAX_NAME_LEN+30];
    static int RetCnt;

    RetCnt = (RetCnt + 1) & 15;

    char *ret = AllRets[RetCnt];

    // The namespace for bit and integer variables is distinct.
    char bit_int;
    if(how == ASBIT) {
        bit_int = 'b';
    } else if(how == ASINT) {
        bit_int = 'i';
    } else if(how == ASSTR) {
        bit_int = 's';
    } else {
        oops();
    }

    // User and internal symbols are distinguished.
    if(*str == '$') {
        sprintf(ret, "I_%c_%s", bit_int, str+1);
    } else {
        sprintf(ret, "U_%c_%s", bit_int, str);
    }
    return ret;
}

static char *MapSym(char *str)
{
    return MapSym(str, ASINT);
}
//-----------------------------------------------------------------------------
// Generate a declaration for an integer var; easy, a static 16-bit qty.
//-----------------------------------------------------------------------------
static void DeclareInt(FILE *f, char *str)
{
    fprintf(f, "STATIC SWORD %s = 0;\n", str);
    fprintf(f, "\n");
}

//-----------------------------------------------------------------------------
// Generate a declaration for a bit var; three cases, input, output, and
// internal relay. An internal relay is just a BOOL variable, but for an
// input or an output someone else must provide read/write functions.
//-----------------------------------------------------------------------------
static void DeclareBit(FILE *f, char *str)
{
    // The mapped symbol has the form U_b_{X,Y,R}name, so look at character
    // four to determine if it's an input, output, internal relay.
    if(str[4] == 'X') {
        fprintf(f, "\n");
        fprintf(f, "/* You provide this function. */\n");
        fprintf(f, "PROTO(extern BOOL Read_%s(void);)\n", str);
        fprintf(f, "\n");
    } else if(str[4] == 'Y') {
        fprintf(f, "\n");
        fprintf(f, "/* You provide these functions. */\n");
        fprintf(f, "PROTO(BOOL Read_%s(void);)\n", str);
        fprintf(f, "PROTO(void Write_%s(BOOL v);)\n", str);
        fprintf(f, "\n");
    } else {
        fprintf(f, "STATIC BOOL %s = 0;\n", str);
        fprintf(f, "#define Read_%s() %s\n", str, str);
        fprintf(f, "#define Write_%s(x) %s = x\n", str, str);
    }
}

//-----------------------------------------------------------------------------
// Generate declarations for all the 16-bit/single bit variables in the ladder
// program.
//-----------------------------------------------------------------------------
static void GenerateDeclarations(FILE *f)
{
    int i;
    for(i = 0; i < IntCodeLen; i++) {
        char *bitVar1 = NULL, *bitVar2 = NULL;
        char *intVar1 = NULL, *intVar2 = NULL, *intVar3 = NULL;
        IntOp *a = &IntCode[i];

        switch(IntCode[i].op) {
            case INT_SET_BIT:
            case INT_CLEAR_BIT:
                bitVar1 = IntCode[i].name1;
                break;

            case INT_COPY_BIT_TO_BIT:
                bitVar1 = IntCode[i].name1;
                bitVar2 = IntCode[i].name2;
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                intVar1 = IntCode[i].name1;
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                intVar1 = IntCode[i].name1;
                intVar2 = IntCode[i].name2;
                break;

            case  INT_READ_SFR_LITERAL:
            case  INT_WRITE_SFR_LITERAL:
            case  INT_SET_SFR_LITERAL:
            case  INT_CLEAR_SFR_LITERAL:
            case  INT_TEST_SFR_LITERAL:
            case  INT_READ_SFR_VARIABLE:
            case  INT_WRITE_SFR_VARIABLE:
            case  INT_SET_SFR_VARIABLE:
            case  INT_CLEAR_SFR_VARIABLE:
            case  INT_TEST_SFR_VARIABLE:
            case  INT_TEST_C_SFR_LITERAL:
            case  INT_WRITE_SFR_LITERAL_L:
            case  INT_WRITE_SFR_VARIABLE_L:
            case  INT_SET_SFR_LITERAL_L:
            case  INT_SET_SFR_VARIABLE_L:
            case  INT_CLEAR_SFR_LITERAL_L:
            case  INT_CLEAR_SFR_VARIABLE_L:
            case  INT_TEST_SFR_LITERAL_L:
            case  INT_TEST_SFR_VARIABLE_L:
            case  INT_TEST_C_SFR_VARIABLE:
            case  INT_TEST_C_SFR_LITERAL_L:
            case  INT_TEST_C_SFR_VARIABLE_L:
                break;


            case INT_SET_VARIABLE_DIVIDE:
            case INT_SET_VARIABLE_MULTIPLY:
            case INT_SET_VARIABLE_SUBTRACT:
            case INT_SET_VARIABLE_ADD:
                intVar1 = IntCode[i].name1;
                intVar2 = IntCode[i].name2;
                intVar3 = IntCode[i].name3;
                break;

            case INT_DECREMENT_VARIABLE:
            case INT_INCREMENT_VARIABLE:
                intVar1 = IntCode[i].name1;
                break;

            case INT_SET_PWM:
                {
                bitVar1 = IntCode[i].name1;
                intVar1 = IntCode[i].name2;
                break;
                }
            case INT_READ_ADC:
                intVar1 = IntCode[i].name1;
                bitVar1 = IntCode[i].name1;
                break;

            case INT_UART_RECV:
            case INT_UART_SEND:
                intVar1 = IntCode[i].name1;
                bitVar1 = IntCode[i].name2;
                break;

            case INT_UART_RECV_AVAIL:
            case INT_UART_SEND_READY:
                bitVar1 = IntCode[i].name1;
                break;

            case INT_IF_BIT_SET:
            case INT_IF_BIT_CLEAR:
                bitVar1 = IntCode[i].name1;
                break;

            case INT_IF_VARIABLE_LES_LITERAL:
                intVar1 = IntCode[i].name1;
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
            case INT_IF_VARIABLE_GRT_VARIABLE:
                intVar1 = IntCode[i].name1;
                intVar2 = IntCode[i].name2;
                break;

            case INT_END_IF:
            case INT_ELSE:
            case INT_COMMENT:
            case INT_SIMULATE_NODE_STATE:
            case INT_EEPROM_BUSY_CHECK:
            case INT_EEPROM_READ:
            case INT_EEPROM_WRITE:
            case INT_WRITE_STRING:
            case INT_AllocKnownAddr:
            case INT_AllocFwdAddr:
            case INT_FwdAddrIsNow:
            case INT_GOTO:
            case INT_GOSUB:
            case INT_RETURN:
                break;

            #ifdef TABLE_IN_FLASH
            case INT_FLASH_INIT:
                break;
            case INT_RAM_READ:
            case INT_FLASH_READ:
                intVar1 = IntCode[i].name1;
                break;
            #endif

            default:
                ooops("INT_%d",a->op);
        }
        bitVar1 = MapSym(bitVar1, ASBIT);
        bitVar2 = MapSym(bitVar2, ASBIT);

        intVar1 = MapSym(intVar1, ASINT);
        intVar2 = MapSym(intVar2, ASINT);
        intVar3 = MapSym(intVar3, ASINT);

        if(bitVar1 && !SeenVariable(bitVar1)) DeclareBit(f, bitVar1);
        if(bitVar2 && !SeenVariable(bitVar2)) DeclareBit(f, bitVar2);

        if(intVar1 && !SeenVariable(intVar1)) DeclareInt(f, intVar1);
        if(intVar2 && !SeenVariable(intVar2)) DeclareInt(f, intVar2);
        if(intVar3 && !SeenVariable(intVar3)) DeclareInt(f, intVar3);
    }
}

//-----------------------------------------------------------------------------
// printf-like comment function
//-----------------------------------------------------------------------------
static void _Comment(FILE *f, char *str, ...)
{
    if(strlen(str)>=MAX_NAME_LEN)
      str[MAX_NAME_LEN-1]='\0';
    va_list v;
    char buf[MAX_NAME_LEN];
    va_start(v, str);
    vsprintf(buf, str, v);
    fprintf(f, "//%s\n", buf);
}
#define Comment(str, ...) _Comment(f, str, __VA_ARGS__)

//-----------------------------------------------------------------------------
static int indent = 1;
static void doIndent(FILE *f, int i)
{
   int j;
   if((IntCode[i].op != INT_SIMULATE_NODE_STATE)
   && (IntCode[i].op != INT_AllocKnownAddr) //
   && (IntCode[i].op != INT_AllocFwdAddr))
       for(j = 0; j < indent; j++) fprintf(f, "    ");
}
//-----------------------------------------------------------------------------
// Actually generate the C source for the program.
//-----------------------------------------------------------------------------
static void GenerateAnsiC(FILE *f, int begin, int end)
{
    indent = 1;
    int i;
    for(i = begin; i <= end; i++) {

        if(IntCode[i].op == INT_END_IF) indent--;
        if(IntCode[i].op == INT_ELSE) indent--;

        doIndent(f, i);

        switch(IntCode[i].op) {
            case INT_SET_BIT:
                fprintf(f, "Write_%s(1);\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_CLEAR_BIT:
                fprintf(f, "Write_%s(0);\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_COPY_BIT_TO_BIT:
                fprintf(f, "Write_%s(Read_%s());\n",
                    MapSym(IntCode[i].name1, ASBIT),
                    MapSym(IntCode[i].name2, ASBIT));
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                fprintf(f, "%s = %d;\n", MapSym(IntCode[i].name1, ASINT),
                    IntCode[i].literal);
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                fprintf(f, "%s = %s;\n", MapSym(IntCode[i].name1, ASINT),
                                         MapSym(IntCode[i].name2, ASINT));
                break;

            {
            char op;
            case INT_SET_VARIABLE_ADD: op = '+'; goto arith;
            case INT_SET_VARIABLE_SUBTRACT: op = '-'; goto arith;
            case INT_SET_VARIABLE_MULTIPLY: op = '*'; goto arith;
            case INT_SET_VARIABLE_DIVIDE: op = '/'; goto arith;
            arith:
                fprintf(f, "%s = %s %c %s;\n",
                MapSym(IntCode[i].name1, ASINT),
                MapSym(IntCode[i].name2, ASINT),
                op,
                MapSym(IntCode[i].name3, ASINT) );
                break;
            }

            case INT_INCREMENT_VARIABLE:
                fprintf(f, "%s++;\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_DECREMENT_VARIABLE:
                fprintf(f, "%s--;\n", MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_IF_BIT_SET:
                fprintf(f, "if(Read_%s()) {\n",
                    MapSym(IntCode[i].name1, ASBIT));
                indent++;
                break;

            case INT_IF_BIT_CLEAR:
                fprintf(f, "if(!Read_%s()) {\n",
                    MapSym(IntCode[i].name1, ASBIT));
                indent++;
                break;

            case INT_IF_VARIABLE_LES_LITERAL:
                fprintf(f, "if(%s < %d) {\n", MapSym(IntCode[i].name1, ASINT),
                    IntCode[i].literal);
                indent++;
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                fprintf(f, "if(%s == %s) {\n", MapSym(IntCode[i].name1, ASINT),
                                               MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_VARIABLE_GRT_VARIABLE:
                fprintf(f, "if(%s > %s) {\n", MapSym(IntCode[i].name1, ASINT),
                                              MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_END_IF:
                fprintf(f, "}\n");
                break;

            case INT_ELSE:
                fprintf(f, "} else {\n"); indent++;
                break;

            case INT_SIMULATE_NODE_STATE:
                // simulation-only
                // fprintf(f, "\n");
                break;

            case INT_COMMENT:
                if(IntCode[i].name1[0]) {
                    fprintf(f, "// %s\n", IntCode[i].name1);
                } else {
                    fprintf(f, "\n");
                }
                break;

            case  INT_READ_SFR_LITERAL:
            case  INT_READ_SFR_VARIABLE:
                fprintf(f, "#warning // Read from SFR\n");
                break;
            case  INT_WRITE_SFR_LITERAL:
            case  INT_WRITE_SFR_VARIABLE_L:
            case  INT_WRITE_SFR_VARIABLE:
                fprintf(f, "// #warning Write to SFR\n");
                break;
            case  INT_WRITE_SFR_LITERAL_L:
                fprintf(f, "// #warning Write to SFR\n");
                break;
            case  INT_SET_SFR_LITERAL:
            case  INT_SET_SFR_VARIABLE:
            case  INT_SET_SFR_LITERAL_L:
            case  INT_SET_SFR_VARIABLE_L:
                fprintf(f, "#warning // Set bit in SFR\n");
                break;
            case  INT_CLEAR_SFR_LITERAL:
            case  INT_CLEAR_SFR_VARIABLE:
            case  INT_CLEAR_SFR_LITERAL_L:
            case  INT_CLEAR_SFR_VARIABLE_L:
                fprintf(f, "#warning // Clear bit in SFR\n");
                break;
            case  INT_TEST_SFR_LITERAL:
            case  INT_TEST_SFR_VARIABLE:
            case  INT_TEST_SFR_LITERAL_L:
            case  INT_TEST_SFR_VARIABLE_L:
                fprintf(f, "#warning // Test if bit Set in SFR\n");
                break;
            case  INT_TEST_C_SFR_LITERAL:
            case  INT_TEST_C_SFR_VARIABLE:
            case  INT_TEST_C_SFR_LITERAL_L:
            case  INT_TEST_C_SFR_VARIABLE_L:
                fprintf(f, "#warning // Test if bit Clear in SFR\n");
                break;

            case INT_UART_RECV:
                    fprintf(f, "%s = UART_Receive();\n",  MapSym(IntCode[i].name1, ASINT) );
                break;

            case INT_UART_SEND:
                fprintf(f, "UART_Transmit(%s);\n", MapSym(IntCode[i].name1, ASINT)/*, MapSym(IntCode[i].name2, ASBIT)*/ );
                break;

            case INT_UART_RECV_AVAIL:
                fprintf(f, "%s = UART_Receive_Avail();\n",  MapSym(IntCode[i].name1, ASBIT));
                indent++;
                break;

            case INT_UART_SEND_READY:
                fprintf(f, "%s = UART_Transmit_Ready();\n", MapSym(IntCode[i].name1, ASBIT));
                indent++;
                break;

            case INT_WRITE_STRING:
                Error(_("ANSI C target does not support peripherals "
                    "(UART, PWM, ADC, EEPROM). Skipping that instruction."));
                break;
            case INT_EEPROM_BUSY_CHECK:
                                fprintf(f, "/* EEprom busy check */\n");
                break;
            case INT_EEPROM_READ:
                                fprintf(f, "/* EEprom read */\n");
                break;
            case INT_EEPROM_WRITE:
                                fprintf(f, "/* EEprom write */\n");
                break;
            case INT_READ_ADC:
                                fprintf(f, "/* Read ADC */\n");
                break;
            case INT_SET_PWM:
                                fprintf(f, "/* Set PWM */\n");
                break;

            case INT_AllocFwdAddr:
                //fprintf(f, "#warning INT_%d\n", IntCode[i].op);
                break;
            case INT_AllocKnownAddr:
                if(IntCode[i].name1)
                    fprintf(f, "//KnownAddr Rung%d %s %s\n", IntCode[i].literal+1, IntCode[i].name2, IntCode[i].name1);
                else
                    fprintf(f, "//KnownAddr Rung%d\n", IntCode[i].literal+1);
                if(strcmp(IntCode[i].name2,"SUBPROG") == 0) {
                    int skip = FindOpNameLast(INT_RETURN, IntCode[i].name1);
                    if(skip <= i) oops();
                    i = skip;
                }
                break;
            case INT_FwdAddrIsNow:
                fprintf(f, "LabelRung%d:;\n", IntCode[i].literal+1);
                break;
            case INT_GOTO:
                fprintf(f, "goto LabelRung%d; // %s\n", IntCode[i].literal+1, IntCode[i].name1);
                break;
            case INT_GOSUB:
                fprintf(f, "Call_SUBPROG_%s(); // LabelRung%d\n", IntCode[i].name1, IntCode[i].literal+1);
                break;
            case INT_RETURN:
                fprintf(f, "return;\n", IntCode[i].name1);
                break;

            #ifdef TABLE_IN_FLASH
            case INT_FLASH_INIT: {
                break;
            }
            case INT_FLASH_READ: {
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f, "%s = %d; // %s[%s]\n", MapSym(IntCode[i].name1), IntCode[i].data[CheckMakeNumber(IntCode[i].name3)], MapSym(IntCode[i].name2), IntCode[i].name3);
                } else {
                    fprintf(f,"#ifdef __GNUC__\n");
    doIndent(f, i); fprintf(f, "%s = pgm_read_word(&%s[%s]);\n", MapSym(IntCode[i].name1), MapSym(IntCode[i].name2), MapSym(IntCode[i].name3));
    doIndent(f, i); fprintf(f,"#else\n");
    doIndent(f, i); fprintf(f, "%s = %s[%s];\n", MapSym(IntCode[i].name1), MapSym(IntCode[i].name2), MapSym(IntCode[i].name3));
    doIndent(f, i); fprintf(f,"#endif\n");
                }
                break;
            }
            case INT_RAM_READ: {
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f, "%s = %s[%d]\n", MapSym(IntCode[i].name1), MapSym(IntCode[i].name2, ASSTR), CheckMakeNumber(IntCode[i].name3));
                } else {
                    fprintf(f, "%s = %s[%s];\n", MapSym(IntCode[i].name1), MapSym(IntCode[i].name2, ASSTR), MapSym(IntCode[i].name3));
                }
                break;
            }
            #endif
            default:
                ooops("INT_%d", IntCode[i].op);
        }
    }
}
//-----------------------------------------------------------------------------
static void GenerateSUBPROG(FILE *f)
{
    int i;
    for(i = 0; i < IntCodeLen; i++) {
        switch(IntCode[i].op) {
            case INT_GOSUB: {
                fprintf(f, "\n");
                fprintf(f, "void Call_SUBPROG_%s() // LabelRung%d\n", IntCode[i].name1, IntCode[i].literal+1);
                fprintf(f, "{\n");
                int indentSave = indent;
                indent = 1;
                GenerateAnsiC(f, FindOpName(INT_AllocKnownAddr, IntCode[i].name1, "SUBPROG")+1,
                                 FindOpNameLast(INT_RETURN, IntCode[i].name1));
                indent = indentSave;
                fprintf(f, "}\n");
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Actually generate the C source for the datas.
//-----------------------------------------------------------------------------
static void GenerateAnsiC_flash_eeprom(FILE *f)
{
#ifdef TABLE_IN_FLASH
    int i;
    for(i = 0; i < IntCodeLen; i++) {
        switch(IntCode[i].op) {
            case INT_FLASH_INIT: {
                int sovElement = IntCode[i].literal2;
                char *sovs;
/*
CodeVision AVR
// Pointer to a char string placed in FLASH
flash char *ptr_to_flash1="This string is placed in FLASH";
char flash *ptr_to_flash2="This string is also placed in FLASH";


// Pointer to a char string placed in EEPROM
eeprom char *ptr_to_eeprom1="This string is placed in EEPROM";
char eeprom *ptr_to_eeprom2="This string is also placed in EEPROM";
*/
                if(sovElement == 1) {
                    sovs = "flash unsigned char";
                } else if(sovElement == 2) {
                    sovs = "flash unsigned int";
                } else if(sovElement == 3) {
                    sovs = "flash unsigned24bit";
                } else if(sovElement == 4) {
                    sovs = "flash unsigned long int";
                } else {
                    ooops("sovElement=%d", sovElement);
                }
                if((isVarInited(IntCode[i].name1) < 0)
                || 1
                || (isVarInited(IntCode[i].name1)==rungNow)) {
                    fprintf(f,"#ifdef __CODEVISIONAVR__\n");
                    fprintf(f, "%s %s[%d] = {", sovs, MapSym(IntCode[i].name1), IntCode[i].literal);
                    int j;
                    for(j = 0; j < (IntCode[i].literal-1); j++) {
                      fprintf(f, "%d, ", IntCode[i].data[j]);
                    }
                    fprintf(f, "%d};\n", IntCode[i].data[IntCode[i].literal-1]);
                    fprintf(f,"#endif\n");
                }
                //
/*
winavr avr gcc

//const char FlashString[] PROGMEM = "This is a string ";
*/
                if(sovElement == 1) {
                    sovs = "unsigned char";
                } else if(sovElement == 2) {
                    sovs = "unsigned int";
                } else if(sovElement == 3) {
                    sovs = "unsigned24bit";
                } else if(sovElement == 4) {
                    sovs = "unsigned long int";
                } else {
                    ooops("sovElement=%d", sovElement);
                }
                if((isVarInited(IntCode[i].name1) < 0)
                || 1
                || (isVarInited(IntCode[i].name1)==rungNow)) {
                    fprintf(f,"#ifdef __GNUC__\n");
                    fprintf(f, "const %s %s[%d] PROGMEM = {", sovs, MapSym(IntCode[i].name1), IntCode[i].literal);
                    int j;
                    for(j = 0; j < (IntCode[i].literal-1); j++) {
                      fprintf(f, "%d, ", IntCode[i].data[j]);
                    }
                    fprintf(f, "%d};\n", IntCode[i].data[IntCode[i].literal-1]);
                    fprintf(f,"#endif\n\n");
                }
                break;
            }
            #ifdef NEW_FEATURE
            case INT_EEPROM_INIT: {
                fprintf(f, "epprom datas;\n");
                break;
            }
            #endif
            default:{
            }
        }
    }
#endif
}

void CompileAnsiC(char *dest)
{
     CompileAnsiC(dest, ISA_ANSIC);
}

void CompileAnsiC(char *dest, int compile_ISA)
{
  if(compile_ISA==ISA_ARDUINO) {
    Error(
" "
"This feature of LDmicro is in testing and refinement.\n"
"1. You can send your LD file at the LDmicro.GitHub@gmail.com\n"
"and get 4 output files for Arduino, as shown in the example for c_demo\n"
"https://github.com/LDmicro/LDmicro/wiki/HOW-TO:-Integrate-LDmicro-and-Arduino-software.\n"
"2. You can sponsor development and pay for it. \n"
"After payment you will get this functionality in a state as is at the time of development \n"
"and you will be able to generate 4 output files for Arduino for any of your LD files.\n"
"On the question of payment, please contact LDmicro.GitHub@gmail.com.\n"
    );
    return;
  }
    _compile_ISA = compile_ISA;
    SeenVariablesCount = 0;

    FILE *f = fopen(dest, "w");
    if(!f) {
        Error(_("Couldn't open file '%s'"), dest);
        return;
    }

    fprintf(f,
"/* This is auto-generated C code from LDmicro. Do not edit this file! Go\n"
"   back to the LDmicro ladder diagram source for changes in the ladder logic, and make\n"
"   any C additions either in ladder.h or in additional .c or .h files linked\n"
"   against this one. */\n"
"\n"
"/* You must provide ladder.h; there you must provide:\n"
"      * a typedef for SWORD and BOOL, signed 16 bit and boolean types\n"
"        (probably typedef signed short SWORD; typedef unsigned char BOOL;)\n"
"\n"
"   You must also provide implementations of all the I/O read/write\n"
"   either as inlines in the header file or in another source file. (The\n"
"   I/O functions are all declared extern.)\n"
"\n"
"   See the generated source code (below) for function names. */\n"
"\n"
"#include \"ladder.h\"\n"
"\n"
"/* Define EXTERN_EVERYTHING in ladder.h if you want all symbols extern.\n"
"   This could be useful to implement `magic variables,' so that for\n"
"   example when you write to the ladder variable duty_cycle, your PLC\n"
"   runtime can look at the C variable U_duty_cycle and use that to set\n"
"   the PWM duty cycle on the micro. That way you can add support for\n"
"   peripherals that LDmicro doesn't know about. */\n"
"#ifdef EXTERN_EVERYTHING\n"
"#define STATIC \n"
"#else\n"
"#define STATIC static\n"
"#endif\n"
"\n"
"/* Define NO_PROTOTYPES if you don't want LDmicro to provide prototypes for\n"
"   all the I/O functions (Read_U_xxx, Write_U_xxx) that you must provide.\n"
"   If you define this then you must provide your own prototypes for these\n"
"   functions in ladder.h, or provide definitions (e.g. as inlines or macros)\n"
"   for them in ladder.h. */\n"
"#ifdef NO_PROTOTYPES\n"
"#define PROTO(x)\n"
"#else\n"
"#define PROTO(x) x\n"
"#endif\n"
"\n"
"/* U_xxx symbols correspond to user-defined names. There is such a symbol\n"
"   for every internal relay, variable, timer, and so on in the ladder\n"
"   program. I_xxx symbols are internally generated. */\n"
        );

    // now generate declarations for all variables
    GenerateDeclarations(f);
    GenerateAnsiC_flash_eeprom(f);
    GenerateSUBPROG(f);

    if(UartFunctionUsed()) {
    fprintf(f,
"void UART_Transmit(unsigned char data) // AVR\n"
"{ // Wait for empty transmit buffer\n"
"  while ( !( UCSRA & (1<<UDRE)) );\n"
"  // Put data into buffer, sends the data\n"
"  UDR = data;\n"
"}\n"
"\n"
"unsigned char UART_Receive(void) // AVR\n"
"{ // Wait for data to be received\n"
"  while ( !(UCSRA & (1<<RXC)) );\n"
"  // Get and return received data from buffer\n"
"  return UDR;\n"
"}\n"
"\n");
    }
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
    fprintf(f,
"\n"
"/* Call this function once per PLC cycle. You are responsible for calling\n"
"   it at the interval that you specified in the LDmicro MCU configuration when you\n"
"   generated this code. */\n"
"void PlcCycle(void)\n"
"{\n"
    );
    GenerateAnsiC(f, 0, IntCodeLen-1);
    fprintf(f, "}\n");
//---------------------------------------------------------------------------

    fprintf(f,
"\n"
"void setupPlc(void)\n"
"{\n"
    );

    Comment("Set up I/O pins");

    Comment("Turn on the pull-ups, and drive the outputs low to start");

    if(UartFunctionUsed()) {
      Comment("Set up UART");
    }

    fprintf(f,
"//Initialise PLC cycle timer here.\n"
"//Watchdog on\n"
"}\n"
    );

    fprintf(f,
"int main(void)\n"
"{\n"
"    setupPlc();\n"
"    return 0;\n"
"}\n"
    );

    fclose(f);

    char str[MAX_PATH+500];
    sprintf(str, _("Compile successful; wrote C source code to '%s'.\r\n\r\n"
        "This is not a complete C program. You have to provide the runtime "
        "and all the I/O routines. See the comments in the source code for "
        "information about how to do this."), dest);
    CompileSuccessfulMessage(str);
}
