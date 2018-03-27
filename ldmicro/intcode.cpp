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
// Generate intermediate code for the ladder logic. Basically generate code
// for a `virtual machine' with operations chosen to be easy to compile to
// AVR or PIC16 code.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "intcode.h"
//#include "display.h"

//// #define NEW_TON // (C) GitHub.LDmicro@gmail.com // fail
//// Restored original TON algorithm because NEW_TON don't enable RESET(TON)

#define NEW_ONE_SHOT // (C) GitHub.LDmicro@gmail.com
//#define DEFAULT_PARALLEL_ALGORITHM
//#define DEFAULT_COIL_ALGORITHM

//-----------------------------------------------------------------------------
#ifdef DEFAULT_PARALLEL_ALGORITHM
int int_comment_level  = 1;
#else
int int_comment_level  = 3;
//                       0 - no comments
//                       1 = Release 2.3 comments
//                       2 - more comments
//                     * 3 - ELEM_XXX comments added
#endif
//-----------------------------------------------------------------------------
int asm_comment_level  = 2;
//                       0- no comment
//                       1- intenal comments if exist
//                     * 2- args
//                       3- + RUNG number in source.ld
//                       4-    -//-        and  pic16.cpp or avr.cpp line number
//                       5     -//-                      -//-        and intcode.cpp line number
//for example
//-----------------------------------------------------------------------------
int asm_discover_names = 0;
//                     * 0- no discover
//                       1- discover name if posible
//                       2-     -//- and add dec value
//                       3- replace name if posible
//                       4-     -//- and add dec value
//for example
//                       0- l_0009:     bcf  0xa,    4
//                       1- l_0009:     bcf  0xa,    4 ; REG_PCLATH
//                       2- l_0009:     bcf  0xa,    4 ; REG_PCLATH ; 10
//                       3- l_0009:     bcf  REG_PCLATH,     4 ; 0xa
//                       4- l_0009:     bcf  REG_PCLATH,     4 ; 0xa ; 10
//-----------------------------------------------------------------------------

DWORD addrRUartRecvErrorFlag;
int    bitRUartRecvErrorFlag;
DWORD addrRUartSendErrorFlag;
int    bitRUartSendErrorFlag;

IntOp IntCode[MAX_INT_OPS];
int IntCodeLen = 0;
int ProgWriteP = 0;
static SDWORD *Tdata;
int rungNow = -INT_MAX;
static int whichNow = -INT_MAX;
static ElemLeaf *leafNow = NULL;

static DWORD GenSymCountParThis;
static DWORD GenSymCountParOut;
static DWORD GenSymCountOneShot;
static DWORD GenSymCountFormattedString;
static DWORD GenSymCountStepper;

DWORD EepromAddrFree;
DWORD RomSection;

//-----------------------------------------------------------------------------
static void CheckConstantInRange(SDWORD v)
{
    /*
    if(v < -0x800000 || v > 0x7FffFF) {
        Error(_("Constant %d out of range: %d to %d inclusive."), v, -0x800000, 0x7FffFF);
        CompileError();
    }
    */
}

