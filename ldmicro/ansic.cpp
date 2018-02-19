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
#include <inttypes.h>

#include "ldmicro.h"
#include "intcode.h"
#include "bits.h"

static char SeenVariables[MAX_IO][MAX_NAME_LEN];
int SeenVariablesCount;

static FILE *fh;
static FILE *flh;

static int mcu_ISA = -1;
int compile_MNU = -1;

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
    if(strlen(str)==0) return NULL;

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
    if(IsNumber(str))
        sprintf(ret, "%s", str);
    else if(*str == '$') {
        sprintf(ret, "I%c_%s", bit_int, str+1);
    } else {
        sprintf(ret, "U%c_%s", bit_int, str);
    }
    return ret;
}

static char *MapSym(char *str)
{
    return MapSym(str, ASINT);
}
//-----------------------------------------------------------------------------
// Generate a declaration for an integer var; easy, a static.
//-----------------------------------------------------------------------------
static void DeclareInt(FILE *f, char *str, int sov)
{
  if(sov==1)
    fprintf(f, "STATIC SBYTE %s = 0;\n", str);
  else if(sov==2)
    fprintf(f, "STATIC SWORD %s = 0;\n", str);
  else if((sov==3)||(sov==4))
    fprintf(f, "STATIC SDWORD %s = 0;\n", str);
  else {
    fprintf(f, "STATIC SWORD %s = 0;\n", str);
//  oops();
  }
  //fprintf(f, "\n");
}

//-----------------------------------------------------------------------------
// Generate a declaration for an integer var.
//-----------------------------------------------------------------------------
static void DeclareStr(FILE *f, char *str, int sov)
{
    fprintf(f,"STATIC char %s[%d];\n", str, sov);
    fprintf(f, "\n");
}

//-----------------------------------------------------------------------------
// Generate a declaration for a bit var; three cases, input, output, and
// internal relay. An internal relay is just a BOOL variable, but for an
// input or an output someone else must provide read/write functions.
//-----------------------------------------------------------------------------
static void DeclareBit(FILE *f, char *str, int set1)
{
    // The mapped symbol has the form U_b_{X,Y,R}name, so look at character
    // four to determine if it's an input, output, internal relay.
    int type = NO_PIN_ASSIGNED;
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, &str[3])==0) {
            type = Prog.io.assignment[i].type;
            break;
        }
    }

    //if(str[3] == 'X') {
    if(type == IO_TYPE_DIG_INPUT) {
        if(compile_MNU == MNU_COMPILE_ARDUINO) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            if(iop) {
              fprintf(flh, "const int pin_%s = %s; // %s\n", str, ArduinoPinName(iop), iop->pinName);
            } else {
              fprintf(flh, "const int pin_%s = -1;\n", str);
            }

            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "// LDmicro provide this macro or function.\n");
            fprintf(fh, "#ifdef USE_MACRO\n");
            fprintf(fh, "    #define Read_%s() digitalRead(pin_%s)\n", str, str);
            fprintf(fh, "#else\n");
            fprintf(fh, "    PROTO(ldBOOL Read_%s(void));\n", str);
            fprintf(fh, "#endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "// LDmicro provide this function.\n");
            fprintf(f, "ldBOOL Read_%s(void) {\n", str);
            fprintf(f, "    return digitalRead(pin_%s);\n", str);
            fprintf(f, "}\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");
        } else {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            if(iop) {
                fprintf(fh, "// LDmicro provide this macro or function.\n");
                fprintf(fh, "#ifdef USE_MACRO\n");
                if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
                fprintf(fh, "  #define Read_%s() input_state(PIN_%c%d)\n", str, iop->port, iop->bit);
                } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
                fprintf(fh, "  #define Read_%s() R%c%d\n", str, iop->port, iop->bit);
                } else {
                fprintf(fh, "  #define Read_%s() (PIN%c & (1<<PIN%c%d))\n", str, iop->port, iop->port, iop->bit);
                }
                fprintf(fh, "#else\n");
                fprintf(fh, "  PROTO(ldBOOL Read_%s(void));\n", str);
                fprintf(fh, "#endif\n");
                fprintf(fh, "\n");

                fprintf(f, "#ifndef USE_MACRO\n");
                fprintf(f, "// LDmicro provide this function.\n");
                fprintf(f, "  ldBOOL Read_%s(void) {\n", str);
                if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
                fprintf(f, "    return input_state(PIN_%c%d);\n", iop->port, iop->bit);
                } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
                fprintf(f, "    return R%c%d;\n", iop->port, iop->bit);
                } else {
                fprintf(f, "    return PIN%c & (1<<PIN%c%d);\n", iop->port, iop->port, iop->bit);
                }
                fprintf(f, "  }\n");
                fprintf(f, "#endif\n");
                fprintf(f, "\n");
            } else {
                fprintf(fh, "// You provide this function.\n");
                fprintf(fh, "PROTO(ldBOOL Read_%s(void));\n", str);
                fprintf(fh, "\n");

                fprintf(f, "\n");
                fprintf(f, "// You provide this function.\n");
                fprintf(f, "LDSTUB%d( ldBOOL Read_%s(void) )\n", set1, str);
                fprintf(f, "\n");
            }
        }