//-----------------------------------------------------------------------------
// Pretty-print the intermediate code to a file, for debugging purposes.
//-----------------------------------------------------------------------------
void IntDumpListing(char *outFile)
{
    FILE *f = fopen(outFile, "w");
    if(!f) {
        Error(_("Couldn't dump intermediate code to '%ls'."), u16(outFile));
    }

    int i;
    int indent = 0;
    for(i = 0; i < IntCodeLen; ++i) {

        if(IntCode[i].op == INT_END_IF) indent--;
        if(IntCode[i].op == INT_ELSE) indent--;

        if(int_comment_level == 1) {
            fprintf(f, "%3d:", i);
        } else {
            if(indent < 0) indent = 0;
            if((IntCode[i].op != INT_SIMULATE_NODE_STATE)
            && (IntCode[i].op != INT_AllocKnownAddr)
            && (IntCode[i].op != INT_AllocFwdAddr))
                fprintf(f, "%4d:", i);
        }
        int j;
        if((int_comment_level == 1) || (IntCode[i].op != INT_SIMULATE_NODE_STATE))
        for(j = 0; j < indent; j++) fprintf(f, "    ");

        switch(IntCode[i].op) {
            case INT_SET_BIT:
                fprintf(f, "set bit '%s'", IntCode[i].name1);
                break;

            case INT_CLEAR_BIT:
                fprintf(f, "clear bit '%s'", IntCode[i].name1);
                break;

            case INT_COPY_BIT_TO_BIT:
                fprintf(f, "let bit '%s' := '%s'", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_COPY_NOT_BIT_TO_BIT:
                fprintf(f, "let bit '%s' := ! '%s'", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_COPY_XOR_BIT_TO_BIT:
                fprintf(f, "let bit '%s' := '%s' ^ '%s'", IntCode[i].name1, IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_COPY_VAR_BIT_TO_VAR_BIT:
                fprintf(f, "if ('%s' & (1<<%d)) {", IntCode[i].name2, IntCode[i].literal2); indent++;
                fprintf(f, "  '%s' |= (1<<%d) } else {", IntCode[i].name1, IntCode[i].literal);
                fprintf(f, "  '%s' &= ~(1<<%d) }", IntCode[i].name1, IntCode[i].literal); indent--;
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                fprintf(f, "let var '%s' := %d", IntCode[i].name1,
                    IntCode[i].literal);
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                fprintf(f, "let var '%s' := '%s'", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_SET_BIN2BCD:
                fprintf(f, "let var '%s' = bin2bcd('%s');", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_SET_BCD2BIN:
                fprintf(f, "let var '%s' = bcd2bin('%s');", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_SET_OPPOSITE:
                fprintf(f, "let var '%s' = opposite('%s');", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_SET_VARIABLE_ROL:
                fprintf(f, "let var '%s' := '%s' rol '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_ROR:
                fprintf(f, "let var '%s' := '%s' ror '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_SHL:
                fprintf(f, "let var '%s' := '%s' << '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_SHR:
                fprintf(f, "let var '%s' := '%s' >> '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_AND:
                fprintf(f, "let var '%s' := '%s' & '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_OR:
                fprintf(f, "let var '%s' := '%s' | '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_XOR:
                fprintf(f, "let var '%s' := '%s' ^ '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_NOT:
                fprintf(f, "let var '%s' := ~ '%s'", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_SET_SWAP:
                fprintf(f, "let var '%s' = swap('%s');", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_SET_VARIABLE_SR0:
                fprintf(f, "let var '%s' := '%s' sr0 '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_NEG:
                fprintf(f, "let var '%s' := - '%s'", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_SET_VARIABLE_ADD:
                fprintf(f, "let var '%s' := '%s' + '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                if(IntCode[i].name4 && strlen(IntCode[i].name4))
                    fprintf(f, "; copy overflow flag to '%s'", IntCode[i].name4);
                break;

            case INT_SET_VARIABLE_SUBTRACT:
                fprintf(f, "let var '%s' := '%s' - '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                if(IntCode[i].name4 && strlen(IntCode[i].name4))
                    fprintf(f, "; copy overflow flag to '%s'", IntCode[i].name4);
                break;

            case INT_SET_VARIABLE_MULTIPLY:
                fprintf(f, "let var '%s' := '%s' * '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_DIVIDE:
                fprintf(f, "let var '%s' := '%s' / '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_MOD:
                fprintf(f, "let var '%s' := '%s' % '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_INCREMENT_VARIABLE:
                fprintf(f, "increment '%s'", IntCode[i].name1);
                if(IntCode[i].name2 && strlen(IntCode[i].name2))
                fprintf(f, "; copy overlap(-1 to 0) flag to '%s'", IntCode[i].name2);
                if(IntCode[i].name3 && strlen(IntCode[i].name3))
                fprintf(f, "; copy overflow flag to '%s'", IntCode[i].name3);
                break;

            case INT_DECREMENT_VARIABLE:
                fprintf(f, "decrement '%s'", IntCode[i].name1);
                if(IntCode[i].name2 && strlen(IntCode[i].name2))
                fprintf(f, "; copy overlap(0 to -1) flag to '%s'", IntCode[i].name2);
                if(IntCode[i].name3 && strlen(IntCode[i].name3))
                fprintf(f, "; copy overflow flag to '%s'", IntCode[i].name3);
                break;

            case INT_READ_ADC:
                fprintf(f, "read adc '%s'", IntCode[i].name1);
                break;

            case INT_SET_SEED_RANDOM:
                fprintf(f, "let var '$seed_%s' := '%s'", IntCode[i].name1, IntCode[i].name2);
                break;

            case INT_SET_VARIABLE_RANDOM:
                fprintf(f, "let var '%s' := random()", IntCode[i].name1);
                break;

            case INT_SET_PWM:
                fprintf(f, "set pwm '%s' %% %s Hz out '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_QUAD_ENCOD:
                fprintf(f, "QUAD ENCOD %d %s %s %s %s %s", IntCode[i].literal, IntCode[i].name1, IntCode[i].name2,
                  IntCode[i].name3, IntCode[i].name4, IntCode[i].name5);
                break;

            case INT_SET_NPULSE:
                fprintf(f, "generate %s pulses %s Hz to %s", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_OFF_NPULSE:
                fprintf(f, "OFF N PULSE");
                break;

            case INT_PWM_OFF:
                fprintf(f, "pwm off '%s'", IntCode[i].name1);
                break;

            case INT_EEPROM_BUSY_CHECK:
                fprintf(f, "set bit '%s' if EEPROM busy", IntCode[i].name1);
                break;

            case INT_EEPROM_READ:{
                int sov = SizeOfVar(IntCode[i].name1);
                if(sov == 1)
                    fprintf(f, "read EEPROM[%d] into '%s'",
                        IntCode[i].literal, IntCode[i].name1);
                else if(sov == 2)
                    fprintf(f, "read EEPROM[%d,%d+1] into '%s'",
                        IntCode[i].literal, IntCode[i].literal, IntCode[i].name1);
                else if(sov == 3)
                    fprintf(f, "read EEPROM[%d,%d+1,%d+2] into '%s'",
                        IntCode[i].literal, IntCode[i].literal, IntCode[i].literal, IntCode[i].name1);
                else if(sov == 4)
                    fprintf(f, "read EEPROM[%d,%d+1,%d+2,%d+3] into '%s'",
                        IntCode[i].literal, IntCode[i].literal, IntCode[i].literal, IntCode[i].literal, IntCode[i].name1);
                else oops();
                break;
            }
            case INT_EEPROM_WRITE:{
                int sov = SizeOfVar(IntCode[i].name1);
                if(sov == 1)
                    fprintf(f, "write '%s' into EEPROM[%d]",
                        IntCode[i].name1, IntCode[i].literal);
                else if(sov == 2)
                    fprintf(f, "write '%s' into EEPROM[%d,%d+1]",
                        IntCode[i].name1, IntCode[i].literal, IntCode[i].literal);
                else if(sov == 3)
                    fprintf(f, "write '%s' into EEPROM[%d,%d+1,%d+2]",
                        IntCode[i].name1, IntCode[i].literal, IntCode[i].literal, IntCode[i].literal);
                else if(sov == 4)
                    fprintf(f, "write '%s' into EEPROM[%d,%d+1,%d+2,%d+3]",
                        IntCode[i].name1, IntCode[i].literal, IntCode[i].literal, IntCode[i].literal, IntCode[i].literal);
                else oops();
                break;
            }
            case INT_SPI:
                fprintf(f, "SPI '%s' send '%s', recieve '%s', done? into '%s'",
                    IntCode[i].name1, IntCode[i].name2, IntCode[i].name3, IntCode[i].name4);
                break;

            case INT_UART_SEND1:
            case INT_UART_SENDn:
                fprintf(f, "uart send from '%s'", IntCode[i].name1);
                break;

            case INT_UART_SEND:
                fprintf(f, "uart send from '%s', done? into '%s'",
                    IntCode[i].name1, IntCode[i].name2);
                break;

            case INT_UART_SEND_READY:
                fprintf(f, "'%s' = is uart ready to send ?", IntCode[i].name1);
                break;

            case INT_UART_SEND_BUSY:
                fprintf(f, "'%s' = is uart busy to send ?", IntCode[i].name1);
                break;

            case INT_UART_RECVn:
            case INT_UART_RECV:
                fprintf(f, "uart recv int '%s', have? into '%s'",
                    IntCode[i].name1, IntCode[i].name2);
                break;

            case INT_UART_RECV_AVAIL:
                fprintf(f, "'%s' = is uart receive data available ?",
                    IntCode[i].name1);
                break;

            case INT_IF_BIT_SET:
                fprintf(f, "if '%s' {", IntCode[i].name1); indent++;
                break;

            case INT_IF_BIT_CLEAR:
                fprintf(f, "if not '%s' {", IntCode[i].name1); indent++;
                break;

            case INT_SLEEP:
                fprintf(f, "SLEEP;");
                break;

            case INT_DELAY:
                fprintf(f, "DELAY %d us;", IntCode[i].literal);
                break;

            case INT_CLRWDT:
                fprintf(f, "CLRWDT;");
                break;

            case INT_LOCK:
                fprintf(f, "LOCK;");
                break;

            case INT_VARIABLE_SET_BIT:
                fprintf(f, "set bit number '%s' in var '%s'", IntCode[i].name2, IntCode[i].name1);
                break;

            case INT_VARIABLE_CLEAR_BIT:
                fprintf(f, "clear bit number '%s' in var '%s'", IntCode[i].name2, IntCode[i].name1);
                break;

            case INT_IF_BIT_SET_IN_VAR: // TODO
                fprintf(f, "if ('%s' & (1<<%d)) != 0  {", IntCode[i].name1, IntCode[i].name2); indent++;
                break;

            case INT_IF_BIT_CLEAR_IN_VAR: // TODO
                fprintf(f, "if ('%s' & (1<<%d)) == 0 {", IntCode[i].name1, IntCode[i].name2); indent++;
                break;
            case INT_IF_BITS_SET_IN_VAR: // TODO
                fprintf(f, "if ('%s' & %d) == %d  {", IntCode[i].name1, IntCode[i].literal, IntCode[i].literal); indent++;
                break;
            case INT_IF_BITS_CLEAR_IN_VAR: // TODO
                fprintf(f, "if ('%s' & %d) == 0 {", IntCode[i].name1, IntCode[i].literal); indent++;
                break;

            #ifndef NEW_CMP
            case INT_IF_VARIABLE_LES_LITERAL:
                fprintf(f, "if '%s' < %d {", IntCode[i].name1,
                    IntCode[i].literal); indent++;
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                fprintf(f, "if '%s' == '%s' {", IntCode[i].name1,
                    IntCode[i].name2); indent++;
                break;

            case INT_IF_VARIABLE_GRT_VARIABLE:
                fprintf(f, "if '%s' > '%s' {", IntCode[i].name1,
                    IntCode[i].name2); indent++;
                break;
            #endif

            #ifdef NEW_CMP
            case INT_IF_GRT:
                fprintf(f, "if '%s' > '%s' {", IntCode[i].name1,
                    IntCode[i].name2); indent++;
                break;

            case INT_IF_GEQ:
                fprintf(f, "if '%s' >= '%s' {", IntCode[i].name1,
                    IntCode[i].name2); indent++;
                break;

            case INT_IF_LES:
                fprintf(f, "if '%s' < '%s' {", IntCode[i].name1,
                    IntCode[i].name2); indent++;
                break;

            case INT_IF_LEQ:
                fprintf(f, "if '%s' <= '%s' {", IntCode[i].name1,
                    IntCode[i].name2); indent++;
                break;

            case INT_IF_NEQ:
                fprintf(f, "if '%s' != '%s' {", IntCode[i].name1,
                    IntCode[i].name2); indent++;
                break;

            case INT_IF_EQU:
                fprintf(f, "if '%s' == '%s' {", IntCode[i].name1,
                    IntCode[i].name2); indent++;
                break;
            #endif

            case INT_END_IF:
                fprintf(f, "}");
                break;

            case INT_ELSE:
                fprintf(f, "} else {"); indent++;
                break;

            case INT_SIMULATE_NODE_STATE:
                // simulation-only; the real code generators don't care
                break;

            case INT_COMMENT:
                fprintf(f, "# %s", IntCode[i].name1);
                break;

            #ifdef USE_SFR
            // Special function
            case INT_READ_SFR_LITERAL:
            case INT_WRITE_SFR_LITERAL:
            case INT_SET_SFR_LITERAL:
            case INT_CLEAR_SFR_LITERAL:
            case INT_TEST_SFR_LITERAL:

            case INT_READ_SFR_VARIABLE:
            case INT_WRITE_SFR_VARIABLE:
            case INT_SET_SFR_VARIABLE:
            case INT_CLEAR_SFR_VARIABLE:
            case INT_TEST_SFR_VARIABLE:

            case INT_WRITE_SFR_LITERAL_L:
            case INT_WRITE_SFR_VARIABLE_L:

            case INT_SET_SFR_LITERAL_L:
            case INT_SET_SFR_VARIABLE_L:

            case INT_CLEAR_SFR_LITERAL_L:
            case INT_CLEAR_SFR_VARIABLE_L:

            case INT_TEST_SFR_LITERAL_L:
            case INT_TEST_SFR_VARIABLE_L:

            case INT_TEST_C_SFR_LITERAL:
            case INT_TEST_C_SFR_VARIABLE:
            case INT_TEST_C_SFR_LITERAL_L:
            case INT_TEST_C_SFR_VARIABLE_L:
                switch(IntCode[i].op) {
                    case INT_TEST_SFR_LITERAL_L:
                    case INT_TEST_SFR_VARIABLE_L:

                    case INT_TEST_C_SFR_LITERAL:
                    case INT_TEST_C_SFR_VARIABLE:
                    case INT_TEST_C_SFR_LITERAL_L:
                    case INT_TEST_C_SFR_VARIABLE_L:
                        fprintf(f, "if ");

                }
                fprintf(f, "SFR %d %s %s %s %d %d",IntCode[i].op, IntCode[i].name1, IntCode[i].name2, IntCode[i].name3, IntCode[i].literal, IntCode[i].literal2);
                switch(IntCode[i].op) {
                    case INT_TEST_SFR_LITERAL_L:
                    case INT_TEST_SFR_VARIABLE_L:

                    case INT_TEST_C_SFR_LITERAL:
                    case INT_TEST_C_SFR_VARIABLE:
                    case INT_TEST_C_SFR_LITERAL_L:
                    case INT_TEST_C_SFR_VARIABLE_L:
                        indent++;
                }
                break;
            // Special function
            #endif

            case INT_AllocKnownAddr:
                //fprintf(f, "AllocKnownAddr %s %s AddrOfRung%d;", IntCode[i].name1, IntCode[i].name2, IntCode[i].literal+1);
                break;

            case INT_AllocFwdAddr:
                //fprintf(f, "AllocFwdAddr AddrOfRung%d",IntCode[i].literal+1);
                break;

            case INT_FwdAddrIsNow:
                fprintf(f, "LabelRung%d: // %s", IntCode[i].literal+1, IntCode[i].name1);
                break;

            case INT_GOTO:
                if(IsNumber(IntCode[i].name1))
                    fprintf(f, "GOTO LabelRung%s; #LabelRung%d", IntCode[i].name1, IntCode[i].literal+1);
                else
                    fprintf(f, "GOTO %s; #LabelRung%d", IntCode[i].name1, IntCode[i].literal+1);
                break;

            case INT_GOSUB:
                if(IsNumber(IntCode[i].name1))
                    fprintf(f, "GOSUB LabelRung%s; #LabelRung%d", IntCode[i].name1, IntCode[i].literal+1);
                else
                    fprintf(f, "GOSUB %s; #LabelRung%d", IntCode[i].name1, IntCode[i].literal+1);
                break;

            case INT_RETURN:
                fprintf(f, "RETURN // %s", IntCode[i].name1);
                break;

            case INT_WRITE_STRING:
                fprintf(f, "sprintf(%s, \"%s\", %s);", IntCode[i].name1, IntCode[i].name2, IntCode[i].name3);
                break;

            #ifdef TABLE_IN_FLASH
            case INT_FLASH_INIT:
                fprintf(f, "INIT TABLE signed %d byte %s[%d] := {", IntCode[i].literal2, IntCode[i].name1, IntCode[i].literal);
                int j;
                for(j = 0; j < (IntCode[i].literal-1); j++) {
                  fprintf(f, "%d, ", IntCode[i].data[j]);
                }
                fprintf(f, "%d}", IntCode[i].data[IntCode[i].literal-1]);
                break;

            case INT_RAM_READ:
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f, "let var '%s' := '%s[%d]'", IntCode[i].name2, IntCode[i].name1, CheckMakeNumber(IntCode[i].name3));
                } else {
                    fprintf(f, "let var '%s' := '%s[%s]'", IntCode[i].name2, IntCode[i].name1, IntCode[i].name3);
                }
                break;

            case INT_FLASH_READ:
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f, "let var '%s' := %d # '%s[%s]'", IntCode[i].name1, IntCode[i].data[hobatoi(IntCode[i].name3)], IntCode[i].name2, IntCode[i].name3);
                } else {
                    fprintf(f, "let var '%s' := '%s[%s]'", IntCode[i].name1, IntCode[i].name2, IntCode[i].name3);
                }
                break;
            #endif

            default:
                ooops("INT_%d",IntCode[i].op);
        }
        if((int_comment_level == 1)
        ||( (IntCode[i].op != INT_SIMULATE_NODE_STATE)
          &&(IntCode[i].op != INT_AllocKnownAddr)
          &&(IntCode[i].op != INT_AllocFwdAddr) ) ) {
            //fprintf(f, " ## INT_%d",IntCode[i].op);
            fprintf(f, "\n");
        }
        fflush(f);
    }
    fclose(f);
}

//-----------------------------------------------------------------------------
// Convert a hex digit (0-9a-fA-F) to its hex value, or return -1 if the
// character is not a hex digit.
//-----------------------------------------------------------------------------
int HexDigit(int c)
{
    if((c >= '0') && (c <= '9')) {
        return c - '0';
    } else if((c >= 'a') && (c <= 'f')) {
        return 10 + (c - 'a');
    } else if((c >= 'A') && (c <= 'F')) {
        return 10 + (c - 'A');
    }
    return -1;
}

//-----------------------------------------------------------------------------
// Generate a unique symbol (unique with each call) having the given prefix
// guaranteed not to conflict with any user symbols.
//-----------------------------------------------------------------------------
static void GenSymParThis(char *dest)
{
    sprintf(dest, "$parThis_%01x", GenSymCountParThis);
    GenSymCountParThis++;
}
static void GenSymParOut(char *dest)
{
    sprintf(dest, "$parOut_%01x", GenSymCountParOut);
    GenSymCountParOut++;
}
void GenSymOneShot(char *dest, const char *name1, const char *name2)
{
    if(int_comment_level == 1)
        sprintf(dest, "$once_%01x", GenSymCountOneShot);
    else
        sprintf(dest, "$once_%01x_%s_%s", GenSymCountOneShot, name1, name2);
    GenSymCountOneShot++;
}
static void GenSymOneShot(char *dest)
{
    GenSymOneShot(dest, "", "");
}
static void GenSymFormattedString(char *dest, const char *name)
{
    sprintf(dest, "$fmtd_%01x_%s", GenSymCountFormattedString, name);
    GenSymCountFormattedString++;
}
static void GenSymFormattedString(char *dest)
{
    GenSymFormattedString(dest, "");
}
static void GenSymStepper(char *dest, char *name)
{
    sprintf(dest, "$step_%01x_%s", GenSymCountStepper, name);
    GenSymCountStepper++;
}

//-----------------------------------------------------------------------------
// Compile an instruction to the program.
//-----------------------------------------------------------------------------
static void _Op(int l, const char *f, const char *args, int op, BOOL *b,
                const char *name1, const char *name2, const char *name3, const char *name4, const char *name5, const char *name6,
                SDWORD lit, SDWORD lit2, SDWORD *data)
{
    memset(&IntCode[IntCodeLen], sizeof(IntCode[IntCodeLen]), 0);
    IntCode[IntCodeLen].op = op;
    if(name1) strcpy(IntCode[IntCodeLen].name1, name1);
    if(name2) strcpy(IntCode[IntCodeLen].name2, name2);
    if(name3) strcpy(IntCode[IntCodeLen].name3, name3);
    if(name4) strcpy(IntCode[IntCodeLen].name4, name4);
    if(name5) strcpy(IntCode[IntCodeLen].name5, name5);
    if(name6) strcpy(IntCode[IntCodeLen].name6, name6);
    IntCode[IntCodeLen].literal = lit;
    #ifdef NEW_CMP
    if((op==INT_IF_LES)
    || (op==INT_IF_VARIABLE_LES_LITERAL))
    if(!name2) {
        sprintf(IntCode[IntCodeLen].name2, "%d", lit);
    }
    #endif
    IntCode[IntCodeLen].literal2 = lit2;
    IntCode[IntCodeLen].data = data;
    IntCode[IntCodeLen].rung = rungNow;
    IntCode[IntCodeLen].which = whichNow;
    IntCode[IntCodeLen].leaf = leafNow;
    if(b)
    IntCode[IntCodeLen].poweredAfter = b;
    else
    IntCode[IntCodeLen].poweredAfter = &(leafNow->poweredAfter);
    IntCode[IntCodeLen].l = l;
    strcpy(IntCode[IntCodeLen].f, f);
    IntCodeLen++;
    if(IntCodeLen >= MAX_INT_OPS) {
        Error(_("Internal limit exceeded (MAX_INT_OPS)"));
        CompileError();
    }
}

static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, SDWORD lit)
{
    _Op(l, f, args, op, NULL, name1, name2, NULL, NULL, NULL, NULL, lit, 0, nullptr);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1, SDWORD lit)
{
    _Op(l, f, args, op, NULL, name1, NULL, NULL, NULL, NULL, NULL, lit, 0, NULL);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2)
{
    _Op(l, f, args, op, NULL, name1, name2, NULL, NULL, NULL, NULL, 0, 0, NULL);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1)
{
    _Op(l, f, args, op, NULL, name1, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL);
}
static void _Op(int l, const char *f, const char *args, int op, SDWORD lit)
{
    _Op(l, f, args, op, NULL, NULL, NULL, NULL, NULL, NULL, NULL, lit, 0, NULL);
}
static void _Op(int l, const char *f, const char *args, int op)
{
    _Op(l, f, args, op, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, NULL);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3, SDWORD lit)
{
    _Op(l, f, args, op, NULL, name1, name2, name3, NULL, NULL, NULL, lit, 0, NULL);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3)
{
    _Op(l, f, args, op, NULL, name1, name2, name3, NULL, NULL, NULL, 0, 0, NULL);
}
//
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3, SDWORD lit, SDWORD lit2)
{
    _Op(l, f, args, op, NULL, name1, name2, name3, NULL, NULL, NULL, lit, lit2, NULL);
}
//
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3, const char *name4)
{
    _Op(l, f, args, op, NULL, name1, name2, name3, name4, NULL, NULL, 0, 0, NULL);
}
//
static void _Op(int l, const char *f, const char *args, int op,
                const char *name1, const char *name2, const char *name3, const char *name4, const char *name5)
{
    _Op(l, f, args, op, NULL, name1, name2, name3, name4, name5, NULL, 0, 0, NULL);
}
//
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3,
                SDWORD lit, SDWORD lit2, SDWORD *data)
{
    _Op(l, f, args, op, NULL, name1, name2, name3, NULL, NULL, NULL, lit, lit2, data);
}

// And use macro for bugtracking
#define Op(...) _Op(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)
//-----------------------------------------------------------------------------
// Compile the instruction that the simulator uses to keep track of which
// nodes are energized (so that it can display which branches of the circuit
// are energized onscreen). The MCU code generators ignore this, of course.
//-----------------------------------------------------------------------------
static void SimState(BOOL *b, const char *name, BOOL *w, const char *name2)
{
    memset(&IntCode[IntCodeLen], sizeof(IntCode[IntCodeLen]), 0);
    IntCode[IntCodeLen].op = INT_SIMULATE_NODE_STATE;
    IntCode[IntCodeLen].poweredAfter = b;
    IntCode[IntCodeLen].workingNow = w;
    strcpy(IntCode[IntCodeLen].name1, name);
    if(name2) strcpy(IntCode[IntCodeLen].name2, name2);
    IntCode[IntCodeLen].rung = rungNow;
    IntCode[IntCodeLen].which = whichNow;
    IntCode[IntCodeLen].leaf = leafNow;
    IntCodeLen++;
    if(IntCodeLen >= MAX_INT_OPS) {
        Error(_("Internal limit exceeded (MAX_INT_OPS)"));
        CompileError();
    }
}

static void SimState(BOOL *b, const char *name)
{
    SimState(b, name, NULL, NULL);
}

//-----------------------------------------------------------------------------
// printf-like comment function
//-----------------------------------------------------------------------------
static void _Comment1(int l, const char *f, const char *str)
{
  if(int_comment_level) {
    char buf[MAX_NAME_LEN];
    strncpy(buf, str, MAX_NAME_LEN-1);
    buf[MAX_NAME_LEN-1] = '\0';
    // http://demin.ws/blog/russian/2013/01/28/use-snprintf-on-different-platforms/
    _Op(l, f, NULL, INT_COMMENT, buf);
  }
}
#define Comment1(str) _Comment1(__LINE__, __FILE__, str)

static void _Comment(int l, const char *f, const char *str, ...)
{
  if(int_comment_level) {
    va_list v;
    char buf[MAX_NAME_LEN];
    va_start(v, str);
    vsnprintf(buf, MAX_NAME_LEN, str, v);
    _Op(l, f, NULL, INT_COMMENT, buf);
  }
}

static void _Comment(int l, const char *f, int level, const char *str, ...)
{
  if(int_comment_level && (int_comment_level >= level)) {
    va_list v;
    char buf[MAX_NAME_LEN];
    va_start(v, str);
    vsnprintf(buf, MAX_NAME_LEN, str, v);
    _Op(l, f, NULL, INT_COMMENT, buf);
  }
}

#define Comment(...) _Comment(__LINE__, __FILE__, __VA_ARGS__)

//-----------------------------------------------------------------------------
SDWORD TestTimerPeriod(char *name, SDWORD delay, int adjust) // delay in us
{
    if(delay <= 0) {
        Error(L"%ls '%ls': %ls", _("Timer"), u16(name), _("Delay cannot be zero or negative."));
        return -1;
    }
    long long int period=0, adjPeriod=0, maxPeriod=0;
    period = delay / Prog.cycleTime; // - 1;
    adjPeriod = (delay + Prog.cycleTime * adjust) / Prog.cycleTime;

    int b = byteNeeded(period);
    if((SizeOfVar(name) != b) && (b<=4))
        SetSizeOfVar(name, b);
    maxPeriod = (long long int)(1) << (SizeOfVar(name)*8-1); maxPeriod--;

    if(period < 0) {
        Error(_("Delay cannot be zero or negative."));
    } else if(period <= 0)  {
        wchar_t s1[1024];
        swprintf_s(s1, L"%ls %ls", _("Timer period too short (needs faster cycle time)."), _("Or increase timer period."));
        wchar_t s2[1024];
        swprintf_s(s2, _("Timer '%ls'=%.3f ms."), u16(name), 1.0*delay/1000);
        wchar_t s3[1024];
        swprintf_s(s3, _("Minimum available timer period = PLC cycle time = %.3f ms."), 1.0*Prog.cycleTime/1000);
        const wchar_t *s4 = _("Not available");
        Error("%ls\n\r%ls %ls\r\n%ls", s1, s4, s2, s3);
    } else if(period+adjust <= 0) {
        Error("%ls '%ls': %ls", _("Timer"), u16(name), _("Total timer delay cannot be zero or negative. Increase the adjust value!"));
        // period = -1;
    } else if(period <= adjust) {
        Error("%ls '%ls': %ls", _("Timer"), u16(name), _("Adjusting the timer delay to a value greater than or near than the timer delay is meaningless. Decrease the adjust value!"));
    }

    if(((period > maxPeriod) || (adjPeriod > maxPeriod))
    && (Prog.mcu)
    && (Prog.mcu->portPrefix != 'L')) {
        wchar_t s1[1024];
        swprintf_s(s1, L"%ls %ls", _("Timer period too long; (use a slower cycle time)."), _("Or decrease timer period."));
        wchar_t s2[1024];
        swprintf_s(s2, _("Timer 'T%ls'=%10.0Lf s   needs %15lld PLC cycle times."), u16(name), 1.0*delay/1000, period);
        long double maxDelay = 1.0 * maxPeriod / 1000000 * Prog.cycleTime; // s
        wchar_t s3[1024];
        swprintf_s(s3, _("Timer 'T%ls'=%10.0Lf s can use %15lld PLC cycle times as the MAXIMUM possible value."), u16(name), maxDelay, maxPeriod);
        Error("%ls\r\n%ls\r\n%ls", s1, s2, s3);
        period = -1;
    }
    return (SDWORD)adjPeriod;
}
//-----------------------------------------------------------------------------
// Calculate the period in scan units from the period in microseconds, and
// raise an error if the given period is unachievable.
//-----------------------------------------------------------------------------
static SDWORD TimerPeriod(ElemLeaf *l)
{
    if(Prog.cycleTime <= 0) {
        Error(" PLC Cycle Time is '0'. TON, TOF, RTO, RTL, TCY timers does not work correctly!");
        return 1;
    }

    SDWORD period = TestTimerPeriod(l->d.timer.name, l->d.timer.delay, l->d.timer.adjust);
    if(period < 1) {
        CompileError();
    }
    return period;
}

//-----------------------------------------------------------------------------
SDWORD CalcDelayClock(long long clocks) // in us
{
    #if 1 // 1
    clocks = clocks * Prog.mcuClock / 1000000;
    if(Prog.mcu) {
        if(Prog.mcu->whichIsa == ISA_AVR) {
            ;
        } else if(Prog.mcu->whichIsa == ISA_PIC16) {
            clocks = clocks / 4;
        } else oops();
    }
    if(clocks <= 0 ) clocks = 1;
    #endif
    return (SDWORD)clocks;
}

//-----------------------------------------------------------------------------
// Is an expression that could be either a variable name or a number a number?
//-----------------------------------------------------------------------------
BOOL IsNumber(const char *str)
{
    while(isspace(*str))
        str++;
    if(isdigit(*str)) {
        return TRUE;
    } else if((*str == '-') || (*str == '+')) {
        str++;
        while(isspace(*str))
            str++;
        if(isdigit(*str))
                return TRUE;
            else
                return FALSE;
    } else if(*str == '\'') {
        // special case--literal single character
        if(strlen(str)>2) {
            if(str[strlen(str)-1] == '\'')
                return TRUE;
            else
                return FALSE;
        } else
            return FALSE;
    } else {
        return FALSE;
    }
}

//-----------------------------------------------------------------------------
BOOL CheckForNumber(char * str)
{
    if(IsNumber(str)) {
        int radix = 0; //auto detect
        char *start_ptr = str;
        while(isspace(*start_ptr) || *start_ptr == '-' || *start_ptr == '+')
            start_ptr++;

        if(*start_ptr == '\'')   {
            // special case--literal single character
            if(strlen(start_ptr)>2) {
                if(str[strlen(start_ptr)-1] == '\'')
                    return TRUE;
                else
                    return FALSE;
            } else
                return FALSE;
        }

        if(*start_ptr == '0') {
            if(toupper(start_ptr[1]) == 'B')
                radix = 2;
            else if(toupper(start_ptr[1]) == 'O')
                radix = 8;
        }

        char *end_ptr = NULL;
        // errno = 0;
        long val = strtol(str, &end_ptr, radix);
        if(*end_ptr) {
            return FALSE;
        }
        if((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
double hobatof(char *str)
{
    return atof(str);
}
//-----------------------------------------------------------------------------
int getradix(const char *str)
{
    int radix = 0;
    const char *start_ptr = str;
    while(isspace(*start_ptr) || *start_ptr == '-' || *start_ptr == '+')
        start_ptr++;
    if     ((start_ptr[0] != '0') && isdigit(start_ptr[0]))
        radix = 10;
    else if((start_ptr[0] == '0') && (strlen(start_ptr) == 1))
        radix = 10;
    else if((start_ptr[0] == '0') && isdigit(start_ptr[1]))
        radix = 8;
    else if((start_ptr[0] == '0') && (toupper(start_ptr[1]) == 'O'))
        radix = 8;
    else if((start_ptr[0] == '0') && (toupper(start_ptr[1]) == 'B'))
        radix = 2;
    else if((start_ptr[0] == '0') && (toupper(start_ptr[1]) == 'X'))
        radix = 16;
    else if(start_ptr[0] == '\'')
        radix = -1;
    if(!radix) {
        ooops("'%s'\r\n'%s'", str, start_ptr);
    }
    return radix;
}
//-----------------------------------------------------------------------------
SDWORD hobatoi(const char *str)
{
    char s[512];
    if(strstr(toupperstr(s, str), "0XFFFFFFFF"))
        return 0xFFFFFFFF;

    long val;
    const char *start_ptr = str;
    while(isspace(*start_ptr))
        start_ptr++;
    if(*start_ptr == '\'') {
        char dest[MAX_NAME_LEN];
        FrmStrToStr(dest, start_ptr);
        if( (strlen(dest) > 3) || (dest[0]!='\'') || (dest[2]!='\'') ) {
            Error(_("Expected single-character or one simple-escape-sequence in single-quotes: <%s>!"), u16(str));
        }
        val = dest[1];
    } else {
       while(isspace(*start_ptr) || *start_ptr == '-' || *start_ptr == '+') {
           start_ptr++;
       }
       int radix = 0; //auto detect
       if(*start_ptr == '0')
           {
               if(toupper(start_ptr[1]) == 'B')
                   radix = 2;
               else if(toupper(start_ptr[1]) == 'O')
                   radix = 8;
               else if(toupper(start_ptr[1]) == 'X')
                   radix = 16;
           }
       char *end_ptr = NULL;
       // errno = 0;
       val = strtol(str, &end_ptr, radix);
       if(*end_ptr) {
//         val = 0;
//         Error("Conversion error the\n'%s' string into number %d at\n'%s' position.", str, val, end_ptr);
       }
       if((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) {
//         val = 0;
//         Error("Conversion overflow error the string\n'%s' into number %d.", str, val);
       }
    }
    return val;
}

//-----------------------------------------------------------------------------
// Try to turn a string into a constant, and raise an error if
// something bad happens when we do so (e.g. out of range).
//-----------------------------------------------------------------------------
SDWORD CheckMakeNumber(const char *str)
{
    SDWORD val;
    val = hobatoi(str);
    CheckConstantInRange(val);
    return val;
}

//-----------------------------------------------------------------------------
// Return an integer power of ten.
//-----------------------------------------------------------------------------
int TenToThe(int x)
{
    int i;
    int r = 1;
    for(i = 0; i < x; i++) {
        r *= 10;
    }
    return r;
}

//-----------------------------------------------------------------------------
int xPowerY(int x, int y)
{
    int i;
    int r = 1;
    for(i = 0; i < y; i++) {
        r *= x;
    }
    return r;
}

//-----------------------------------------------------------------------------
static BOOL CheckStaySameElem(int which, void *elem)
{
    ElemLeaf *l = (ElemLeaf *)elem;

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            int i;
            for(i = 0; i < s->count; i++) {
                if(!CheckStaySameElem(s->contents[i].which, s->contents[i].data.any))
                    return FALSE;
            }
            return TRUE;
        }
        case ELEM_PARALLEL_SUBCKT: {
            int i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(i = 0; i < p->count; i++) {
                if(!CheckStaySameElem(p->contents[i].which, p->contents[i].data.any))
                     return FALSE;
            }
            return TRUE;
        }
        default:
            return StaySameElem(which);
    }
}

//-----------------------------------------------------------------------------
static BOOL CheckEndOfRungElem(int which, void *elem)
{
    ElemLeaf *l = (ElemLeaf *)elem;

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;

            return CheckEndOfRungElem(s->contents[s->count-1].which,s->contents[s->count-1].data.any);
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            int i;
            for(i = 0; i < p->count; i++) {
                if(CheckEndOfRungElem(p->contents[i].which, p->contents[i].data.any))
                    return TRUE;
            }
            return FALSE;
        }
        default:
            return EndOfRungElem(which);
    }
}

//-----------------------------------------------------------------------------
static BOOL CheckCanChangeOutputElem(int which, void *elem)
{
    ElemLeaf *l = (ElemLeaf *)elem;

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;

            int i;
            for(i = 0; i < s->count; i++) {
                if(CheckCanChangeOutputElem(s->contents[i].which, s->contents[i].data.any))
                    return TRUE;
            }
            return FALSE;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            int i;
            for(i = 0; i < p->count; i++) {
                if(CheckCanChangeOutputElem(p->contents[i].which, p->contents[i].data.any))
                    return TRUE;
            }
            return FALSE;
        }
        default:
            return CanChangeOutputElem(which);
    }
}

//-----------------------------------------------------------------------------
void OpSetVar(char *op1, char *op2)
{
    if(IsNumber(op2))
      Op(INT_SET_VARIABLE_TO_LITERAL, op1, (SDWORD)hobatoi(op2));
    else
      Op(INT_SET_VARIABLE_TO_VARIABLE, op1, op2);
}
//-----------------------------------------------------------------------------
static void InitVarsCircuit(int which, void *elem, int *n)
{
    ElemLeaf *l = (ElemLeaf *)elem;
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            int i;
            for(i = 0; i < s->count; i++)
                InitVarsCircuit(s->contents[i].which, s->contents[i].data.any, n);
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            int i;
            for(i = 0; i < p->count; i++)
                InitVarsCircuit(p->contents[i].which, p->contents[i].data.any, n);
            break;
        }
        #ifndef NEW_TON
        case ELEM_TOF: {
            if(n)
                (*n)++; // counting the number of variables
            else {
                SDWORD period = TimerPeriod(l);
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            }
            break;
        }
        #endif
        #ifdef NEW_TON
        case ELEM_TCY: {
            if(n)
                (*n)++; // counting the number of variables
            else {
                SDWORD period = TimerPeriod(l) - 1;
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            }
            break;
        }
        case ELEM_TON: {
            if(n)
                (*n)++; // counting the number of variables
            else {
                SDWORD period = TimerPeriod(l);
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            }
            break;
        }
        #endif
        case ELEM_SEED_RANDOM: {
            char name[MAX_NAME_LEN];
            sprintf(name, "$seed_%s", l->d.readAdc.name);
            if(n)
                (*n)++; // counting the number of variables
            else {
                Op(INT_SET_VARIABLE_TO_LITERAL, name, 1);
            }
            break;
        }
        case ELEM_CTR:
        case ELEM_CTC:
        case ELEM_CTU:
        case ELEM_CTD: {
            if(IsNumber(l->d.counter.init) || IsNumber(l->d.counter.max)) {
                int init = CheckMakeNumber(l->d.counter.init);
                int max = CheckMakeNumber(l->d.counter.max);
                int b = max(byteNeeded(init), byteNeeded(max));
                if(SizeOfVar(l->d.counter.name) != b)
                    SetSizeOfVar(l->d.counter.name, b);
                //if(init != 0) { // need reinit if CTD(c1), CTU(c1)
                    if(n)
                        (*n)++; // counting the number of variables
                    else {
                        Op(INT_SET_VARIABLE_TO_LITERAL, l->d.counter.name, init);
                    }
                //}
            }
            break;
        }
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
static void InitVars(void)
{
    int n = 0;
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        InitVarsCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i], &n);
    }
    if(n) {
      Comment("INIT VARS");
      char storeInit[MAX_NAME_LEN];
      GenSymOneShot(storeInit, "INIT", "VARS");
      Op(INT_IF_BIT_CLEAR, storeInit);
        Op(INT_SET_BIT, storeInit);
        for(i = 0; i < Prog.numRungs; i++) {
            rungNow = i;
            InitVarsCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i], NULL);
        }
      Op(INT_END_IF);
    }
}

#ifdef TABLE_IN_FLASH
//-----------------------------------------------------------------------------
static void InitTablesCircuit(int which, void *elem)
{
    int sovElement = 0;
    ElemLeaf *l = (ElemLeaf *)elem;
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            int i;
            for(i = 0; i < s->count; i++)
                InitTablesCircuit(s->contents[i].which, s->contents[i].data.any);
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            int i;
            for(i = 0; i < p->count; i++)
                InitTablesCircuit(p->contents[i].which, p->contents[i].data.any);
            break;
        }
        case ELEM_LOOK_UP_TABLE:
        case ELEM_PIECEWISE_LINEAR: {
            ElemLookUpTable *t = &(l->d.lookUpTable);

            char nameTable[MAX_NAME_LEN];
            sprintf(nameTable, "%s%s", t->name,""); // "LutElement");

            int sovElement;

            if((isVarInited(nameTable) < 0)
            || (isVarInited(nameTable)==rungNow)) {
                sovElement =  TestByteNeeded(t->count, t->vals);
                Op(INT_FLASH_INIT, nameTable, NULL, NULL, t->count, sovElement, t->vals);
            } else {
                sovElement = SizeOfVar(nameTable);
                if(sovElement < 1)
                    sovElement = 1;
                Comment(to_utf8(_("INIT TABLE: signed %d bit %s[%d] see above")).c_str(), 8*sovElement, nameTable);
            }
            break;
        }
        {
        const char *nameTable;
        case ELEM_7SEG:  nameTable = "char7seg";  goto xseg;
        case ELEM_9SEG:  nameTable = "char9seg";  goto xseg;
        case ELEM_14SEG: nameTable = "char14seg"; goto xseg;
        case ELEM_16SEG: nameTable = "char16seg"; goto xseg;
        xseg:
            break;
        }
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
static void InitTables(void)
{
    if(TablesUsed()) {
        Comment("INIT TABLES");
        int i;
        for(i = 0; i < Prog.numRungs; i++) {
            rungNow = i;
            InitTablesCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
        }
    }
}
#endif
//-----------------------------------------------------------------------------
// Compile code to evaluate the given bit of ladder logic. The rung input
// state is in stateInOut before calling and will be in stateInOut after
// calling.
//-----------------------------------------------------------------------------
static const char *VarFromExpr(const char *expr, const char *tempName)
{
    if(IsNumber(expr)) {
        Op(INT_SET_VARIABLE_TO_LITERAL, tempName, CheckMakeNumber(expr));
        return tempName;
    } else {
        return expr;
    }
}
#define PULSE \
    Op(INT_SET_BIT, l->d.stepper.coil); \
    Op(INT_CLEAR_BIT, l->d.stepper.coil);

//-----------------------------------------------------------------------------
static void IntCodeFromCircuit(int which, void *any, const char *stateInOut, int rung)
{
    const char *stateInOut2 = "$overlap";
    whichNow = which;
    leafNow = (ElemLeaf *)any;
    ElemLeaf *l = (ElemLeaf *)any;
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            int i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;

            Comment("start series [");
            for(i = 0; i < s->count; i++) {
                IntCodeFromCircuit(s->contents[i].which, s->contents[i].data.any,
                    stateInOut, rung);
            }
            Comment("] finish series");
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            char parThis[MAX_NAME_LEN];
            char parOut[MAX_NAME_LEN];

            Comment("start parallel [");
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;

            int i;
            BOOL ExistEnd = FALSE; //FALSE indicates that it is NEED to calculate the parOut
            for(i = 0; i < p->count; i++) {
              if(CheckEndOfRungElem(p->contents[i].which, p->contents[i].data.any)) {
                ExistEnd = TRUE; // TRUE indicates that it is NOT NEED to calculate the parOut
                break;
              }
            }
            BOOL CanChange = FALSE; // FALSE indicates that it is NOT NEED to calculate the parThis
            for(i = 0; i < p->count; i++) {
              if(!CheckStaySameElem(p->contents[i].which, p->contents[i].data.any)) {
                CanChange = TRUE; // TRUE indicates that it is NEED to calculate the parThis
                break;
              }
            }

            #ifdef DEFAULT_PARALLEL_ALGORITHM
            // Return to default ELEM_PARALLEL_SUBCKT algorithm
            CanChange = TRUE;
            ExistEnd = FALSE;
            #endif

            if(ExistEnd == FALSE) {
              GenSymParOut(parOut);

              Op(INT_CLEAR_BIT, parOut);
            }
            if(CanChange) {
              GenSymParThis(parThis);
            }
            for(i = 0; i < p->count; i++) {
              #ifndef DEFAULT_PARALLEL_ALGORITHM
              if(CheckStaySameElem(p->contents[i].which, p->contents[i].data.any))
                IntCodeFromCircuit(p->contents[i].which, p->contents[i].data.any,
                    stateInOut, rung);
              else
              #endif
              {
                Op(INT_COPY_BIT_TO_BIT, parThis, stateInOut);

                IntCodeFromCircuit(p->contents[i].which, p->contents[i].data.any,
                    parThis, rung);

                if(ExistEnd == FALSE) {
                  Op(INT_IF_BIT_SET, parThis);
                    Op(INT_SET_BIT, parOut);
                  Op(INT_END_IF);
                }
              }
            }
            if(ExistEnd == FALSE) {
              Op(INT_COPY_BIT_TO_BIT, stateInOut, parOut);
            }
            Comment("] finish parallel");

            break;
        }
        case ELEM_CONTACTS: {
            Comment(3, "ELEM_CONTACTS");
            if(l->d.contacts.negated) {
              Op(INT_IF_BIT_SET, l->d.contacts.name, l->d.contacts.set1);
            } else {
              Op(INT_IF_BIT_CLEAR, l->d.contacts.name, l->d.contacts.set1);
            }
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);
            break;
        }
        case ELEM_COIL: {
            Comment(3, "ELEM_COIL");
            #ifdef DEFAULT_COIL_ALGORITHM
            if(l->d.coil.negated) {
                Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_CLEAR_BIT, l->d.coil.name);
                Op(INT_ELSE);
                Op(INT_SET_BIT, l->d.coil.name);
                Op(INT_END_IF);
            } else if(l->d.coil.setOnly) {
                Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SET_BIT, l->d.coil.name);
                Op(INT_END_IF);
            } else if(l->d.coil.resetOnly) {
                Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_CLEAR_BIT, l->d.coil.name);
                Op(INT_END_IF);
            } else {
                Op(INT_COPY_BIT_TO_BIT, l->d.coil.name, stateInOut);
            }
            #else
              //Load SAMPLE\coil_s_r_n.ld into LDmicto.exe.
              //Switch Xnew1, Xnew2, Xnew3 and see end of rungs.
              // variant 0 display a state of stateInOut and not a ELEM_COIL. Not good.
              // variant 1 display a state of a ELEM_COIL. So so.
              // variant 2 is redundant in hex code. Better then variant 0.
              // variant 3 is equivalent variant 2. Best.
              if(l->d.coil.negated) {
                  Op(INT_IF_BIT_SET, stateInOut);
                    Op(INT_CLEAR_BIT, l->d.coil.name);
                  Op(INT_ELSE);
                    Op(INT_SET_BIT, l->d.coil.name);
                  Op(INT_END_IF);
              } else if(l->d.coil.setOnly) {
                  Op(INT_IF_BIT_SET, stateInOut);
                    Op(INT_SET_BIT, l->d.coil.name);
                  Op(INT_END_IF);
              } else if(l->d.coil.resetOnly) {
                  Op(INT_IF_BIT_SET, stateInOut);
                    Op(INT_CLEAR_BIT, l->d.coil.name);
                  Op(INT_END_IF);
              } else if(l->d.coil.ttrigger) {
                  char storeName[MAX_NAME_LEN];
                  GenSymOneShot(storeName, "TTRIGGER", l->d.coil.name);
                  Op(INT_IF_BIT_SET, stateInOut);
                    Op(INT_IF_BIT_CLEAR, storeName);
                      Op(INT_SET_BIT, storeName);
                      Op(INT_IF_BIT_SET, l->d.coil.name);
                        Op(INT_CLEAR_BIT, l->d.coil.name);
                      Op(INT_ELSE);
                        Op(INT_SET_BIT, l->d.coil.name);
                      Op(INT_END_IF);
                    Op(INT_END_IF);
                  Op(INT_ELSE);
                    Op(INT_CLEAR_BIT, storeName);
                  Op(INT_END_IF);
              } else {
                  Op(INT_COPY_BIT_TO_BIT, l->d.coil.name, stateInOut);
              }
              SimState(&(l->poweredAfter), l->d.coil.name, &(l->workingNow), l->d.coil.name); // variant 6
            #endif
          break;
        }
        //-------------------------------------------------------------------
        case ELEM_RTL: {
            Comment(3, "ELEM_RTL");
            SDWORD period = TimerPeriod(l);

            Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);

              Op(INT_IF_BIT_CLEAR, stateInOut);
                Op(INT_INCREMENT_VARIABLE, l->d.timer.name);
              Op(INT_END_IF);
              Op(INT_CLEAR_BIT, stateInOut);

            Op(INT_ELSE);

              Op(INT_SET_BIT, stateInOut);

            Op(INT_END_IF);

            break;
        }
        case ELEM_RTO: {
            Comment(3, "ELEM_RTO");
            SDWORD period = TimerPeriod(l);

            Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);

              Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_INCREMENT_VARIABLE, l->d.timer.name);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);

            Op(INT_ELSE);

              Op(INT_SET_BIT, stateInOut);

            Op(INT_END_IF);

            break;
        }
        case ELEM_RES:
            Comment(3, "ELEM_RES");
            Op(INT_IF_BIT_SET, stateInOut);
              if(l->d.reset.name[0]=='P') {
                  Op(INT_PWM_OFF, l->d.reset.name);
                  char s[MAX_NAME_LEN];
                  sprintf(s, "$%s", l->d.reset.name);
                  Op(INT_CLEAR_BIT, s);
              } else
                  Op(INT_SET_VARIABLE_TO_LITERAL, l->d.reset.name, (SDWORD)0);
            Op(INT_END_IF);
            break;

        case ELEM_TIME2COUNT: {
            Comment(3, "ELEM_TIME2COUNT");
            SDWORD period = TimerPeriod(l);
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            Op(INT_END_IF);
            break;
        }
        case ELEM_TCY: {
            SDWORD period = TimerPeriod(l) - 1;
            Comment(3, "ELEM_TCY %s %d ms %d(0x%X)", l->d.timer.name, l->d.timer.delay, period, period);
/*
              logic
              level
                   ^  The duration of the input pulse is longer than 1 s
           TCY     |     ______________________
           input   | ___/                      \_______
                   |    |                      |
                   |    |  1s    1s    1s      |
                   |    |<--->|<--->|<--->|    |
                        |     |     |     |    v
           TCY     |    v   __|   __|   __|   _
           output  | ______/  \__/  \__/  \__/ \_______
                 --+----------------------------------------> time,s
                   |
*/
            char store[MAX_NAME_LEN];
            GenSymOneShot(store, "TCY", l->d.timer.name);

            #ifndef NEW_TON
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);
                Op(INT_INCREMENT_VARIABLE, l->d.timer.name);
              Op(INT_ELSE);
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, (SDWORD)0);
                Op(INT_IF_BIT_CLEAR, store);
                  Op(INT_SET_BIT, store);
                Op(INT_ELSE);
                  Op(INT_CLEAR_BIT, store);
                Op(INT_END_IF);
              Op(INT_END_IF);
              Op(INT_IF_BIT_CLEAR, store);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, (SDWORD)0);
            Op(INT_END_IF);
            #else
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_DECREMENT_VARIABLE, l->d.timer.name, stateInOut);
              Op(INT_IF_BIT_SET, stateInOut); // overlap(0 to -1) flag is TRUE
                Op(INT_IF_BIT_CLEAR, store);
                  Op(INT_SET_BIT, store);
                Op(INT_ELSE);
                  Op(INT_CLEAR_BIT, store);
                Op(INT_END_IF);
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
              Op(INT_END_IF);
              Op(INT_COPY_BIT_TO_BIT, stateInOut, store);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, store);
              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            Op(INT_END_IF);
            #endif
            break;
        }
        case ELEM_TON: {
            Comment(3, "ELEM_TON");
            SDWORD period = TimerPeriod(l);
/*
              logic
              level
                   ^  The duration of the input pulse
                   |  must be longer than 1s
           TON     |     _________
           input   | ___/         \_______
                   |    |         |
                   |    | 1s      |
                   |    |<-->|    |
                   |         |    |
                   |         v    v
           TON     |          ____
           output  | ________/    \_______
                 --+---------------------------> time,s
                   |
*/
            #ifndef NEW_TON
            Op(INT_IF_BIT_SET, stateInOut);

              Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);
                Op(INT_CLEAR_BIT, stateInOut); //1
                Op(INT_INCREMENT_VARIABLE, l->d.timer.name); //2
              Op(INT_END_IF);

            Op(INT_ELSE);

              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, (SDWORD)0);

            Op(INT_END_IF);
            #else
            char store[MAX_NAME_LEN];
            GenSymOneShot(store, "TON", l->d.timer.name);

            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_IF_BIT_CLEAR, store);
                Op(INT_DECREMENT_VARIABLE, l->d.timer.name, stateInOut);
                Op(INT_IF_BIT_SET, stateInOut); // overlap(0 to -1) flag is TRUE
                  Op(INT_SET_BIT, store);
                  Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
                Op(INT_END_IF);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, store);
            Op(INT_END_IF);
            #endif
            break;
        }
        case ELEM_TOF: {
            Comment(3, "ELEM_TOF");
            SDWORD period = TimerPeriod(l);
/*
              logic
              level
                   ^  The duration of the input pulse must be
                   |     longer than the PLC cycle
           TOF     |        _
           input   | ______/ \_________
                   |       | |
                   |       | | 1s
                   |       | |<-->|
                   |       |      |
                   |       v      v
           TOF     |        ______
           output  | ______/      \____
                 --+----------------------> time,s
                   |
*/
            #ifndef NEW_TON
            /*
            // All variables start at zero by default, so by default the
            // TOF timer would start out with its output forced HIGH, until
            // it finishes counting up. This does not seem to be what
            // people expect, so add a special case to fix that up.
            char antiGlitchName[MAX_NAME_LEN];
            sprintf(antiGlitchName, "$%s_antiglitch", l->d.timer.name);
            Op(INT_IF_BIT_CLEAR, antiGlitchName);
              Op(INT_SET_BIT, antiGlitchName);
              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            Op(INT_END_IF);
            /**/
            Op(INT_IF_BIT_CLEAR, stateInOut);

              Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);
                Op(INT_INCREMENT_VARIABLE, l->d.timer.name);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_END_IF);

            Op(INT_ELSE);

              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, (SDWORD)0);

            Op(INT_END_IF);
            #else
            char store[MAX_NAME_LEN];
            GenSymOneShot(store, "TOF", l->d.timer.name);

            Op(INT_IF_BIT_CLEAR, stateInOut);
              Op(INT_IF_BIT_CLEAR, store);
                Op(INT_DECREMENT_VARIABLE, l->d.timer.name, store);
              Op(INT_END_IF);
              Op(INT_IF_BIT_SET, store); // overlap(0 to -1) flag is TRUE
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
              Op(INT_ELSE);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, store);
            Op(INT_END_IF);
            #endif
            break;
        }
        case ELEM_THI: {
            Comment(3, "ELEM_THI");
            SDWORD period = TimerPeriod(l);
/*
              logic
              level
                   ^  The duration of the input pulse must be
                   |     longer than the PLC cycle
           THI     |     _           ________
           input   | ___/ \_________/        \_______
                   |    |           |
                   |    | 1s        | 1s
                   |    |<-->|      |<-->|
                   |    |    |      |    |
                   |    v    v      v    v
           THI     |     ____        ____
           output  | ___/    \______/    \___________
                 --+------------------------------------> time,s
                   |
*/
            char store[MAX_NAME_LEN];
            GenSymOneShot(store, "THI", l->d.timer.name);

            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_IF_BIT_CLEAR, store);
                Op(INT_SET_BIT, store);
              Op(INT_END_IF);
            Op(INT_END_IF);

            Op(INT_IF_BIT_SET, store);
              Op(INT_IF_LES, l->d.timer.name, period);
                Op(INT_INCREMENT_VARIABLE, l->d.timer.name);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_ELSE);
                Op(INT_IF_BIT_CLEAR, stateInOut);
                  Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, (SDWORD)0);
                  Op(INT_CLEAR_BIT, store);
                Op(INT_END_IF);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_TLO: {
            Comment(3, "ELEM_TLO");
            SDWORD period = TimerPeriod(l);
/*
              logic
              level
                   ^  The duration of the input pulse must be
                   |     longer than the PLC cycle
           TLO     | ___   _________          _______
           input   |    \_/         \________/
                   |    |           |
                   |    | 1s        | 1s
                   |    |<-->|      |<-->|
                   |    |    |      |    |
                   |    v    v      v    v
           TLO     | ___      ______      ___________
           output  |    \____/      \____/
                 --+------------------------------------> time,s
                   |
*/
            char store[MAX_NAME_LEN];
            GenSymOneShot(store, "TLO", l->d.timer.name);
            char storeNameHi[MAX_NAME_LEN];
            GenSymOneShot(storeNameHi, "ONE_SHOT_HI", "");

            Op(INT_IF_BIT_CLEAR, stateInOut);
              Op(INT_IF_BIT_SET, storeNameHi);
                Op(INT_IF_BIT_CLEAR, store);
                  Op(INT_SET_BIT, store);
                Op(INT_END_IF);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_SET_BIT, storeNameHi);
            Op(INT_END_IF);

            Op(INT_IF_BIT_SET, store);
              Op(INT_IF_LES, l->d.timer.name, period);
                Op(INT_INCREMENT_VARIABLE, l->d.timer.name);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_ELSE);
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, (SDWORD)0);
                  Op(INT_CLEAR_BIT, store);
                Op(INT_END_IF);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_IF_BIT_SET, storeNameHi);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_END_IF);
            Op(INT_END_IF);
            break;
        }
        //-------------------------------------------------------------------
        case ELEM_CTU: {
            Comment(3, "ELEM_CTU");
            if(IsNumber(l->d.counter.max))
                CheckVarInRange(l->d.counter.name, l->d.counter.max, CheckMakeNumber(l->d.counter.max));
            char storeInit[MAX_NAME_LEN];
            if(IsNumber(l->d.counter.init)) {
                CheckVarInRange(l->d.counter.name, l->d.counter.init, CheckMakeNumber(l->d.counter.init));
                //inited in InitVar()
            } else {
                GenSymOneShot(storeInit, "CTU_INIT", l->d.counter.name);
                Op(INT_IF_BIT_CLEAR, storeInit);
                  Op(INT_SET_BIT, storeInit);
                  Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.counter.name, l->d.counter.init);
                Op(INT_END_IF);
            }
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "CTU", l->d.counter.name);
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_IF_BIT_CLEAR, storeName);
                Op(INT_SET_BIT, storeName);
                if(IsNumber(l->d.counter.max)) {
                  Op(INT_IF_VARIABLE_LES_LITERAL, l->d.counter.name,
                      CheckMakeNumber(l->d.counter.max));
                } else {
                  Op(INT_IF_VARIABLE_GRT_VARIABLE, l->d.counter.max, l->d.counter.name);
                }
                    Op(INT_INCREMENT_VARIABLE, l->d.counter.name);
                  Op(INT_END_IF);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, storeName);
            Op(INT_END_IF);

            if(IsNumber(l->d.counter.max)) {
              Op(INT_IF_VARIABLE_LES_LITERAL, l->d.counter.name,
                  CheckMakeNumber(l->d.counter.max));
            } else {
              Op(INT_IF_VARIABLE_GRT_VARIABLE, l->d.counter.max, l->d.counter.name);
            }
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_ELSE);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_END_IF);
            break;
        }
        case ELEM_CTD: {
            Comment(3, "ELEM_CTD");
            if(IsNumber(l->d.counter.max))
                CheckVarInRange(l->d.counter.name, l->d.counter.max, CheckMakeNumber(l->d.counter.max));
            char storeInit[MAX_NAME_LEN];
            if(IsNumber(l->d.counter.init)) {
                CheckVarInRange(l->d.counter.name, l->d.counter.init, CheckMakeNumber(l->d.counter.init));
                //inited in InitVar()
            } else {
                GenSymOneShot(storeInit, "CTD_INIT", l->d.counter.name);
                Op(INT_IF_BIT_CLEAR, storeInit);
                  Op(INT_SET_BIT, storeInit);
                  Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.counter.name, l->d.counter.init);
                Op(INT_END_IF);
            }
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "CTD", l->d.counter.name);

            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_IF_BIT_CLEAR, storeName);
                    Op(INT_SET_BIT, storeName);
                    Op(INT_DECREMENT_VARIABLE, l->d.counter.name);
                Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, storeName);
            Op(INT_END_IF);

            //dbp("%s %s", l->d.counter.name, l->d.counter.max);
            if(IsNumber(l->d.counter.max)) {
              Op(INT_IF_VARIABLE_LES_LITERAL, l->d.counter.name, CheckMakeNumber(l->d.counter.max) + 1);
                // the transition 1-> 0 will be at a given limit
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_ELSE);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_END_IF);
            } else {
              Op(INT_IF_VARIABLE_GRT_VARIABLE, l->d.counter.name, l->d.counter.max);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_ELSE);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);
            }
            break;
        }

        case ELEM_CTR: {
            Comment(3, "ELEM_CTR");
            if(IsNumber(l->d.counter.max))
                CheckVarInRange(l->d.counter.name, l->d.counter.max, CheckMakeNumber(l->d.counter.max));
            char storeInit[MAX_NAME_LEN];
            if(IsNumber(l->d.counter.init)) {
                CheckVarInRange(l->d.counter.name, l->d.counter.init, CheckMakeNumber(l->d.counter.init));
                //inited in InitVar()
            } else {
                GenSymOneShot(storeInit, "CTR_INIT", l->d.counter.name);
                Op(INT_IF_BIT_CLEAR, storeInit);
                  Op(INT_SET_BIT, storeInit);
                  Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.counter.name, l->d.counter.init);
                Op(INT_END_IF);
            }
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "CTR", l->d.counter.name);

            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_CLEAR_BIT, stateInOut);

              Op(INT_IF_BIT_CLEAR, storeName);
                Op(INT_SET_BIT, storeName);
                Op(INT_DECREMENT_VARIABLE, l->d.counter.name);

              //Use max as min, and init as max
              // -5 --> -10
              // ^init  ^max
              if(IsNumber(l->d.counter.max)) {
                Op(INT_IF_VARIABLE_LES_LITERAL, l->d.counter.name, CheckMakeNumber(l->d.counter.max));
              } else {
                Op(INT_IF_VARIABLE_GRT_VARIABLE, l->d.counter.max, l->d.counter.name);
              }
                  if(IsNumber(l->d.counter.init)) {
                    Op(INT_SET_VARIABLE_TO_LITERAL, l->d.counter.name, CheckMakeNumber(l->d.counter.init));
                  } else {
                    Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.counter.name, l->d.counter.init);
                  }
                  Op(INT_SET_BIT, stateInOut); // overload impulse
                Op(INT_END_IF);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, storeName);
            Op(INT_END_IF);
            break;
        }

        case ELEM_CTC: {
            Comment(3, "ELEM_CTC");
            if(IsNumber(l->d.counter.max))
                CheckVarInRange(l->d.counter.name, l->d.counter.max, CheckMakeNumber(l->d.counter.max));
            char storeInit[MAX_NAME_LEN];
            if(IsNumber(l->d.counter.init)) {
                CheckVarInRange(l->d.counter.name, l->d.counter.init, CheckMakeNumber(l->d.counter.init));
                //inited in InitVar()
            } else {
                GenSymOneShot(storeInit, "CTC_INIT", l->d.counter.name);
                Op(INT_IF_BIT_CLEAR, storeInit);
                  Op(INT_SET_BIT, storeInit);
                  Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.counter.name, l->d.counter.init);
                Op(INT_END_IF);
            }
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "CTC", l->d.counter.name);

            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_IF_BIT_CLEAR, storeName);
                Op(INT_SET_BIT, storeName); // This line1
                Op(INT_INCREMENT_VARIABLE, l->d.counter.name);

              if(IsNumber(l->d.counter.max)) {
                Op(INT_IF_VARIABLE_LES_LITERAL, l->d.counter.name, CheckMakeNumber(l->d.counter.max)+1);
                Op(INT_ELSE);
              } else {
                Op(INT_IF_VARIABLE_GRT_VARIABLE, l->d.counter.name, l->d.counter.max);
              }
                  if(IsNumber(l->d.counter.init)) {
                    Op(INT_SET_VARIABLE_TO_LITERAL, l->d.counter.name, CheckMakeNumber(l->d.counter.init));
                  } else {
                    Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.counter.name, l->d.counter.init);
                  }
                  Op(INT_SET_BIT, stateInOut); // overload impulse
                Op(INT_END_IF);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, storeName); // This line2
            Op(INT_END_IF);
        ////Op(INT_COPY_BIT_TO_BIT, storeName, stateInOut); // This line3 equivalently line1 + line2

            break;
        }
        #ifdef USE_SFR
        // Special Function
        case ELEM_RSFR:
            Comment(3, "ELEM_RSFR");
            if(IsNumber(l->d.move.dest)) {
                Error(_("Read SFR instruction: '%s' not a valid destination."),
                    l->d.move.dest);
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.src)) {
                if(!IsNumber(l->d.move.dest)) {
                    CheckVarInRange(l->d.move.dest, l->d.move.src, CheckMakeNumber(l->d.move.src));
                }
                Op(INT_READ_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
            } else {
                Op(INT_READ_SFR_VARIABLE, l->d.move.src, l->d.move.dest);
            }
            Op(INT_END_IF);
            break;
        case ELEM_WSFR:
            Comment(3, "ELEM_WSFR");
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.sfr.op)) {
                if(IsNumber(l->d.sfr.sfr)) {
                    Op(INT_WRITE_SFR_LITERAL_L,NULL,NULL,NULL, CheckMakeNumber(l->d.sfr.sfr), CheckMakeNumber(l->d.sfr.op));
                } else {
                    Op(INT_WRITE_SFR_VARIABLE_L,l->d.sfr.sfr, CheckMakeNumber(l->d.sfr.op));
                }
            }
            else {
                if(IsNumber(l->d.sfr.sfr)) {
                    CheckVarInRange(l->d.sfr.op, l->d.sfr.sfr, CheckMakeNumber(l->d.sfr.sfr));
                    Op(INT_WRITE_SFR_LITERAL, l->d.sfr.op, CheckMakeNumber(l->d.sfr.sfr));
                } else {
                    Op(INT_WRITE_SFR_VARIABLE, l->d.sfr.sfr, l->d.sfr.op);
                }
            }
            Op(INT_END_IF);
            break;
        case ELEM_SSFR:
            Comment(3, "ELEM_SSFR");
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.dest)) {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_SET_SFR_LITERAL_L,NULL,NULL,NULL, CheckMakeNumber(l->d.move.src), CheckMakeNumber(l->d.move.dest));
                } else {
                    Op(INT_SET_SFR_VARIABLE_L,l->d.move.src, CheckMakeNumber(l->d.move.dest));
                }
            }
            else {
                if(IsNumber(l->d.move.src)) {
                    CheckVarInRange(l->d.move.dest, l->d.move.src, CheckMakeNumber(l->d.move.src));
                    Op(INT_SET_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
                } else {
                    Op(INT_SET_SFR_VARIABLE, l->d.move.src, l->d.move.dest);
                }
            }
            Op(INT_END_IF);
            break;
        case ELEM_CSFR:
            Comment(3, "ELEM_CSFR");
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.dest)) {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_CLEAR_SFR_LITERAL_L,NULL,NULL,NULL, CheckMakeNumber(l->d.move.src), CheckMakeNumber(l->d.move.dest));
                } else {
                    Op(INT_CLEAR_SFR_VARIABLE_L,l->d.move.src, CheckMakeNumber(l->d.move.dest));
                }
            }
            else {
                if(IsNumber(l->d.move.src)) {
                    CheckVarInRange(l->d.move.dest, l->d.move.src, CheckMakeNumber(l->d.move.src));
                    Op(INT_CLEAR_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
                } else {
                    Op(INT_CLEAR_SFR_VARIABLE, l->d.move.src, l->d.move.dest);
                }
            }
            Op(INT_END_IF);
            break;
        case ELEM_TSFR: {
            Comment(3, "ELEM_TSFR");
            if(IsNumber(l->d.move.dest)) {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_TEST_SFR_LITERAL_L,NULL,NULL,NULL, CheckMakeNumber(l->d.move.src), CheckMakeNumber(l->d.move.dest));
                } else {
                    Op(INT_TEST_SFR_VARIABLE_L,l->d.move.src, CheckMakeNumber(l->d.move.dest));
                }
            }
            else {
                if(IsNumber(l->d.move.src)) {
                    CheckVarInRange(l->d.move.dest, l->d.move.src, CheckMakeNumber(l->d.move.src));
                    Op(INT_TEST_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
                } else {
                    Op(INT_TEST_SFR_VARIABLE, l->d.move.src, l->d.move.dest);
                }
            }
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_T_C_SFR: {
            Comment(3, "ELEM_T_C_SFR");
            if(IsNumber(l->d.move.dest)) {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_TEST_C_SFR_LITERAL_L,NULL,NULL,NULL, CheckMakeNumber(l->d.move.src), CheckMakeNumber(l->d.move.dest));
                } else {
                    Op(INT_TEST_C_SFR_VARIABLE_L,l->d.move.src, CheckMakeNumber(l->d.move.dest));
                }
            }
            else {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_TEST_C_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
                } else {
                    CheckVarInRange(l->d.move.dest, l->d.move.src, CheckMakeNumber(l->d.move.src));
                    Op(INT_TEST_C_SFR_VARIABLE, l->d.move.src, l->d.move.dest);
                }
            }
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        // Special function
        #endif

        #ifdef NEW_CMP
        {
        int intOp;
        case ELEM_GRT: intOp=INT_IF_LEQ; Comment(3, "ELEM_GRT"); goto cmp;
        case ELEM_GEQ: intOp=INT_IF_LES; Comment(3, "ELEM_GEQ"); goto cmp;
        case ELEM_LES: intOp=INT_IF_GEQ; Comment(3, "ELEM_LES"); goto cmp;
        case ELEM_LEQ: intOp=INT_IF_GRT; Comment(3, "ELEM_LEQ"); goto cmp;
        case ELEM_NEQ: intOp=INT_IF_EQU; Comment(3, "ELEM_NEQ"); goto cmp;
        case ELEM_EQU: intOp=INT_IF_NEQ; Comment(3, "ELEM_EQU"); goto cmp;
        cmp: {
            Op(INT_IF_BIT_SET, stateInOut);
                Op(intOp, l->d.cmp.op1, l->d.cmp.op2);
                    Op(INT_CLEAR_BIT, stateInOut);
                Op(INT_END_IF);
            Op(INT_END_IF);
            break;
            }
        }
        #else
        case ELEM_GRT: Comment(3, "ELEM_GRT"); goto cmp;
        case ELEM_GEQ: Comment(3, "ELEM_GEQ"); goto cmp;
        case ELEM_LES: Comment(3, "ELEM_LES"); goto cmp;
        case ELEM_LEQ: Comment(3, "ELEM_LEQ"); goto cmp;
        case ELEM_NEQ: Comment(3, "ELEM_NEQ"); goto cmp;
        case ELEM_EQU: Comment(3, "ELEM_EQU"); goto cmp;
        cmp: {
            {
              char *op1 = VarFromExpr(l->d.cmp.op1, "$scratch1");
              char *op2 = VarFromExpr(l->d.cmp.op2, "$scratch2");

              if(which == ELEM_GRT) {
                  Op(INT_IF_VARIABLE_GRT_VARIABLE, op1, op2);
                  Op(INT_ELSE);
              } else if(which == ELEM_GEQ) {
                  Op(INT_IF_VARIABLE_GRT_VARIABLE, op2, op1);
              } else if(which == ELEM_LES) {
                  Op(INT_IF_VARIABLE_GRT_VARIABLE, op2, op1);
                  Op(INT_ELSE);
              } else if(which == ELEM_LEQ) {
                  Op(INT_IF_VARIABLE_GRT_VARIABLE, op1, op2);
              } else if(which == ELEM_EQU) {
                  Op(INT_IF_VARIABLE_EQUALS_VARIABLE, op1, op2);
                  Op(INT_ELSE);
              } else if(which == ELEM_NEQ) {
                  Op(INT_IF_VARIABLE_EQUALS_VARIABLE, op1, op2);
              } else oops();
            }
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);
          break;
        }
        #endif

        case ELEM_IF_BIT_SET:
            Comment(3, "ELEM_IF_BIT_SET");
            Op(INT_IF_BIT_CLEAR_IN_VAR, l->d.cmp.op1, l->d.cmp.op2);
                Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;

        case ELEM_IF_BIT_CLEAR:
            Comment(3, "ELEM_IF_BIT_CLEAR");
            Op(INT_IF_BIT_SET_IN_VAR, l->d.cmp.op1, l->d.cmp.op2);
                Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;

        case ELEM_ONE_SHOT_RISING: {
            Comment(3, "ELEM_ONE_SHOT_RISING");
            /*
                     ____
            INPUT __/    \__
                     _
            OUTPUT__/ \_____

                    | |
                     Tcycle
            */
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "ONE_SHOT_RISING", "");

            #ifndef NEW_ONE_SHOT
            Op(INT_COPY_BIT_TO_BIT, "$scratch", stateInOut);
            Op(INT_IF_BIT_SET, storeName);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            Op(INT_COPY_BIT_TO_BIT, storeName, "$scratch");
            #else
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_IF_BIT_SET, storeName);
                Op(INT_CLEAR_BIT, stateInOut); // impulse
              Op(INT_ELSE);
                Op(INT_SET_BIT, storeName);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, storeName);
            Op(INT_END_IF);
            #endif
            break;
        }
        case ELEM_ONE_SHOT_LOW: {
            Comment(3, "ELEM_ONE_SHOT_LOW");
            /*
                  __      ___
            INPUT   \____/
                  __   ______
            OUTPUT  \_/

                    | |
                     Tcycle
            */
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "ONE_SHOT_LOW", "");
            char storeNameHi[MAX_NAME_LEN];
            GenSymOneShot(storeNameHi, "ONE_SHOT_HI", "");

            Op(INT_IF_BIT_CLEAR, stateInOut);
              Op(INT_COPY_BIT_TO_BIT, stateInOut, storeName);
              Op(INT_IF_BIT_SET, storeNameHi);
              Op(INT_IF_BIT_CLEAR, storeName);
                Op(INT_SET_BIT, storeName);
              Op(INT_END_IF);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, storeName);
              Op(INT_SET_BIT, storeNameHi);
            Op(INT_END_IF);
            break;
        }
        case ELEM_ONE_SHOT_FALLING: {
            Comment(3, "ELEM_ONE_SHOT_FALLING");
            /*
                  __      __
            INPUT   \____/
                     _
            OUTPUT__/ \_____

                    | |
                     Tcycle
            */
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "ONE_SHOT_FALLING", "");

            #ifndef NEW_ONE_SHOT
            Op(INT_COPY_BIT_TO_BIT, "$scratch", stateInOut);

            Op(INT_IF_BIT_CLEAR, stateInOut);
              Op(INT_IF_BIT_SET, storeName);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);

            Op(INT_COPY_BIT_TO_BIT, storeName, "$scratch");
            #else
            Op(INT_IF_BIT_CLEAR, stateInOut);
              Op(INT_IF_BIT_SET, storeName);
                Op(INT_CLEAR_BIT, storeName);
                Op(INT_SET_BIT, stateInOut); // impulse
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_SET_BIT, storeName);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            #endif
            break;
        }
        case ELEM_OSC: {
            Comment(3, "ELEM_OSC");
            /*
                     ________
            INPUT __/        \__
                     _   _
            OUTPUT__/ \_/ \_/\__

                    | | | | |
                     Tcycle
            */
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "OSC", "");

            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_IF_BIT_SET, storeName);
                Op(INT_CLEAR_BIT, storeName);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_ELSE);
                Op(INT_SET_BIT, storeName);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, storeName);
            Op(INT_END_IF);
            break;
        }
        {
        case ELEM_7SEG:  Comment(3, stringer(ELEM_7SEG));  goto xseg;
        case ELEM_9SEG:  Comment(3, stringer(ELEM_9SEG));  goto xseg;
        case ELEM_14SEG: Comment(3, stringer(ELEM_14SEG)); goto xseg;
        case ELEM_16SEG: Comment(3, stringer(ELEM_16SEG)); goto xseg;
        xseg:
        {
            break;
        }
        }
        case ELEM_STEPPER: {
            Comment(3, "ELEM_STEPPER");
            break;
        }
        case ELEM_PULSER: {
            Comment(3, "ELEM_PULSER");
            break;
        }

        case ELEM_MOVE: {
            Comment(3, "ELEM_MOVE");
            if(IsNumber(l->d.move.dest)) {
                Error(_("Move instruction: '%ls' not a valid destination."),
                    u16(l->d.move.dest));
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.src)) {
                CheckVarInRange(l->d.move.dest, l->d.move.src, CheckMakeNumber(l->d.move.src));
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.move.dest, hobatoi(l->d.move.src));
            } else {
//              Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.move.dest, l->d.move.src);
               _Op(__LINE__, __FILE__, "args", INT_SET_VARIABLE_TO_VARIABLE, NULL, l->d.move.dest, l->d.move.src, NULL, NULL, NULL, NULL, 0, 0, NULL);
            }
            Op(INT_END_IF);
            break;
        }

        case ELEM_BUS: {
              Comment(3, "ELEM_BUS");
            break;
        }

        case ELEM_BIN2BCD: {
            Comment(3, "ELEM_BIN2BCD");
            if(IsNumber(l->d.move.dest)) {
                Error(_("BIN2BCD instruction: '%ls' not a valid destination."),
                    u16(l->d.move.dest));
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SET_BIN2BCD, l->d.move.dest, l->d.move.src);
            Op(INT_END_IF);
            break;
        }

        case ELEM_BCD2BIN: {
            Comment(3, "ELEM_BCD2BIN");
            if(IsNumber(l->d.move.dest)) {
                Error(_("BCD2BIN instruction: '%ls' not a valid destination."),
                    u16(l->d.move.dest));
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SET_BCD2BIN, l->d.move.dest, l->d.move.src);
            Op(INT_END_IF);
            break;
        }

        case ELEM_OPPOSITE: {
            Comment(3, "ELEM_OPPOSITE");
            if(IsNumber(l->d.move.dest)) {
                Error(_("OPPOSITE instruction: '%ls' not a valid destination."),
                    u16(l->d.move.dest));
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SET_OPPOSITE, l->d.move.dest, l->d.move.src);
            Op(INT_END_IF);
            break;
        }

        case ELEM_SWAP: {
            Comment(3, "ELEM_SWAP");
            if(IsNumber(l->d.move.dest)) {
                Error(_("SWAP instruction: '%ls' not a valid destination."),
                    u16(l->d.move.dest));
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SET_SWAP, l->d.move.dest, l->d.move.src);
            Op(INT_END_IF);
            break;
        }

        case ELEM_STRING: {
            Comment(3, "ELEM_STRING");
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_WRITE_STRING, l->d.fmtdStr.dest, l->d.fmtdStr.string, l->d.fmtdStr.var);
            Op(INT_END_IF);
            break;
        }

        case ELEM_RANDOM:
            Comment(3, "ELEM_RANDOM");
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_SET_VARIABLE_RANDOM, l->d.readAdc.name);
            Op(INT_END_IF);
            break;

        case ELEM_SEED_RANDOM:
            Comment(3, "ELEM_SEED_RANDOM");
            char name[MAX_NAME_LEN];
            sprintf(name, "$seed_%s", l->d.readAdc.name);
            SetSizeOfVar(name, 4);

            if(IsNumber(l->d.move.dest)) {
                Error(_("SRAND instruction: '%ls' not a valid destination."),
                    u16(l->d.move.dest));
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.src)) {
                CheckVarInRange(name, l->d.move.src, CheckMakeNumber(l->d.move.src));
                Op(INT_SET_VARIABLE_TO_LITERAL, name, hobatoi(l->d.move.src));
            } else {
                Op(INT_SET_VARIABLE_TO_VARIABLE, name, l->d.move.src);
            }
            Op(INT_END_IF);
            break;

        // These ELEM's are highly processor-dependent; the int code op does
        // most of the highly specific work
        {
        case ELEM_READ_ADC:
            Comment(3, "ELEM_READ_ADC");
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_READ_ADC, l->d.readAdc.name);
            Op(INT_END_IF);
            break;

        case ELEM_SET_PWM: {
            Comment(3, "ELEM_SET_PWM");
            char s[MAX_NAME_LEN];
            sprintf(s, "$%s", l->d.setPwm.name);
            Op(INT_IF_BIT_SET, stateInOut);
              // ugh; need a >16 bit literal though, could be >64 kHz
              Op(INT_SET_PWM, l->d.setPwm.duty_cycle, l->d.setPwm.targetFreq, l->d.setPwm.name, l->d.setPwm.resolution);
              Op(INT_SET_BIT, s);
            Op(INT_END_IF);
            SimState(&(l->poweredAfter), s, &(l->workingNow), s); // variant 6
            break;
        }
        case ELEM_NPULSE: {
            Comment(3, "ELEM_NPULSE");
            break;
        }

        case ELEM_NPULSE_OFF: {
            Comment(3, "ELEM_NPULSE_OFF");
            break;
        }
        case ELEM_QUAD_ENCOD: {
            Comment(3, "ELEM_QUAD_ENCOD");
            break;
        }

        case ELEM_PERSIST: {
          Comment(3, "ELEM_PERSIST");
          Op(INT_IF_BIT_SET, stateInOut);

            // At startup, get the persistent variable from flash.
            char isInit[MAX_NAME_LEN];
            GenSymOneShot(isInit, "PERSIST", l->d.persist.var);
            Op(INT_IF_BIT_CLEAR, isInit);
                Op(INT_CLEAR_BIT, "$scratch");
                Op(INT_EEPROM_BUSY_CHECK, "$scratch");
                Op(INT_IF_BIT_CLEAR, "$scratch");
                    Op(INT_SET_BIT, isInit);
                    Op(INT_EEPROM_READ, l->d.persist.var, EepromAddrFree);
                Op(INT_END_IF);
            Op(INT_ELSE);
                // While running, continuously compare the EEPROM copy of
                // the variable against the RAM one; if they are different,
                // write the RAM one to EEPROM.
                Op(INT_CLEAR_BIT, "$scratch");
                Op(INT_EEPROM_BUSY_CHECK, "$scratch");
                Op(INT_IF_BIT_CLEAR, "$scratch");
                    Op(INT_EEPROM_READ, "$tmpVar24bit", EepromAddrFree);
                    Op(INT_IF_VARIABLE_EQUALS_VARIABLE, "$tmpVar24bit",
                        l->d.persist.var);
                    Op(INT_ELSE);
                        Op(INT_EEPROM_WRITE, l->d.persist.var, EepromAddrFree);
                    Op(INT_END_IF);
                Op(INT_END_IF);
            Op(INT_END_IF);

          Op(INT_END_IF);

          EepromAddrFree += SizeOfVar(l->d.persist.var);
          break;
        }
        case ELEM_UART_SENDn: {
            Comment(3, "ELEM_UART_SENDn");
            char store[MAX_NAME_LEN];
            GenSymOneShot(store, "SENDn", l->d.uart.name);
            char value[MAX_NAME_LEN];
            GenSymOneShot(value, "SENDv", l->d.uart.name);
            int sov = SizeOfVar(l->d.uart.name);
            SetSizeOfVar(value, sov);
          //SetSizeOfVar(store, 1);

            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_IF_VARIABLE_LES_LITERAL, store, 1); // == 0
                  Op(INT_SET_VARIABLE_TO_LITERAL, store, sov);
                  Op(INT_SET_VARIABLE_TO_VARIABLE, value, l->d.uart.name);
                Op(INT_END_IF);
              Op(INT_END_IF);
            Op(INT_END_IF);

            Op(INT_IF_VARIABLE_LES_LITERAL, store, 1); // == 0
            Op(INT_ELSE);
              Op(INT_UART_SEND_READY, stateInOut);
              Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_DECREMENT_VARIABLE, store);
               //value = X[value[addr1] + sov - 1 - store[addr3]]
                Op(INT_UART_SEND1, value, stateInOut, store);
              Op(INT_END_IF);
            Op(INT_END_IF);

            Op(INT_IF_VARIABLE_LES_LITERAL, store, 1); // == 0
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_ELSE);
              Op(INT_SET_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_UART_SEND:
            Comment(3, "ELEM_UART_SEND");
/**
            // Why in this place do not controlled stateInOut, as in the ELEM_UART_RECV ?
            // 1. It's need in Simulation Mode.
            // 2. It's need for Arduino.
        ////Op(INT_IF_BIT_SET, stateInOut); // ???
            Op(INT_UART_SEND, l->d.uart.name, stateInOut); // stateInOut returns BUSY flag
        ////Op(INT_END_IF); // ???
/**/
/**/
            // This is equivalent to the original algorithm !!!
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_UART_SEND1, l->d.uart.name);
            Op(INT_END_IF);
            Op(INT_UART_SEND_BUSY, stateInOut); // stateInOut returns BUSY flag
/**/
            break;

        case ELEM_UART_RECV:
            Comment(3, "ELEM_UART_RECV");
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_UART_RECV, l->d.uart.name, stateInOut);
            Op(INT_END_IF);
            break;

        case ELEM_UART_RECV_AVAIL:
            Comment(3, "ELEM_UART_RECV_AVAIL");
            //Op(INT_IF_BIT_SET, stateInOut);
            Op(INT_UART_RECV_AVAIL, stateInOut);
            //Op(INT_END_IF);
            break;

        case ELEM_UART_SEND_READY:
            Comment(3, "ELEM_UART_SEND_READY");
            //Op(INT_IF_BIT_SET, stateInOut);
            Op(INT_UART_SEND_READY, stateInOut);
            //Op(INT_END_IF);
            break;
        }
        case ELEM_SET_BIT:
                Comment(3, "ELEM_SET_BIT");
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_VARIABLE_SET_BIT, l->d.math.dest, l->d.math.op1);
                Op(INT_END_IF);
                break;
        case ELEM_CLEAR_BIT:
                Comment(3, "ELEM_CLEAR_BIT");
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_VARIABLE_CLEAR_BIT, l->d.math.dest, l->d.math.op1);
                Op(INT_END_IF);
                break;
        {
        int intOp;
        case ELEM_NEG: intOp = INT_SET_VARIABLE_NEG;      Comment(3, "ELEM_NEG"); goto mathBit;
        case ELEM_SR0: intOp = INT_SET_VARIABLE_SR0;      Comment(3, "ELEM_SR0"); goto mathBit;
        case ELEM_SHL: intOp = INT_SET_VARIABLE_SHL;      Comment(3, "ELEM_SHL"); goto mathBit;
        case ELEM_SHR: intOp = INT_SET_VARIABLE_SHR;      Comment(3, "ELEM_SHR"); goto mathBit;
        case ELEM_ROL: intOp = INT_SET_VARIABLE_ROL;      Comment(3, "ELEM_ROL"); goto mathBit;
        case ELEM_ROR: intOp = INT_SET_VARIABLE_ROR;      Comment(3, "ELEM_ROR"); goto mathBit;
        case ELEM_AND: intOp = INT_SET_VARIABLE_AND;      Comment(3, "ELEM_AND"); goto mathBit;
        case ELEM_OR:  intOp = INT_SET_VARIABLE_OR;       Comment(3, "ELEM_OR" ); goto mathBit;
        case ELEM_XOR: intOp = INT_SET_VARIABLE_XOR;      Comment(3, "ELEM_XOR"); goto mathBit;
        case ELEM_NOT: intOp = INT_SET_VARIABLE_NOT;      Comment(3, "ELEM_NOT"); goto mathBit;
          mathBit: {
            if(IsNumber(l->d.math.dest)) {
                Error(_("Math instruction: '%ls' not a valid destination."),
                    u16(l->d.math.dest));
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
            if((intOp == INT_SET_VARIABLE_NEG)
            || (intOp == INT_SET_VARIABLE_NOT)
            ) {
                Op(intOp, l->d.math.dest, l->d.math.op1);
            } else {
                if((which == ELEM_SR0)
                || (which == ELEM_SHL) || (which == ELEM_SHR)
                || (which == ELEM_ROL) || (which == ELEM_ROR)
                ) {
                    if((hobatoi(l->d.math.op2) < 0) || (SizeOfVar(l->d.math.op1)*8 < hobatoi(l->d.math.op2))) {
                        Error(_("Shift constant %ls=%d out of range of the '%ls' variable: 0 to %d inclusive."),
                              u16(l->d.math.op2), hobatoi(l->d.math.op2), u16(l->d.math.op1), SizeOfVar(l->d.math.op1)*8);
                        CompileError();
                    }
                }
                Op(intOp, l->d.math.dest, l->d.math.op1, l->d.math.op2, stateInOut2);
            }
            Op(INT_END_IF);
            break;
          }
        }
        //
        {
        int intOp;
        case ELEM_ADD: intOp = INT_SET_VARIABLE_ADD;      Comment(3, "ELEM_ADD"); goto math;
        case ELEM_SUB: intOp = INT_SET_VARIABLE_SUBTRACT; Comment(3, "ELEM_SUB"); goto math;
        case ELEM_MUL: intOp = INT_SET_VARIABLE_MULTIPLY; Comment(3, "ELEM_MUL"); goto math;
        case ELEM_DIV: intOp = INT_SET_VARIABLE_DIVIDE;   Comment(3, "ELEM_DIV"); goto math;
          math: {
            if(IsNumber(l->d.math.dest)) {
                Error(_("Math instruction: '%ls' not a valid destination."),
                    u16(l->d.math.dest));
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
            const char *op1 = VarFromExpr(l->d.math.op1, "$scratch1");
            if((intOp == INT_SET_VARIABLE_SUBTRACT)
            &&(int_comment_level != 1)
            &&(strcmp(l->d.math.dest,l->d.math.op1)==0)
            &&(strcmp(l->d.math.op2,"1")==0)) {
                Op(INT_DECREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");
            } else if((intOp == INT_SET_VARIABLE_SUBTRACT)
            &&(int_comment_level != 1)
            &&(strcmp(l->d.math.dest,l->d.math.op1)==0)
            &&(strcmp(l->d.math.op2,"-1")==0)) {
                Op(INT_INCREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");

            } else if((intOp == INT_SET_VARIABLE_ADD)
            &&(int_comment_level != 1)
            &&(strcmp(l->d.math.dest,l->d.math.op1)==0)
            &&(strcmp(l->d.math.op2,"1")==0)) {
                Op(INT_INCREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");
            } else if((intOp == INT_SET_VARIABLE_ADD)
            &&(int_comment_level != 1)
            &&(strcmp(l->d.math.dest,l->d.math.op2)==0)
            &&(strcmp(l->d.math.op1,"1")==0)) {
                Op(INT_INCREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");
            } else if((intOp == INT_SET_VARIABLE_ADD)
            &&(int_comment_level != 1)
            &&(strcmp(l->d.math.dest,l->d.math.op1)==0)
            &&(strcmp(l->d.math.op2,"-1")==0)) {
                Op(INT_DECREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");
            } else if((intOp == INT_SET_VARIABLE_ADD)
            &&(int_comment_level != 1)
            &&(strcmp(l->d.math.dest,l->d.math.op2)==0)
            &&(strcmp(l->d.math.op1,"-1")==0)) {
                Op(INT_DECREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");

            } else {
                const char *op2 = VarFromExpr(l->d.math.op2, "$scratch2");
                Op(intOp, l->d.math.dest, op1, op2, stateInOut2, "ROverflowFlagV");
            }
            Op(INT_END_IF);
            break;
          }
        }
        case ELEM_SLEEP:
            Comment(3, "ELEM_SLEEP");
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SLEEP);
            Op(INT_END_IF);
            break;

        case ELEM_CLRWDT:
            Comment(3, "ELEM_CLRWDT");
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_CLRWDT);
            Op(INT_END_IF);
            break;

        case ELEM_LOCK:
            Comment(3, "ELEM_LOCK");
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_LOCK);
            Op(INT_END_IF);
            break;

        case ELEM_DELAY:
            Comment(3, "ELEM_DELAY");
            Op(INT_IF_BIT_SET, stateInOut); // fat overhead
                Op(INT_DELAY, l->d.timer.name);
            Op(INT_END_IF);
            break;

        case ELEM_TIME2DELAY: {
            Comment(3, "ELEM_TIME2DELAY");
            SDWORD clocks = CalcDelayClock(l->d.timer.delay);
            if(Prog.mcu) {
                if(Prog.mcu->whichIsa == ISA_AVR) {
                    clocks = (clocks - 1) / 4;
                    if(clocks > 0x10000)
                        clocks = 0x10000;
                } else if(Prog.mcu->whichIsa == ISA_PIC16) {
                    clocks = (clocks - 10) / 6;
                    if(clocks > 0xffff)
                        clocks = 0xffff;
                } else oops();
            }
            if(clocks <= 0 ) clocks = 1;
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, clocks);
            Op(INT_END_IF);
            break;
        }
        case ELEM_GOTO: {
            Comment(3, "ELEM_GOTO %s", l->d.doGoto.rung);
            Op(INT_IF_BIT_SET, stateInOut);
                int r;
                if(IsNumber(l->d.doGoto.rung)) {
                    r = hobatoi(l->d.doGoto.rung);
                    r = min(r, Prog.numRungs+1) - 1;
                } else {
                    r = FindRung(ELEM_LABEL, l->d.doGoto.rung);
                    if(r < 0) {
                        Error(_("GOTO: LABEL '%ls' not found!"), u16(l->d.doGoto.rung));
                        CompileError();
                    }
                }
                Op(INT_GOTO, l->d.doGoto.rung, r);
                //Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_GOSUB: {
            Comment(3, "ELEM_GOSUB %s", l->d.doGoto.rung);
            Op(INT_IF_BIT_SET, stateInOut);
                int r;
                if(IsNumber(l->d.doGoto.rung)) {
                    Error(_("GOSUB: SUBPROG as number '%ls' not allowed !"), u16(l->d.doGoto.rung));
                    CompileError();
                    r = hobatoi(l->d.doGoto.rung);
                    r = min(r, Prog.numRungs+1);
                } else {
                    r = FindRung(ELEM_SUBPROG, l->d.doGoto.rung);
                    if(r < 0) {
                        Error(_("GOSUB: SUBPROG '%ls' not found!"), u16(l->d.doGoto.rung));
                        CompileError();
                    }
                    r++;
                }
                Op(INT_GOSUB, l->d.doGoto.rung, r);
            Op(INT_END_IF);
            break;
        }
        case ELEM_LABEL:
            Comment(3, "ELEM_LABEL %s", l->d.doGoto.rung);
            break;

        case ELEM_SUBPROG: {
            Comment(3, "ELEM_SUBPROG %s", l->d.doGoto.rung);
            if((Prog.rungs[rungNow]->contents[0].which == ELEM_SUBPROG)
            && ((Prog.rungs[rungNow]->count == 1)
            ||  ((Prog.rungs[rungNow]->count == 2)
            &&   (Prog.rungs[rungNow]->contents[1].which == ELEM_COMMENT)))) {
                ;//;
            } else {
                Error(_("SUBPROG: '%ls' declaration must be single inside a rung %d"), u16(l->d.doGoto.rung), rungNow+1);
                CompileError();
            }
            int r = -1;
            if(!IsNumber(l->d.doGoto.rung)) {
              r = FindRungLast(ELEM_ENDSUB, l->d.doGoto.rung);
            }
            if(r >= 0) {
                Op(INT_GOTO, l->d.doGoto.rung, "ENDSUB", r+1);
                Op(INT_AllocKnownAddr, l->d.doGoto.rung, "SUBPROG", rungNow);
            } else {
                Error(_("SUBPROG: ENDSUB '%ls' not found!"), u16(l->d.doGoto.rung));
                CompileError();
            }
            break;
        }
        case ELEM_ENDSUB: {
            Comment(3, "ELEM_ENDSUB %s rung %d", l->d.doGoto.rung, rungNow+1);
            int r = -1;
            if(!IsNumber(l->d.doGoto.rung)) {
              r = FindRung(ELEM_SUBPROG, l->d.doGoto.rung);
            }
            if(r >= 0) {
            } else {
                Error(_("ENDSUB: SUBPROG '%ls' not found!"), u16(l->d.doGoto.rung));
                CompileError();
            }
            Op(INT_RETURN, l->d.doGoto.rung, r);
            break;
        }
        case ELEM_RETURN:
            Comment(3, "ELEM_RETURN");
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_RETURN);
//              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;

        case ELEM_MASTER_RELAY:
            Comment(3, "ELEM_MASTER_RELAY");
            // Tricky: must set the master control relay if we reach this
            // instruction while the master control relay is cleared, because
            // otherwise there is no good way for it to ever become set
            // again.
            Op(INT_IF_BIT_CLEAR, "$mcr");
              Op(INT_SET_BIT, "$mcr");
            Op(INT_ELSE);
              Op(INT_COPY_BIT_TO_BIT, "$mcr", stateInOut);
            Op(INT_END_IF);
            break;

        case ELEM_SHIFT_REGISTER: {
            Comment(3, "ELEM_SHIFT_REGISTER");
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "SHIFT_REGISTER", l->d.shiftRegister.name);
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_IF_BIT_CLEAR, storeName);
                    int i;
                    for(i = (l->d.shiftRegister.stages-2); i >= 0; i--) {
                        char dest[MAX_NAME_LEN], src[MAX_NAME_LEN];
                        sprintf(src, "%s%d", l->d.shiftRegister.name, i);
                        sprintf(dest, "%s%d", l->d.shiftRegister.name, i+1);
                        Op(INT_SET_VARIABLE_TO_VARIABLE, dest, src);
                    }
                Op(INT_END_IF);
            Op(INT_END_IF);
            Op(INT_COPY_BIT_TO_BIT, storeName, stateInOut);
            break;
        }

        case ELEM_LOOK_UP_TABLE: {
            #ifdef TABLE_IN_FLASH
            ElemLookUpTable *t = &(l->d.lookUpTable);
            Comment(3, "ELEM_LOOK_UP_TABLE %s", t->name);

            char nameTable[MAX_NAME_LEN];
            sprintf(nameTable, "%s%s", t->name,""); // "LutElement");

            int sovElement;
            sovElement =  TestByteNeeded(t->count, t->vals);

            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_FLASH_READ,t->dest,nameTable,t->index,t->count,sovElement,t->vals);
            Op(INT_END_IF);
            #else
            Comment(3, "ELEM_LOOK_UP_TABLE");
            // God this is stupid; but it will have to do, at least until I
            // add new int code instructions for this.
            int i;
            Op(INT_IF_BIT_SET, stateInOut);
            ElemLookUpTable *t = &(l->d.lookUpTable);
            for(i = 0; i < t->count; i++) {
                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", i);
                Op(INT_IF_VARIABLE_EQUALS_VARIABLE, t->index, "$scratch");
                    Op(INT_SET_VARIABLE_TO_LITERAL, t->dest, t->vals[i]);
                Op(INT_END_IF);
            }
            Op(INT_END_IF);
            #endif
            break;
        }

        case ELEM_PIECEWISE_LINEAR: {
            Comment(3, "ELEM_PIECEWISE_LINEAR");
            // This one is not so obvious; we have to decide how best to
            // perform the linear interpolation, using our 16-bit fixed
            // point math.
            ElemPiecewiseLinear *t = &(l->d.piecewiseLinear);
            if(t->count == 0) {
                Error(_("Piecewise linear lookup table with zero elements!"));
                CompileError();
            }
            int i;
            int xThis = t->vals[0];
            for(i = 1; i < t->count; i++) {
                if(t->vals[i*2] <= xThis) {
                    Error(_("x values in piecewise linear table must be "
                        "strictly increasing."));
                    CompileError();
                }
                xThis = t->vals[i*2];
            }
            Op(INT_IF_BIT_SET, stateInOut);
            for(i = t->count - 1; i >= 1; i--) {
                int thisDx = t->vals[i*2] - t->vals[(i-1)*2];
                int thisDy = t->vals[i*2 + 1] - t->vals[(i-1)*2 + 1];
                // The output point is given by
                //    yout = y[i-1] + (xin - x[i-1])*dy/dx
                // and this is the best form in which to keep it, numerically
                // speaking, because you can always fix numerical problems
                // by moving the PWL points closer together.

                // Check for numerical problems, and fail if we have them.
                if((thisDx*thisDy) >= 32767 || (thisDx*thisDy) <= -32768) {
                    Error(_("Numerical problem with piecewise linear lookup "
                        "table. Either make the table entries smaller, "
                        "or space the points together more closely.\r\n\r\n"
                        "See the help file for details."));
                    CompileError();
                }

                // Hack to avoid AVR brge issue again, since long jumps break
                Op(INT_CLEAR_BIT, "$scratch");
                Op(INT_IF_VARIABLE_LES_LITERAL, t->index, t->vals[i*2]+1);
                    Op(INT_SET_BIT, "$scratch");
                Op(INT_END_IF);

                Op(INT_IF_BIT_SET, "$scratch");
                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", t->vals[(i-1)*2]);
                Op(INT_SET_VARIABLE_SUBTRACT, "$scratch", t->index, "$scratch");
                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch2", thisDx);
                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch3", thisDy);
                Op(INT_SET_VARIABLE_MULTIPLY, t->dest, "$scratch", "$scratch3");
                Op(INT_SET_VARIABLE_DIVIDE, t->dest, t->dest, "$scratch2");

                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch",
                    t->vals[(i-1)*2 + 1]);
                Op(INT_SET_VARIABLE_ADD, t->dest, t->dest, "$scratch");
                Op(INT_END_IF);
            }
            Op(INT_END_IF);
            break;
        }
        case ELEM_FORMATTED_STRING: {
            Comment(3, "ELEM_FORMATTED_STRING");
            // Okay, this one is terrible and ineffcient, and it's a huge pain
            // to implement, but people want it a lot. The hard part is that
            // we have to let the PLC keep cycling, of course, and also that
            // we must do the integer to ASCII conversion sensisbly, with
            // only one divide per PLC cycle.

            // This variable is basically our sequencer: it is a counter that
            // increments every time we send a character.
            char seq[MAX_NAME_LEN];
            GenSymFormattedString(seq, "seq");

            // The variable whose value we might interpolate.
            char *var = l->d.fmtdStr.var;

            // This is the state variable for our integer-to-string conversion.
            // It contains the absolute value of var, possibly with some
            // of the higher powers of ten missing.
            char convertState[MAX_NAME_LEN];
            GenSymFormattedString(convertState, "convertState");

            // We might need to suppress some leading zeros.
            char isLeadingZero[MAX_NAME_LEN];
            GenSymFormattedString(isLeadingZero, "isLeadingZero");

            // This is a table of characters to transmit, as a function of the
            // sequencer position (though we might have a hole in the middle
            // for the variable output)
            char outputChars[MAX_LOOK_UP_TABLE_LEN*2];

            // This is a table of flags which was output:
            // positive is an unsigned character,
            // negative is special flag values
            enum {
                OUTPUT_UCHAR =  1,
                OUTPUT_DIGIT = -1,
                OUTPUT_SIGN = -2,
            };
            char outputWhich[sizeof(outputChars)];
            // outputWhich is added to be able to send the full unsigned char range
            // as hexadecimal numbers in formatted string element.
            // \xFF == -1   and   \xFE == -2  in output string.
            // Release 2.2 can raise error with formated strings "\0x00" and "\0x01"
            // Release 2.3 can raise error with formated strings "\0xFF" and "\0xFE"

            BOOL mustDoMinus = FALSE;

            // The total number of characters that we transmit, including
            // those from the interpolated variable.
            size_t steps;

            // The total number of digits to convert.
            int digits = -1;

            // So count that now, and build up our table of fixed things to
            // send.
            steps = 0;
            char *p = l->d.fmtdStr.string;
            while(*p) {
                if(*p == '\\' && (isdigit(p[1]) || p[1] == '-')) {
                    if(digits >= 0) {
                        Error(_("Multiple escapes (\\0-9) present in format "
                            "string, not allowed."));
                        CompileError();
                    }
                    p++;
                    if(*p == '-') {
                        mustDoMinus = TRUE;
                        outputWhich[steps  ] = OUTPUT_SIGN;
                        outputChars[steps++] = OUTPUT_SIGN;
                        p++;
                    }
                    if(!isdigit(*p) || (*p - '0') > 5 || *p == '0') {
                        Error(_("Bad escape sequence following \\; for a "
                            "literal backslash, use \\\\"));
                        CompileError();
                    }
                    digits = (*p - '0');
                    int i;
                    for(i = 0; i < digits; i++) {
                        outputWhich[steps  ] = OUTPUT_DIGIT;
                        outputChars[steps++] = OUTPUT_DIGIT;
                    }
                } else if(*p == '\\') {
                    p++;
                    switch(*p) {
                        case 'r': outputWhich[steps  ] = OUTPUT_UCHAR; outputChars[steps++] = '\r'; break;
                        case 'n': outputWhich[steps  ] = OUTPUT_UCHAR; outputChars[steps++] = '\n'; break;
                        case 'b': outputWhich[steps  ] = OUTPUT_UCHAR; outputChars[steps++] = '\b'; break;
                        case 'f': outputWhich[steps  ] = OUTPUT_UCHAR; outputChars[steps++] = '\f'; break;
                        case 't': outputWhich[steps  ] = OUTPUT_UCHAR; outputChars[steps++] = '\t'; break;
                        case 'v': outputWhich[steps  ] = OUTPUT_UCHAR; outputChars[steps++] = '\v'; break;
                        case 'a': outputWhich[steps  ] = OUTPUT_UCHAR; outputChars[steps++] = '\a'; break;
                        case '\\':outputWhich[steps  ] = OUTPUT_UCHAR; outputChars[steps++] = '\\'; break;
                        case 'x': {
                            int h, l;
                            p++;
                            h = HexDigit(*p);
                            if(h >= 0) {
                                p++;
                                l = HexDigit(*p);
                                if(l >= 0) {
                                    outputWhich[steps  ] = OUTPUT_UCHAR;
                                    outputChars[steps++] = (h << 4) | l;
                                    break;
                                }
                            }
                            Error(_("Bad escape: correct form is \\xAB."));
                            CompileError();
                            break;
                        }
                        default:
                            Error(_("Bad escape '\\%c'"), *p);
                            CompileError();
                            break;
                    }
                } else {
                    outputWhich[steps  ] = OUTPUT_UCHAR;
                    outputChars[steps++] = *p;
                }
                if(*p) p++;

                if(steps >= sizeof(outputChars)) {
                    oops();
                }
            }

            if(digits >= 0 && (strlen(var) == 0)) {
                Error(_("Variable is interpolated into formatted string, but "
                    "none is specified."));
                CompileError();
            } else if(digits < 0 && (strlen(var) > 0)) {
                Error(_("No variable is interpolated into formatted string, "
                    "but a variable name is specified. Include a string like "
                    "'\\-3', or leave variable name blank."));
                CompileError();
            }

            // We want to respond to rising edges, so yes we need a one shot.
            char oneShot[MAX_NAME_LEN];
            GenSymOneShot(oneShot, "FMTD_STR", l->d.fmtdStr.dest);

            // If no a one shot, that no Sending and no 'still running' in Rung-out state.
            char doSend[MAX_NAME_LEN];
            GenSymFormattedString(doSend, "doSend");

            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_IF_BIT_CLEAR, oneShot);
                    Op(INT_SET_BIT, oneShot); //v2
                    Op(INT_SET_VARIABLE_TO_LITERAL, seq, (SDWORD)0);
                    Op(INT_SET_BIT, doSend);
                Op(INT_END_IF);
            Op(INT_ELSE); //v2
                Op(INT_CLEAR_BIT, oneShot); //v2
            Op(INT_END_IF);
            //Op(INT_COPY_BIT_TO_BIT, oneShot, stateInOut); //v1

            // Everything that involves seqScratch is a terrible hack to
            // avoid an if statement with a big body, which is the risk
            // factor for blowing up on PIC16 page boundaries.

            const char *seqScratch = "$seqScratch";

            Op(INT_SET_VARIABLE_TO_VARIABLE, seqScratch, seq);

            // No point doing any math unless we'll get to transmit this
            // cycle, so check that first.

            Op(INT_IF_VARIABLE_LES_LITERAL, seq, steps);
            Op(INT_ELSE);
                Op(INT_SET_VARIABLE_TO_LITERAL, seqScratch, -1);
            Op(INT_END_IF);

            Op(INT_IF_BIT_SET, doSend);
                // Now check UART busy.
                /*
                // this is original code
                Op(INT_CLEAR_BIT, "$scratch");
                Op(INT_UART_SEND, "$scratch", "$scratch");
                Op(INT_IF_BIT_SET, "$scratch");
                    Op(INT_SET_VARIABLE_TO_LITERAL, seqScratch, -1);
                Op(INT_END_IF);
                */
                Op(INT_CLEAR_BIT, "$scratch"); // optional, needs only to prevent "Internal relay '%s' never assigned" message
                Op(INT_UART_SEND_READY, "$scratch");
                Op(INT_IF_BIT_CLEAR, "$scratch");
                    Op(INT_SET_VARIABLE_TO_LITERAL, seqScratch, -1);
                Op(INT_END_IF);
            Op(INT_END_IF);

            // So we transmit this cycle, so check out which character.
            int digit = 0;
            for(size_t i = 0; i < steps; i++) {
                if(outputWhich[i] == OUTPUT_DIGIT) {
                    // Note gross hack to work around limit of range for
                    // AVR brne op, which is +/- 64 instructions.
                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", i);
                    Op(INT_CLEAR_BIT, "$scratch");
                    Op(INT_IF_VARIABLE_EQUALS_VARIABLE, "$scratch", seqScratch);
                        Op(INT_SET_BIT, "$scratch");
                    Op(INT_END_IF);

                    Op(INT_IF_BIT_SET, "$scratch");

                    // Start the integer-to-string

                    // If there's no minus, then we have to load up
                    // convertState ourselves the first time.
                    if(digit == 0 && !mustDoMinus) {
                        Op(INT_SET_VARIABLE_TO_VARIABLE, convertState, var);
                    }
                    if(digit == 0) {
                        Op(INT_SET_BIT, isLeadingZero);
                    }

                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch",
                        TenToThe((digits-digit)-1));
                    Op(INT_SET_VARIABLE_DIVIDE, "$charToUart", convertState,
                        "$scratch");
                    Op(INT_SET_VARIABLE_MULTIPLY, "$scratch", "$scratch",
                        "$charToUart");
                    Op(INT_SET_VARIABLE_SUBTRACT, convertState,
                        convertState, "$scratch");
                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", '0');
                    Op(INT_SET_VARIABLE_ADD, "$charToUart", "$charToUart",
                        "$scratch");

                    // Suppress all but the last leading zero.
                    if(digit != (digits - 1)) {
                        Op(INT_IF_VARIABLE_EQUALS_VARIABLE, "$scratch",
                            "$charToUart");
                          Op(INT_IF_BIT_SET, isLeadingZero);
                            Op(INT_SET_VARIABLE_TO_LITERAL,
                                "$charToUart", ' '); // '0' %04d
                          Op(INT_END_IF);
                        Op(INT_ELSE);
                          Op(INT_CLEAR_BIT, isLeadingZero);
                        Op(INT_END_IF);
                    }

                    Op(INT_END_IF);

                    digit++;
                } else if(outputWhich[i] == OUTPUT_SIGN) {
                    // do the minus; ugliness to get around the BRNE jump
                    // size limit, though
                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", i);
                    Op(INT_CLEAR_BIT, "$scratch");
                    Op(INT_IF_VARIABLE_EQUALS_VARIABLE, "$scratch", seqScratch);
                        Op(INT_SET_BIT, "$scratch");
                    Op(INT_END_IF);
                    Op(INT_IF_BIT_SET, "$scratch");

                        // Also do the `absolute value' calculation while
                        // we're at it.
                        Op(INT_SET_VARIABLE_TO_LITERAL, "$charToUart", ' ');
                        Op(INT_IF_VARIABLE_LES_LITERAL, var, (SDWORD)0);
                            Op(INT_SET_VARIABLE_TO_LITERAL, "$charToUart", '-');
                            Op(INT_SET_VARIABLE_TO_LITERAL, convertState, (SDWORD)0);
                            Op(INT_SET_VARIABLE_SUBTRACT, convertState,
                                convertState, var);
                        Op(INT_ELSE);
                            Op(INT_SET_VARIABLE_TO_VARIABLE, convertState, var);
                        Op(INT_END_IF);

                    Op(INT_END_IF);
                } else if(outputWhich[i] == OUTPUT_UCHAR) {
                    // just another character
                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", i);
                    Op(INT_IF_VARIABLE_EQUALS_VARIABLE, "$scratch", seqScratch);
                      Op(INT_SET_VARIABLE_TO_LITERAL, "$charToUart",
                          outputChars[i]);
                    Op(INT_END_IF);
                } else oops();
            }

            Op(INT_IF_VARIABLE_LES_LITERAL, seqScratch, (SDWORD)0);
            Op(INT_ELSE);
              Op(INT_IF_BIT_SET, doSend);
                /*
                Op(INT_SET_BIT, "$scratch");
                Op(INT_UART_SEND, "$charToUart", "$scratch");
                */
                Op(INT_UART_SEND1, "$charToUart");
                /**/
                Op(INT_INCREMENT_VARIABLE, seq);
              Op(INT_END_IF);
            Op(INT_END_IF);

            // Rung-out state: true if we're still running, else false
            Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_IF_VARIABLE_LES_LITERAL, seq, steps);
              Op(INT_IF_BIT_SET, doSend);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_END_IF);
            Op(INT_ELSE);
                Op(INT_CLEAR_BIT, doSend);
            Op(INT_END_IF);
            break;
        }
        case ELEM_OPEN:
            Comment(3, "ELEM_OPEN");
            Op(INT_CLEAR_BIT, stateInOut);
            break;

        case ELEM_SHORT:
            // goes straight through
            Comment(3, "ELEM_SHORT");    // can comment // need only for debuging to align the lines in pl,asm
            break;

        case ELEM_PLACEHOLDER: {
            //Comment(3, "ELEM_PLACEHOLDER");
            Error(
              _("Empty row; delete it or add instructions before compiling."));
            CompileError();
            break;
        }
        case ELEM_COMMENT: {
            char s1[MAX_COMMENT_LEN];
            char *s2;
            AnsiToOem(l->d.comment.str,s1);
            s2 = s1;
            for(; *s2; s2++) {
                if(*s2 == '\r') {
                    *s2 = '\0';
                    s2++;
                    if(*s2 == '\n')
                        s2++;
                    break;
                }
            }
            if(int_comment_level>=2) {
                if(s1) Comment1(s1); // bypass % in comments
                if(s2) Comment1(s2); // bypass % in comments
            }
            break;
        }
        default:
            ooops("ELEM_0x%X", which);
            break;
    }
    #ifndef DEFAULT_COIL_ALGORITHM
    if((which == ELEM_COIL)
    || (which == ELEM_SET_PWM)
    ) { // ELEM_COIL is a special case, see above
        return;
    }
    if(which == ELEM_CONTACTS) { // ELEM_CONTACTS is a special case, see above
        SimState(&(l->poweredAfter), stateInOut, &(l->workingNow), l->d.contacts.name); // variant 5
        return;
    }
    #endif
    if(which != ELEM_SERIES_SUBCKT && which != ELEM_PARALLEL_SUBCKT) {
        // then it is a leaf; let the simulator know which leaf it
        // should be updating for display purposes
        SimState(&(l->poweredAfter), stateInOut);
    }
}
//-----------------------------------------------------------------------------
static BOOL CheckMasterCircuit(int which, void *elem)
{
    ElemLeaf *l = (ElemLeaf *)elem;

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            int i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(i = 0; i < s->count; i++) {
                if(CheckMasterCircuit(s->contents[i].which, s->contents[i].data.any))
                    return TRUE;
            }
            break;
        }

        case ELEM_PARALLEL_SUBCKT: {
            int i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(i = 0; i < p->count; i++) {
                if(CheckMasterCircuit(p->contents[i].which, p->contents[i].data.any))
                    return TRUE;
            }
            break;
        }
        case ELEM_MASTER_RELAY:
            return TRUE;

        default:
            break;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