//  } else if(str[3] == 'Y') {
    } else if(type == IO_TYPE_DIG_OUTPUT) {
        if(compile_MNU == MNU_COMPILE_ARDUINO) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            if(iop) {
              fprintf(flh, "const int pin_%s = %s; // %s\n", str, ArduinoPinName(iop), iop->pinName);
            } else {
              fprintf(flh, "const int pin_%s = -1;\n", str);
            }

            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "// LDmicro provide these macros or functions.\n");
            fprintf(fh, "#ifdef USE_MACRO\n");
            fprintf(fh, "  #define Read_%s() digitalRead(pin_%s)\n", str, str);
            fprintf(fh, "  #define Write0_%s() digitalWrite(pin_%s, LOW)\n", str, str);
            fprintf(fh, "  #define Write1_%s() digitalWrite(pin_%s, HIGH)\n", str, str);
            fprintf(fh, "  #define Write_%s(b) (b) ? Write1_%s() : Write0_%s()\n", str, str, str);
            fprintf(fh, "#else\n");
            fprintf(fh, "  PROTO(ldBOOL Read_%s(void));\n", str);
            fprintf(fh, "  PROTO(void Write_%s(ldBOOL b));\n", str);
            fprintf(fh, "  PROTO(void Write1_%s(void));\n", str);
            fprintf(fh, "  PROTO(void Write0_%s(void));\n", str);
            fprintf(fh, "#endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "// LDmicro provide these functions.\n");
            fprintf(f, "ldBOOL Read_%s(void) {\n", str);
            fprintf(f, "    return digitalRead(pin_%s);\n", str);
            fprintf(f, "}\n");
            fprintf(f, "void Write_%s(ldBOOL b) {\n", str);
            fprintf(f, "    digitalWrite(pin_%s,b);\n",str);
            fprintf(f, "}\n");
            fprintf(f, "void Write1_%s(void) {\n", str);
            fprintf(f, "    digitalWrite(pin_%s,HIGH);\n",str);
            fprintf(f, "}\n");
            fprintf(f, "void Write0_%s(void) {\n", str);
            fprintf(f, "    digitalWrite(pin_%s,LOW);\n",str);
            fprintf(f, "}\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");
        } else {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            if(iop) {
                fprintf(fh, "#ifdef USE_MACRO\n");
                fprintf(fh, "// LDmicro provide these functions.\n");
                if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
                fprintf(fh, "  #define Read_%s() input_state(PIN_%c%d)\n", str, iop->port, iop->bit);
                } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
                fprintf(fh, "  #define Read_%s() R%c%d\n", str, iop->port, iop->bit);
                } else {
                fprintf(fh, "  #define Read_%s() (PORT%c & (1<<PORT%c%d))\n", str, iop->port, iop->port, iop->bit);
                }
                if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
                fprintf(fh, "  #define Write0_%s() output_low(PIN_%c%d)\n", str, iop->port, iop->bit);
                fprintf(fh, "  #define Write1_%s() output_high(PIN_%c%d)\n", str, iop->port, iop->bit);
                fprintf(fh, "  #define Write_%s(b) (b) ? Write1_%s() : Write0_%s()\n", str, str, str);
                } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
                fprintf(fh, "  #define Write0_%s() (R%c%d = 0)\n", str, iop->port, iop->bit);
                fprintf(fh, "  #define Write1_%s() (R%c%d = 1)\n", str, iop->port, iop->bit);
                fprintf(fh, "  #define Write_%s(b) R%c%d = (b) ? 1 : 0\n", str, iop->port, iop->bit);
                } else {
                fprintf(fh, "  #define Write0_%s() (PORT%c &= ~(1<<PORT%c%d))\n", str, iop->port, iop->port, iop->bit);
                fprintf(fh, "  #define Write1_%s() (PORT%c |= 1<<PORT%c%d)\n", str, iop->port, iop->port, iop->bit);
                fprintf(fh, "  #define Write_%s(b) (b) ? Write1_%s() : Write0_%s()\n", str, str, str);
                }
                fprintf(fh, "#else\n");
                fprintf(fh, "  PROTO(ldBOOL Read_%s(void));\n", str);
                fprintf(fh, "  PROTO(void Write_%s(ldBOOL b));\n", str);
                fprintf(fh, "#endif\n");
                fprintf(fh, "\n");

                fprintf(f, "#ifndef USE_MACRO\n");
                fprintf(f, "// LDmicro provide these functions.\n");
                fprintf(f, "  ldBOOL Read_%s(void) {\n", str);
                if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
                fprintf(f, "    return input_state(PIN_%c%d);\n", iop->port, iop->bit);
                } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
                fprintf(f, "    return R%c%d;\n", iop->port, iop->bit);
                } else {
                fprintf(f, "    return PORT%c & (1<<PORT%c%d);\n", iop->port, iop->port, iop->bit);
                }
                fprintf(f, "  }\n");
                fprintf(f, "  void Write_%s(ldBOOL b) {\n", str);
                if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
                fprintf(f, "      R%c%d = b != 0;\n", iop->port, iop->bit);
                } else if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
                fprintf(f, "    if(b)\n");
                fprintf(f, "      output_high(PIN_%c%d);\n", iop->port, iop->bit);
                fprintf(f, "    else\n");
                fprintf(f, "      output_low(PIN_%c%d);\n", iop->port, iop->bit);
                } else {
                fprintf(f, "    if(b)\n");
                fprintf(f, "      PORT%c |= 1<<PORT%c%d;\n", iop->port, iop->port, iop->bit);
                fprintf(f, "    else\n");
                fprintf(f, "      PORT%c &= ~(1<<PORT%c%d);\n", iop->port, iop->port, iop->bit);
                }
                fprintf(f, "  }\n");
                fprintf(f, "  void Write1_%s(void) {\n", str);
                if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
                fprintf(f, "      output_high(PIN_%c%d);\n", iop->port, iop->bit);
                } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
                fprintf(f, "      R%c%d = 1;\n", iop->port, iop->bit);
                } else {
                fprintf(f, "      PORT%c |= 1<<PORT%c%d;\n", iop->port, iop->port, iop->bit);
                }
                fprintf(f, "  }\n");
                fprintf(f, "  void Write0_%s(void) {\n", str);
                if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
                fprintf(f, "      output_low(PIN_%c%d);\n", iop->port, iop->bit);
                } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
                fprintf(f, "      R%c%d = 0;\n", iop->port, iop->bit);
                } else {
                fprintf(f, "      PORT%c &= ~(1<<PORT%c%d);\n", iop->port, iop->port, iop->bit);
                }
                fprintf(f, "  }\n");
                fprintf(f, "#endif\n");
                fprintf(f, "\n");
            } else {
                fprintf(fh, "// You provide these functions.\n");
                fprintf(fh, "PROTO(ldBOOL Read_%s(void));\n", str);
                fprintf(fh, "PROTO(void Write_%s(ldBOOL b));\n", str);
                fprintf(fh, "\n");

                fprintf(f, "/* You provide these functions. */\n");
                fprintf(f, "LDSTUB%d( ldBOOL Read_%s(void) )\n", set1, str);
                fprintf(f, "LDSTUB( void Write_%s(ldBOOL v) )\n", str);
                fprintf(f, "\n");
            }
        }

    } else if (type == IO_TYPE_PWM_OUTPUT) {
        if(compile_MNU == MNU_COMPILE_ARDUINO) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            if(iop) {
              fprintf(flh, "const int pin_%s = %s; // %s // Check that it's a PWM pin!\n", str, ArduinoPinName(iop), iop->pinName);
            } else {
              fprintf(flh, "const int pin_%s = -1; // Check that it's a PWM pin!\n", str);
            }

            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "// LDmicro provide this macro or function.\n");
            fprintf(fh, "#ifdef USE_MACRO\n");
            fprintf(fh, "  #define Write_%s(x) analogWrite(pin_%s, x)\n", str, str);
            fprintf(fh, "#else\n");
            fprintf(fh, "  void Write_%s(SWORD x);\n", str);
            fprintf(fh, "#endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "// LDmicro provide this function.\n");
            fprintf(f, "  void Write_%s(SWORD x) {\n", str);
            fprintf(f, "    analogWrite(pin_%s, x);\n", str);
            fprintf(f, "  }\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");
        } else if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "// LDmicro provide this macro or function.\n");
            fprintf(fh, "#ifdef USE_MACRO\n");
            fprintf(fh, "  #define Write_%s(x) pwm_set_duty_percent(x); pwm_on();\n", str);
            fprintf(fh, "#else\n");
            fprintf(fh, "  void Write_%s(SWORD x);\n", str);
            fprintf(fh, "#endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "// LDmicro provide this function.\n");
            fprintf(f, "  void Write_%s(SWORD x) {\n", str);
            fprintf(f, "    pwm_set_duty_percent(x);\n");
            fprintf(f, "    pwm_on();\n");
            fprintf(f, "  }\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");
        } else {
            fprintf(f, "/* You provide this function. */\n");
            fprintf(f, "SWORD Read_%s(void) {\n", str);
            fprintf(f, "  return 0;\n");
            fprintf(f, "}\n");
        }
    } else if (type == IO_TYPE_READ_ADC) {
        if(compile_MNU == MNU_COMPILE_ARDUINO) {
            McuIoPinInfo *iop = PinInfoForName(&str[3]);
            if(iop) {
              fprintf(flh, "const int pin_%s = %s; // %s // Check that it's a ADC pin!\n", str, ArduinoPinName(iop), iop->pinName);
            } else {
              fprintf(flh, "const int pin_%s = -1; // Check that it's a ADC pin!\n", str);
            }

            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "// LDmicro provide this macro or function.\n");
            fprintf(fh, "#ifdef USE_MACRO\n");
            fprintf(fh, "  #define Read_%s() analogRead(pin_%s)\n", str, str);
            fprintf(fh, "#else\n");
            fprintf(fh, "  SWORD Read_%s(void);\n", str);
            fprintf(fh, "#endif\n");
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "#ifndef USE_MACRO\n");
            fprintf(f, "// LDmicro provide this function.\n");
            fprintf(f, "SWORD Read_%s(void) {\n", str);
            fprintf(f, "  return analogRead(pin_%s);\n", str);
            fprintf(f, "}\n");
            fprintf(f, "#endif\n");
            fprintf(f, "\n");
        } else if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
            fprintf(fh, "#ifndef NO_PROTOTYPES\n");
            fprintf(fh, "// LDmicro provide this function.\n");
            fprintf(fh, "  SWORD Read_%s(void);\n", str);
            fprintf(fh, "#endif\n");
            fprintf(fh, "\n");

            fprintf(f, "// LDmicro provide this function.\n");
            fprintf(f, "SWORD Read_%s(void) {\n", str);
            fprintf(f, "  set_adc_channel(%d);\n", MuxForAdcVariable(&str[3]));
            fprintf(f, "  return read_adc();\n");
            fprintf(f, "}\n");
            fprintf(f, "\n");
        } else {
            fprintf(f, "/* You provide this function. */\n");
            fprintf(f, "SWORD Read_%s(void) {\n", str);
            fprintf(f, "  return 0;\n");
            fprintf(f, "}\n");
        }
    } else {
        fprintf(f, "STATIC ldBOOL %s = 0;\n", str);
//      fprintf(f, "\n");
        fprintf(fh, "#define Read_%s() %s\n", str, str);
        fprintf(fh, "#define Write_%s(x) (%s = x)\n", str, str);
        fprintf(fh, "#define Write0_%s() (%s = 0)\n", str, str);
        fprintf(fh, "#define Write1_%s() (%s = 1)\n", str, str);
        fprintf(fh, "\n");
    }
}

static void DeclareBit(FILE *f, char *str)
{
    DeclareBit(f, str, 0);
}