static BOOL CheckMasterRelay(void)
{
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        if(CheckMasterCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]))
            return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
void WipeIntMemory(void)
{
    int i;
    for(i = 0; i < IntCodeLen; i++) {
//    if(IntCode[i].data)
//      CheckFree(IntCode[i].data); //???
    }
    IntCodeLen = 0;
    memset(IntCode, 0, sizeof(IntCode));
}

//-----------------------------------------------------------------------------
// Generate intermediate code for the entire program. Return TRUE if it worked,
// else FALSE.
//-----------------------------------------------------------------------------
BOOL GenerateIntermediateCode(void)
{
    Comment("GenerateIntermediateCode");
    if(setjmp(CompileErrorBuf) != 0) {
        return FALSE;
    }
    GenSymCountParThis = 0;
    GenSymCountParOut = 0;
    GenSymCountOneShot = 0;
    GenSymCountFormattedString = 0;
    GenSymCountStepper = 0;

    // The EEPROM addresses for the `Make Persistent' op are assigned at
    // int code generation time.
    EepromAddrFree = 0;

    rungNow = -100;//INT_MAX;
    whichNow = -INT_MAX;
    leafNow = NULL;

    WipeIntMemory();

    AllocStart();

    CheckVariableNames();

    #ifdef TABLE_IN_FLASH
    InitTables();
    #endif
    InitVars();

    rungNow++;
    BOOL ExistMasterRelay = CheckMasterRelay();
    if(int_comment_level == 1) {
        // ExistMasterRelay = TRUE; // Comment this for optimisation
    }
    if (ExistMasterRelay)
      Op(INT_SET_BIT, "$mcr");

    rungNow++;
    char s1[MAX_COMMENT_LEN];
    char *s2;
    ElemLeaf *l;
    int rung;
    for(rung = 0; rung <= Prog.numRungs; rung++) {
        rungNow = rung;
        whichNow = -INT_MAX;
        leafNow = NULL;
        Prog.OpsInRung[rung] = 0;
        Prog.HexInRung[rung] = 0;
        Op(INT_AllocFwdAddr, (SDWORD)rung);
    }

    for(rung = 0; rung < Prog.numRungs; rung++) {
        rungNow = rung;
        whichNow = -INT_MAX;
        leafNow = NULL;
        if(int_comment_level != 1) {
            Comment("");
            Comment("======= START RUNG %d =======", rung+1);
        }
        Op(INT_AllocKnownAddr, (SDWORD)rung);
        Op(INT_FwdAddrIsNow, (SDWORD)rung);

        if(Prog.rungs[rung]->count > 0 &&
            Prog.rungs[rung]->contents[0].which == ELEM_COMMENT)
        {
            // nothing to do for this one
            // Yes, I do! Push comment into interpretable OP code for C and PASCAL.
            l=(ElemLeaf *) Prog.rungs[rung]->contents[0].data.any;
            AnsiToOem(l->d.comment.str,s1);
            s2 = s1;
            for(; *s2; s2++) {
                if(*s2 == '\r') {
                    *s2 = '\0';
                    s2++;
                    if(*s2 == '\n')
                        s2++;
                    break;
                }
            }
            if(int_comment_level>=2) {
                if(s1) Comment1(s1); // bypass % in comments
                if(s2) Comment1(s2); // bypass % in comments
            }
            continue;
        }
        if(int_comment_level == 1) {
            Comment("");
            Comment("start rung %d", rung+1);
        }
        if (ExistMasterRelay)
            Op(INT_COPY_BIT_TO_BIT, "$rung_top", "$mcr");
        else
            Op(INT_SET_BIT, "$rung_top");
        SimState(&(Prog.rungPowered[rung]), "$rung_top");
        IntCodeFromCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[rung], "$rung_top", rung);
    }
    rungNow++;
    Op(INT_AllocKnownAddr, (SDWORD)rung);
    Op(INT_FwdAddrIsNow, (SDWORD)Prog.numRungs);
    rungNow++;
    //Calculate amount of intermediate codes in rungs
    int i;
    for(i = 0; i < MAX_RUNGS; i++)
        Prog.OpsInRung[i] = 0;
    for(i = 0; i < IntCodeLen; i++) {
        //dbp("IntPc=%d rung=%d ELEM_%x", i, IntCode[i].rung, IntCode[i].which);
        if((IntCode[i].rung >= 0)
        && (IntCode[i].rung < MAX_RUNGS)
        && (IntCode[i].op != INT_SIMULATE_NODE_STATE))
            Prog.OpsInRung[IntCode[i].rung]++;
    }

    //Listing of intermediate codes
    char CurrentPlFile[MAX_PATH] = "temp.pl";
    if(CurrentSaveFile)
        SetExt(CurrentPlFile, CurrentSaveFile, ".pl");
    IntDumpListing(CurrentPlFile);
    return TRUE;
}

BOOL GotoGosubUsed(void)
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i],
            ELEM_GOTO, ELEM_GOSUB, -1)
        )
            return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Are either of the UART functions (send or recv) used? Need to know this
// to know whether we must receive their pins.
//-----------------------------------------------------------------------------
BOOL UartFunctionUsed(void)
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if((ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i],
            ELEM_UART_RECV, ELEM_UART_SEND, ELEM_FORMATTED_STRING))
        ||(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i],
            ELEM_UART_RECVn, ELEM_UART_SENDn, -1))
        ||(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i],
            ELEM_UART_SEND_READY, ELEM_UART_RECV_AVAIL, -1)))
            return TRUE;
    }

    for(int i = 0; i < IntCodeLen; i++) {
        if((IntCode[i].op == INT_UART_SEND)
        || (IntCode[i].op == INT_UART_SEND1)
        || (IntCode[i].op == INT_UART_SENDn)
        || (IntCode[i].op == INT_UART_SEND_READY)
        || (IntCode[i].op == INT_UART_SEND_BUSY)
        || (IntCode[i].op == INT_UART_RECV_AVAIL)
        || (IntCode[i].op == INT_UART_RECVn)
        || (IntCode[i].op == INT_UART_RECV))
            return TRUE;
    }
    return FALSE;
}