//-----------------------------------------------------------------------------
// Generate declarations for all the 16-bit/single bit variables in the ladder
// program.
//-----------------------------------------------------------------------------
static void GenerateDeclarations(FILE *f)
{
    DWORD addr, addr2;
    int bit, bit2;

    int i;
    for(i = 0; i < IntCodeLen; i++) {
        char *bitVar1 = NULL, *bitVar2 = NULL;
        char *intVar1 = NULL, *intVar2 = NULL, *intVar3 = NULL;
        char *adcVar1 = NULL;
        char *strVar1 = NULL;
        int sov1=0, sov2=0, sov3=0;

        int bitVar1set1 = 0;

        addr=0;  bit=0;
        addr2=0; bit2=0;

        IntOp *a = &IntCode[i];

        switch(IntCode[i].op) {
            case INT_SET_BIT:
            case INT_CLEAR_BIT:
                isPinAssigned(a->name1);
                bitVar1 = IntCode[i].name1;
                break;

            case INT_COPY_BIT_TO_BIT:
                isPinAssigned(a->name1);
                isPinAssigned(a->name2);
                bitVar1 = IntCode[i].name1;
                bitVar2 = IntCode[i].name2;
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
            case INT_SET_VARIABLE_RANDOM:
                intVar1 = IntCode[i].name1;
                break;

            case INT_SET_BIN2BCD:
            case INT_SET_BCD2BIN:
            case INT_SET_OPPOSITE:
            case INT_COPY_VAR_BIT_TO_VAR_BIT:
            case INT_SET_VARIABLE_NOT:
            case INT_SET_SWAP:
            case INT_SET_VARIABLE_NEG:
            case INT_SET_VARIABLE_TO_VARIABLE:
                intVar1 = IntCode[i].name1;
                intVar2 = IntCode[i].name2;
                break;

            #ifdef USE_SFR
            case  INT_WRITE_SFR_VARIABLE:
                break;

            case  INT_WRITE_SFR_LITERAL:
            case  INT_READ_SFR_LITERAL:
            case  INT_SET_SFR_LITERAL:
            case  INT_CLEAR_SFR_LITERAL:
            case  INT_TEST_SFR_LITERAL:
            case  INT_READ_SFR_VARIABLE:
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
            #endif

            case INT_SET_VARIABLE_ROL:
            case INT_SET_VARIABLE_ROR:
            case INT_SET_VARIABLE_SHL:
            case INT_SET_VARIABLE_SHR:
            case INT_SET_VARIABLE_AND:
            case INT_SET_VARIABLE_OR :
            case INT_SET_VARIABLE_MOD:
                break;
            case INT_SET_VARIABLE_XOR:
            case INT_SET_VARIABLE_SR0:
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
                intVar1 = IntCode[i].name1;
                if(!IsNumber(IntCode[i].name2))
                  intVar2 = IntCode[i].name2;
                bitVar1 = IntCode[i].name3;
                break;

            case INT_READ_ADC:
                intVar1 = IntCode[i].name1;
                bitVar1 = IntCode[i].name1;
                break;

            case INT_UART_RECV:
            case INT_UART_SEND:
                intVar1 = IntCode[i].name1;
                bitVar1 = IntCode[i].name2;
                break;

            case INT_SPI:
                intVar1 = IntCode[i].name1;
                intVar2 = IntCode[i].name2;
                break;

            case INT_UART_SEND1:
            case INT_UART_SENDn:
                intVar1 = IntCode[i].name1;
                break;

            case INT_UART_RECV_AVAIL:
            case INT_UART_SEND_READY:
            case INT_UART_SEND_BUSY:
                bitVar1 = IntCode[i].name1;
                break;

            case INT_IF_BIT_SET:
            case INT_IF_BIT_CLEAR:
                isPinAssigned(a->name1);
                bitVar1 = IntCode[i].name1;
                bitVar1set1 = IntCode[i].literal;
                break;

            #ifdef NEW_CMP
            case INT_IF_EQU:
            case INT_IF_NEQ:
            case INT_IF_LES:
            case INT_IF_GRT:
            case INT_IF_LEQ:
            case INT_IF_GEQ:
                if(!IsNumber(IntCode[i].name1))
                  intVar1 = IntCode[i].name1;
                if(!IsNumber(IntCode[i].name2))
                  intVar2 = IntCode[i].name2;
                break;
            #else
            case INT_IF_VARIABLE_LES_LITERAL:
                intVar1 = IntCode[i].name1;
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
            case INT_IF_VARIABLE_GRT_VARIABLE:
                intVar1 = IntCode[i].name1;
                intVar2 = IntCode[i].name2;
                break;
            #endif

            case INT_END_IF:
            case INT_ELSE:
            case INT_COMMENT:
            case INT_DELAY:
            case INT_LOCK:
            case INT_CLRWDT:
            case INT_SIMULATE_NODE_STATE:
                break;

            case INT_EEPROM_BUSY_CHECK:
                bitVar1 = IntCode[i].name1;
                break;

            case INT_EEPROM_READ:
            case INT_EEPROM_WRITE:
                intVar1 = IntCode[i].name1;
                break;

            case INT_WRITE_STRING:
            case INT_SLEEP:
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

        if(intVar1) sov1 = SizeOfVar(intVar1);
        if(intVar2) sov2 = SizeOfVar(intVar2);
        if(intVar3) sov3 = SizeOfVar(intVar3);

        intVar1 = MapSym(intVar1, ASINT);
        intVar2 = MapSym(intVar2, ASINT);
        intVar3 = MapSym(intVar3, ASINT);

        if(bitVar1 && !SeenVariable(bitVar1)) DeclareBit(f, bitVar1, bitVar1set1);
        if(bitVar2 && !SeenVariable(bitVar2)) DeclareBit(f, bitVar2);

        if(intVar1 && !SeenVariable(intVar1)) DeclareInt(f, intVar1, sov1);
        if(intVar2 && !SeenVariable(intVar2)) DeclareInt(f, intVar2, sov2);
        if(intVar3 && !SeenVariable(intVar3)) DeclareInt(f, intVar3, sov3);

        if(strVar1) sov1 = SizeOfVar(strVar1);
        strVar1 = MapSym(strVar1, ASSTR);
        if(strVar1 && !SeenVariable(strVar1)) DeclareStr(f, strVar1, sov1);
    }
    if(Prog.cycleDuty) {
        char *bitVar1 = NULL;
        bitVar1 = MapSym("YPlcCycleDuty", ASBIT);
        if(bitVar1 && !SeenVariable(bitVar1)) DeclareBit(f, bitVar1);
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
    int lock_label = 0;
    indent = 1;
    int i;
    for(i = begin; i <= end; i++) {

        if(IntCode[i].op == INT_END_IF) indent--;
        if(IntCode[i].op == INT_ELSE) indent--;

        doIndent(f, i);

        switch(IntCode[i].op) {
            case INT_SET_BIT:
                if((IntCode[i].name1[0] != '$')
                && (IntCode[i].name1[0] != 'R'))
                  fprintf(f, "Write1_%s();\n", MapSym(IntCode[i].name1, ASBIT));
                else
                  fprintf(f, "Write_%s(1);\n", MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_CLEAR_BIT:
                if((IntCode[i].name1[0] != '$')
                && (IntCode[i].name1[0] != 'R'))
                  fprintf(f, "Write0_%s();\n", MapSym(IntCode[i].name1, ASBIT));
                else
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

            case INT_COPY_VAR_BIT_TO_VAR_BIT:
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                fprintf(f, "%s = %s;\n", MapSym(IntCode[i].name1, ASINT),
                                         MapSym(IntCode[i].name2, ASINT));
                break;

            case INT_SET_BIN2BCD:
            case INT_SET_BCD2BIN:
            case INT_SET_OPPOSITE:
            case INT_SET_VARIABLE_NOT:
                break;

            case INT_SET_VARIABLE_RANDOM:
                fprintf(f, "%s = rand();\n", MapSym(IntCode[i].name1, ASINT));
                break;
            {
            char op;
            case INT_SET_VARIABLE_SR0: op = '>'; goto arith_shift;
            arith_shift:
                fprintf(f, "%s = %s %c%c %s;\n",
                    MapSym(IntCode[i].name1, ASINT),
                    MapSym(IntCode[i].name2, ASINT),
                    op,
                    op,
                    MapSym(IntCode[i].name3, ASINT) );
                break;
            }
            {
            char *op;
            case INT_SET_VARIABLE_ROL: op = "rol"; goto cicle_shift;
            case INT_SET_VARIABLE_ROR: op = "ror"; goto cicle_shift;
            cicle_shift:
                break;
            }

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

            #ifdef NEW_CMP
            case INT_IF_EQU:
                fprintf(f, "if(%s == %s) {\n", MapSym(IntCode[i].name1, ASINT),
                                               MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_NEQ:
                fprintf(f, "if(%s != %s) {\n", MapSym(IntCode[i].name1, ASINT),
                                               MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_LES:
                fprintf(f, "if(%s < %s) {\n", MapSym(IntCode[i].name1, ASINT),
                                              MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_GRT:
                fprintf(f, "if(%s > %s) {\n", MapSym(IntCode[i].name1, ASINT),
                                              MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_LEQ:
                fprintf(f, "if(%s <= %s) {\n", MapSym(IntCode[i].name1, ASINT),
                                               MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;

            case INT_IF_GEQ:
                fprintf(f, "if(%s >= %s) {\n", MapSym(IntCode[i].name1, ASINT),
                                               MapSym(IntCode[i].name2, ASINT));
                indent++;
                break;
            #else
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
            #endif

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

            case INT_LOCK:
                lock_label++;
                fprintf(f, "lock_label%d: goto lock_label%d;\n", lock_label, lock_label);
                break;

            case INT_DELAY:
                fprintf(f,"#ifdef CCS_PIC_C\n");
doIndent(f, i); fprintf(f,"  delay_us(%d);\n", IntCode[i].literal);
doIndent(f, i); fprintf(f,"#elif defined(HI_TECH_C)\n");
doIndent(f, i); fprintf(f,"  __delay_us(%d);\n", IntCode[i].literal);
doIndent(f, i); fprintf(f,"#elif defined(__CODEVISIONAVR__)\n");
doIndent(f, i); fprintf(f,"  delay_us(%d);\n", IntCode[i].literal);
doIndent(f, i); fprintf(f,"#elif defined(__GNUC__)\n");
doIndent(f, i); fprintf(f,"  _delay_us(%d);\n", IntCode[i].literal);
doIndent(f, i); fprintf(f,"#else\n");
doIndent(f, i); fprintf(f,"  delayMicroseconds(%d);\n", IntCode[i].literal);
doIndent(f, i); fprintf(f,"#endif\n");
                break;

            #ifdef USE_SFR
            case  INT_READ_SFR_LITERAL:
            case  INT_READ_SFR_VARIABLE:
                fprintf(f, "#warning // Read from SFR\n");
                break;
            case  INT_WRITE_SFR_LITERAL:
            case  INT_WRITE_SFR_VARIABLE_L:
            case  INT_WRITE_SFR_VARIABLE:
                fprintf(f, "// #warning Write to SFR\n");
                //doIndent(f, i); fprintf(f, "PORTB = %s;\n", MapSym(IntCode[i].name2, ASINT));
                doIndent(f, i); fprintf(f, "pokeb(%s, %s);\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASINT));
                break;
            case  INT_WRITE_SFR_LITERAL_L:
                fprintf(f, "// #warning Write to SFR\n");
                doIndent(f, i); fprintf(f, "pokeb(%d, %d);\n", IntCode[i].literal, IntCode[i].literal2);
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
            #endif

            case INT_UART_RECV:
                fprintf(f, "%s=0; if(UART_Receive_Avail()) {%s = UART_Receive(); %s=1;};\n", MapSym(IntCode[i].name2, ASBIT), MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name2, ASBIT));
                break;

            case INT_UART_SEND1:
            case INT_UART_SENDn:
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

            case INT_UART_SEND_BUSY:
                fprintf(f, "%s = UART_Transmit_Busy();\n", MapSym(IntCode[i].name1, ASBIT));
                indent++;
                break;
            case INT_EEPROM_BUSY_CHECK:
                if(compile_MNU == MNU_COMPILE_ARDUINO) {
                    fprintf(f, "Write1_%s(); // EEPROM is ready\n", MapSym(IntCode[i].name1, ASBIT));
                } else {
                    fprintf(f, "#warning //INT_EEPROM_BUSY_CHECK to %s\n", IntCode[i].name1);
                }
                break;

            case INT_EEPROM_READ:
                    fprintf(f, "%s = EEPROM_read(%d) + (EEPROM_read(%d) << 8);\n", MapSym(IntCode[i].name1, ASINT), IntCode[i].literal, IntCode[i].literal+1);
                break;

            case INT_EEPROM_WRITE:
                    fprintf(f, "EEPROM_write(%d, %s & 0xFF);\n", IntCode[i].literal, MapSym(IntCode[i].name1, ASINT));
    doIndent(f, i); fprintf(f, "EEPROM_write(%d, %s >> 8);\n", IntCode[i].literal+1, MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_READ_ADC:
                    fprintf(f, "%s = Read_%s();\n", MapSym(IntCode[i].name1, ASINT), MapSym(IntCode[i].name1, ASBIT));
                break;

            case INT_SET_PWM:
                  if(IsNumber(IntCode[i].name2))
                    fprintf(f, "%s = %d;\n", MapSym(IntCode[i].name1, ASINT),
                                             hobatoi(IntCode[i].name2));
                  else
                    fprintf(f, "%s = %s;\n", MapSym(IntCode[i].name1, ASINT),
                                             MapSym(IntCode[i].name2, ASINT));
    doIndent(f, i); fprintf(f, "Write_%s(%s);\n", MapSym(IntCode[i].name3, ASBIT), MapSym(IntCode[i].name1, ASINT));
                break;

            case INT_AllocFwdAddr:
                //fprintf(f, "#warning INT_%d\n", IntCode[i].op);
                break;
            case INT_AllocKnownAddr:
                /*
                if(IntCode[i].name1)
                    fprintf(f, "//KnownAddr Rung%d %s %s\n", IntCode[i].literal+1, IntCode[i].name2, IntCode[i].name1);
                else
                    fprintf(f, "//KnownAddr Rung%d\n", IntCode[i].literal+1);
                */
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
                fprintf(f, "return;\n");
                break;
            case INT_SLEEP:
                fprintf(f,"cli();\n");
doIndent(f, i); fprintf(f,"sleep_enable();\n");
doIndent(f, i); fprintf(f,"sei();\n");
doIndent(f, i); fprintf(f,"sleep_cpu();\n");
doIndent(f, i); fprintf(f,"sleep_disable();\n");
doIndent(f, i); fprintf(f,"sei();\n");
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
                fprintf(f, "void Call_SUBPROG_%s() { // LabelRung%d\n", IntCode[i].name1, IntCode[i].literal+1);
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
void CompileAnsiC(char *dest, int MNU)
{
    if(Prog.mcu)
        mcu_ISA = Prog.mcu->whichIsa;
    if(MNU > 0)
        compile_MNU = MNU;

    SeenVariablesCount = 0;

    char CurrentLdName[MAX_PATH];
    GetFileName(CurrentLdName, dest);

    char desth[MAX_PATH];
    SetExt(desth, dest, ".h")  ;

    char ladderhName[MAX_PATH];
    sprintf(ladderhName,"%s\\ladder.h_", CurrentCompilePath);

    flh = fopen(ladderhName, "w");
    if(!flh) {
        Error(_("Couldn't open file '%s'"), ladderhName);
        return;
    }
    fprintf(flh,
"/* This is example for ladder.h file!\n"
"   This is auto-generated C header from LDmicro.\n"
"   Rename this file as ladder.h or copy content(or part) of this file\n"
"   to existing ladder.h. Remove this comment from ladder.h. */\n"
"\n"
"#ifndef __LADDER_H__\n"
"#define __LADDER_H__\n"
"\n");

    fprintf(flh,
"/* Uncomment EXTERN_EVERYTHING if you want all symbols in %s.c extern. */\n"
"//#define EXTERN_EVERYTHING\n"
"\n"
"/* Uncomment NO_PROTOTYPES if you want own prototypes for functions. */\n"
"//#define NO_PROTOTYPES\n"
"\n"
"/* Define NO_PROTOTYPES in ladder.h if you don't want LDmicro to provide prototypes for\n"
"   all the I/O functions (Read_Ux_xxx, Write_Ux_xxx) that you must provide.\n"
"   If you define this then you must provide your own prototypes for these\n"
"   functions in %s.h, or provide definitions (e.g. as inlines or macros)\n"
"   for them in %s.h. */\n"
"#ifdef NO_PROTOTYPES\n"
"  #define PROTO(x)\n"
"#else\n"
"  #define PROTO(x) x\n"
"#endif\n"
"\n"
"/* Uncomment DO_LDSTUBS if you want to use the empty stub-functions\n"
"   from %s.c instead the prototypes for functions.\n"
"   Use DO_LDSTUBS to just check the compilation of the generated files. */\n"
"//#define DO_LDSTUBS\n"
"\n"
"#ifdef DO_LDSTUBS\n"
"#define LDSTUB(x)   x { ; }\n"
"#define LDSTUB0(x)  x { return 0; }\n"
"#define LDSTUB1(x)  x { return 1; }\n"
"#else\n"
"#define LDSTUB(x)   PROTO(extern x;)\n"
"#define LDSTUB0(x)  PROTO(extern x;)\n"
"#define LDSTUB1(x)  PROTO(extern x;)\n"
"#endif\n"
"\n"
"/* Uncomment USE_WDT when you need to. */\n"
"//#define USE_WDT\n"
"\n"
"/* Comment out USE_MACRO in next line, if you want to use functions instead of macros. */\n"
"#define USE_MACRO\n"
"\n"
    , CurrentLdName, CurrentLdName, CurrentLdName, CurrentLdName);
    if(compile_MNU == MNU_COMPILE_ARDUINO) {
        fprintf(flh,
"#if ARDUINO >= 100\n"
"    #include \"Arduino.h\"\n"
"#else\n"
"    #include \"WProgram.h\"\n"
"#endif\n"
"\n"
        );
    }
    fprintf(flh,
"/*\n"
"  Type                  Size(bits)\n"
"  ldBOOL    unsigned       1 or 8, the smallest possible\n"
"  SBYTE     signed integer      8\n"
"  SWORD     signed integer     16\n"
"  SDWORD    signed integer     32\n"
"*/\n"
    );
    if(compile_MNU == MNU_COMPILE_ARDUINO) {
       fprintf(flh,
"typedef boolean       ldBOOL;\n"
"typedef char           SBYTE;\n"
"typedef int            SWORD;\n"
"typedef long int      SDWORD;\n"
"\n"
       );
    } else if(mcu_ISA == ISA_PIC16) {
      fprintf(flh,
"#ifdef CCS_PIC_C\n"
"    typedef int1             ldBOOL;\n"
"    typedef signed  int8      SBYTE;\n"
"    typedef signed int16      SWORD;\n"
"    typedef signed int32     SDWORD;\n"
"#elif defined(HI_TECH_C)\n"
"  #ifdef _USE_MACRO\n"
"    typedef bit              ldBOOL;\n"
"  #else\n"
"    typedef unsigned char    ldBOOL;\n"
"  #endif\n"
"    typedef signed char       SBYTE;\n"
"    typedef signed short int  SWORD;\n"
"    typedef signed long int  SDWORD;\n"
"#endif\n"
      );
    } else if(mcu_ISA == ISA_AVR) {
      fprintf(flh,
/*
"#ifdef __CODEVISIONAVR__\n"
"//#define ldBOOL              bit\n"
"//typedef bool             ldBOOL;\n"
"#endif\n"
*/
"  typedef unsigned char    ldBOOL;\n"
"  typedef signed char       SBYTE;\n"
"  typedef signed short int  SWORD;\n"
"  typedef signed long int  SDWORD;\n"
      );
    } else {
      fprintf(flh,
"  typedef unsigned char    ldBOOL;\n"
"  typedef signed char       SBYTE;\n"
"  typedef signed short int  SWORD;\n"
"  typedef signed long int  SDWORD;\n"
      );
    }
    if(mcu_ISA == ISA_AVR) {
      fprintf(flh,
"\n"
"#ifndef UCSRA\n"
"  #define UCSRA UCSR0A\n"
"#endif\n"
"#ifndef UDRE\n"
"  #define UDRE UDRE0\n"
"#endif\n"
"#ifndef UDR\n"
"  #define UDR UDR0\n"
"#endif\n"
"#ifndef RXC\n"
"  #define RXC RXC0\n"
"#endif\n"
"#ifndef UBRRH\n"
"  #define UBRRH UBRR0H\n"
"#endif\n"
"#ifndef UBRRL\n"
"  #define UBRRL UBRR0L\n"
"#endif\n"
"#ifndef UCSRB\n"
"  #define UCSRB UCSR0B\n"
"#endif\n"
"/*\n"
"#ifndef \n"
"  #define \n"
"#endif\n"
"*/\n"
      );
    }

    fh = fopen(desth, "w");
    if(!fh) {
        Error(_("Couldn't open file '%s'"), desth);
        fclose(flh);
        return;
    }
    fprintf(fh,
"/* This is auto-generated C header from LDmicro. Do not edit this file!\n"
"   Go back to the LDmicro ladder diagram source for changes in the ladder logic,\n"
"   and make any C additions in additional .c or .h files linked against this one. */\n"
"#ifndef __%s_H__\n"
"#define __%s_H__\n"
"\n"
"#include \"ladder.h\"\n"
"\n"
    ,CurrentLdName, CurrentLdName);

    if(compile_MNU == MNU_COMPILE_ARDUINO) {
      fprintf(fh,
"// PLC cycle interval, set this according to LDmicro settings. (micro seconds)\n"
"#define PLC_INTERVAL %lld // us\n"
"\n"
      , Prog.cycleTime);

      fprintf(fh,
"#ifdef USE_WDT\n"
"#include <avr\\wdt.h>\n"
"#endif\n"
      );
      if(SleepFunctionUsed()) {
        fprintf(fh,
"#include <avr\\sleep.h>\n"
        );
      }

      if(EepromFunctionUsed())
        fprintf(fh,
"#include <EEPROM.h>\n");
    } else if(mcu_ISA == ISA_PIC16) {
    } else if(mcu_ISA == ISA_AVR) {
      fprintf(fh,
"#include <stdio.h>\n"
"\n"
"#ifdef __CODEVISIONAVR__\n"
"    //typedef char PROGMEM prog_char\n"
"    //#include <mem.h>\n"
"    #include <%s.h>\n"
"#elif defined(__GNUC__)\n"
"    //#warning __GNUC__\n"
"    #include <avr/io.h>\n"
"    #include <avr/pgmspace.h>\n"
"    #define __PROG_TYPES_COMPAT__ //arduino\n"
"    #ifdef USE_WDT\n"
"    #include <avr/wdt.h>\n"
"    #endif\n"
"#endif\n"
     , Prog.mcu->mcuH);
/*
      fprintf(fh,
"#ifdef __GNUC__\n"
"    //mem.h vvv\n"
"    //CodeVisionAVR V2.0 C Compiler\n"
"    //(C) 1998-2007 Pavel Haiduc, HP InfoTech S.R.L.\n"
"    //\n"
"    //  Memory access macros\n"
"\n"
"    #ifndef _MEM_INCLUDED_\n"
"    #define _MEM_INCLUDED_\n"
"\n"
"    #define pokeb(addr,data) *((unsigned char *)(addr))=(data)\n"
"    #define pokew(addr,data) *((unsigned int *)(addr))=(data)\n"
"    #define peekb(addr) *((unsigned char *)(addr))\n"
"    #define peekw(addr) *((unsigned int *)(addr))\n"
"\n"
"    #endif\n"
"    //mem.h ^^^\n"
"#endif\n"
     );
*/
    } else {
    }
    if(DelayUsed()) {
      fprintf(fh,
"#ifdef __CODEVISIONAVR__\n"
"    #include <delay.h>\n"
"#elif defined(__GNUC__)\n"
"    #define F_CPU %d\n"
"    #include <util/delay.h>\n"
"#endif\n"
      , Prog.mcuClock);
    }

    if(compile_MNU == MNU_COMPILE_ARDUINO) {
        fprintf(fh,
"extern void loopPlc(void);  // Call loopPlc() function in loop() of your arduino project\n"
"extern void setupPlc(void); //  or initialise PLC cycle timer in this function\n"
"extern void PlcCycle(void); //  and call PlcCycle() function once per PLC cycle timer.\n"
"\n"
        );
    } else {
        fprintf(fh,
"//extern void loopPlc(void);  // Call loopPlc() function in main() of your project\n"
"extern void setupPlc(void); //  or initialise PLC cycle timer\n"
"extern void PlcCycle(void); //  and call PlcCycle() function once per PLC cycle timer.\n"
"\n"
        );
    }
    if(UartFunctionUsed()) {
        fprintf(fh,
"void UART_Init(void);\n"
"void UART_Transmit(unsigned char data);\n"
"unsigned char UART_Receive(void);\n"
"ldBOOL UART_Receive_Avail(void);\n"
"ldBOOL UART_Transmit_Ready(void);\n"
"\n"
        );
    }
    if(EepromFunctionUsed()) {
        fprintf(fh,
"#define eeprom_busy_wait() do {} while(!eeprom_is_ready())\n"
"\n"
"#ifdef USE_MACRO\n"
"  #define EEPROM_write( addr, x ) EEPROM.write(addr, x)\n"
"  #define EEPROM_read( addr ) EEPROM.read(addr)\n"
"#else\n"
"  void EEPROM_write(int addr, unsigned char data);\n"
"  unsigned char EEPROM_read(int addr);\n"
"#endif\n"
"\n"
        );
    }

    if(compile_MNU == MNU_COMPILE_ARDUINO) {
          SetExt(dest, dest, ".cpp");
    }
    FILE *f = fopen(dest, "w");
    if(!f) {
        Error(_("Couldn't open file '%s'"), dest);
        fclose(flh);
        fclose(fh);
        return;
    }

    fprintf(f,
"/* This is auto-generated C code from LDmicro. Do not edit this file! Go\n"
"   back to the LDmicro ladder diagram source for changes in the ladder logic, and make\n"
"   any C additions either in ladder.h or in additional .c or .h files linked\n"
"   against this one. */\n"
"\n"
"/* You must provide ladder.h; there you must provide:\n"
"      * a typedef for SWORD and ldBOOL, signed 16 bit and boolean types\n"
"        (probably typedef signed short SWORD; typedef unsigned char BOOL;)\n"
"\n"
"   You must also provide implementations of all the I/O read/write\n"
"   either as inlines in the header file or in another source file. (The\n"
"   I/O functions are all declared extern.)\n"
"\n"
"   See the generated source code (below) for function names. */\n"
"\n"
    );

  int divisor = 0;
  double actual = 0;
  double percentErr = 0;
  if(UartFunctionUsed()) {
    calcAvrUsart(&divisor, &actual, &percentErr);
    testAvrUsart(divisor, actual, percentErr);
  }
  if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
    fprintf(f,
"#define CCS_PIC_C // CCS C Compiler, Custom Computer Services, Inc.\n"
"#ifdef CCS_PIC_C\n"
"  #device %s\n"
"  #include <%s.h>\n"
    , Prog.mcu->mcuH2, Prog.mcu->mcuH
    );
    if(AdcFunctionUsed()){
      fprintf(f,
"  #device ADC=10\n"
      );
    }
    if(PwmFunctionUsed()){
      fprintf(f,
"  #USE PWM\n"
      );
    }
    fprintf(f,
"  #FUSES 1=0x%04X\n"
    , (WORD)Prog.configurationWord & 0xFFFF);
    if(Prog.configurationWord & 0xFFFF0000) {
    fprintf(f,
"   #FUSES 2=0x%04X\n"
    , (WORD)(Prog.configurationWord >> 16) & 0xFFFF);
    }
    if(DelayUsed() || UartFunctionUsed()) {
      fprintf(f,
"  #USE DELAY(CLOCK=%d)\n"
      , Prog.mcuClock);
/*
      fprintf(f,
"  #USE DELAY(INTERNAL=%d)\n"
      , Prog.mcuClock);
*/
    }
    if(UartFunctionUsed()) {
      fprintf(f,
"  #USE RS232(BAUD=%d, BITS=8, PARITY=N, STOP=1, ERRORS, UART1) // ENABLE=pin\n"
      , Prog.baudRate);
    }
    int i;
    if(Prog.mcu)
    for(i = 0; i < MAX_IO_PORTS; i++) {
        if(IS_MCU_REG(i))
            fprintf(f,
"  #USE FAST_IO(%c)\n"
            , 'A'+i);
    }
    fprintf(f,
"#endif\n"
    );
  } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
    fprintf(f,
"#include <htc.h>\n"
"#define _XTAL_FREQ %d\n"
"__CONFIG(0x%X);\n"
    , Prog.mcuClock, (WORD)Prog.configurationWord & 0xFFFF);
    if(Prog.configurationWord & 0xFFFF0000) {
    fprintf(f,
"__CONFIG(0x%X);\n"
    , (WORD)(Prog.configurationWord >> 16) & 0xFFFF);
    }
    /*
    int i;
    if(Prog.mcu)
    for(i = 0; i < MAX_IO_PORTS; i++) {
        if(IS_MCU_REG(i))
            fprintf(f,
"  #define PIN%c PORT%c\n"
            , 'A'+i, 'A'+i);
    }
    */
  } else if(compile_MNU == MNU_COMPILE_CODEVISIONAVR) {
  }
    fprintf(f,
"#include \"ladder.h\"\n"
"#include \"%s.h\" // Copy this line into main project file where is function main().\n"
"\n"
"/* Define EXTERN_EVERYTHING in ladder.h if you want all symbols extern.\n"
"   This could be useful to implement `magic variables,' so that for\n"
"   example when you write to the ladder variable duty_cycle, your PLC\n"
"   runtime can look at the C variable U_duty_cycle and use that to set\n"
"   the PWM duty cycle on the micro. That way you can add support for\n"
"   peripherals that LDmicro doesn't know about. */\n"
"#ifdef EXTERN_EVERYTHING\n"
"  #define STATIC\n"
"#else\n"
"  #define STATIC static\n"
"#endif\n"
"\n"
"/* Ux_xxx symbols correspond to user-defined names. There is such a symbol\n"
"   for every internal relay, variable, timer, and so on in the ladder\n"
"   program. Ix_xxx symbols are internally generated. */\n"
"/* Ix_xxx\n"
"   Ux_xxx\n"
"    ^\n"
"    b means BOOL type\n"
"    i means int type */\n"
"\n"
        , CurrentLdName);

  if(compile_MNU == MNU_COMPILE_ARDUINO) {
    fprintf(fh,
"// You provide I/O pin mapping for ARDUINO in ladder.h.\n"
"// See example lader.h_.\n"
    "\n"
);
    fprintf(flh,
"// You provide analog reference type for ARDUINO in ladder.h here.\n"
"const int analogReference_type = DEFAULT;\n"
"\n"
"// You must provide the I/O pin mapping for ARDUINO board in ladder.h here.\n"
    );
//  if(Prog.cycleDuty) {
//      fprintf(flh,
// const int pin_Ub_YPlcCycleDuty = %d;\n
  } else if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
  } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
  }
    // now generate declarations for all variables
    GenerateDeclarations(f);
    GenerateAnsiC_flash_eeprom(f);
    GenerateSUBPROG(f);

  if(compile_MNU == MNU_COMPILE_ARDUINO) {
    if(UartFunctionUsed()) {
      fprintf(f,
"void UART_Transmit(unsigned char data) {\n"
"    Serial.write(data);\n"
"    //Serial.flush();\n"
"}\n"
"\n"
"unsigned char UART_Receive(void) {\n"
"    return Serial.read();\n"
"}\n"
"\n"
"ldBOOL UART_Receive_Avail(void) {\n"
"    return Serial.available();\n"
"}\n"
"\n"
"ldBOOL UART_Transmit_Ready(void) {\n"
"    Serial.flush();\n"
"    return 1;\n"
"}\n"
"\n"
"ldBOOL UART_Transmit_Busy(void) {\n"
"    Serial.flush();\n"
"    return 0;\n"
"}\n"
"\n"
      );
    }
    if(EepromFunctionUsed()) {
      fprintf(f,
"#ifndef USE_MACRO\n"
"void EEPROM_write(int addr, unsigned char data) {\n"
"    EEPROM.write(addr, data);\n"
"}\n"
"\n"
"unsigned char EEPROM_read(int addr) {\n"
"    return EEPROM.read(addr);\n"
"}\n"
"\n"
"#endif\n"
"void EEPROM_fill(int addr1, int addr2, unsigned char data) {\n"
"    for (int i = max(0,addr1) ; i < min(addr2+1, EEPROM.length()) ; i++)\n"
"        EEPROM.write(i, data);\n"
"}\n"
"\n"
      );
    }
  } else if(mcu_ISA == ISA_AVR) {
    if(UartFunctionUsed())
#define          RXEN   BIT4
#define          TXEN   BIT3
      fprintf(f,
"void UART_Init(void) {\n"
"  UBRRH = %d; \n"
"  UBRRL = %d; \n"
"  UCSRB = %d; \n"
"}\n",
      divisor >> 8, divisor & 0xff, (1 << RXEN) | (1 << TXEN));
      fprintf(f,
"\n"
"void UART_Transmit(unsigned char data) {\n"
"  // Wait for empty transmit buffer\n"
"  while( !( UCSRA & (1<<UDRE)) );\n"
"  // Put data into buffer, sends the data\n"
"  UDR = data;\n"
"}\n"
"\n"
"unsigned char UART_Receive(void) {\n"
"  // Wait for data to be received\n"
"  while( !(UCSRA & (1<<RXC)) );\n"
"  // Get and return received data from buffer\n"
"  return UDR;\n"
"}\n"
"\n"
"ldBOOL UART_Transmit_Ready(void) {\n"
"  return UCSRA & (1<<UDRE);\n"
"}\n"
"\n"
"ldBOOL UART_Transmit_Busy(void) {\n"
"  return (UCSRA & (1<<UDRE)) == 0;\n"
"}\n"
"\n"
"ldBOOL UART_Receive_Avail(void) {\n"
"  return UCSRA & (1<<RXC);\n"
"}\n"
"\n"
      );
  } else if(mcu_ISA == ISA_PIC16) {
    if(EepromFunctionUsed()) {
      if(compile_MNU == MNU_COMPILE_CCS_PIC_C)
        fprintf(f,
"void EEPROM_write(int addr, unsigned char data) {\n"
"    write_eeprom(addr, data);\n"
"}\n"
"\n"
"unsigned char EEPROM_read(int addr) {\n"
"    return read_eeprom(addr);\n"
"}\n"
"\n"
        );
      else
        fprintf(f,
"void EEPROM_write(int addr, unsigned char data) {\n"
"    EEADR = addr;\n"
"    EEDATA = data;\n"
"    EEPGD = 0;\n"
"    WREN = 1;\n"
"    EECON2 = 0x55;\n"
"    EECON2 = 0xAA;\n"
"    WR = 1;\n"
"    WREN = 0;\n"
"}\n"
"\n"
"unsigned char EEPROM_read(int addr) {\n"
"    EEADR = addr;\n"
"    EEPGD = 0;\n"
"    RD = 1;\n"
"    return EEDATA;\n"
"}\n"
"\n"
"void EEPROM_fill(int addr1, int addr2, unsigned char data) {\n"
"//  for (int i = max(0,addr1) ; i < min(addr2+1, EEPROM.length()) ; i++)\n"
"//      EEPROM.write(i, data);\n"
"}\n"
"\n"
        );
    }
    if(AdcFunctionUsed()){
      fprintf(f,
"void ADC_Init(void) {\n"
      );
      if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
        fprintf(f,
"  setup_adc_ports(ALL_ANALOG);\n"
"  setup_adc(ADC_CLOCK_INTERNAL | ADC_CLOCK_DIV_32);\n"
        );
      } else {
      }
      fprintf(f,
"}\n"
"\n"
      );
    }
    if(PwmFunctionUsed()) {
      fprintf(f,
"void PWM_Init(void) {\n"
      );
      fprintf(f,
"}\n"
"\n"
      );
    }
    if(UartFunctionUsed()) {
      if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
        fprintf(f,
"void UART_Init(void) {\n"
"  // UART baud rate setup\n"
"  // setup_uart(%d);\n"
"}\n"
"\n"
        , Prog.baudRate);
      } else {
        fprintf(f,
"void UART_Init(void) {\n"
"  // UART baud rate setup\n"
        );
       if(compile_MNU != MNU_COMPILE_ANSIC) {
        int divisor = (Prog.mcuClock + Prog.baudRate*32)/(Prog.baudRate*64) - 1;
        fprintf(f,
"  SPBRG = %d;\n"
"  TXEN = 1; SPEN = 1; CREN = 1;\n"
        , divisor);
       }
        fprintf(f,
"}\n"
"\n"
        );
      }
      if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
        fprintf(f,
"void UART_Transmit(unsigned char data) {\n"
"  // Wait for empty transmit buffer\n"
"  //while( !TRMT );\n"
"  // Put data into buffer, sends the data\n"
"  putc(data);\n"
"}\n"
"\n"
"unsigned char UART_Receive(void) {\n"
"  // Wait for data to be received\n"
"  while( !kbhit() );\n"
"  // Get and return received data from buffer\n"
"  return getc();\n"
"}\n"
"\n"
"ldBOOL UART_Transmit_Ready(void) {\n"
"  return 1; // ???\n"
"}\n"
"\n"
"ldBOOL UART_Transmit_Busy(void) {\n"
"  return 0; // ???\n"
"}\n"
"\n"
"ldBOOL UART_Receive_Avail(void) {\n"
"  // Check if a character has been received\n"
"  return kbhit();\n"
"}\n"
"\n"
        );
      } else {
        fprintf(f,
"void UART_Transmit(unsigned char data) {\n"
"  // Wait for empty transmit buffer\n"
"  while( !TRMT );\n"
"  // Put data into buffer, sends the data\n"
"  TXREG = data;\n"
"}\n"
"\n"
"unsigned char UART_Receive(void) {\n"
"  // Wait for data to be received\n"
"  while( !RCIF );\n"
"  // Get and return received data from buffer\n"
"  return RCREG;\n"
"}\n"
"\n"
"ldBOOL UART_Transmit_Ready(void) {\n"
"  return TRMT;\n"
"}\n"
"\n"
"ldBOOL UART_Transmit_Busy(void) {\n"
"  return ! TRMT;\n"
"}\n"
"\n"
"ldBOOL UART_Receive_Avail(void) {\n"
"  return RCIF;\n"
"}\n"
"\n"
        );
      }
    }
  }
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
    fprintf(f,
"\n"
"/* Call this function once per PLC cycle. You are responsible for calling\n"
"   it at the interval that you specified in the LDmicro MCU configuration when you\n"
"   generated this code. */\n"
"void PlcCycle(void) {\n"
    );
    GenerateAnsiC(f, 0, IntCodeLen-1);
    fprintf(f, "}\n");
//---------------------------------------------------------------------------
  if(compile_MNU == MNU_COMPILE_ARDUINO) {
    fprintf(f,
"\n"
"// PLC Cycle timing function.\n"
"boolean IsPlcInterval() {\n"
"    static unsigned long last_run;\n"
"    unsigned long micros_now = micros();\n"
"    if (micros_now - last_run >= PLC_INTERVAL) {\n"
"        last_run = micros_now;\n"
"        return true;\n"
"    }\n"
"    return false;\n"
"}\n"
"\n"
"// Call loopPlc() function in loop() of your arduino project once.\n"
"void loopPlc() {\n"
"    if (IsPlcInterval()) {\n"
"        #ifdef USE_WDT\n"
"        wdt_reset();\n"
"        #endif\n"
    );
    if(Prog.cycleDuty) {
      fprintf(f,
"        Write1_Ub_YPlcCycleDuty();\n"
      );
    }
    fprintf(f,
"        PlcCycle();\n"
    );
    if(Prog.cycleDuty) {
      fprintf(f,
"        Write0_Ub_YPlcCycleDuty();\n"
      );
    }
    fprintf(f,
"    }\n"
"}\n"
"\n"
"// Call setupPlc() function in setup() function of your arduino project once.\n"
"void setupPlc(void) {\n"
"   #ifdef USE_WDT\n"
"   wdt_enable(WDTO_2S);\n"
"   #endif\n"
"// Initialise PLC cycle timer here if you need.\n"
"// ...\n"
"\n");
    if(UartFunctionUsed()) {
      Comment("Set up UART");
      fprintf(f,
"   Serial.begin(%d);\n"
"   while(!Serial) {\n"
"       ; // We expect to connect the serial port. It is only necessary for the Leonardo.\n"
"   }\n"
      , Prog.baudRate);
    }
    Comment(" Set up analog reference type");
    fprintf(f,"    analogReference(analogReference_type);\n\n");

    Comment(" Set up I/O pins");
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if((Prog.io.assignment[i].type == IO_TYPE_INT_INPUT)
        || (Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT))
            fprintf(f,"    pinMode(pin_%s, INPUT_PULLUP);\n", MapSym(Prog.io.assignment[i].name, ASBIT));
        else if (Prog.io.assignment[i].type == IO_TYPE_PWM_OUTPUT)
            fprintf(f,"    pinMode(pin_%s, OUTPUT);\n", MapSym(Prog.io.assignment[i].name, ASBIT));
        else if (Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT)
            fprintf(f,"    pinMode(pin_%s, OUTPUT);\n", MapSym(Prog.io.assignment[i].name, ASBIT));
/*
      if((Prog.io.assignment[i].type == IO_TYPE_INT_INPUT)
      || (Prog.io.assignment[i].type == IO_TYPE_PWM_OUTPUT)
      || (Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT)
      || (Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT)) {
        if(Prog.io.assignment[i].name[0] == 'X')
            fprintf(f,"    pinMode(pin_%s, INPUT);\n", MapSym(Prog.io.assignment[i].name, ASBIT));
        else if(Prog.io.assignment[i].name[0] == 'Y')
            fprintf(f,"    pinMode(pin_%s, OUTPUT);\n", MapSym(Prog.io.assignment[i].name, ASBIT));
//      else if(Prog.io.assignment[i].name[0] == 'A')
//          fprintf(f,"    analogReference(analogReference_type);\n");
      }
*/
    }
    if(SleepFunctionUsed()) {
      fprintf(f,
"\n"
"    set_sleep_mode(SLEEP_MODE_PWR_DOWN);\n"
      );
    }

    fprintf(f,
"}\n");
  } else {
    fprintf(f,
"\n"
"void setupPlc(void) {\n"
"    // Set up I/O pins direction, and drive the outputs low to start.\n"
    );
    BYTE isInput[MAX_IO_PORTS], isAnsel[MAX_IO_PORTS], isOutput[MAX_IO_PORTS];
    BuildDirectionRegisters(isInput, isAnsel, isOutput);
    if(Prog.mcu) {
      int i;
      for(i = 0; i < MAX_IO_PORTS; i++) {
        if(IS_MCU_REG(i)) {
          if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
            // drive the outputs low to start
            fprintf(f,"    output_%c(0x%02X);\n", 'a'+i, 0x00);
            // Set up I/O pins direction
            fprintf(f,"    set_tris_%c(0x%02X);\n", 'a'+i, ~isOutput[i] & 0xff);
          } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
            // drive the outputs low to start
            fprintf(f,"    PORT%c = 0x%02X;\n", 'A'+i, 0x00);
            // Set up I/O pins direction
            fprintf(f,"    TRIS%c = 0x%02X;\n", 'A'+i, ~isOutput[i] & 0xff);
          } else {
            //fprintf(f,"      pokeb(0x%X, 0x%X); // PORT%c\n",Prog.mcu->dirRegs[i], isOutput[i], Prog.mcu->pinInfo[i].port);
            //fprintf(f,"    pokeb(0x%X, 0x%X);\n",Prog.mcu->dirRegs[i], isOutput[i]);
            fprintf(f,"    DDR%c = 0x%02X;\n", 'A'+i, isOutput[i]);
            // turn on the pull-ups, and drive the outputs low to start
            //fprintf(f,"    pokeb(0x%X, 0x%X);\n",Prog.mcu->outputRegs[i], isInput[i]);
            fprintf(f,"    PORT%c = 0x%02X;\n", 'A'+i, isInput[i]);
          }
        }
      }
    }

    fprintf(f,
"\n"
"    // Turn on the pull-ups.\n"
"    #ifdef CCS_PIC_C\n"
"        port_b_pullups(TRUE);\n"
"    #elif defined(HI_TECH_C)\n"
"        nRBPU = 0;\n"
"    #endif\n"
"\n"
"  #ifdef USE_WDT\n"
"    // Watchdog on\n"
"    #ifdef __CODEVISIONAVR__\n"
"        #ifndef WDTCR\n"
"            #define WDTCR WDTCSR\n"
"        #endif\n"
"        #asm(\"wdr\")\n"
"        WDTCR |= (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);\n"
"    #elif defined(__GNUC__)\n"
"        wdt_reset();\n"
"        wdt_enable(WDTO_2S);\n"
"    #elif defined(CCS_PIC_C)\n"
"        setup_wdt(WDT_2304MS);\n"
"    #elif defined(HI_TECH_C)\n"
"        //WDTCON=1;\n"
"    #else\n"
"        // Watchdog Init is required. // You must provide this.\n"
"    #endif\n"
"  #endif\n"
"\n"
    );

  if(Prog.cycleTime > 0) {
    CalcPicPlcCycle(Prog.cycleTime, PicProgLdLen);

    fprintf(f,
"    //Initialise PLC cycle timer here.\n"
"    // Configure Timer %d\n"
         , Prog.cycleTimer
    );

    if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
      fprintf(f,
"    #if getenv(\"TIMER%d\") == 0\n"
"        #error Don't exist TIMER%d\n"
"    #endif\n"
      , Prog.cycleTimer, Prog.cycleTimer);

      if(Prog.cycleTimer==0) {
          fprintf(f,
"    setup_timer_0(T0_INTERNAL | T0_DIV_%d);\n"
          , plcTmr.prescaler
          );
      } else {
          fprintf(f,
"    setup_timer_1(T1_INTERNAL | T1_DIV_BY_%d);\n"
          , plcTmr.prescaler
          );
      }
    } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
        if(Prog.cycleTimer == 0) {
          fprintf(f,
"    #ifdef USE_WDT\n"
"    CLRWDT();\n"
"    #endif\n"
"    TMR0 = 0;\n"
"    PSA = %d;\n"
"    T0CS = 0;\n"
"    OPTION_REGbits.PS = %d;\n"
          , plcTmr.prescaler == 1 ? 1 : 0, plcTmr.PS
          );
//"          T1CON = plcTmr.PS;\n"
        } else {
          fprintf(f,
"    #ifdef USE_WDT\n"
"    CLRWDT(); // Clear WDT and prescaler\n"
"    #endif\n"
"    CCPR1L = 0x%X;\n"
"    CCPR1H = 0x%X;\n"
"    TMR1L = 0;\n"
"    TMR1H = 0;\n"
"    T1CON = 0x%X;\n"
"    CCP1CON = 0x0B; // compare mode, reset TMR1 on trigger\n"
          , plcTmr.tmr & 0xff, plcTmr.tmr >> 8, plcTmr.PS
          );

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
            fprintf(f,
"    TMR1GE = 1;\n"
            );
          }
        }
  }
    } else if(mcu_ISA == ISA_AVR) {
      if(Prog.cycleTime > 0) {
        CalcAvrPlcCycle(Prog.cycleTime, AvrProgLdLen);

        int counter = plcTmr.tmr - 1/* + CorrectorPlcCycle*/; // TODO
        // the counter is less than the divisor at 1
        if(counter < 0) counter = 0;
        if(counter > 0xffff) counter = 0xffff;
        //dbp("divider=%d EQU counter=%d", divider, counter);

        fprintf(f,
"        TCCR1A = 0x00; // WGM11=0, WGM10=0\n"
"        TCCR1B = (1<<WGM12) | %d; // WGM13=0, WGM12=1\n"
"        // `the high byte must be written before the low byte\n"
"        OCR1AH = (%d >> 8) & 0xff;\n"
"        OCR1AL = %d  & 0xff;\n"
        , plcTmr.cs, counter, counter);
      }
    } else {
        fprintf(f,
"    //  You must init PLC timer.\n"
        );
    }
    if(UartFunctionUsed()) {
      fprintf(f,
"    UART_Init();\n"
      );
    }
    if(AdcFunctionUsed()) {
      fprintf(f,
"    ADC_Init();\n"
      );
    }
    if(PwmFunctionUsed()) {
      fprintf(f,
"    PWM_Init();\n"
      );
    }
    fprintf(f,
"}\n");

    fprintf(f,
"\n"
"void mainPlc(void) { // Call mainPlc() function in main() of your project.\n"
"    setupPlc();\n"
"    while(1) {\n"
    );
/*
    McuIoPinInfo *iop;
    if(Prog.cycleDuty) {
        iop = PinInfoForName(YPlcCycleDuty);
        if(iop) {
          if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
            fprintf(f,
"        output_low(PIN_%c%d); // YPlcCycleDuty\n", iop->port, iop->bit);
          } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
            fprintf(f,
"        R%c%d = 0; // YPlcCycleDuty\n", iop->port, iop->bit);
          } else {
            fprintf(f,
"        PORT%c &= ~(1<<PORT%c%d); // YPlcCycleDuty\n", iop->port, iop->port, iop->bit);
          }
        }
    }
*/
    fprintf(f,
"        // Test PLC cycle timer interval here.\n"
    );
    if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
        fprintf(f,
"        while(get_timer%d() < (%d-1));\n"
"        set_timer%d(0); // Try it when the PLC cycle time is more than 1 ms.\n"
"      //set_timer%d(1); // Try it when the PLC cycle time is less than 1 ms.\n"
        , Prog.cycleTimer, plcTmr.tmr
        , Prog.cycleTimer, Prog.cycleTimer);
        fprintf(f,
"      //set_timer%d(get_timer%d() - (%d-1)); // Try it.\n"
        , Prog.cycleTimer, Prog.cycleTimer, plcTmr.tmr);
    } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
      if(Prog.cycleTimer == 0) {
        fprintf(f,
"        #ifndef T0IF\n"
"          #define T0IF TMR0IF\n"
"        #endif\n"
"        while(T0IF == 0);\n"
"        TMR0 += %d;\n" // reprogram TMR0
"        T0IF = 0;\n"
        , 256 - plcTmr.tmr + 1);
      } else {
        fprintf(f,
"        while(CCP1IF == 0);\n"
"        CCP1IF = 0;\n"
        );
      }
    } else if(mcu_ISA == ISA_AVR) {
        fprintf(f,
"        #ifndef TIFR\n"
"        #define TIFR TIFR1\n"
"        #endif\n"
"        while((TIFR & (1<<OCF1A)) == 0);\n"
"        TIFR |= 1<<OCF1A; // OCF1A can be cleared by writing a logic one to its bit location\n"
        );
    } else {
        fprintf(f,
"        //  You must check PLC timer interval.\n"
        );
    }
    fprintf(f,"\n");
    if(Prog.cycleDuty) {
            fprintf(f,
"        Write1_Ub_YPlcCycleDuty();\n");
/*
        if(iop) {
          if(compile_MNU == MNU_COMPILE_CCS_PIC_C) {
            fprintf(f,
"        output_high(PIN_%c%d); // YPlcCycleDuty\n", iop->port, iop->bit);
          } else if(compile_MNU == MNU_COMPILE_HI_TECH_C) {
            fprintf(f,
"        R%c%d = 1; // YPlcCycleDuty\n", iop->port, iop->bit);
          } else if(compile_MNU == MNU_COMPILE_ARDUINO) {
            fprintf(f,
"        Write1_Ub_YPlcCycleDuty();\n");
          } else {
            fprintf(f,
"        PORT%c |= 1<<PORT%c%d; // YPlcCycleDuty\n", iop->port, iop->port, iop->bit);
          }
        }
*/
    }
    fprintf(f,
"\n"
"        PlcCycle();\n"
"        // You can place your code here, if you don't generate C code from LDmicro again.\n"
"        // ...\n"
"\n"
"      #ifdef USE_WDT\n"
"        // Watchdog reset\n"
"        #ifdef __CODEVISIONAVR__\n"
"            #asm(\"wdr\")\n"
"        #elif defined(__GNUC__)\n"
"            wdt_reset();\n"
"        #elif defined(CCS_PIC_C)\n"
"            restart_wdt();\n"
"        #elif defined(HI_TECH_C)\n"
"            CLRWDT();\n"
"        #else\n"
"            // Watchdog Reset is required. // You must provide this.\n"
"        #endif\n"
"      #endif\n"
"\n"
    );
    if(Prog.cycleDuty) {
            fprintf(f,
"        Write0_Ub_YPlcCycleDuty();\n");
    }
    fprintf(f,
"    }\n"
"}\n"
"\n"
    );

    fprintf(f,
"#ifdef __CODEVISIONAVR__\n"
"void main(void) { // You can use this as is.\n"
"    mainPlc();\n"
"    return;\n"
"}\n"
"#else\n"
"int main(void) { // You can use this as is.\n"
"    mainPlc();\n"
"    return 0;\n"
"}\n"
"#endif\n"
    );
    }

    fclose(f);

    fprintf(fh,"#endif\n");
    fclose(fh);

    if(compile_MNU == MNU_COMPILE_ARDUINO) {
      fprintf(flh,"   You can comment or delete this line after provide the I/O pin mapping for ARDUINO board in ladder.h above.\n");
    }
    fprintf(flh,"\n");
    fprintf(flh,"#endif\n");
    fclose(flh);

    if(compile_MNU == MNU_COMPILE_ARDUINO) {
        SetExt(ladderhName, dest, ".ino_")  ;

        flh = fopen(ladderhName, "w");
        if(!flh) {
            Error(_("Couldn't open file '%s'"), ladderhName);
            //return;
        }
        fprintf(flh,
"/* This is example for %s.ino file!\n"
"   This is auto-generated ARDUINO C code from LDmicro.\n"
"   Rename this file as %s.ino or copy content(or part) of this file\n"
"   to existing %s.ino. Remove this comment from %s.ino */\n"
"\n"
"#include \"%s.h\"\n"
"\n"
"void setup() {\n"
"  // put your setup code here, to run once:\n"
"  setupPlc();\n"
"}\n"
"\n"
"void loop() {\n"
"  // put your main code here, to run repeatedly:\n"
"  loopPlc();\n"
"}\n"
"\n"
    , CurrentLdName, CurrentLdName, CurrentLdName, CurrentLdName, CurrentLdName);
        fclose(flh);
    }

    char str[MAX_PATH+500];
    sprintf(str, _("Compile successful; wrote C source code to '%s'.\r\n\r\n"
        "This is not a complete C program. You have to provide the runtime "
        "and all the I/O routines. See the comments in the source code for "
        "information about how to do this."), dest);
    CompileSuccessfulMessage(str);
}
/*
"    #ifdef __CODEVISIONAVR__\n"
"        #asm\n"
"            nop\n"
"        #endasm\n"
"    #endif\n"
*/