BOOL UartRecvUsed(void)
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i],
            ELEM_UART_RECV, ELEM_UART_RECVn, -1))
            return TRUE;
    }

    for(int i = 0; i < IntCodeLen; i++) {
        if((IntCode[i].op == INT_UART_RECV)
        || (IntCode[i].op == INT_UART_RECV_AVAIL)
        || (IntCode[i].op == INT_UART_RECVn))
            return TRUE;
    }
    return FALSE;
}

BOOL UartSendUsed(void)
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i],
            ELEM_UART_SEND, ELEM_UART_SENDn, ELEM_FORMATTED_STRING))
            return TRUE;
    }

    for(int i = 0; i < IntCodeLen; i++) {
        if((IntCode[i].op == INT_UART_SEND)
        || (IntCode[i].op == INT_UART_SEND_READY)
        || (IntCode[i].op == INT_UART_SEND_BUSY)
        || (IntCode[i].op == INT_UART_SEND1)
        || (IntCode[i].op == INT_UART_SENDn))
            return TRUE;
    }
    return FALSE;
}
//-----------------------------------------------------------------------------
BOOL SpiFunctionUsed(void)
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i],
            ELEM_SPI))
            return TRUE;
    }

    for(int i = 0; i < IntCodeLen; i++) {
        if((IntCode[i].op == INT_SPI)
        || (IntCode[i].op == INT_SPI))
            return TRUE;
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
BOOL Bin32BcdRoutineUsed(void)
{
    for(int i = 0; i < IntCodeLen; i++) {
        if(IntCode[i].op == INT_SET_BIN2BCD) {
            return TRUE;
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Are either of the MultiplyRoutine functions used?
//-----------------------------------------------------------------------------
BOOL MultiplyRoutineUsed(void)
{
    for(int i = 0; i < Prog.numRungs; i++)
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_MUL, ELEM_SET_PWM, -1))
            return TRUE;

    for(int i = 0; i < IntCodeLen; i++)
        if(IntCode[i].op == INT_SET_VARIABLE_MULTIPLY)
            return TRUE;

    return FALSE;
}

//-----------------------------------------------------------------------------
// Are either of the DivideRoutine functions used?
//-----------------------------------------------------------------------------
BOOL DivideRoutineUsed(void)
{
    for(int i = 0; i < Prog.numRungs; i++)
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_DIV, ELEM_SET_PWM, -1))
            return TRUE;

    for(int i = 0; i < IntCodeLen; i++)
        if(IntCode[i].op == INT_SET_VARIABLE_DIVIDE)
            return TRUE;

    return FALSE;
}
