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
// Generate intermediate code for the ladder logic. Basically generate code
// for a `virtual machine' with operations chosen to be easy to compile to
// AVR or PIC16 code.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "intcode.h"

//// #define NEW_TON // (C) LDmicro.GitHub@gmail.com // fail
//// Restored original TON algorithm because NEW_TON don't enable RESET(TON)

#define NEW_ONE_SHOT // (C) LDmicro.GitHub@gmail.com
//#define DEFAULT_PARALLEL_ALGORITHM
//#define DEFAULT_COIL_ALGORITHM

//-----------------------------------------------------------------------------
#ifdef DEFAULT_PARALLEL_ALGORITHM
int int_comment_level = 1;
#else
int int_comment_level = 3;
//                      0 - no comments
//                      1 = Release 2.3 comments
//                      2 - more comments
//                    * 3 - ELEM_XXX comments added
#endif
//-----------------------------------------------------------------------------
int asm_comment_level = 2;
//                      0- no comment
//                      1- intenal comments if exist
//                    * 2- args
//                      3- + RUNG number in source.ld
//                      4-    -//-        and  pic16.cpp or avr.cpp line number
//                      5     -//-                      -//-        and intcode.cpp line number
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
int   bitRUartRecvErrorFlag;
DWORD addrRUartSendErrorFlag;
int   bitRUartSendErrorFlag;

std::vector<IntOp> IntCode;
int                ProgWriteP = 0;
static SDWORD *    Tdata;
int                rungNow = -INT_MAX;
static int         whichNow = -INT_MAX;
static ElemLeaf *  leafNow = nullptr;

static DWORD GenSymCount;
static DWORD GenSymCountParThis;
static DWORD GenSymCountParOut;
static DWORD GenSymCountOneShot;
static DWORD GenSymCountFormattedString;
static DWORD GenSymCountStepper;

DWORD EepromAddrFree;
DWORD RomSection;

namespace {
    std::unordered_set<std::string> persistVariables;
}

//-----------------------------------------------------------------------------
// Report an error if a constant doesn't fit in 16 bits.
//-----------------------------------------------------------------------------
static void CheckConstantInRange(SDWORD v)
{
    /*
    if(v < -0x800000 || v > 0x7FffFF) {
        THROW_COMPILER_EXCEPTION_FMT(_("Constant %d out of range: %d to %d inclusive."), v, -0x800000, 0x7FffFF);
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
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't dump intermediate code to '%s'."), outFile);
    }

    int indent = 0;
    for(uint32_t i = 0; i < IntCode.size(); i++) {

        if(IntCode[i].op == INT_END_IF)
            indent--;
        if(IntCode[i].op == INT_ELSE)
            indent--;

        if(int_comment_level == 1) {
            fprintf(f, "%3d:", i);
        } else {
            if(indent < 0)
                indent = 0;
            if((IntCode[i].op != INT_SIMULATE_NODE_STATE) // && (IntCode[i].op != INT_AllocKnownAddr)
               && (IntCode[i].op != INT_AllocFwdAddr))
                fprintf(f, "%4d:", i);
        }
        int j;
        if((int_comment_level == 1) || (IntCode[i].op != INT_SIMULATE_NODE_STATE))
            for(j = 0; j < indent; j++)
                fprintf(f, "    ");

        ElemLeaf *l = IntCode[i].leaf;

        switch(IntCode[i].op) {
            case INT_SET_BIT:
                fprintf(f, "set bit '%s'", IntCode[i].name1.c_str());
                break;

            case INT_CLEAR_BIT:
                fprintf(f, "clear bit '%s'", IntCode[i].name1.c_str());
                break;

            case INT_COPY_BIT_TO_BIT:
                fprintf(f, "let bit '%s' := '%s'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_COPY_NOT_BIT_TO_BIT:
                fprintf(f, "let bit '%s' := ! '%s'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_SET_BIT_AND_BIT:
                fprintf(f,
                        "let bit '%s' := '%s' & '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_BIT_OR_BIT:
                fprintf(f,
                        "let bit '%s' := '%s' | '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_BIT_XOR_BIT:
                fprintf(f,
                        "let bit '%s' := '%s' ^ '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_COPY_VAR_BIT_TO_VAR_BIT:
                fprintf(f, "if ('%s' & (1<<%d)) {", IntCode[i].name2.c_str(), IntCode[i].literal2);
                indent++;
                fprintf(f, "  '%s' |= (1<<%d) } else {", IntCode[i].name1.c_str(), IntCode[i].literal);
                fprintf(f, "  '%s' &= ~(1<<%d) }", IntCode[i].name1.c_str(), IntCode[i].literal);
                indent--;
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                fprintf(f, "let var '%s' := %d", IntCode[i].name1.c_str(), IntCode[i].literal);
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                fprintf(f, "let var '%s' := '%s'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_SET_BIN2BCD:
                fprintf(f, "let var '%s' = bin2bcd('%s');", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_SET_BCD2BIN:
                fprintf(f, "let var '%s' = bcd2bin('%s');", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_SET_OPPOSITE:
                fprintf(f, "let var '%s' = opposite('%s');", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_SET_VARIABLE_ROL:
                fprintf(f,
                        "let var '%s' := '%s' rol '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_ROR:
                fprintf(f,
                        "let var '%s' := '%s' ror '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_SHL:
                fprintf(f,
                        "let var '%s' := '%s' << '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_SHR:
                fprintf(f,
                        "let var '%s' := '%s' >> '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_AND:
                fprintf(f,
                        "let var '%s' := '%s' & '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_OR:
                fprintf(f,
                        "let var '%s' := '%s' | '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_XOR:
                fprintf(f,
                        "let var '%s' := '%s' ^ '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_NOT:
                fprintf(f, "let var '%s' := ~ '%s'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_SET_SWAP:
                fprintf(f, "let var '%s' = swap('%s');", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_SET_VARIABLE_SR0:
                fprintf(f,
                        "let var '%s' := '%s' sr0 '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_NEG:
                fprintf(f, "let var '%s' := - '%s'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_SET_VARIABLE_ADD:
                fprintf(f,
                        "let var '%s' := '%s' + '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                if(IntCode[i].name4.size())
                    fprintf(f, "; copy overflow flag to '%s'", IntCode[i].name4.c_str());
                break;

            case INT_SET_VARIABLE_SUBTRACT:
                fprintf(f,
                        "let var '%s' := '%s' - '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                if(IntCode[i].name4.size())
                    fprintf(f, "; copy overflow flag to '%s'", IntCode[i].name4.c_str());
                break;

            case INT_SET_VARIABLE_MULTIPLY:
                fprintf(f,
                        "let var '%s' := '%s' * '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_DIVIDE:
                fprintf(f,
                        "let var '%s' := '%s' / '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_SET_VARIABLE_MOD:
                fprintf(f,
                        "let var '%s' := '%s' %% '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_INCREMENT_VARIABLE:
                fprintf(f, "increment '%s'", IntCode[i].name1.c_str());
                if(IntCode[i].name2.size())
                    fprintf(f, "; copy overlap(-1 to 0) flag to '%s'", IntCode[i].name2.c_str());
                if(IntCode[i].name3.size())
                    fprintf(f, "; copy overflow flag to '%s'", IntCode[i].name3.c_str());
                break;

            case INT_DECREMENT_VARIABLE:
                fprintf(f, "decrement '%s'", IntCode[i].name1.c_str());
                if(IntCode[i].name2.size())
                    fprintf(f, "; copy overlap(0 to -1) flag to '%s'", IntCode[i].name2.c_str());
                if(IntCode[i].name3.size())
                    fprintf(f, "; copy overflow flag to '%s'", IntCode[i].name3.c_str());
                break;

            case INT_READ_ADC:
                fprintf(f, "read adc '%s', refs is '%d'", IntCode[i].name1.c_str(), IntCode[i].literal);
                break;

            case INT_SET_SEED_RANDOM:
                fprintf(f, "let var '$seed_%s' := '%s'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_SET_VARIABLE_RANDOM:
                fprintf(f, "let var '%s' := random()", IntCode[i].name1.c_str());
                break;

            case INT_SET_PWM:
                fprintf(f,
                        "set pwm '%s' %% %s Hz out '%s'",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_QUAD_ENCOD:
                fprintf(f,
                        "QUAD ENCOD %d %s %s %s %s %s",
                        IntCode[i].literal,
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str(),
                        IntCode[i].name4.c_str(),
                        IntCode[i].name5.c_str());
                break;

            case INT_SET_NPULSE:
                fprintf(f,
                        "generate %s pulses %s Hz to %s",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

            case INT_OFF_NPULSE:
                fprintf(f, "OFF N PULSE");
                break;

            case INT_PWM_OFF:
                fprintf(f, "pwm off '%s'", IntCode[i].name1.c_str());
                break;

            case INT_EEPROM_BUSY_CHECK:
                fprintf(f, "set bit '%s' if EEPROM busy", IntCode[i].name1.c_str());
                break;

            case INT_EEPROM_READ: {
                int sov = SizeOfVar(IntCode[i].name1);
                if(sov == 1)
                    fprintf(f, "read EEPROM[%d] into '%s'", IntCode[i].literal, IntCode[i].name1.c_str());
                else if(sov == 2)
                    fprintf(f,
                            "read EEPROM[%d,%d+1] into '%s'",
                            IntCode[i].literal,
                            IntCode[i].literal,
                            IntCode[i].name1.c_str());
                else if(sov == 3)
                    fprintf(f,
                            "read EEPROM[%d,%d+1,%d+2] into '%s'",
                            IntCode[i].literal,
                            IntCode[i].literal,
                            IntCode[i].literal,
                            IntCode[i].name1.c_str());
                else if(sov == 4)
                    fprintf(f,
                            "read EEPROM[%d,%d+1,%d+2,%d+3] into '%s'",
                            IntCode[i].literal,
                            IntCode[i].literal,
                            IntCode[i].literal,
                            IntCode[i].literal,
                            IntCode[i].name1.c_str());
                else
                    THROW_COMPILER_EXCEPTION(_("Internal error."));
                break;
            }
            case INT_EEPROM_WRITE: {
                int sov = SizeOfVar(IntCode[i].name1);
                if(sov == 1)
                    fprintf(f, "write '%s' into EEPROM[%d]", IntCode[i].name1.c_str(), IntCode[i].literal);
                else if(sov == 2)
                    fprintf(f,
                            "write '%s' into EEPROM[%d,%d+1]",
                            IntCode[i].name1.c_str(),
                            IntCode[i].literal,
                            IntCode[i].literal);
                else if(sov == 3)
                    fprintf(f,
                            "write '%s' into EEPROM[%d,%d+1,%d+2]",
                            IntCode[i].name1.c_str(),
                            IntCode[i].literal,
                            IntCode[i].literal,
                            IntCode[i].literal);
                else if(sov == 4)
                    fprintf(f,
                            "write '%s' into EEPROM[%d,%d+1,%d+2,%d+3]",
                            IntCode[i].name1.c_str(),
                            IntCode[i].literal,
                            IntCode[i].literal,
                            IntCode[i].literal,
                            IntCode[i].literal);
                else
                    THROW_COMPILER_EXCEPTION(_("Internal error."));
                break;
            }
            case INT_SPI_COMPLETE:
                fprintf(f, "SPI_COMPLETE '%s', done? into '%s'", l->d.spi.name, IntCode[i].name1.c_str());
                break;
            case INT_SPI_BUSY:
                fprintf(f, "SPI_BUSY '%s', done? into '%s'", l->d.spi.name, IntCode[i].name1.c_str());
                break;
            case INT_SPI:
                fprintf(f,
                        "SPI '%s' send '%s', receive '%s', done? into '%s'",
                        l->d.spi.name,
                        l->d.spi.send,
                        l->d.spi.recv,
                        IntCode[i].name1.c_str());
                break;

            ///// Added by JG
            case INT_SPI_WRITE:
                fprintf(f,
                        "SPI_WRITE '%s' send '%s', receive '%s', done? into '%s'",
                        l->d.spi.name,
                        l->d.spi.send,
                        l->d.spi.recv,
                        IntCode[i].name1.c_str());
                break;

            case INT_I2C_READ:
                fprintf(f,
                        "I2C_READ '%s' receive '%s', done? into '%s'",
                        l->d.i2c.name,
                        l->d.i2c.recv,
                        IntCode[i].name1.c_str());
                break;
            case INT_I2C_WRITE:
                fprintf(f,
                        "I2C_WRITE '%s' send '%s', done? into '%s'",
                        l->d.i2c.name,
                        l->d.i2c.send,
                        IntCode[i].name1.c_str());
                break;
            /////

            case INT_UART_SEND1:
            case INT_UART_SENDn:
                fprintf(f, "uart send from '%s[%s+%d]'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str(), IntCode[i].literal);
                break;

            case INT_UART_SEND:
                fprintf(f, "uart send from '%s', done? into '%s'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_UART_SEND_READY:
                fprintf(f, "'%s' = is uart ready to send ?", IntCode[i].name1.c_str());
                break;

            case INT_UART_SEND_BUSY:
                fprintf(f, "'%s' = is uart busy to send ?", IntCode[i].name1.c_str());
                break;

            case INT_UART_RECVn:
            case INT_UART_RECV1:
                fprintf(f, "uart recv into '%s[%s+%d]'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str(), IntCode[i].literal);
                break;

            case INT_UART_RECV:
                fprintf(f, "uart recv into '%s', have? into '%s'", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                break;

            case INT_UART_RECV_AVAIL:
                fprintf(f, "'%s' = is uart receive data available ?", IntCode[i].name1.c_str());
                break;

            case INT_IF_BIT_SET:
                fprintf(f, "if '%s' {", IntCode[i].name1.c_str());
                indent++;
                break;

            case INT_IF_BIT_CLEAR:
                fprintf(f, "if not '%s' {", IntCode[i].name1.c_str());
                indent++;
                break;

            case INT_IF_BIT_EQU_BIT:
                fprintf(f, "if '%s' == '%s' {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;

            case INT_IF_BIT_NEQ_BIT:
                fprintf(f, "if '%s' != '%s' {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;

            case INT_SLEEP:
                fprintf(f, "SLEEP;");
                break;

            case INT_DELAY:
                fprintf(f, "DELAY %s us;", IntCode[i].name1.c_str());
                break;

            case INT_CLRWDT:
                fprintf(f, "CLRWDT;");
                break;

            case INT_LOCK:
                fprintf(f, "LOCK;");
                break;

            case INT_VARIABLE_SET_BIT:
                fprintf(f, "set bit number '%s' in var '%s'", IntCode[i].name2.c_str(), IntCode[i].name1.c_str());
                break;

            case INT_VARIABLE_CLEAR_BIT:
                fprintf(f, "clear bit number '%s' in var '%s'", IntCode[i].name2.c_str(), IntCode[i].name1.c_str());
                break;

            case INT_IF_BIT_SET_IN_VAR:
                fprintf(f, "if ('%s' & (1<<%s)) != 0  {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;

            case INT_IF_BIT_CLEAR_IN_VAR:
                fprintf(f, "if ('%s' & (1<<%s)) == 0 {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;

            case INT_IF_BITS_SET_IN_VAR:
                fprintf(f, "if ('%s' & %d) == %d  {", IntCode[i].name1.c_str(), IntCode[i].literal, IntCode[i].literal);
                indent++;
                break;

            case INT_IF_BITS_CLEAR_IN_VAR:
                fprintf(f, "if ('%s' & %d) == 0 {", IntCode[i].name1.c_str(), IntCode[i].literal);
                indent++;
                break;

#ifndef NEW_CMP
            case INT_IF_VARIABLE_LES_LITERAL:
                fprintf(f, "if '%s' < %d {", IntCode[i].name1, IntCode[i].literal);
                indent++;
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                fprintf(f, "if '%s' == '%s' {", IntCode[i].name1, IntCode[i].name2);
                indent++;
                break;

            case INT_IF_VARIABLE_GRT_VARIABLE:
                fprintf(f, "if '%s' > '%s' {", IntCode[i].name1, IntCode[i].name2);
                indent++;
                break;
#endif

#ifdef NEW_CMP
            case INT_IF_GRT:
                fprintf(f, "if '%s' > '%s' {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;

            case INT_IF_GEQ:
                fprintf(f, "if '%s' >= '%s' {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;

            case INT_IF_LES:
                fprintf(f, "if '%s' < '%s' {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;

            case INT_IF_LEQ:
                fprintf(f, "if '%s' <= '%s' {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;

            case INT_IF_NEQ:
                fprintf(f, "if '%s' != '%s' {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;

            case INT_IF_EQU:
                fprintf(f, "if '%s' == '%s' {", IntCode[i].name1.c_str(), IntCode[i].name2.c_str());
                indent++;
                break;
#endif

            case INT_END_IF:
                fprintf(f, "}");
                break;

            case INT_ELSE:
                fprintf(f, "} else {");
                indent++;
                break;

            case INT_SIMULATE_NODE_STATE:
                // simulation-only; the real code generators don't care
                break;

            case INT_COMMENT:
                fprintf(f, "# %s", IntCode[i].name1.c_str());
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
                fprintf(f,
                        "SFR %d %s %s %s %d %d",
                        IntCode[i].op,
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str(),
                        IntCode[i].literal,
                        IntCode[i].literal2);
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
                fprintf(f, "Label%s:", IntCode[i].name1.c_str());
                break;

            case INT_AllocFwdAddr:
                //fprintf(f, "AllocFwdAddr AddrOfRung%d",IntCode[i].literal+1);
                break;

            case INT_FwdAddrIsNow:
                //fprintf(f, "Label%d: # %s", IntCode[i].literal + 1, IntCode[i].name1.c_str());
                break;

            case INT_GOTO:
                fprintf(f, "GOTO Label%s # %s %d", IntCode[i].name1.c_str(), IntCode[i].name2.c_str(), IntCode[i].literal);
                break;

            case INT_GOSUB:
                fprintf(f, "GOSUB Label%s # %s %d", IntCode[i].name1.c_str(), IntCode[i].name2.c_str(), IntCode[i].literal);
                break;

            case INT_RETURN:
                fprintf(f, "RETURN # %s", IntCode[i].name1.c_str());
                break;

            case INT_WRITE_STRING:
                fprintf(f,
                        "sprintf(%s, \"%s\", %s);",
                        IntCode[i].name1.c_str(),
                        IntCode[i].name2.c_str(),
                        IntCode[i].name3.c_str());
                break;

#ifdef TABLE_IN_FLASH
            case INT_FLASH_INIT:
                fprintf(f,
                        "INIT TABLE signed %d byte %s[%d] := {",
                        IntCode[i].literal2,
                        IntCode[i].name1.c_str(),
                        IntCode[i].literal);
                for(int j = 0; j < (IntCode[i].literal - 1); j++) {
                    fprintf(f, "%d, ", IntCode[i].data[j]);
                }
                fprintf(f, "%d}", IntCode[i].data[IntCode[i].literal - 1]);
                break;

            case INT_RAM_READ:
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f,
                            "let var '%s' := '%s[%d]'",
                            IntCode[i].name2.c_str(),
                            IntCode[i].name1.c_str(),
                            CheckMakeNumber(IntCode[i].name3));
                } else {
                    fprintf(f,
                            "let var '%s' := '%s[%s]'",
                            IntCode[i].name2.c_str(),
                            IntCode[i].name1.c_str(),
                            IntCode[i].name3.c_str());
                }
                break;

            case INT_FLASH_READ:
                if(IsNumber(IntCode[i].name3)) {
                    fprintf(f,
                            "let var '%s' := %d # '%s[%s]'",
                            IntCode[i].name1.c_str(),
                            IntCode[i].data[hobatoi(IntCode[i].name3.c_str())],
                            IntCode[i].name2.c_str(),
                            IntCode[i].name3.c_str());
                } else {
                    fprintf(f,
                            "let var '%s' := '%s[%s]'",
                            IntCode[i].name1.c_str(),
                            IntCode[i].name2.c_str(),
                            IntCode[i].name3.c_str());
                }
                break;
#endif

            default:
                Error("INT_%d", IntCode[i].op);
        }
        if((int_comment_level == 1)
           || ((IntCode[i].op != INT_SIMULATE_NODE_STATE) // && (IntCode[i].op != INT_AllocKnownAddr)
               && (IntCode[i].op != INT_AllocFwdAddr))) {
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
//-----------------------------------------------------------------------------
static void GenSym(char *dest, char *name, char *name1, char *name2)
{
    sprintf(dest, "%s_%01lx_%s_%s", name, GenSymCount, name1, name2);
    GenSymCount++;
}

static void GenVar(char *dest, char *name1, char *name2)
{
    sprintf(dest, "$var_%01lx_%s_%s", GenSymCount, name1, name2);
    GenSymCount++;
}

static void GenSymParThis(char *dest)
{
    sprintf(dest, "$parThis_%01lx", GenSymCountParThis);
    GenSymCountParThis++;
}
static void GenSymParOut(char *dest)
{
    sprintf(dest, "$parOut_%01lx", GenSymCountParOut);
    GenSymCountParOut++;
}
void GenSymOneShot(char *dest, const char *name1, const char *name2)
{
    if(int_comment_level == 1)
        sprintf(dest, "$once_%01lx", GenSymCountOneShot);
    else
        sprintf(dest, "$once_%01lx_%s_%s", GenSymCountOneShot, name1, name2);
    GenSymCountOneShot++;
}
static void GenSymOneShot(char *dest)
{
    GenSymOneShot(dest, "", "");
}
static void GenSymFormattedString(char *dest, const char *name)
{
    sprintf(dest, "$fmtd_%01lx_%s", GenSymCountFormattedString, name);
    GenSymCountFormattedString++;
}
static void GenSymFormattedString(char *dest)
{
    GenSymFormattedString(dest, "");
}
static void GenSymStepper(char *dest, char *name)
{
    sprintf(dest, "$step_%01lx_%s", GenSymCountStepper, name);
    GenSymCountStepper++;
}

//-----------------------------------------------------------------------------
// Compile an instruction to the program.
//-----------------------------------------------------------------------------
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3,
                const char *name4, const char *name5, const char *name6, SDWORD lit, SDWORD lit2, int32_t *data)
{
    IntOp intOp;
    intOp.op = op;
    if(name1)
        intOp.name1 = name1;
    if(name2)
        intOp.name2 = name2;
    if(name3)
        intOp.name3 = name3;
    if(name4)
        intOp.name4 = name4;
    if(name5)
        intOp.name5 = name5;
    if(name6)
        intOp.name6 = name6;
    intOp.literal = lit;
#ifdef NEW_CMP
    if((op == INT_IF_LES) || (op == INT_IF_VARIABLE_LES_LITERAL))
        if(!name2) {
            sprintf(intOp.name2.data(), "%d", lit);
        }
#endif
    intOp.literal2 = lit2;
    intOp.data = data;
    intOp.rung = rungNow;
    intOp.which = whichNow;
    intOp.leaf = leafNow;
    intOp.poweredAfter = &(leafNow->poweredAfter);
    intOp.fileLine = l;
    intOp.fileName = f;
    IntCode.emplace_back(intOp);
}

static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, SDWORD lit)
{
    _Op(l, f, args, op, name1, name2, nullptr, nullptr, nullptr, nullptr, lit, 0, nullptr);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1, SDWORD lit)
{
    _Op(l, f, args, op, name1, nullptr, nullptr, nullptr, nullptr, nullptr, lit, 0, nullptr);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2)
{
    _Op(l, f, args, op, name1, name2, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1)
{
    _Op(l, f, args, op, name1, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr);
}
static void _Op(int l, const char *f, const char *args, int op, SDWORD lit)
{
    _Op(l, f, args, op, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, lit, 0, nullptr);
}
static void _Op(int l, const char *f, const char *args, int op)
{
    _Op(l, f, args, op, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 0, 0, nullptr);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3,
                SDWORD lit)
{
    _Op(l, f, args, op, name1, name2, name3, nullptr, nullptr, nullptr, lit, 0, nullptr);
}
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3)
{
    _Op(l, f, args, op, name1, name2, name3, nullptr, nullptr, nullptr, 0, 0, nullptr);
}
//
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3,
                SDWORD lit, SDWORD lit2)
{
    _Op(l, f, args, op, name1, name2, name3, nullptr, nullptr, nullptr, lit, lit2, nullptr);
}
//
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3,
                const char *name4)
{
    _Op(l, f, args, op, name1, name2, name3, name4, nullptr, nullptr, 0, 0, nullptr);
}
//
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3,
                const char *name4, const char *name5)
{
    _Op(l, f, args, op, name1, name2, name3, name4, name5, nullptr, 0, 0, nullptr);
}
//
static void _Op(int l, const char *f, const char *args, int op, const char *name1, const char *name2, const char *name3,
                SDWORD lit, SDWORD lit2, int32_t *data)
{
    _Op(l, f, args, op, name1, name2, name3, nullptr, nullptr, nullptr, lit, lit2, data);
}

// And use macro for bugtracking
#define Op(...) _Op(__LINE__, __FILE__, #__VA_ARGS__, __VA_ARGS__)
//-----------------------------------------------------------------------------
// Compile the instruction that the simulator uses to keep track of which
// nodes are energized (so that it can display which branches of the circuit
// are energized onscreen). The MCU code generators ignore this, of course.
//-----------------------------------------------------------------------------
static void SimState(bool *b, const char *name, bool *w, const char *name2)
{
    IntOp intOp;
    intOp.op = INT_SIMULATE_NODE_STATE;
    intOp.poweredAfter = b;
    intOp.workingNow = w;
    intOp.name1 = name;
    if(name2)
        intOp.name2 = name2;
    intOp.rung = rungNow;
    intOp.which = whichNow;
    intOp.leaf = leafNow;
    IntCode.emplace_back(intOp);
}

static void SimState(bool *b, const char *name)
{
    SimState(b, name, nullptr, nullptr);
}

//-----------------------------------------------------------------------------
// printf-like comment function
//-----------------------------------------------------------------------------
static void _Comment1(int l, const char *f, const char *str)
{
    if(int_comment_level) {
        char buf[MAX_NAME_LEN];
        strncpy(buf, str, MAX_NAME_LEN - 1);
        buf[MAX_NAME_LEN - 1] = '\0';
        // http://demin.ws/blog/russian/2013/01/28/use-snprintf-on-different-platforms/
        _Op(l, f, nullptr, INT_COMMENT, buf);
    }
}
#define Comment1(str) _Comment1(__LINE__, __FILE__, str)

static void _Comment(int l, const char *f, const char *str, ...)
{
    if(int_comment_level) {
        va_list v;
        char    buf[MAX_NAME_LEN];
        va_start(v, str);
        vsnprintf(buf, MAX_NAME_LEN, str, v);
        _Op(l, f, nullptr, INT_COMMENT, buf);
    }
}

static void _Comment(int l, const char *f, int level, const char *str, ...)
{
    if(int_comment_level && (int_comment_level >= level)) {
        va_list v;
        char    buf[MAX_NAME_LEN];
        va_start(v, str);
        vsnprintf(buf, MAX_NAME_LEN, str, v);
        _Op(l, f, nullptr, INT_COMMENT, buf);
    }
}

#define Comment(...) _Comment(__LINE__, __FILE__, __VA_ARGS__)

//-----------------------------------------------------------------------------
SDWORD TestTimerPeriod(char *name, SDWORD delay, int adjust) // delay in us
{
    if(delay <= 0) {
        Error("%s '%s': %s", _("Timer"), name, _("Delay cannot be zero or negative."));
        return -1;
    }
    long long int period = 0, adjPeriod = 0, maxPeriod = 0;
    period = delay / Prog.cycleTime; // - 1;
    adjPeriod = (delay + Prog.cycleTime * adjust) / Prog.cycleTime;

    int b = byteNeeded(period);
    if((SizeOfVar(name) != b) && (b <= 4))
        SetSizeOfVar(name, b);
    maxPeriod = (long long int)(1) << (SizeOfVar(name) * 8 - 1);
    maxPeriod--;

    if(period < 0) {
        Error(_("Delay cannot be zero or negative."));
    } else if(period <= 0) {
        char s1[1024];
        sprintf(s1, "%s %s", _("Timer period too short (needs faster cycle time)."), _("Or increase timer period."));
        char s2[1024];
        sprintf(s2, _("Timer '%s'=%.3f ms."), name, 1.0 * delay / 1000);
        char s3[1024];
        sprintf(s3, _("Minimum available timer period = PLC cycle time = %.3f ms."), 1.0 * Prog.cycleTime / 1000);
        const char *s4 = _("Not available");
        Error("%s\n\r%s %s\r\n%s", s1, s4, s2, s3);
    } else if(period + adjust <= 0) {
        Error("%s '%s': %s",
              _("Timer"),
              name,
              _("Total timer delay cannot be zero or negative. Increase the adjust value!"));
        // period = -1;
    } else if(period <= adjust) {
        Error(
            "%s '%s': %s",
            _("Timer"),
            name,
            _("Adjusting the timer delay to a value greater than or near than the timer delay is meaningless. Decrease the adjust value!"));
        // period = -1;
    }

    if(((period > maxPeriod) || (adjPeriod > maxPeriod)) //
       && (Prog.mcu())                                     //
       && (Prog.mcu()->whichIsa != ISA_PC)                 //
    ) {
        char s1[1024];
        sprintf(s1, "%s %s", _("Timer period too long; (use a slower cycle time)."), _("Or decrease timer period."));
        char s2[1024];
        sprintf(s2, _("Timer 'T%s'=%10.0Lf s   needs %15lld PLC cycle times."), name, 1.0 * delay / 1000, period);
        long double maxDelay = 1.0 * maxPeriod / 1000000 * Prog.cycleTime; // s
        char        s3[1024];
        sprintf(s3,
                _("Timer 'T%s'=%10.0Lf s can use %15lld PLC cycle times as the MAXIMUM possible value."),
                name,
                maxDelay,
                maxPeriod);
        Error("%s\r\n%s\r\n%s", s1, s2, s3);
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
        Warning("PLC Cycle Time is '0'. TON, TOF, RTO, RTL, TCY timers does not work correctly!");
        return 1;
    }

    SDWORD period = TestTimerPeriod(l->d.timer.name, hobatoi(l->d.timer.delay), l->d.timer.adjust);
    if(period < 1) {
        Error(_("Internal error."));
    }
    return period;
}

//-----------------------------------------------------------------------------
SDWORD CalcDelayClock(long long clocks) // in us
{
#if 1 // 1
    clocks = clocks * Prog.mcuClock / 1000000;
    if(Prog.mcu()) {
        if(Prog.mcu()->whichIsa == ISA_AVR) {
            ;
        } else if(Prog.mcu()->whichIsa == ISA_PIC16) {
            clocks = clocks / 4;
        } else
            Error(_("Internal error."));
    }
    if(clocks <= 0)
        clocks = 1;
#endif
    return (SDWORD)clocks;
}

//-----------------------------------------------------------------------------
// Is an expression that could be either a variable name or a number a number?
//-----------------------------------------------------------------------------
bool IsNumber(const char *str)
{
    while(isspace(*str))
        str++;
    if(isdigit(*str)) {
        return true;
    } else if((*str == '-') || (*str == '+')) {
        str++;
        while(isspace(*str))
            str++;
        if(isdigit(*str))
            return true;
        else
            return false;
    } else if(*str == '\'') {
        // special case--literal single character
        if(strlen(str) > 2) {
            if(str[strlen(str) - 1] == '\'')
                return true;
            else
                return false;
        } else
            return false;
    }

    return false;
}

bool IsNumber(const NameArray &name)
{
    return IsNumber(name.c_str());
}

//-----------------------------------------------------------------------------
bool CheckForNumber(const char *str)
{
    if(IsNumber(str)) {
        int         radix = 0; //auto detect
        const char *start_ptr = str;
        while(isspace(*start_ptr) || *start_ptr == '-' || *start_ptr == '+')
            start_ptr++;

        if(*start_ptr == '\'') {
            // special case--literal single character
            if(strlen(start_ptr) > 2) {
                if(str[strlen(start_ptr) - 1] == '\'')
                    return true;
                else
                    return false;
            } else
                return false;
        }

        if(*start_ptr == '0') {
            if(toupper(start_ptr[1]) == 'B')
                radix = 2;
            else if(toupper(start_ptr[1]) == 'O')
                radix = 8;
        }

        char *end_ptr = nullptr;
        // errno = 0;
        long val = strtol(str, &end_ptr, radix);
        if(*end_ptr) {
            return false;
        }
        if((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) {
            return false;
        }
        return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
double hobatof(const char *str)
{
    return atof(str);
}
//-----------------------------------------------------------------------------
int getradix(const char *str)
{
    int         radix = 0;
    const char *start_ptr = str;
    while(isspace(*start_ptr) || *start_ptr == '-' || *start_ptr == '+')
        start_ptr++;
    if((start_ptr[0] != '0') && isdigit(start_ptr[0]))
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
        THROW_COMPILER_EXCEPTION_FMT("'%s'\r\n'%s'", str, start_ptr);
    }
    return radix;
}
//-----------------------------------------------------------------------------
long hobatoi(const char *str)
{
    long        val;
    const char *start_ptr = str;
    while(isspace(*start_ptr))
        start_ptr++;
    if(*start_ptr == '\'') {
        char dest[MAX_NAME_LEN];
        FrmStrToStr(dest, start_ptr);
        if((strlen(dest) > 3) || (dest[0] != '\'') || (dest[2] != '\'')) {
            THROW_COMPILER_EXCEPTION_FMT(_("Expected single-character or one simple-escape-sequence in single-quotes: <%s>!"), str);
        }
        val = dest[1];
    } else {
        while(isspace(*start_ptr) || *start_ptr == '-' || *start_ptr == '+') {
            start_ptr++;
        }
        int radix = 0; //auto detect
        if(*start_ptr == '0') {
            if(toupper(start_ptr[1]) == 'B')
                radix = 2;
            else if(toupper(start_ptr[1]) == 'O')
                radix = 8;
            else if(toupper(start_ptr[1]) == 'X')
                radix = 16;
        }
        char *end_ptr = nullptr;
        // errno = 0;
        if(radix == 16) {
            val = strtoul(str, &end_ptr, radix);
        } else {
            val = strtol(str, &end_ptr, radix);
        }
        if(*end_ptr) {
            //         val = 0;
            //         THROW_COMPILER_EXCEPTION_FMT("Conversion error the\n'%s' string into number %d at\n'%s' position.", str, val, end_ptr);
        }
        if((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) {
            //         val = 0;
            //         THROW_COMPILER_EXCEPTION_FMT("Conversion overflow error the string\n'%s' into number %d.", str, val);
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
    SDWORD val = hobatoi(str);
    CheckConstantInRange(val);
    return val;
}

SDWORD CheckMakeNumber(const NameArray &str)
{
    return CheckMakeNumber(str.c_str());
}

//-----------------------------------------------------------------------------
// Return an integer power of ten.
//-----------------------------------------------------------------------------
int TenToThe(int x)
{
    int r = 1;
    for(int i = 0; i < x; i++) {
        r *= 10;
    }
    return r;
}

//-----------------------------------------------------------------------------
int xPowerY(int x, int y)
{
    int r = 1;
    for(int i = 0; i < y; i++) {
        r *= x;
    }
    return r;
}

//-----------------------------------------------------------------------------
static bool CheckStaySameElem(int which, void *elem)
{
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(int i = 0; i < s->count; i++) {
                if(!CheckStaySameElem(s->contents[i].which, s->contents[i].data.any))
                    return false;
            }
            return true;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(int i = 0; i < p->count; i++) {
                if(!CheckStaySameElem(p->contents[i].which, p->contents[i].data.any))
                    return false;
            }
            return true;
        }
        default:
            return StaySameElem(which);
    }
    return false;
}

//-----------------------------------------------------------------------------
static bool CheckEndOfRungElem(int which, void *elem)
{
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            return CheckEndOfRungElem(s->contents[s->count - 1].which, s->contents[s->count - 1].data.any);
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(int i = 0; i < p->count; i++) {
                if(CheckEndOfRungElem(p->contents[i].which, p->contents[i].data.any))
                    return true;
            }
            return false;
        }
        default:
            return EndOfRungElem(which);
    }
    return false;
}

//-----------------------------------------------------------------------------
static bool CheckCanChangeOutputElem(int which, void *elem)
{
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(int i = 0; i < s->count; i++) {
                if(CheckCanChangeOutputElem(s->contents[i].which, s->contents[i].data.any))
                    return true;
            }
            return false;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(int i = 0; i < p->count; i++) {
                if(CheckCanChangeOutputElem(p->contents[i].which, p->contents[i].data.any))
                    return true;
            }
            return false;
        }
        default:
            return CanChangeOutputElem(which);
    }
    return false;
}

//-----------------------------------------------------------------------------
char *GetLabelName(int which, char *name, char *label)
{
   int r;
   if(IsNumber(label)) {
       r = hobatoi(label);
       r = std::max(r, 1);
       r = std::min(r, Prog.numRungs + 1);
   } else {
       r = FindRung(which, label);
       if(r < 0)
           oops();
       r++;
       if(which == ELEM_SUBPROG) {
           r++; // Call the next rung.
       }
   }
   sprintf(name, "Rung%d", r);
   return name;
}

char *GetLabelName(char *name, int r)
{
   sprintf(name, "Rung%d", r);
   return name;
}
//-----------------------------------------------------------------------------
void OpSetVar(char *op1, char *op2)
{
    if(IsNumber(op2))
        Op(INT_SET_VARIABLE_TO_LITERAL, op1, (SDWORD)CheckMakeNumber(op2));
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
            for(int i = 0; i < s->count; i++)
                InitVarsCircuit(s->contents[i].which, s->contents[i].data.any, n);
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(int i = 0; i < p->count; i++)
                InitVarsCircuit(p->contents[i].which, p->contents[i].data.any, n);
            break;
        }
#ifndef NEW_TON
        case ELEM_TOF: {
            if(n) {
                (*n)++; // counting the number of variables
                return;
            }
            SDWORD period = TimerPeriod(l);
            Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            break;
        }
#endif
#ifdef NEW_TON
        case ELEM_TCY: {
            if(n) {
                (*n)++; // counting the number of variables
                return;
            }
            SDWORD period = TimerPeriod(l) - 1;
            Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            break;
        }
        case ELEM_TON: {
            if(n) {
                (*n)++; // counting the number of variables
                return;
            }
            SDWORD period = TimerPeriod(l);
            Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            break;
        }
#endif
        case ELEM_SEED_RANDOM: {
            if(n) {
                (*n)++; // counting the number of variables
                return;
            }
            char name[MAX_NAME_LEN];
            sprintf(name, "$seed_%s", l->d.readAdc.name);
            Op(INT_SET_VARIABLE_TO_LITERAL, name, rand());
            break;
        }
        case ELEM_CTR:
        case ELEM_CTC:
        case ELEM_CTU:
        case ELEM_CTD: {
            if(n) {
                (*n)++; // counting the number of variables
                return;
            }
            if(IsNumber(l->d.counter.init)) {
                int init = CheckMakeNumber(l->d.counter.init);
                if(!n)
                    Op(INT_SET_VARIABLE_TO_LITERAL, l->d.counter.name, init);
            }
            if(IsNumber(l->d.counter.init) && IsNumber(l->d.counter.max)) {
                int init = CheckMakeNumber(l->d.counter.init);
                int max_ = CheckMakeNumber(l->d.counter.max);
                int b = std::max(byteNeeded(init), byteNeeded(max_));
                if(SizeOfVar(l->d.counter.name) != b)
                    SetSizeOfVar(l->d.counter.name, b);
            }
            break;
        }
#define QUAD_ALGO 2 // 222 // 1, 2, 4, 22, 222 are available
#if QUAD_ALGO == 222
        case ELEM_QUAD_ENCOD: {
            if(n) {
                (*n)+=2; // counting the number of variables
                return;
            }
            char prevA[MAX_NAME_LEN];
            char prevB[MAX_NAME_LEN];
            sprintf(prevA, "$prev_%s", l->d.QuadEncod.inputA);
            sprintf(prevB, "$prev_%s", l->d.QuadEncod.inputB);
            Op(INT_COPY_BIT_TO_BIT, prevA, l->d.QuadEncod.inputA);
            Op(INT_COPY_BIT_TO_BIT, prevB, l->d.QuadEncod.inputB);
            break;
        }
#endif
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
static void InitVars()
{
    int n = 0;
    for(int i = 0; i < Prog.numRungs; i++) {
        InitVarsCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i], &n);
    }
    if(n) {
        Comment("INIT VARS");
        char storeInit[MAX_NAME_LEN];
        GenSymOneShot(storeInit, "INIT", "VARS");
        Op(INT_IF_BIT_CLEAR, storeInit);
        Op(INT_SET_BIT, storeInit);
        for(int i = 0; i < Prog.numRungs; i++) {
            rungNow = i;
            InitVarsCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i], nullptr);
        }
        Op(INT_END_IF);
    }
}

#ifdef TABLE_IN_FLASH
//-----------------------------------------------------------------------------
static void InitTablesCircuit(int which, void *elem)
{
    int       sovElement = 0;
    ElemLeaf *l = (ElemLeaf *)elem;
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(int i = 0; i < s->count; i++)
                InitTablesCircuit(s->contents[i].which, s->contents[i].data.any);
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(int i = 0; i < p->count; i++)
                InitTablesCircuit(p->contents[i].which, p->contents[i].data.any);
            break;
        }
#if QUAD_ALGO <= 4
        case ELEM_QUAD_ENCOD: {
            char nameTable[MAX_NAME_LEN];
            strcpy(nameTable, "ELEM_QUAD_ENCOD");
            int32_t count = 16;
#if QUAD_ALGO == 1
            static int32_t vals[16] = {0, 0,1,0,0,0,0, 0,-1,0,0,0,0,0, 0,0}; // x1
#elif QUAD_ALGO == 2
            static int32_t vals[16] = {0, 0,1,0,0,0,0,-1,-1,0,0,0,0,1, 0,0}; // x2
#elif QUAD_ALGO == 4
            static int32_t vals[16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0}; // x4
#endif
            int32_t sovElement = 1;
            //SetSizeOfVar(nameTable, count);
            Op(INT_FLASH_INIT, nameTable, nullptr, nullptr, count, sovElement, vals);
            break;
        }
#endif
        // case ELEM_PIECEWISE_LINEAR:
        case ELEM_LOOK_UP_TABLE: {
            ElemLookUpTable *t = &(l->d.lookUpTable);

            char nameTable[MAX_NAME_LEN];
            sprintf(nameTable, "%s%s", t->name, ""); // "LutElement");

            int sovElement;

            if((isVarInited(nameTable) < 0) || (isVarInited(nameTable) == rungNow)) {
                sovElement = TestByteNeeded(t->count, t->vals);
                Op(INT_FLASH_INIT, nameTable, nullptr, nullptr, t->count, sovElement, t->vals);
            } else {
                sovElement = SizeOfVar(nameTable);
                if(sovElement < 1)
                    sovElement = 1;
                Comment(_("INIT TABLE: signed %d bit %s[%d] see above"), 8 * sovElement, nameTable, t->count);
            }
            break;
        }
        {
        // clang-format off
        const char *nameTable;
        case ELEM_7SEG:  nameTable = "char7seg";  goto xseg;
        case ELEM_9SEG:  nameTable = "char9seg";  goto xseg;
        case ELEM_14SEG: nameTable = "char14seg"; goto xseg;
        case ELEM_16SEG: nameTable = "char16seg"; goto xseg;
        xseg:
        // clang-format on
            if(!IsNumber(l->d.segments.src)) {
                if((isVarInited(nameTable) < 0)) {
                    if(which == ELEM_7SEG) {
                        sovElement = 1;
                        Op(INT_FLASH_INIT, nameTable, nullptr, nullptr, LEN7SEG, sovElement, char7seg);
                    } else
                        oops();
                    MarkInitedVariable(nameTable);
                } else {
                    Comment(_("INIT TABLE: signed %d bit %s[%d] see above"), 8*sovElement, nameTable, LEN7SEG);
                }
            }
            break;
        }
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
static void InitTables()
{
    if(TablesUsed()) {
        Comment("INIT TABLES");
        for(int i = 0; i < Prog.numRungs; i++) {
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
#define PULSE                           \
    Op(INT_SET_BIT, l->d.stepper.coil); \
    Op(INT_CLEAR_BIT, l->d.stepper.coil);

//-----------------------------------------------------------------------------
bool IsAddrInVar(const char *name)
{
    if(((strstr(name, "#PORT")) && (strlen(name) == 6)) || // #PORTx
       ((strstr(name, "#PIN")) && (strlen(name) == 5)) ||  // #PINx
       ((strstr(name, "#TRIS")) && (strlen(name) == 6)))   // #TRISx
        return false;
    return (name[0] == '#') && (!IsNumber(&name[1]));
}
//-----------------------------------------------------------------------------
// clang-format off
static void IntCodeFromCircuit(int which, void *any, const char *stateInOut, int rung)
{
    const char *stateInOut2 = "$overlap";
    whichNow = which;
    leafNow = (ElemLeaf *)any;
    ElemLeaf *l = (ElemLeaf *)any;
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;

            Comment("start series [");
            for(int i = 0; i < s->count; i++) {
                IntCodeFromCircuit(s->contents[i].which, s->contents[i].data.any, stateInOut, rung);
            }
            Comment("] finish series");
            break;
        }
        case ELEM_PARALLEL_SUBCKT: {
            char parThis[MAX_NAME_LEN];
            char parOut[MAX_NAME_LEN];

            Comment("start parallel [");
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;

            bool ExistEnd = false; //false indicates that it is NEED to calculate the parOut
            for(int i = 0; i < p->count; i++) {
                if(CheckEndOfRungElem(p->contents[i].which, p->contents[i].data.any)) {
                    ExistEnd = true; // true indicates that it is NOT NEED to calculate the parOut
                    break;
                }
            }
            bool CanChange = false; // false indicates that it is NOT NEED to calculate the parThis
            for(int i = 0; i < p->count; i++) {
                if(!CheckStaySameElem(p->contents[i].which, p->contents[i].data.any)) {
                    CanChange = true; // true indicates that it is NEED to calculate the parThis
                    break;
                }
            }

#ifdef DEFAULT_PARALLEL_ALGORITHM
            // Return to default ELEM_PARALLEL_SUBCKT algorithm
            CanChange = true;
            ExistEnd = false;
#endif

            if(ExistEnd == false) {
                GenSymParOut(parOut);

                Op(INT_CLEAR_BIT, parOut);
            }
            if(CanChange) {
                GenSymParThis(parThis);
            }
            for(int i = 0; i < p->count; i++) {
#ifndef DEFAULT_PARALLEL_ALGORITHM
                if(CheckStaySameElem(p->contents[i].which, p->contents[i].data.any))
                    IntCodeFromCircuit(p->contents[i].which, p->contents[i].data.any, stateInOut, rung);
                else
#endif
                {
                    Op(INT_COPY_BIT_TO_BIT, parThis, stateInOut);

                    IntCodeFromCircuit(p->contents[i].which, p->contents[i].data.any, parThis, rung);

                    if(ExistEnd == false) {
                        Op(INT_IF_BIT_SET, parThis);
                          Op(INT_SET_BIT, parOut);
                        Op(INT_END_IF);
                    }
                }
            }
            if(ExistEnd == false) {
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
              Op(INT_ELSE);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);

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
              if(l->d.reset.name[0] == 'P') {
                  Op(INT_PWM_OFF, l->d.reset.name);
                  char s[MAX_NAME_LEN];
                  sprintf(s, "$%s", l->d.reset.name);
                  Op(INT_CLEAR_BIT, s);
              } else if(l->d.reset.name[0] == 'C') {
                  void *v;
                  v = FindElem(ELEM_CTU, l->d.reset.name);
                  if(!v) {
                      v = FindElem(ELEM_CTD, l->d.reset.name);
                      if(!v) {
                          v = FindElem(ELEM_CTC, l->d.reset.name);
                          if(!v) {
                              v = FindElem(ELEM_CTR, l->d.reset.name);
                          }
                      }
                  }
                  if(v) {
                      ElemCounter *c = (ElemCounter *)v;
                      if(IsNumber(c->init))
                         Op(INT_SET_VARIABLE_TO_LITERAL, l->d.reset.name, hobatoi(c->init));
                      else
                         Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.reset.name, c->init);
                  } else
                      Op(INT_SET_VARIABLE_TO_LITERAL, l->d.reset.name, (SDWORD)0);
              } else
                  Op(INT_SET_VARIABLE_TO_LITERAL, l->d.reset.name, (SDWORD)0);
            Op(INT_END_IF);
            break;

        case ELEM_TIME2COUNT: {
            Comment(3, "ELEM_TIME2COUNT");
            if(!IsNumber(l->d.timer.delay))
                Error(_("The TIME to COUNTER converter T2CNT '%S' delay must be a number in ms!"), l->d.timer.name);
            SDWORD period = TimerPeriod(l);
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            Op(INT_END_IF);
            break;
        }
        case ELEM_TCY: {
            Comment(3, "ELEM_TCY %s %s", l->d.timer.name, l->d.timer.delay);
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
              if(IsNumber(l->d.timer.delay)) {
                SDWORD period = TimerPeriod(l);
                Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);
              } else {
                Op(INT_IF_LES, l->d.timer.name, l->d.timer.delay);
              }
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
              Op(INT_IF_BIT_SET, stateInOut); // overlap(0 to -1) flag is true
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
            Comment(3, "ELEM_TON %s %s", l->d.timer.name, l->d.timer.delay);
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

              if(IsNumber(l->d.timer.delay)) {
                SDWORD period = TimerPeriod(l);
                Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);
              } else {
                Op(INT_IF_LES, l->d.timer.name, l->d.timer.delay);
              }
                  Op(INT_CLEAR_BIT, stateInOut);               //1
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
                Op(INT_IF_BIT_SET, stateInOut); // overlap(0 to -1) flag is true
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

              if(IsNumber(l->d.timer.delay)) {
                SDWORD period = TimerPeriod(l);
                Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);
              } else {
                Op(INT_IF_LES, l->d.timer.name, l->d.timer.delay);
              }
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
              Op(INT_IF_BIT_SET, store); // overlap(0 to -1) flag is true
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
              if(IsNumber(l->d.timer.delay)) {
                SDWORD period = TimerPeriod(l);
                Op(INT_IF_LES, l->d.timer.name, period);
              } else {
                Op(INT_IF_LES, l->d.timer.name, l->d.timer.delay);
              }
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
            GenSymOneShot(storeNameHi, "ONE_SHOT_HI", l->d.timer.name);

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
              if(IsNumber(l->d.timer.delay)) {
                SDWORD period = TimerPeriod(l);
                Op(INT_IF_LES, l->d.timer.name, period);
              } else {
                Op(INT_IF_LES, l->d.timer.name, l->d.timer.delay);
              }
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

            if(l->d.counter.inputKind == '/') {
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_IF_BIT_CLEAR, storeName);
                    Op(INT_SET_BIT, storeName);
                      Op(INT_IF_LES, l->d.counter.name, l->d.counter.max);
                        Op(INT_INCREMENT_VARIABLE, l->d.counter.name);
                      Op(INT_END_IF);
                  Op(INT_END_IF);
                Op(INT_ELSE);
                  Op(INT_CLEAR_BIT, storeName);
                Op(INT_END_IF);
            } else if(l->d.counter.inputKind == '\\') {
                Op(INT_IF_BIT_CLEAR, stateInOut);
                  Op(INT_IF_BIT_SET, storeName);
                    Op(INT_CLEAR_BIT, storeName);
                      Op(INT_IF_LES, l->d.counter.name, l->d.counter.max);
                        Op(INT_INCREMENT_VARIABLE, l->d.counter.name);
                      Op(INT_END_IF);
                  Op(INT_END_IF);
                Op(INT_ELSE);
                  Op(INT_SET_BIT, storeName);
                Op(INT_END_IF);
            } else if(l->d.counter.inputKind == '-') {
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_IF_LES, l->d.counter.name, l->d.counter.max);
                    Op(INT_INCREMENT_VARIABLE, l->d.counter.name);
                  Op(INT_END_IF);
                Op(INT_END_IF);
            } else if(l->d.counter.inputKind == 'o') {
                Op(INT_IF_BIT_CLEAR, stateInOut);
                  Op(INT_IF_LES, l->d.counter.name, l->d.counter.max);
                    Op(INT_INCREMENT_VARIABLE, l->d.counter.name);
                  Op(INT_END_IF);
                Op(INT_END_IF);
            } else oops();

            Op(INT_IF_LES, l->d.counter.name, l->d.counter.max);
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

            if(l->d.counter.inputKind == '/') {
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_IF_BIT_CLEAR, storeName);
                    Op(INT_SET_BIT, storeName);
                    Op(INT_IF_GRT, l->d.counter.name, l->d.counter.max);
                      Op(INT_DECREMENT_VARIABLE, l->d.counter.name);
                    Op(INT_END_IF);
                  Op(INT_END_IF);
                Op(INT_ELSE);
                  Op(INT_CLEAR_BIT, storeName);
                Op(INT_END_IF);
            } else if(l->d.counter.inputKind == '\\') {
                Op(INT_IF_BIT_CLEAR, stateInOut);
                  Op(INT_IF_BIT_SET, storeName);
                    Op(INT_CLEAR_BIT, storeName);
                    Op(INT_IF_GRT, l->d.counter.name, l->d.counter.max);
                      Op(INT_DECREMENT_VARIABLE, l->d.counter.name);
                    Op(INT_END_IF);
                  Op(INT_END_IF);
                Op(INT_ELSE);
                  Op(INT_SET_BIT, storeName);
                Op(INT_END_IF);
            } else if(l->d.counter.inputKind == '-') {
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_IF_GRT, l->d.counter.name, l->d.counter.max);
                    Op(INT_DECREMENT_VARIABLE, l->d.counter.name);
                  Op(INT_END_IF);
                Op(INT_END_IF);
            } else if(l->d.counter.inputKind == 'o') {
                Op(INT_IF_BIT_CLEAR, stateInOut);
                  Op(INT_IF_GRT, l->d.counter.name, l->d.counter.max);
                    Op(INT_DECREMENT_VARIABLE, l->d.counter.name);
                  Op(INT_END_IF);
                Op(INT_END_IF);
            } else oops();

            Op(INT_IF_GRT, l->d.counter.name, l->d.counter.max);
              Op(INT_SET_BIT, stateInOut);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
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

            if(l->d.counter.inputKind == '/') {
              Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_CLEAR_BIT, stateInOut);
                Op(INT_IF_BIT_CLEAR, storeName);
                  Op(INT_SET_BIT, storeName);
                  Op(INT_DECREMENT_VARIABLE, l->d.counter.name);

                  //Use max as min, and init as max
                  // -5 --> -10
                  // ^init  ^max
                  Op(INT_IF_LES, l->d.counter.name, l->d.counter.max);
                    OpSetVar(l->d.counter.name, l->d.counter.init);
                    Op(INT_SET_BIT, stateInOut); // overload impulse
                  Op(INT_END_IF);
                Op(INT_END_IF);
              Op(INT_ELSE);
                Op(INT_CLEAR_BIT, storeName);
              Op(INT_END_IF);
            } else if(l->d.counter.inputKind == '\\') {
              Op(INT_IF_BIT_CLEAR, stateInOut);
                Op(INT_IF_BIT_SET, storeName);
                  Op(INT_CLEAR_BIT, storeName);
                  Op(INT_DECREMENT_VARIABLE, l->d.counter.name);

                  Op(INT_IF_LES, l->d.counter.name, l->d.counter.max);
                    OpSetVar(l->d.counter.name, l->d.counter.init);
                    Op(INT_SET_BIT, stateInOut); // overload impulse
                  Op(INT_END_IF);
                Op(INT_END_IF);
              Op(INT_ELSE);
                Op(INT_SET_BIT, storeName);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);
            } else if(l->d.counter.inputKind == '-') {
              Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_DECREMENT_VARIABLE, l->d.counter.name);

                Op(INT_IF_LES, l->d.counter.name, l->d.counter.max);
                  OpSetVar(l->d.counter.name, l->d.counter.init);
                  // overload impulse
                Op(INT_ELSE);
                  Op(INT_CLEAR_BIT, stateInOut);
                Op(INT_END_IF);
              Op(INT_END_IF);
            } else if(l->d.counter.inputKind == 'o') {
              Op(INT_IF_BIT_CLEAR, stateInOut);
                Op(INT_DECREMENT_VARIABLE, l->d.counter.name);

                Op(INT_IF_LES, l->d.counter.name, l->d.counter.max);
                  OpSetVar(l->d.counter.name, l->d.counter.init);
                  Op(INT_SET_BIT, stateInOut); // overload impulse
                Op(INT_END_IF);
              Op(INT_ELSE);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);
            } else oops();
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

            if(l->d.counter.inputKind == '/') {
              Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_CLEAR_BIT, stateInOut);
                Op(INT_IF_BIT_CLEAR, storeName);
                  Op(INT_SET_BIT, storeName); // This line1
                  Op(INT_INCREMENT_VARIABLE, l->d.counter.name);

                  Op(INT_IF_GRT, l->d.counter.name, l->d.counter.max);
                    OpSetVar(l->d.counter.name, l->d.counter.init);
                    Op(INT_SET_BIT, stateInOut); // overload impulse
                  Op(INT_END_IF);
                Op(INT_END_IF);
              Op(INT_ELSE);
                Op(INT_CLEAR_BIT, storeName); // This line2
              Op(INT_END_IF);
          ////Op(INT_COPY_BIT_TO_BIT, storeName, stateInOut); // This line3 equivalently line1 + line2
            } else if(l->d.counter.inputKind == '\\') {
              Op(INT_IF_BIT_CLEAR, stateInOut);
                Op(INT_IF_BIT_SET, storeName);
                  Op(INT_CLEAR_BIT, storeName);
                  Op(INT_INCREMENT_VARIABLE, l->d.counter.name);

                  Op(INT_IF_GRT, l->d.counter.name, l->d.counter.max);
                    OpSetVar(l->d.counter.name, l->d.counter.init);
                    Op(INT_SET_BIT, stateInOut); // overload impulse
                  Op(INT_END_IF);
                Op(INT_END_IF);
              Op(INT_ELSE);
                Op(INT_SET_BIT, storeName);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);
            } else if(l->d.counter.inputKind == '-') {
              Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_INCREMENT_VARIABLE, l->d.counter.name);

                Op(INT_IF_GRT, l->d.counter.name, l->d.counter.max);
                  OpSetVar(l->d.counter.name, l->d.counter.init);
                  // overload impulse
                Op(INT_ELSE);
                  Op(INT_CLEAR_BIT, stateInOut);
                Op(INT_END_IF);
              Op(INT_END_IF);
            } else if(l->d.counter.inputKind == 'o') {
              Op(INT_IF_BIT_CLEAR, stateInOut);
                Op(INT_INCREMENT_VARIABLE, l->d.counter.name);

                Op(INT_IF_GRT, l->d.counter.name, l->d.counter.max);
                  OpSetVar(l->d.counter.name, l->d.counter.init);
                  Op(INT_SET_BIT, stateInOut); // overload impulse
                Op(INT_END_IF);
              Op(INT_ELSE);
                Op(INT_CLEAR_BIT, stateInOut);
              Op(INT_END_IF);
            } else oops();
            break;
        }
        //-------------------------------------------------------------------
        #ifdef USE_SFR
        // Special Function
        case ELEM_RSFR:
            Comment(3, "ELEM_RSFR");
            if(IsNumber(l->d.move.dest)) {
                THROW_COMPILER_EXCEPTION_FMT(_("Read SFR instruction: '%s' not a valid destination."), l->d.move.dest);
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
                    Op(INT_WRITE_SFR_LITERAL_L,
                       nullptr,
                       nullptr,
                       nullptr,
                       CheckMakeNumber(l->d.sfr.sfr),
                       CheckMakeNumber(l->d.sfr.op));
                } else {
                    Op(INT_WRITE_SFR_VARIABLE_L, l->d.sfr.sfr, CheckMakeNumber(l->d.sfr.op));
                }
            } else {
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
                    Op(INT_SET_SFR_LITERAL_L,
                       nullptr,
                       nullptr,
                       nullptr,
                       CheckMakeNumber(l->d.move.src),
                       CheckMakeNumber(l->d.move.dest));
                } else {
                    Op(INT_SET_SFR_VARIABLE_L, l->d.move.src, CheckMakeNumber(l->d.move.dest));
                }
            } else {
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
                    Op(INT_CLEAR_SFR_LITERAL_L,
                       nullptr,
                       nullptr,
                       nullptr,
                       CheckMakeNumber(l->d.move.src),
                       CheckMakeNumber(l->d.move.dest));
                } else {
                    Op(INT_CLEAR_SFR_VARIABLE_L, l->d.move.src, CheckMakeNumber(l->d.move.dest));
                }
            } else {
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
                    Op(INT_TEST_SFR_LITERAL_L,
                       nullptr,
                       nullptr,
                       nullptr,
                       CheckMakeNumber(l->d.move.src),
                       CheckMakeNumber(l->d.move.dest));
                } else {
                    Op(INT_TEST_SFR_VARIABLE_L, l->d.move.src, CheckMakeNumber(l->d.move.dest));
                }
            } else {
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
                    Op(INT_TEST_C_SFR_LITERAL_L,
                       nullptr,
                       nullptr,
                       nullptr,
                       CheckMakeNumber(l->d.move.src),
                       CheckMakeNumber(l->d.move.dest));
                } else {
                    Op(INT_TEST_C_SFR_VARIABLE_L, l->d.move.src, CheckMakeNumber(l->d.move.dest));
                }
            } else {
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
              } else
                  THROW_COMPILER_EXCEPTION(_("Internal error."));
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
        // clang-format off
        int deg, len;
        case ELEM_7SEG:  Comment(3, stringer(ELEM_7SEG));  deg=DEGREE7;  len=LEN7SEG;  goto xseg;
        case ELEM_9SEG:  Comment(3, stringer(ELEM_9SEG));  deg=DEGREE9;  len=LEN9SEG;  goto xseg;
        case ELEM_14SEG: Comment(3, stringer(ELEM_14SEG)); deg=DEGREE14; len=LEN14SEG; goto xseg;
        case ELEM_16SEG: Comment(3, stringer(ELEM_16SEG)); deg=DEGREE16; len=LEN16SEG; goto xseg;
        xseg :
        // clang-format on
#ifdef TABLE_IN_FLASH
            if(IsNumber(l->d.segments.dest)) {
                THROW_COMPILER_EXCEPTION_FMT(_("Segments instruction: '%s' not a valid destination."), l->d.segments.dest);
            }
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.segments.src)) {
                int Xseg = CheckMakeNumber(l->d.segments.src);
                if(Xseg == DEGREE_CHAR)
                    Xseg = deg;
                else if((Xseg < 0x00) || (len <= Xseg))
                    Xseg = ' ';
                switch(which) {
                    case ELEM_7SEG:
                        Xseg = char7seg[Xseg];
                        break;
                    default:
                        oops();
                }
                char s[MAX_NAME_LEN];
                sprintf(s, "0x%X", Xseg);
                CheckVarInRange(l->d.segments.dest, s, Xseg);
                if(l->d.segments.common == 'A')
                    Op(INT_SET_VARIABLE_NOT, l->d.segments.dest, Xseg);
                else
                    Op(INT_SET_VARIABLE_TO_LITERAL, l->d.segments.dest, Xseg);
            } else {
                char *Xseg = l->d.segments.src;

                char nameTable[MAX_NAME_LEN];
                int  sovElement = 0;
                /**/
                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", DEGREE_CHAR);
                Op(INT_IF_VARIABLE_EQUALS_VARIABLE, Xseg, "$scratch");
                  Op(INT_SET_VARIABLE_TO_LITERAL, Xseg, (SDWORD)deg);
                  Op(INT_ELSE);
                    Op(INT_IF_VARIABLE_LES_LITERAL, Xseg, (SDWORD)0x00);
                      Op(INT_SET_VARIABLE_TO_LITERAL, Xseg, (SDWORD)0x20); // ' '
                    Op(INT_ELSE);
                      Op(INT_IF_VARIABLE_LES_LITERAL, Xseg, len);
                      Op(INT_ELSE);
                        Op(INT_SET_VARIABLE_TO_LITERAL, Xseg, (SDWORD)0x20); // ' '
                      Op(INT_END_IF);
                    Op(INT_END_IF);
                Op(INT_END_IF);
                /**/
                switch(which) {
                    case ELEM_7SEG:
                        strcpy(nameTable, "char7seg");
                        sovElement = 1;
                        Op(INT_FLASH_READ, "$scratch", nameTable, Xseg, LEN7SEG, sovElement, char7seg);
                        break;
                    default:
                        oops();
                }
                if(l->d.segments.common != 'C')
                    Op(INT_SET_VARIABLE_NOT, "$scratch", "$scratch");
                Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.segments.dest, "$scratch");
            }
            Op(INT_END_IF);
#endif
            break;
        }
        case ELEM_STEPPER: {
            Comment(3, "ELEM_STEPPER");
            // Pulse generator for STEPPER motor with acceleration and deceleration.
            ElemStepper *s = &l->d.stepper;
            ResSteps     r;
            CalcSteps(s, &r);

            int speed = 4; //ðàçãîí ïî òàáëèöå Pmul=P*Tmul >> shrt; ñ ðàñ÷åòîì
            if(IsNumber(l->d.stepper.P) || (l->d.stepper.graph <= 0)) {
                if((l->d.stepper.n <= 1) || (l->d.stepper.graph <= 0)) {
                    if(CheckMakeNumber(l->d.stepper.P) == 1)
                        speed = 1; //èìïóëüñû íà êàæäûé Tcycle
                    else
                        speed = 2; //èìïóëüñû íà êàæäûé Tcycle*P
                } else {
                    speed = 3; //ðàçãîí ïî òàáëèöå Pmul=P*Tmul >> shrt; óæå â êîíñòàíòàõ
                }
            }
            char decCounter[MAX_NAME_LEN];
            sprintf(decCounter, "C%s%s", l->d.stepper.name, "Dec");
            SetSizeOfVar(decCounter, byteNeeded(CheckMakeNumber(l->d.stepper.max)));

            char incCounter[MAX_NAME_LEN];
            sprintf(incCounter, "C%s%s", l->d.stepper.name, "Inc");
            SetSizeOfVar(incCounter, SizeOfVar(decCounter));

            char workP[MAX_NAME_LEN];
            sprintf(workP, "C%s%s", l->d.stepper.name, "P");
            SetSizeOfVar(workP, std::max(r.sovElement, SizeOfVar(l->d.stepper.P))); //may bee overload

            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "STEPPER", "");

            char Tmul[MAX_NAME_LEN];
            GenSymStepper(Tmul, "Tmul");

            char nameTable[MAX_NAME_LEN];
            if(speed >= 3) {
                sprintf(nameTable, "%s%s", l->d.stepper.name, ""); // "LutElement");
                //SizeOfVar(nameTable, max(r.sovElement, SizeOfVar(nameTable)));
                SetSizeOfVar(nameTable, std::max(r.sovElement, 1));
                r.sovElement = std::max(r.sovElement, SizeOfVar(nameTable));
                SetSizeOfVar(Tmul, r.sovElement);
                Tdata = (SDWORD *)CheckMalloc((l->d.stepper.n + 2) * sizeof(SDWORD)); //+2 Ok
                // CheckFree(Tdata) in WipeIntMemory();

                for(int i = 0; i < l->d.stepper.n; i++) {
                    Tdata[i] = r.T[i+1].dtMul; // Tdata from 0 to n-1 // r.T from 1 to n // Ok
                }
            }
            //
            if(speed >= 3) {
                //IJMP inside INT_FLASH_INIT

                if((isVarInited(nameTable) < 0) || (isVarInited(nameTable) == rungNow)) {
                    Op(INT_FLASH_INIT, nameTable, nullptr, nullptr, l->d.stepper.n, r.sovElement, Tdata);
                } else {
                    Comment(_("INIT TABLE: signed %d bit %s[%d] see above"), 8 * r.sovElement, nameTable);
                }
            }
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_IF_BIT_CLEAR, storeName);
                Op(INT_SET_BIT, storeName);
                // Ýòîò ôðàãìåíò êîäà âûïîëíÿåòñÿ 1 ðàç ïðè ïåðåêëþ÷åíèè 0->1.
                // This code fragment is executed 1 time when switching 0->1.
                Op(INT_IF_VARIABLE_LES_LITERAL, decCounter, (SDWORD)1); // § ¯à¥â ¯®¢â®à­®© § £àã§ª¨ - ¯®¢â®à­®£® à¥áâ àâ
                  OpSetVar(decCounter, l->d.stepper.max);

                  if(speed == 2) {
                    Op(INT_SET_VARIABLE_TO_LITERAL, workP, (SDWORD)1);

                  } else if(speed >= 3) {
                    Op(INT_SET_VARIABLE_TO_LITERAL, workP, (SDWORD)1);
                    Op(INT_SET_VARIABLE_TO_LITERAL, incCounter, (SDWORD)(0));
                    //vvv
                    //Op(INT_SET_VARIABLE_TO_LITERAL, Tmul, l->d.stepper.n);
                    char strn[20];
                    sprintf(strn, "%d", l->d.stepper.n - 1);
                    Op(INT_FLASH_READ, Tmul, nameTable, strn, l->d.stepper.n - 1, r.sovElement, Tdata);
                  }
                Op(INT_END_IF);
              Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, storeName);
            Op(INT_END_IF);
            //
            Op(INT_IF_VARIABLE_LES_LITERAL, decCounter, (SDWORD)1);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_ELSE);
              Op(INT_SET_BIT, stateInOut);
                if(speed == 1) {
                  //PULSE
                  Op(INT_SET_BIT, l->d.stepper.coil);
                  Op(INT_DECREMENT_VARIABLE, decCounter);
                  Op(INT_CLEAR_BIT, l->d.stepper.coil);

                } else if(speed == 2) {
                  Op(INT_DECREMENT_VARIABLE, workP);
                  Op(INT_IF_VARIABLE_LES_LITERAL, workP, (SDWORD)1);
                    //PULSE
                    Op(INT_SET_BIT, l->d.stepper.coil);
                    Op(INT_DECREMENT_VARIABLE, decCounter);
                    Op(INT_CLEAR_BIT, l->d.stepper.coil);
                    OpSetVar(workP, l->d.stepper.P);
                    //Op(INT_ELSE);
                    //Op(INT_DECREMENT_VARIABLE, workP);
                  Op(INT_END_IF);

                } else if(speed >= 3) {
                  Op(INT_DECREMENT_VARIABLE, workP);
                  Op(INT_IF_VARIABLE_LES_LITERAL, workP, (SDWORD)1);
                    //PULSE
                    Op(INT_SET_BIT, l->d.stepper.coil);
                    //Op(INT_DECREMENT_VARIABLE, decCounter); //n downto 1
                    //Op(INT_INCREMENT_VARIABLE, incCounter); //0 up to n-1
                  Op(INT_END_IF);
                } else {
                  oops();
                }
              Op(INT_END_IF);

              if(speed >= 3) {
                  Op(INT_IF_BIT_SET, l->d.stepper.coil);
                    Op(INT_CLEAR_BIT, l->d.stepper.coil);
                    Op(INT_IF_VARIABLE_LES_LITERAL, decCounter, (SDWORD)l->d.stepper.n); // DEC is more then INC
                      Op(INT_FLASH_READ, workP, nameTable, decCounter, l->d.stepper.n, r.sovElement, Tdata);
                    Op(INT_ELSE);
                      Op(INT_IF_VARIABLE_LES_LITERAL, incCounter, (SDWORD)l->d.stepper.n);
                        Op(INT_FLASH_READ, workP, nameTable, incCounter, l->d.stepper.n, r.sovElement, Tdata);
                      Op(INT_ELSE);
                        OpSetVar(workP, Tmul);
                      Op(INT_END_IF);
                    Op(INT_END_IF);
                    Op(INT_INCREMENT_VARIABLE, incCounter); //0 up to n-1
                    Op(INT_DECREMENT_VARIABLE, decCounter); //n-1 downto 0
                    if(speed == 3) {
                      //OpSetVar(workP,Tmul);
                    } else {
                      //Op(INT_SET_VARIABLE_MULTIPLY, workP, Tmul, l->d.stepper.P); //may bee overload
                      Op(INT_SET_VARIABLE_MULTIPLY, workP, workP, l->d.stepper.P); //may bee overload
                      if(r.shrt) {
                        char rshrt[MAX_NAME_LEN];
                        sprintf(rshrt, "%d", r.shrt);
                        Op(INT_SET_VARIABLE_SHR, workP, workP, rshrt);
                      }
                    }
                  Op(INT_END_IF);
              }
            CheckFree(r.T);
            break;
        }
        case ELEM_PULSER: {
            Comment(3, "ELEM_PULSER");
            // Variable duty cycle pulse generator.
            // ƒ¥­¥à â®à ¨¬¯ã«ìá®¢ ¯¥à¥¬¥­­®© áª¢ ¦­®áâ¨.
            char decCounter[MAX_NAME_LEN];
            GenSymStepper(decCounter, "decCounter");
            char workT1[MAX_NAME_LEN];
            GenSymStepper(workT1, "workT1");
            char workT0[MAX_NAME_LEN];
            GenSymStepper(workT0, "workT0");
            char doSetT[MAX_NAME_LEN];
            GenSymStepper(doSetT, "doSetT");
            char T1mul[MAX_NAME_LEN];
            GenSymStepper(T1mul, "T1mul");
            char T0mul[MAX_NAME_LEN];
            GenSymStepper(T0mul, "T0mul");

            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "PULSER", "");
            char Osc[MAX_NAME_LEN];
            GenSymOneShot(Osc, "PULSER", "Osc");
            /*
            char busy[MAX_NAME_LEN];
            GenSymOneShot(busy);
            char OneShot1[MAX_NAME_LEN];
            GenSymOneShot(OneShot1);
            char OneShot0[MAX_NAME_LEN];
            GenSymOneShot(OneShot0);
            */
            int Meander = 0;
            if(IsNumber(l->d.pulser.P1) && IsNumber(l->d.pulser.P0)) {
                if((CheckMakeNumber(l->d.pulser.P1) == 1) && (CheckMakeNumber(l->d.pulser.P0) == 1)) {
                    Meander = 11;
                    //dbp("meander11");
                } else if(CheckMakeNumber(l->d.pulser.P1) == CheckMakeNumber(l->d.pulser.P0)) {
                    Meander = 2;
                    //dbp("meander2");
                }
            }

            //if(!Meander11) {
            const char *P1 = VarFromExpr(l->d.pulser.P1, "$scratch11");
            const char *P0 = VarFromExpr(l->d.pulser.P0, "$scratch12");
            const char *accel = VarFromExpr(l->d.pulser.accel, "$scratch13");
            //}
            const char *counter = VarFromExpr(l->d.pulser.counter, "$scratch14");
            const char *busy = l->d.pulser.busy;

            Op(INT_IF_BIT_SET, stateInOut);
            Op(INT_IF_BIT_CLEAR, storeName);
            Op(INT_SET_BIT, storeName);
            // â®â äà £¬¥­â ª®¤  ¢ë¯®«­ï¥âáï 1 à § ¯à¨ ¯¥à¥ª«îç¥­¨¨ 0->1.
            // This code fragment is executed 1 time when switching 0->1.
            Op(INT_SET_BIT, busy);
            Op(INT_SET_BIT, Osc);
            Op(INT_SET_VARIABLE_TO_VARIABLE, decCounter, counter);
            if(Meander < 11) {
                Op(INT_CLEAR_BIT, doSetT);
                Op(INT_SET_VARIABLE_MULTIPLY, T1mul, P1, accel);
                Op(INT_SET_VARIABLE_TO_VARIABLE, workT1, T1mul);
                if(Meander < 2) {
                    Op(INT_SET_VARIABLE_MULTIPLY, T0mul, P0, accel);
                    Op(INT_SET_VARIABLE_TO_VARIABLE, workT0, T0mul);
                }
            }
            Op(INT_END_IF);
            Op(INT_ELSE);
            Op(INT_CLEAR_BIT, storeName);
            Op(INT_END_IF);
            //Op(INT_COPY_BIT_TO_BIT, storeName, stateInOut);
            //
            Op(INT_IF_VARIABLE_LES_LITERAL, decCounter, (SDWORD)1);
            Op(INT_IF_BIT_SET, stateInOut);
            Op(INT_SET_VARIABLE_TO_VARIABLE, decCounter, counter);
            Op(INT_ELSE);
            Op(INT_CLEAR_BIT, busy);
            Op(INT_END_IF);
            Op(INT_ELSE);
            if(Meander == 11) {
                Op(INT_IF_BIT_SET, Osc);
                Op(INT_SET_BIT, stateInOut); // 1
                Op(INT_CLEAR_BIT, Osc);
                Op(INT_ELSE);
                Op(INT_CLEAR_BIT, stateInOut); // 1
                Op(INT_SET_BIT, Osc);

                Op(INT_DECREMENT_VARIABLE, decCounter);
                Op(INT_END_IF);
            } else if(Meander == 2) {
                Op(INT_IF_BIT_SET, busy);
                /*
                  Op(INT_IF_BIT_SET, Osc);
                    Op(INT_SET_BIT, stateInOut);// 1
                  Op(INT_ELSE);
                    Op(INT_CLEAR_BIT, stateInOut);// 1
                  Op(INT_END_IF);
                  */
                Op(INT_COPY_BIT_TO_BIT, stateInOut, Osc);
                //
                Op(INT_DECREMENT_VARIABLE, workT1);
                Op(INT_IF_VARIABLE_LES_LITERAL, workT1, (SDWORD)1);
                Op(INT_IF_BIT_SET, Osc);
                Op(INT_CLEAR_BIT, Osc);

                Op(INT_DECREMENT_VARIABLE, decCounter);
                Op(INT_ELSE);
                Op(INT_SET_BIT, Osc);
                Op(INT_END_IF);
                //
                //Op(INT_SET_BIT, doSetT);
                Op(INT_IF_VARIABLE_GRT_VARIABLE, T1mul, P1);
                Op(INT_DECREMENT_VARIABLE, T1mul);
                Op(INT_END_IF);
                Op(INT_SET_VARIABLE_TO_VARIABLE, workT1, T1mul);
                Op(INT_END_IF);
                Op(INT_END_IF);
            } else { // (Meander==0)
                Op(INT_DECREMENT_VARIABLE, workT1);
                Op(INT_IF_VARIABLE_LES_LITERAL, workT1, (SDWORD)0);
                Op(INT_CLEAR_BIT, stateInOut); // 1
                Op(INT_DECREMENT_VARIABLE, workT0);

                Op(INT_IF_VARIABLE_LES_LITERAL, workT0, (SDWORD)1);
                Op(INT_DECREMENT_VARIABLE, decCounter);
                //
                Op(INT_SET_BIT, doSetT);
                /*
                        Op(INT_IF_VARIABLE_GRT_VARIABLE, T1mul, P1);
                          Op(INT_DECREMENT_VARIABLE, T1mul);
                        Op(INT_END_IF);
                        Op(INT_SET_VARIABLE_TO_VARIABLE, workT1, T1mul);

                        Op(INT_IF_VARIABLE_GRT_VARIABLE, T0mul, P0);
                          Op(INT_DECREMENT_VARIABLE, T0mul);
                        Op(INT_END_IF);
                        Op(INT_SET_VARIABLE_TO_VARIABLE, workT0, T0mul);
                        /**/
                //Op(INT_ELSE);
                //    Op(INT_CLEAR_BIT, OneShot0);
                Op(INT_END_IF);
                Op(INT_ELSE);
                Op(INT_SET_BIT, stateInOut); // 1
                //Op(INT_CLEAR_BIT, OneShot1);
                Op(INT_END_IF);
            }
            Op(INT_END_IF);
            //
            if(Meander < 11) {
                Op(INT_IF_BIT_SET, doSetT);
                Op(INT_IF_VARIABLE_GRT_VARIABLE, T1mul, P1);
                Op(INT_DECREMENT_VARIABLE, T1mul);
                Op(INT_END_IF);
                Op(INT_SET_VARIABLE_TO_VARIABLE, workT1, T1mul);

                if(Meander == 0) {
                    Op(INT_IF_VARIABLE_GRT_VARIABLE, T0mul, P0);
                    Op(INT_DECREMENT_VARIABLE, T0mul);
                    Op(INT_END_IF);
                    Op(INT_SET_VARIABLE_TO_VARIABLE, workT0, T0mul);
                }
                Op(INT_CLEAR_BIT, doSetT);
                Op(INT_END_IF);
            }
            break;
        }

        case ELEM_MOVE: {
            Comment(3, "ELEM_MOVE");
            if(IsNumber(l->d.move.dest)) {
                THROW_COMPILER_EXCEPTION_FMT(_("Move instruction: '%s' not a valid destination."), l->d.move.dest);
            }
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.src)) {
                CheckVarInRange(l->d.move.dest, l->d.move.src, CheckMakeNumber(l->d.move.src));
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.move.dest, hobatoi(l->d.move.src));
            } else {
                Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.move.dest, l->d.move.src);
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
                THROW_COMPILER_EXCEPTION_FMT(_("BIN2BCD instruction: '%s' not a valid destination."), l->d.move.dest);
            }
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SET_BIN2BCD, l->d.move.dest, l->d.move.src);
            Op(INT_END_IF);
            break;
        }

        case ELEM_BCD2BIN: {
            Comment(3, "ELEM_BCD2BIN");
            if(IsNumber(l->d.move.dest)) {
                THROW_COMPILER_EXCEPTION_FMT(_("BCD2BIN instruction: '%s' not a valid destination."), l->d.move.dest);
            }
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SET_BCD2BIN, l->d.move.dest, l->d.move.src);
            Op(INT_END_IF);
            break;
        }

        case ELEM_OPPOSITE: {
            Comment(3, "ELEM_OPPOSITE");
            if(IsNumber(l->d.move.dest)) {
                THROW_COMPILER_EXCEPTION_FMT(_("OPPOSITE instruction: '%s' not a valid destination."), l->d.move.dest);
            }
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SET_OPPOSITE, l->d.move.dest, l->d.move.src);
            Op(INT_END_IF);
            break;
        }

        case ELEM_SWAP: {
            Comment(3, "ELEM_SWAP");
            if(IsNumber(l->d.move.dest)) {
                THROW_COMPILER_EXCEPTION_FMT(_("SWAP instruction: '%s' not a valid destination."), l->d.move.dest);
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
                THROW_COMPILER_EXCEPTION_FMT(_("SRAND instruction: '%s' not a valid destination."), l->d.move.dest);
            }
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.src)) {
                CheckVarInRange(name, l->d.move.src, CheckMakeNumber(l->d.move.src));
                Op(INT_SET_VARIABLE_TO_LITERAL, name, hobatoi(l->d.move.src));
            } else {
                Op(INT_SET_VARIABLE_TO_VARIABLE, name, l->d.move.src);
            }
            Op(INT_SET_SEED_RANDOM, name);
            Op(INT_END_IF);
            break;

        // These ELEM's are highly processor-dependent; the int code op does
        // most of the highly specific work
        {
        case ELEM_READ_ADC:
            Comment(3, "ELEM_READ_ADC");
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_READ_ADC, l->d.readAdc.name, l->d.readAdc.refs);
            Op(INT_END_IF);
            break;

        case ELEM_SET_PWM: {
            McuIoPinInfo *iop = PinInfoForName(l->d.setPwm.name);
            if(iop) {
                McuPwmPinInfo *ioPWM = PwmPinInfo(iop->pin);
                if(ioPWM && (ioPWM->timer == Prog.cycleTimer)) {
                    Error(_("PWM '%s' and  PLC cycle timer can not use the same timer %d!\nChange the PLC cycle timer to 0.\nMenu Settings->MCU parameters..."), l->d.setPwm.name, Prog.cycleTimer);
                }
            }
            Comment(3, "ELEM_SET_PWM");
            char s[MAX_NAME_LEN];
            sprintf(s, "$%s", l->d.setPwm.name);
            Op(INT_IF_BIT_SET, stateInOut);
              // ugh; need a >16 bit literal though, could be >64 kHz
              Op(INT_SET_PWM, l->d.setPwm.duty_cycle, l->d.setPwm.targetFreq, l->d.setPwm.name, l->d.setPwm.resolution);
              Op(INT_SET_BIT, s);
            Op(INT_END_IF);
            SimState(&(l->poweredAfter), s, &(l->workingNow), s);
            break;
        }
        case ELEM_NPULSE: {
            Comment(3, "ELEM_NPULSE");
            const char *counter = VarFromExpr(l->d.Npulse.counter, "$scratch");
            Op(INT_SET_NPULSE, counter, l->d.Npulse.targetFreq, l->d.Npulse.coil, stateInOut);
            break;
        }

        case ELEM_NPULSE_OFF: {
            Comment(3, "ELEM_NPULSE_OFF");
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_OFF_NPULSE);
            Op(INT_END_IF);
            break;
        }
        case ELEM_QUAD_ENCOD: {
            Comment(3, "ELEM_QUAD_ENCOD");
            /**/
            char nowA[MAX_NAME_LEN];
            char nowB[MAX_NAME_LEN];
            char prevA[MAX_NAME_LEN];
            char prevB[MAX_NAME_LEN];
            sprintf(nowA, "$now_%s", l->d.QuadEncod.inputA);
            sprintf(nowB, "$now_%s", l->d.QuadEncod.inputB);
            sprintf(prevA, "$prev_%s", l->d.QuadEncod.inputA);
            sprintf(prevB, "$prev_%s", l->d.QuadEncod.inputB);
            /**/
            char nowZ[MAX_NAME_LEN];
            char prevZ[MAX_NAME_LEN];
            sprintf(nowZ, "$now_%s", l->d.QuadEncod.inputZ);
            sprintf(prevZ, "$prev_%s", l->d.QuadEncod.inputZ);

            char pins[MAX_NAME_LEN];
            sprintf(pins, "$pins_%s", l->d.QuadEncod.counter);
            SetSizeOfVar(pins, 1);

            char state[MAX_NAME_LEN];
            sprintf(state, "$state_%s", l->d.QuadEncod.counter);
            SetSizeOfVar(state, 1);

            char dir[MAX_NAME_LEN];
            sprintf(dir, "$dir_%s", l->d.QuadEncod.counter);
            SetSizeOfVar(dir, 1);

            char revol[MAX_NAME_LEN];
            sprintf(revol, "$revol_%s", l->d.QuadEncod.counter);
            SetSizeOfVar(revol, SizeOfVar(l->d.QuadEncod.counter));

            char ticks[MAX_NAME_LEN];
            sprintf(ticks, "$ticks_%s", l->d.QuadEncod.counter);
            SetSizeOfVar(ticks, byteNeeded(l->d.QuadEncod.countPerRevol));

            DWORD addr1 = -1, addr2 = -1;
            int   bit1 = -1, bit2 = -1;
            MemForSingleBit(l->d.QuadEncod.inputA, true, &addr1, &bit1);
            MemForSingleBit(l->d.QuadEncod.inputB, true, &addr2, &bit2);

            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_CLEAR_BIT, stateInOut);
#if QUAD_ALGO <= 4
              // table algorithm
              // https://hifiduino.wordpress.com/2010/10/20/rotaryencoder-hw-sw-no-debounce/
              Op(INT_SET_VARIABLE_SHL, state, state, "2");
              Op(INT_SET_VARIABLE_AND, state, state, "0x0F");

              if(addr1 == addr2) {
                char s[MAX_NAME_LEN];
                sprintf(s, "#PIN%c", 'A' + InputRegIndex(addr1));
                Op(INT_SET_VARIABLE_TO_VARIABLE, pins, s);
                sprintf(s, "%d", bit1);
                Op(INT_IF_BIT_SET_IN_VAR, pins, s);
                  Op(INT_VARIABLE_SET_BIT, state, "0");
                Op(INT_END_IF);
                sprintf(s, "%d", bit2);
                Op(INT_IF_BIT_SET_IN_VAR, pins, s);
                  Op(INT_VARIABLE_SET_BIT, state, "1");
                Op(INT_END_IF);
              } else {
                #if 1
                Error(_("Inputs '%s' and '%s' must be located in same MCU PORT!"), l->d.QuadEncod.inputA, l->d.QuadEncod.inputB);
                #else
                Op(INT_IF_BIT_SET, l->d.QuadEncod.inputA);
                  Op(INT_VARIABLE_SET_BIT, state, "0");
                Op(INT_END_IF);
                Op(INT_IF_BIT_SET, l->d.QuadEncod.inputB);
                  Op(INT_VARIABLE_SET_BIT, state, "1");
                Op(INT_END_IF);
                #endif
              }

              int count = 16;
              int sovElement = 1;
              Op(INT_FLASH_READ, dir, "ELEM_QUAD_ENCOD", state, count, sovElement);
              Op(INT_IF_GRT, dir, "0");
                Op(INT_INCREMENT_VARIABLE, l->d.QuadEncod.counter); // +
                if(l->d.QuadEncod.countPerRevol > 0)
                  Op(INT_INCREMENT_VARIABLE, ticks);
                if(strlen(l->d.QuadEncod.dir))
                  Op(INT_SET_BIT, l->d.QuadEncod.dir);
                Op(INT_SET_BIT, stateInOut);
              Op(INT_ELSE);
                Op(INT_IF_LES, dir, "0");
                  Op(INT_DECREMENT_VARIABLE, l->d.QuadEncod.counter); // -
                  if(l->d.QuadEncod.countPerRevol > 0)
                    Op(INT_DECREMENT_VARIABLE, ticks);
                  if(strlen(l->d.QuadEncod.dir))
                    Op(INT_CLEAR_BIT, l->d.QuadEncod.dir);
                  Op(INT_SET_BIT, stateInOut);
                Op(INT_END_IF);
              Op(INT_END_IF);
#elif QUAD_ALGO == 22
              // http://henrysbench.capnfatz.com/henrys-bench/arduino-sensors-and-input/keyes-ky-040-arduino-rotary-encoder-user-manual/
              Op(INT_COPY_BIT_TO_BIT, nowA, l->d.QuadEncod.inputA);
              Op(INT_IF_BIT_NEQ_BIT, prevA, nowA);
                Op(INT_IF_BIT_EQU_BIT, l->d.QuadEncod.inputB, nowA);
                  Op(INT_INCREMENT_VARIABLE, l->d.QuadEncod.counter); // +
                  if(l->d.QuadEncod.countPerRevol > 0)
                    Op(INT_INCREMENT_VARIABLE, ticks);
                  if(strlen(l->d.QuadEncod.dir))
                    Op(INT_SET_BIT, l->d.QuadEncod.dir);
                Op(INT_ELSE);
                  Op(INT_DECREMENT_VARIABLE, l->d.QuadEncod.counter); // -
                  if(l->d.QuadEncod.countPerRevol > 0)
                    Op(INT_DECREMENT_VARIABLE, ticks);
                  if(strlen(l->d.QuadEncod.dir))
                    Op(INT_CLEAR_BIT, l->d.QuadEncod.dir);
                Op(INT_END_IF);
                Op(INT_SET_BIT, stateInOut);
                Op(INT_COPY_BIT_TO_BIT, prevA, nowA);
              Op(INT_END_IF);
#elif QUAD_ALGO == 222
              // http://www.technoblogy.com/show?1YHJ
              #if 1
              if(addr1 == addr2) {
                char s[MAX_NAME_LEN];
                sprintf(s, "#PIN%c", 'A' + InputRegIndex(addr1));
                Op(INT_SET_VARIABLE_TO_VARIABLE, pins, s);
                Op(INT_CLEAR_BIT, nowA);
                sprintf(s, "%d", bit1);
                Op(INT_IF_BIT_SET_IN_VAR, pins, s);
                  Op(INT_SET_BIT, nowA);
                Op(INT_END_IF);
                Op(INT_CLEAR_BIT, nowB);
                sprintf(s, "%d", bit2);
                Op(INT_IF_BIT_SET_IN_VAR, pins, s);
                  Op(INT_SET_BIT, nowB);
                Op(INT_END_IF);
              } else {
                Error(_("Inputs '%s' and '%s' must be located in same MCU PORT!"), l->d.QuadEncod.inputA, l->d.QuadEncod.inputB);
              }
              #else
              Op(INT_COPY_BIT_TO_BIT, nowA, l->d.QuadEncod.inputA);
              Op(INT_COPY_BIT_TO_BIT, nowB, l->d.QuadEncod.inputB);
              #endif

              Op(INT_IF_BIT_NEQ_BIT, prevA, nowA);
                Op(INT_IF_BIT_NEQ_BIT, prevB, nowB);
                  Op(INT_IF_BIT_EQU_BIT, nowA, nowB);
                    Op(INT_INCREMENT_VARIABLE, l->d.QuadEncod.counter); // +
                    if(l->d.QuadEncod.countPerRevol > 0)
                      Op(INT_INCREMENT_VARIABLE, ticks);
                    if(strlen(l->d.QuadEncod.dir))
                      Op(INT_SET_BIT, l->d.QuadEncod.dir);
                  Op(INT_ELSE);
                    Op(INT_DECREMENT_VARIABLE, l->d.QuadEncod.counter); // -
                    if(l->d.QuadEncod.countPerRevol > 0)
                      Op(INT_DECREMENT_VARIABLE, ticks);
                    if(strlen(l->d.QuadEncod.dir))
                      Op(INT_CLEAR_BIT, l->d.QuadEncod.dir);
                  Op(INT_END_IF);
                  Op(INT_COPY_BIT_TO_BIT, prevB, nowB);
                  Op(INT_SET_BIT, stateInOut);
                Op(INT_END_IF);
                Op(INT_COPY_BIT_TO_BIT, prevA, nowA);
              Op(INT_END_IF);
#elif QUAD_ALGO == 40
              char A_xorB[MAX_NAME_LEN];
              sprintf(A_xorB, "$A_xorB_%s", l->d.QuadEncod.counter);
#endif
              if(strlen(l->d.QuadEncod.inputZ)) {
                char storeName[MAX_NAME_LEN];
                GenSymOneShot(storeName, l->d.QuadEncod.counter, "");
                Op(INT_COPY_BIT_TO_BIT, nowZ, l->d.QuadEncod.inputZ);
                if(l->d.QuadEncod.inputZKind == '/') {
                  Op(INT_IF_BIT_SET, nowZ);
                    Op(INT_IF_BIT_CLEAR, prevZ);
                      Op(INT_SET_BIT, storeName);
                    Op(INT_ELSE);
                      Op(INT_CLEAR_BIT, storeName);
                    Op(INT_END_IF);
                  Op(INT_ELSE);
                    Op(INT_CLEAR_BIT, storeName);
                  Op(INT_END_IF);
                } else if(l->d.QuadEncod.inputZKind == '\\') {
                  Op(INT_IF_BIT_CLEAR, nowZ);
                    Op(INT_IF_BIT_SET, prevZ);
                      Op(INT_SET_BIT, storeName);
                    Op(INT_ELSE);
                      Op(INT_CLEAR_BIT, storeName);
                    Op(INT_END_IF);
                  Op(INT_ELSE);
                    Op(INT_CLEAR_BIT, storeName);
                  Op(INT_END_IF);
                } else if(l->d.QuadEncod.inputZKind == '-') {
                  Op(INT_COPY_BIT_TO_BIT, storeName, l->d.QuadEncod.inputZ);
                } else if(l->d.QuadEncod.inputZKind == 'o') {
                  Op(INT_COPY_NOT_BIT_TO_BIT, storeName, l->d.QuadEncod.inputZ);
                } else oops();
                Op(INT_COPY_BIT_TO_BIT, prevZ, nowZ);

                Op(INT_IF_BIT_SET, storeName);
                  Op(INT_SET_BIT, stateInOut); //
                  if(l->d.QuadEncod.countPerRevol == 0) {
                    Op(INT_SET_VARIABLE_TO_LITERAL, l->d.QuadEncod.counter, (SDWORD)0x00);
                  } else if(l->d.QuadEncod.countPerRevol > 0) {
                    char s[MAX_NAME_LEN];
                    sprintf(s, "%d", l->d.QuadEncod.countPerRevol / 2);
                    Op(INT_IF_GRT, ticks, s);
                      Op(INT_INCREMENT_VARIABLE, revol);
                      sprintf(s, "%d", l->d.QuadEncod.countPerRevol);
                      Op(INT_SET_VARIABLE_MULTIPLY, l->d.QuadEncod.counter, revol, s);
                      Op(INT_SET_VARIABLE_TO_LITERAL, ticks, (SDWORD)0x00);
                    Op(INT_ELSE);
                      sprintf(s, "%d", -l->d.QuadEncod.countPerRevol / 2);
                      Op(INT_IF_LES, ticks, s);
                        Op(INT_DECREMENT_VARIABLE, revol);
                        sprintf(s, "%d", l->d.QuadEncod.countPerRevol);
                        Op(INT_SET_VARIABLE_MULTIPLY, l->d.QuadEncod.counter, revol, s);
                        Op(INT_SET_VARIABLE_TO_LITERAL, ticks, (SDWORD)0x00);
                      Op(INT_END_IF);
                    Op(INT_END_IF);
                  }
                Op(INT_END_IF);
              }
            Op(INT_END_IF);
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
                    Op(INT_IF_VARIABLE_EQUALS_VARIABLE, "$tmpVar24bit", l->d.persist.var);
                    Op(INT_ELSE);
                        Op(INT_EEPROM_WRITE, l->d.persist.var, EepromAddrFree);
                    Op(INT_END_IF);
                Op(INT_END_IF);
            Op(INT_END_IF);

          Op(INT_END_IF);

          EepromAddrFree += SizeOfVar(l->d.persist.var);
          break;
        }
        case ELEM_UART_SENDn: {                     ///// JG: this function is a huge mystery !
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
        case ELEM_UART_SEND: {
            Comment(3, "ELEM_UART_SEND");
#if 0
            // Why in this place do not controlled stateInOut, as in the ELEM_UART_RECV ?
            // 1. It's need in Simulation Mode.
            // 2. It's need for Arduino.
        ////Op(INT_IF_BIT_SET, stateInOut); // ???
            Op(INT_UART_SEND, l->d.uart.name, stateInOut); // stateInOut returns BUSY flag
        ////Op(INT_END_IF); // ???
#else
            if(l->d.uart.bytes == 1) {
              // This is modified algorithm !!!
              Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_UART_SEND1, l->d.uart.name);
              Op(INT_END_IF);

              if(!l->d.uart.wait) {
                Op(INT_UART_SEND_BUSY, stateInOut); // stateInOut returns BUSY flag
              } else {
                char label[MAX_NAME_LEN];
                GenSym(label, "_wait", "UART_SEND", l->d.uart.name);

                Op(INT_AllocKnownAddr, label);
                Op(INT_UART_SEND_BUSY, stateInOut); // stateInOut returns BUSY flag
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_GOTO, label, 1);
                Op(INT_END_IF);
              }
            } else {
                if(l->d.uart.wait) {
                  Op(INT_IF_BIT_SET, stateInOut);
                    Op(INT_UART_SEND1, l->d.uart.name);

                    char label[MAX_NAME_LEN];
                    for(int i = 1; i < l->d.uart.bytes; i++) {
                      GenSym(label, "_wait", "UART_SEND", l->d.uart.name);

                      Op(INT_AllocKnownAddr, label);
                      Op(INT_UART_SEND_BUSY, stateInOut); // stateInOut returns BUSY flag
                      Op(INT_IF_BIT_SET, stateInOut);
                        Op(INT_GOTO, label, 1);
                      Op(INT_END_IF);

                      Op(INT_UART_SEND1, l->d.uart.name, i);
                    }
                  Op(INT_END_IF);
                  Op(INT_UART_SEND_BUSY, stateInOut); // stateInOut returns BUSY flag
                } else { // don't wait
                  char storeName[MAX_NAME_LEN];
                  GenSymOneShot(storeName, "UART_SEND", l->d.uart.name);

                  Op(INT_IF_BIT_SET, stateInOut);
                    Op(INT_IF_BIT_CLEAR, storeName);
                      Op(INT_SET_BIT, storeName);
                    Op(INT_END_IF);
                  Op(INT_END_IF);

                  Op(INT_IF_BIT_SET, storeName);
                    char saved[MAX_NAME_LEN];
                    GenVar(saved, "saved_UART_SEND", l->d.uart.name);
                    SetSizeOfVar(saved, l->d.uart.bytes);
                    Op(INT_SET_VARIABLE_TO_VARIABLE, saved, l->d.uart.name);

                    char bytes[MAX_NAME_LEN];
                    sprintf(bytes, "%d", l->d.uart.bytes);

                    char numb[MAX_NAME_LEN];
                    GenVar(numb, "numb_UART_SEND", l->d.uart.name);

                    Op(INT_IF_LES, numb,  l->d.uart.bytes);
                      Op(INT_UART_SEND_BUSY, stateInOut); // stateInOut returns BUSY flag
                      Op(INT_IF_BIT_CLEAR, stateInOut);
                        Op(INT_UART_SEND1, saved, numb);
                        Op(INT_INCREMENT_VARIABLE, numb);
                      Op(INT_END_IF);
                    Op(INT_END_IF);

                    Op(INT_IF_GEQ, numb, bytes);
                      Op(INT_SET_VARIABLE_TO_LITERAL, numb, 0);
                      Op(INT_CLEAR_BIT, storeName);
                    Op(INT_END_IF);
                  Op(INT_END_IF);

                  Op(INT_IF_BIT_SET, storeName);
                    Op(INT_SET_BIT, stateInOut); // busy
                  Op(INT_ELSE);
                    Op(INT_UART_SEND_BUSY, stateInOut); // stateInOut returns BUSY flag
                  Op(INT_END_IF);
                }
              }
#endif
            break;
        }
        case ELEM_UART_RECV: {
            Comment(3, "ELEM_UART_RECV");
            int sov = SizeOfVar(l->d.uart.name);
            if(l->d.uart.bytes > sov) {
                Error("ELEM_UART_RECV '%s' bytes > sov", l->d.uart.name);
                oops();
            }
#if 0
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_UART_RECV, l->d.uart.name, stateInOut);
            Op(INT_END_IF);
#else
            Op(INT_IF_BIT_SET, stateInOut);
              if(l->d.uart.bytes == 1) {
                Op(INT_UART_RECV_AVAIL, stateInOut);
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_SET_VARIABLE_TO_LITERAL, l->d.uart.name, 0);
                  Op(INT_UART_RECV1, l->d.uart.name);
                Op(INT_END_IF);
              } else {
                if(l->d.uart.wait) {
                  Op(INT_UART_RECV_AVAIL, stateInOut);
                  Op(INT_IF_BIT_SET, stateInOut);
                    Op(INT_SET_VARIABLE_TO_LITERAL, l->d.uart.name, 0);
                    Op(INT_UART_RECV1, l->d.uart.name);

                    char label[MAX_NAME_LEN];
                    for(int i = 1; i < l->d.uart.bytes; i++) {
                      GenSym(label, "_wait", "UART_RECV", l->d.uart.name);
                      Op(INT_AllocKnownAddr, label);
                      Op(INT_UART_RECV_AVAIL, stateInOut);
                      Op(INT_IF_BIT_CLEAR, stateInOut);
                        Op(INT_GOTO, label, 1);
                      Op(INT_END_IF);
                      Op(INT_UART_RECV1, l->d.uart.name, i);
                    }
                  Op(INT_END_IF);
                } else {
                  char saved[MAX_NAME_LEN];
                  GenVar(saved, "saved_UART_RECV", l->d.uart.name);
                  SetSizeOfVar(saved, l->d.uart.bytes);

                  char bytes[MAX_NAME_LEN];
                  sprintf(bytes, "%d", l->d.uart.bytes);

                  char numb[MAX_NAME_LEN];
                  GenVar(numb, "numb_UART_RECV", l->d.uart.name);

                  Op(INT_IF_LES, numb,  l->d.uart.bytes);
                    Op(INT_UART_RECV_AVAIL, stateInOut);
                    Op(INT_IF_BIT_SET, stateInOut);
                      Op(INT_UART_RECV1, saved, numb);
                      Op(INT_INCREMENT_VARIABLE, numb);
                    Op(INT_END_IF);
                  Op(INT_END_IF);

                  Op(INT_IF_GEQ, numb, bytes);
                    Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.uart.name, saved);
                    Op(INT_SET_VARIABLE_TO_LITERAL, numb, 0);
                    Op(INT_SET_BIT, stateInOut);
                  Op(INT_ELSE);
                    Op(INT_CLEAR_BIT, stateInOut);
                  Op(INT_END_IF);
                }
              }
            Op(INT_END_IF);
#endif
            break;
        }
        case ELEM_UART_RECV_AVAIL:
            Comment(3, "ELEM_UART_RECV_AVAIL");
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_UART_RECV_AVAIL, stateInOut);
            Op(INT_END_IF);
            break;

        case ELEM_UART_SEND_READY:
            Comment(3, "ELEM_UART_SEND_READY");
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_UART_SEND_READY, stateInOut);
            Op(INT_END_IF);
            break;
        }

        case ELEM_SPI: {
            Comment(3, "ELEM_SPI");
            ///// Added by JG
            Op(INT_IF_BIT_SET, stateInOut);                                 // if(Read_Ib_rung_top()) {
              Op(INT_SPI, l->d.spi.name, l->d.spi.send, l->d.spi.recv);     //   SpiSendRecv[name1=name, name2=send, name3=recv]
              Op(INT_SET_BIT, stateInOut);                                  //   activate output line
            Op(INT_END_IF);                                                 // }
            /////
            break;
        }

        ///// Added by JG
        case ELEM_SPI_WR: {
            Comment(3, "ELEM_SPI_WR");
            Op(INT_IF_BIT_SET, stateInOut);                                 // if(Read_Ib_rung_top()) {
              Op(INT_SPI_WRITE, l->d.spi.name, l->d.spi.send);              //   SpiWrite[name1=name, name2=send]
              Op(INT_SET_BIT, stateInOut);                                  //   activate output line
            Op(INT_END_IF);                                                 // }
            break;
        }

        case ELEM_I2C_RD: {
            Comment(3, "ELEM_I2C_RD");
            Op(INT_IF_BIT_SET, stateInOut);                                                     // if(Read_Ib_rung_top()) {
            Op(INT_I2C_READ, l->d.i2c.name, l->d.i2c.recv, l->d.i2c.address, l->d.i2c.registr); //   I2cRead[name1=name, name2=recv, name3=addr, name4= reg]
              Op(INT_SET_BIT, stateInOut);                                                      //   activate output line
            Op(INT_END_IF);                                                                     // }
            break;
        }

        case ELEM_I2C_WR: {
            Comment(3, "ELEM_I2C_WR");
            Op(INT_IF_BIT_SET, stateInOut);                                                         // if(Read_Ib_rung_top()) {
              Op(INT_I2C_WRITE, l->d.i2c.name, l->d.i2c.send, l->d.i2c.address, l->d.i2c.registr);  //   I2cWrite[name1=name, name2=send, name3=addr, name4= reg]);
              Op(INT_SET_BIT, stateInOut);                                                          //   activate output line
            Op(INT_END_IF);                                                                         // }
            break;
        }
        /////

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
          mathBit : {
            if(IsNumber(l->d.math.dest)) {
                THROW_COMPILER_EXCEPTION_FMT(_("Math instruction: '%s' not a valid destination."), l->d.math.dest);
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
                    if((hobatoi(l->d.math.op2) < 0)
                    || (SizeOfVar(l->d.math.op1) * 8 < hobatoi(l->d.math.op2))) {
                        THROW_COMPILER_EXCEPTION_FMT(_("Shift constant %s=%d out of range of the '%s' variable: 0 to %d inclusive."), l->d.math.op2, hobatoi(l->d.math.op2), l->d.math.op1, SizeOfVar(l->d.math.op1) * 8);
                    }
                }
                Op(intOp, l->d.math.dest, l->d.math.op1, l->d.math.op2/*, stateInOut2*/);
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
        case ELEM_MOD: intOp = INT_SET_VARIABLE_MOD;      Comment(3, "ELEM_MOD"); goto math;
        math : {
            if(IsNumber(l->d.math.dest)) {
                THROW_COMPILER_EXCEPTION_FMT(_("Math instruction: '%s' not a valid destination."), l->d.math.dest);
            }
            Op(INT_IF_BIT_SET, stateInOut);
            const char *op1 = VarFromExpr(l->d.math.op1, "$scratch1");
            if((intOp == INT_SET_VARIABLE_SUBTRACT)
            && (int_comment_level != 1)
            && (strcmp(l->d.math.dest, l->d.math.op1) == 0)
            && (strcmp(l->d.math.op2, "1") == 0)) {
                Op(INT_DECREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");
            } else if((intOp == INT_SET_VARIABLE_SUBTRACT)
            && (int_comment_level != 1)
            && (strcmp(l->d.math.dest, l->d.math.op1) == 0)
            && (strcmp(l->d.math.op2, "-1") == 0)) {
                Op(INT_INCREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");

            } else if((intOp == INT_SET_VARIABLE_ADD)
            && (int_comment_level != 1)
            && (strcmp(l->d.math.dest, l->d.math.op1) == 0)
            && (strcmp(l->d.math.op2, "1") == 0)) {
                Op(INT_INCREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");
            } else if((intOp == INT_SET_VARIABLE_ADD)
            && (int_comment_level != 1)
            && (strcmp(l->d.math.dest, l->d.math.op2) == 0)
            && (strcmp(l->d.math.op1, "1") == 0)) {
                Op(INT_INCREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");
            } else if((intOp == INT_SET_VARIABLE_ADD)
            && (int_comment_level != 1)
            && (strcmp(l->d.math.dest, l->d.math.op1) == 0)
            && (strcmp(l->d.math.op2, "-1") == 0)) {
                Op(INT_DECREMENT_VARIABLE, l->d.math.dest, stateInOut2, "ROverflowFlagV");
            } else if((intOp == INT_SET_VARIABLE_ADD)
            && (int_comment_level != 1)
            && (strcmp(l->d.math.dest, l->d.math.op2) == 0)
            && (strcmp(l->d.math.op1, "-1") == 0)) {
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
            SDWORD clocks = CalcDelayClock(hobatoi(l->d.timer.delay));
            if(Prog.mcu()) {
                if(Prog.mcu()->whichIsa == ISA_AVR) {
                    clocks = (clocks - 1) / 4;
                    if(clocks > 0x10000)
                        clocks = 0x10000;
                } else if(Prog.mcu()->whichIsa == ISA_PIC16) {
                    clocks = (clocks - 10) / 6;
                    if(clocks > 0xffff)
                        clocks = 0xffff;
                } else
                    THROW_COMPILER_EXCEPTION(_("Internal error."));
            }
            if(clocks <= 0)
                clocks = 1;
            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, clocks);
            Op(INT_END_IF);
            break;
        }
        case ELEM_GOTO: {
            Comment(3, "ELEM_GOTO %s", l->d.doGoto.label);
            char name[MAX_NAME_LEN];
            GetLabelName(ELEM_LABEL, name, l->d.doGoto.label);
            int r;
            if(IsNumber(l->d.doGoto.label)) {
                r = hobatoi(l->d.doGoto.label);
                r = std::min(r, Prog.numRungs + 1) - 1;
            } else {
                r = FindRung(ELEM_LABEL, l->d.doGoto.label);
                if(r < 0) {
                    THROW_COMPILER_EXCEPTION_FMT(_("GOTO: LABEL '%s' not found!"), l->d.doGoto.label);
                }
            }
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_GOTO, name, l->d.doGoto.label, r <= rungNow ? 1 : 0);
                //Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_GOSUB: {
            Comment(3, "ELEM_GOSUB %s", l->d.doGoto.label);
            char name[MAX_NAME_LEN];
            GetLabelName(ELEM_SUBPROG, name, l->d.doGoto.label);
            int r;
            if(IsNumber(l->d.doGoto.label)) {
                THROW_COMPILER_EXCEPTION_FMT(_("GOSUB: SUBPROG as number '%s' not allowed !"), l->d.doGoto.label);
                r = hobatoi(l->d.doGoto.label);
                r = std::min(r, Prog.numRungs + 1) - 1;
            } else {
                r = FindRung(ELEM_SUBPROG, l->d.doGoto.label);
                if(r < 0) {
                    THROW_COMPILER_EXCEPTION_FMT(_("GOSUB: SUBPROG '%s' not found!"), l->d.doGoto.label);
                }
            }
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_GOSUB, name, l->d.doGoto.label, r <= rungNow ? 1 : 0);
            Op(INT_END_IF);
            break;
        }
        case ELEM_LABEL: {
            Comment(3, "ELEM_LABEL %s", l->d.doGoto.label);
            // TODO: Check is ELEM_LABEL
            int n = CountWhich(ELEM_LABEL, l->d.doGoto.label);
            if(n != 1) {
                Error(_("LABEL: Only one LABEL '%s' is allowed!"), l->d.doGoto.label);
            }
            //Op(INT_AllocKnownAddr, l->d.doGoto.label);
            //Op(INT_AllocFwdAddr, l->d.doGoto.label);
            break;
        }
        case ELEM_SUBPROG: {
            Comment(3, "ELEM_SUBPROG %s", l->d.doGoto.label);
            int  n;
            n = CountWhich(ELEM_SUBPROG, l->d.doGoto.label);
            if(n != 1) {
                Error(_("SUBPROG: Only one SUBPROG '%s' is allowed!"), l->d.doGoto.label);
            }
            n = CountWhich(ELEM_ENDSUB, l->d.doGoto.label);
            if(n < 1) {
                Error(_("SUBPROG: ENDSUB '%s' not found!"), l->d.doGoto.label);
            }
            if((Prog.rungs[rungNow]->contents[0].which == ELEM_SUBPROG)
            && (Prog.rungs[rungNow]->count == 1)) {
                ; //
            } else {
                Error(_("SUBPROG: '%s' declaration must be a single inside the rung %d"), l->d.doGoto.label, rungNow + 1);
            }
            int r = -1;
            if(!IsNumber(l->d.doGoto.label)) {
                r = FindRungLast(ELEM_ENDSUB, l->d.doGoto.label);
            }
            if(r >= 0) {
                char name[MAX_NAME_LEN];
                GetLabelName(name, r + 2);
                Op(INT_GOTO, name, l->d.doGoto.label, "ENDSUB", 0);
                //char s[MAX_NAME_LEN];
                //sprintf(s,"SUBPROG%s", l->d.doGoto.label);
                //Op(INT_AllocKnownAddr, s, l->d.doGoto.label);
            }
            break;
        }
        case ELEM_ENDSUB: {
            Comment(3, "ELEM_ENDSUB %s", l->d.doGoto.label);
            int n;
            n = CountWhich(ELEM_ENDSUB, l->d.doGoto.label);
            if(n != 1) {
                Error(_("ENDSUB: Only one ENDSUB '%s' is allowed!"), l->d.doGoto.label);
            }
            n = CountWhich(ELEM_SUBPROG, l->d.doGoto.label);
            if(n < 1) {
                Error(_("ENDSUB: SUBPROG '%s' not found!"), l->d.doGoto.label);
            }
            if((Prog.rungs[rungNow]->contents[0].which == ELEM_ENDSUB)
            && (Prog.rungs[rungNow]->count == 1)) {
                ; //
            } else {
                Error(_("ENDSUB: '%s' declaration must be a single inside the rung %d"), l->d.doGoto.label, rungNow + 1);
            }
            int r = -1;
            if(!IsNumber(l->d.doGoto.label)) {
                r = FindRung(ELEM_SUBPROG, l->d.doGoto.label);
            }
            Op(INT_RETURN, l->d.doGoto.label, r);
            break;
        }
        case ELEM_RETURN:
            Comment(3, "ELEM_RETURN");
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_RETURN);
              //Op(INT_CLEAR_BIT, stateInOut);
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
            SimState(&(l->poweredAfter), stateInOut, &(l->workingNow), "$mcr");
            break;

        case ELEM_SHIFT_REGISTER: {
            Comment(3, "ELEM_SHIFT_REGISTER");
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName, "SHIFT_REGISTER", l->d.shiftRegister.name);
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_IF_BIT_CLEAR, storeName);
                for(int i = (l->d.shiftRegister.stages - 2); i >= 0; i--) {
                    char dest[MAX_NAME_LEN], src[MAX_NAME_LEN];
                    sprintf(src, "%s%d", l->d.shiftRegister.name, i);
                    sprintf(dest, "%s%d", l->d.shiftRegister.name, i + 1);
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

            int sovElement;
            sovElement = TestByteNeeded(t->count, t->vals);

            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_FLASH_READ, t->dest, t->name, t->index, t->count, sovElement, t->vals);
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
                THROW_COMPILER_EXCEPTION(_("Piecewise linear lookup table with zero elements!"));
            }
            int xThis = t->vals[0];
            for(int i = 1; i < t->count; i++) {
                if(t->vals[i * 2] <= xThis) {
                    THROW_COMPILER_EXCEPTION(_("x values in piecewise linear table must be strictly increasing."));
                }
                xThis = t->vals[i * 2];
            }
            #ifdef TABLE_IN_FLASH_LINEAR
            int sovElement;
            sovElement = TestByteNeeded(t->count, t->vals);
            #endif
            Op(INT_IF_BIT_SET, stateInOut);
            for(int i = t->count - 1; i >= 1; i--) {
                Comment("PWL %d", i);
                int xThis = t->vals[i * 2];
                int xPrev = t->vals[(i - 1) * 2];
                int yThis = t->vals[i * 2 + 1];
                int yPrev = t->vals[(i - 1) * 2 + 1];
                int thisDx = xThis - xPrev;
                int thisDy = yThis - yPrev;
                char sxThis[21]; // -9223372036854775808
                char sxPrev[21];
                char syThis[21];
                char syPrev[21];
                char sthisDx[21];
                char sthisDy[21];
                sprintf(sxThis,"%d", xThis);
                sprintf(sxPrev,"%d", xPrev);
                sprintf(syThis,"%d", yThis);
                sprintf(syPrev,"%d", yPrev);
                sprintf(sthisDx,"%d", thisDx);
                sprintf(sthisDy,"%d", thisDy);
                // The output point is given by
                //    yout = y[i-1] + (xin - x[i-1])*dy/dx
                //    yout = yPrev + (xin - xPrev)*dy/dx
                //    xin between the [xPrev and xThis]
                //    xin is t->index
                //    yout is t->dest
                // and this is the best form in which to keep it, numerically
                // speaking, because you can always fix numerical problems
                // by moving the PWL points closer together.

                // Check for numerical problems, and fail if we have them.
                if((thisDx * thisDy) >= 32767 || (thisDx * thisDy) <= -32768) {
                    THROW_COMPILER_EXCEPTION(_("Numerical problem with piecewise linear lookup "
                        "table. Either make the table entries smaller, "
                        "or space the points together more closely.\r\n\r\n"
                        "See the help file for details."));
                }

                #ifndef TABLE_IN_FLASH_LINEAR
                // Hack to avoid AVR brge issue again, since long jumps break
                Op(INT_CLEAR_BIT, "$scratch");
                Op(INT_IF_VARIABLE_LES_LITERAL, t->index, xThis + 1);
                    Op(INT_SET_BIT, "$scratch");
                Op(INT_END_IF);

                Op(INT_IF_BIT_SET, "$scratch");
#if 0
                  Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", xPrev);
                  Op(INT_SET_VARIABLE_SUBTRACT, "$scratch", t->index, "$scratch");
                  Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch2", thisDx);
                  Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch3", thisDy);
                  Op(INT_SET_VARIABLE_MULTIPLY, t->dest, "$scratch", "$scratch3");
                  Op(INT_SET_VARIABLE_DIVIDE, t->dest, t->dest, "$scratch2");

                  Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", yPrev);
                  Op(INT_SET_VARIABLE_ADD, t->dest, t->dest, "$scratch");
#else
                  Op(INT_SET_VARIABLE_SUBTRACT, t->dest, t->index, sxPrev);
                  Op(INT_SET_VARIABLE_MULTIPLY, t->dest, t->dest, sthisDy);
                  Op(INT_SET_VARIABLE_DIVIDE, t->dest, t->dest, sthisDx);
                  Op(INT_SET_VARIABLE_ADD, t->dest, t->dest, syPrev);
#endif
                Op(INT_END_IF);
                #endif
            }
            #ifdef TABLE_IN_FLASH_LINEAR // WIP
                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", t->count);
                Op(INT_FLASH_READ, t->dest, t->name, "$scratch", t->count, sovElement, t->vals);
                Op(INT_LES, t->dest, t->index)
                Op(INT_SET_VARIABLE_SUBTRACT, t->dest, t->index, sxPrev);
                Op(INT_SET_VARIABLE_MULTIPLY, t->dest, t->dest, sthisDy);
                Op(INT_SET_VARIABLE_DIVIDE, t->dest, t->dest, sthisDx);
                Op(INT_SET_VARIABLE_ADD, t->dest, t->dest, syPrev);
            #endif
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
            char outputChars[MAX_LOOK_UP_TABLE_LEN * 2];

            // This is a table of flags which was output:
            // positive is an unsigned character,
            // negative is special flag values
            enum {
                OUTPUT_UCHAR = 1,
                OUTPUT_DIGIT = 2,
                OUTPUT_SIGN = 3,
            };
            char outputWhich[sizeof(outputChars)];
            // outputWhich is added to be able to send the full unsigned char range
            // as hexadecimal numbers in formatted string element.
            // \xFF == -1   and   \xFE == -2  in output string.
            // Release 2.2 can raise error with formated strings "\0x00" and "\0x01"
            // Release 2.3 can raise error with formated strings "\0xFF" and "\0xFE"

            bool mustDoMinus = false;

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
                if((*p == '\\') && (isdigit(p[1]) || p[1] == '-') && (p[1] != '0')) {
                    if(digits >= 0) {
                        Error(_("Multiple escapes (\\0-9) present in format string, not allowed."));
                    }
                    p++;
                    if(*p == '-') {
                        mustDoMinus = true;
                        outputWhich[steps] = OUTPUT_SIGN;
                        outputChars[steps++] = OUTPUT_SIGN;
                        p++;
                    }
                    if(!isdigit(*p) || (*p - '0') > 5 || *p == '0') {
                        Error(_("Bad escape sequence following \\; for a literal backslash, use \\\\"));
                    }
                    digits = (*p - '0');
                    for(int i = 0; i < digits; i++) {
                        outputWhich[steps] = OUTPUT_DIGIT;
                        outputChars[steps++] = OUTPUT_DIGIT;
                    }
                } else if(*p == '\\') {
                    L1:
                    p++;
                    switch(*p) {
                        case 'a': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = 0x07; break;
                        case 'b': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\b'; break;
                        case 'e': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = 0x1B; break;
                        case 'f': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\f'; break;
                        case 'n': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\n'; break;
                        case 'r': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\r'; break;
                        case 't': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\t'; break;
                        case 'v': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\v'; break;
                        case '\'':outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\''; break;
                        case '"': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\"'; break;
                        case '?': outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\?'; break;
                        case '\\':outputWhich[steps] = OUTPUT_UCHAR; outputChars[steps++] = '\\'; break;
                        case '0': goto L1; break;
                        case 'X':
                        case 'x': {
                            int h, l;
                            p++;
                            h = HexDigit(*p);
                            if(h >= 0) {
                                p++;
                                l = HexDigit(*p);
                                if(l >= 0) {
                                    outputWhich[steps] = OUTPUT_UCHAR;
                                    outputChars[steps++] = (h << 4) | l;
                                    break;
                                }
                            }
                            Error(_("Bad escape: correct form is \\xAB."));
                            break;
                        }
                        default:
                            Error(_("Bad escape '\\%c'"), *p);
                            break;
                    }
                } else {
                    outputWhich[steps] = OUTPUT_UCHAR;
                    outputChars[steps++] = *p;
                }
                if(*p)
                    p++;

                if(steps >= sizeof(outputChars)) {
                    Error(_("Internal error."));
                }
            }

            if(digits >= 0 && (strlen(var) == 0)) {
                Error(_("Variable is interpolated into formatted string, but none is specified."));
            } else if(digits < 0 && (strlen(var) > 0)) {
                Error(_("No variable is interpolated into formatted string, but a variable name is specified. Include a string like '\\-3', or leave variable name blank."));
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
            Op(INT_ELSE);                   //v2
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

                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", TenToThe((digits - digit) - 1));
                    Op(INT_SET_VARIABLE_DIVIDE, "$charToUart", convertState, "$scratch");
                    Op(INT_SET_VARIABLE_MULTIPLY, "$scratch", "$scratch", "$charToUart");
                    Op(INT_SET_VARIABLE_SUBTRACT, convertState, convertState, "$scratch");
                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", '0');
                    Op(INT_SET_VARIABLE_ADD, "$charToUart", "$charToUart", "$scratch");

                    // Suppress all but the last leading zero.
                    if(digit != (digits - 1)) {
                        Op(INT_IF_VARIABLE_EQUALS_VARIABLE, "$scratch", "$charToUart");
                          Op(INT_IF_BIT_SET, isLeadingZero);
                            Op(INT_SET_VARIABLE_TO_LITERAL, "$charToUart", ' '); // '0' %04d
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
                            Op(INT_SET_VARIABLE_SUBTRACT, convertState, convertState, var);
                        Op(INT_ELSE);
                            Op(INT_SET_VARIABLE_TO_VARIABLE, convertState, var);
                        Op(INT_END_IF);

                    Op(INT_END_IF);
                } else if(outputWhich[i] == OUTPUT_UCHAR) {
                    // just another character
                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", i);
                    Op(INT_IF_VARIABLE_EQUALS_VARIABLE, "$scratch", seqScratch);
                      Op(INT_SET_VARIABLE_TO_LITERAL, "$charToUart", outputChars[i]);
                    Op(INT_END_IF);
                } else
                    Error(_("Internal error."));
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
            Comment(3, "ELEM_SHORT"); // can comment // need only for debuging to align the lines in pl,asm
            break;

        case ELEM_PLACEHOLDER: {
            //Comment(3, "ELEM_PLACEHOLDER");
            THROW_COMPILER_EXCEPTION(_("Empty row; delete it or add instructions before compiling."));
            break;
        }
        case ELEM_COMMENT: {
            char  s1[MAX_COMMENT_LEN];
            char *s2;
            AnsiToOem(l->d.comment.str, s1);
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
            if(int_comment_level >= 2) {
                if(s1)
                    Comment1(s1); // bypass % in comments
                if(s2)
                    Comment1(s2); // bypass % in comments
            }
            break;
        }
        default:
            THROW_COMPILER_EXCEPTION_FMT("ELEM_0x%X", which);
            break;
    }
    #ifndef DEFAULT_COIL_ALGORITHM
    if((which == ELEM_COIL) || (which == ELEM_SET_PWM)) {
        // ELEM_COIL is a special case, see above
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
// clang-format on
//-----------------------------------------------------------------------------
static bool PersistVariable(char *name)
{
    if(persistVariables.count(name))
        return true;

    persistVariables.emplace(name);
    return false;
}

//-----------------------------------------------------------------------------
static void CheckPersistCircuit(int which, void *elem)
{
    ElemLeaf *l = (ElemLeaf *)elem;

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            int               i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(i = 0; i < s->count; i++) {
                CheckPersistCircuit(s->contents[i].which, s->contents[i].data.any);
            }
            break;
        }

        case ELEM_PARALLEL_SUBCKT: {
            int                 i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(i = 0; i < p->count; i++) {
                CheckPersistCircuit(p->contents[i].which, p->contents[i].data.any);
            }
            break;
        }
        case ELEM_PERSIST: {
            PersistVariable(l->d.persist.var);
            break;
        }
        default:
            break;
    }
}

//-----------------------------------------------------------------------------
static void CheckPersist()
{
    persistVariables.clear();
    for(int i = 0; i < Prog.numRungs; i++) {
        rungNow = i;
        CheckPersistCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    }
}

//-----------------------------------------------------------------------------
static bool CheckMasterCircuit(int which, void *elem)
{
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(int i = 0; i < s->count; i++) {
                if(CheckMasterCircuit(s->contents[i].which, s->contents[i].data.any))
                    return true;
            }
            break;
        }

        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(int i = 0; i < p->count; i++) {
                if(CheckMasterCircuit(p->contents[i].which, p->contents[i].data.any))
                    return true;
            }
            break;
        }
        case ELEM_MASTER_RELAY:
            return true;

        default:
            break;
    }
    return false;
}

//-----------------------------------------------------------------------------
static bool CheckMasterRelay()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(CheckMasterCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]))
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void WipeIntMemory()
{
    IntCode.clear();
}

//-----------------------------------------------------------------------------
// Generate intermediate code for the entire program. Return true if it worked,
// else false.
//-----------------------------------------------------------------------------
bool GenerateIntermediateCode()
{
  try {
    Comment("GenerateIntermediateCode");
    GenSymCount = 0;
    GenSymCountParThis = 0;
    GenSymCountParOut = 0;
    GenSymCountOneShot = 0;
    GenSymCountFormattedString = 0;
    GenSymCountStepper = 0;

    // The EEPROM addresses for the `Make Persistent' op are assigned at
    // int code generation time.
    EepromAddrFree = 0;

    rungNow = -100; //INT_MAX;
    whichNow = -INT_MAX;
    leafNow = nullptr;

    WipeIntMemory();

    AllocStart();

    CheckVariableNames();

#ifdef TABLE_IN_FLASH
    InitTables();
#endif
    InitVars();

    rungNow++;
    bool ExistMasterRelay = CheckMasterRelay();
    if(int_comment_level == 1) {
        // ExistMasterRelay = true; // Comment this for optimisation
    }
    if(ExistMasterRelay)
        Op(INT_SET_BIT, "$mcr");

    rungNow++;
    char      s1[MAX_COMMENT_LEN];
    char *    s2 = nullptr;
    ElemLeaf *leaf = nullptr;
    int       rung;
    for(rung = 0; rung <= Prog.numRungs; rung++) {
        rungNow = rung;
        whichNow = -INT_MAX;
        leafNow = nullptr;
        Prog.OpsInRung[rung] = 0;
        Prog.HexInRung[rung] = 0;
        sprintf(s1,"Rung%d", rung + 1);
        Op(INT_AllocFwdAddr, s1, (SDWORD)rung);
    }

    for(rung = 0; rung < Prog.numRungs; rung++) {
        rungNow = rung;
        whichNow = -INT_MAX;
        leafNow = nullptr;
        if(int_comment_level != 1) {
            Comment("");
            Comment("======= START RUNG %d =======", rung + 1);
        }
        sprintf(s1,"Rung%d", rung + 1);
        Op(INT_AllocKnownAddr, s1, (SDWORD)rung);
        Op(INT_FwdAddrIsNow, s1, (SDWORD)rung);

        if(Prog.rungs[rung]->count > 0 && Prog.rungs[rung]->contents[0].which == ELEM_COMMENT) {
            // nothing to do for this one
            // Yes, I do! Push comment into interpretable OP code for C and PASCAL.
            leaf = (ElemLeaf *)Prog.rungs[rung]->contents[0].data.any;
            AnsiToOem(leaf->d.comment.str, s1);
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
            if(int_comment_level >= 2) {
                if(strlen(s1))
                    Comment1(s1); // bypass % in comments
                if(strlen(s2))
                    Comment1(s2); // bypass % in comments
            }
            continue;
        }
        if(int_comment_level == 1) {
            Comment("");
            Comment("start rung %d", rung + 1);
        }
        if(ExistMasterRelay)
            Op(INT_COPY_BIT_TO_BIT, "$rung_top", "$mcr");
        else
            Op(INT_SET_BIT, "$rung_top");
        SimState(&(Prog.rungPowered[rung]), "$rung_top");
        IntCodeFromCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[rung], "$rung_top", rung);
    }
    rungNow++;
    sprintf(s1,"Rung%d", rung + 1);
    Op(INT_AllocKnownAddr, s1, (SDWORD)rung);
    Op(INT_FwdAddrIsNow, s1, (SDWORD)Prog.numRungs);
    rungNow++;
    //Calculate amount of intermediate codes in rungs
    for(int i = 0; i < MAX_RUNGS; i++)
        Prog.OpsInRung[i] = 0;
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        //dbp("IntPc=%d rung=%d ELEM_%x", i, IntCode[i].rung, IntCode[i].which);
        if((IntCode[i].rung >= 0) && (IntCode[i].rung < MAX_RUNGS) && (IntCode[i].op != INT_SIMULATE_NODE_STATE))
            Prog.OpsInRung[IntCode[i].rung]++;
    }

    //Listing of intermediate codes
    char CurrentPlFile[MAX_PATH] = "temp.pl";
    if(strlen(CurrentSaveFile))
        SetExt(CurrentPlFile, CurrentSaveFile, ".pl");
    IntDumpListing(CurrentPlFile);
  } catch (const std::exception &e) {
      char    buf[1024];
      sprintf(buf, "%s%s", _("Error when generate intermediate code:\n"), e.what());
      Error(buf);
      return false;
  }
    return true;
}

bool GotoGosubUsed()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_GOTO, ELEM_GOSUB, -1))
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
// Are either of the UART functions (send or recv) used? Need to know this
// to know whether we must receive their pins.
//-----------------------------------------------------------------------------
bool UartFunctionUsed()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if((ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_UART_RECV, ELEM_UART_SEND, ELEM_FORMATTED_STRING))
           || (ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_UART_RECVn, ELEM_UART_SENDn, -1))
           || (ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_UART_SEND_READY, ELEM_UART_RECV_AVAIL, -1)))
            return true;
    }

    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if((IntCode[i].op == INT_UART_SEND) ||       //
           (IntCode[i].op == INT_UART_SEND1) ||      //
           (IntCode[i].op == INT_UART_SENDn) ||      //
           (IntCode[i].op == INT_UART_SEND_READY) || //
           (IntCode[i].op == INT_UART_SEND_BUSY) ||  //
           (IntCode[i].op == INT_UART_RECV_AVAIL) || //
           (IntCode[i].op == INT_UART_RECVn) ||      //
           (IntCode[i].op == INT_UART_RECV))
            return true;
    }
    return false;
}

bool UartRecvUsed()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_UART_RECV, ELEM_UART_RECVn, -1))
            return true;
    }

    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if((IntCode[i].op == INT_UART_RECV) ||       //
           (IntCode[i].op == INT_UART_RECV_AVAIL) || //
           (IntCode[i].op == INT_UART_RECVn))
            return true;
    }
    return false;
}

bool UartSendUsed()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_UART_SEND, ELEM_UART_SENDn, ELEM_FORMATTED_STRING))
            return true;
    }

    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if((IntCode[i].op == INT_UART_SEND) ||       //
           (IntCode[i].op == INT_UART_SEND_READY) || //
           (IntCode[i].op == INT_UART_SEND_BUSY) ||  //
           (IntCode[i].op == INT_UART_SEND1) ||      //
           (IntCode[i].op == INT_UART_SENDn))
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------     ///// Modified by JG
bool SpiFunctionUsed()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_SPI))
            return true;
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_SPI_WR))
            return true;
    }

    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if((IntCode[i].op == INT_SPI) || (IntCode[i].op == INT_SPI_WRITE))
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------         ///// Added by JG
bool I2cFunctionUsed()
{
    for(int i = 0; i < Prog.numRungs; i++) {
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_I2C_RD))
            return true;
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_I2C_WR))
            return true;

    }

    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if((IntCode[i].op == INT_I2C_READ) || (IntCode[i].op == INT_I2C_WRITE))
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool Bin32BcdRoutineUsed()
{
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if(IntCode[i].op == INT_SET_BIN2BCD) {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// Are either of the MultiplyRoutine functions used?
//-----------------------------------------------------------------------------
bool MultiplyRoutineUsed()
{
    for(int i = 0; i < Prog.numRungs; i++)
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_MUL, ELEM_SET_PWM, -1))
            return true;

    for(uint32_t i = 0; i < IntCode.size(); i++)
        if(IntCode[i].op == INT_SET_VARIABLE_MULTIPLY)
            return true;

    return false;
}

//-----------------------------------------------------------------------------
// Are either of the DivideRoutine functions used?
//-----------------------------------------------------------------------------
bool DivideRoutineUsed()
{
    for(int i = 0; i < Prog.numRungs; i++)
        if(ContainsWhich(ELEM_SERIES_SUBCKT, Prog.rungs[i], ELEM_DIV, ELEM_SET_PWM, -1))
            return true;

    for(uint32_t i = 0; i < IntCode.size(); i++)
        if((IntCode[i].op == INT_SET_VARIABLE_DIVIDE) || (IntCode[i].op == INT_SET_VARIABLE_MOD))
            return true;

    return false;
}

// clang-format off

IntOp::IntOp() :
    op(0),
    literal(0),
    literal2(0),
    literal3(0),
    data(nullptr),
    poweredAfter(nullptr),
    workingNow(nullptr),
    rung(0),
    which(0),
    leaf(nullptr),
    fileLine(0),
    simulated(false)
{

}

// clang-format on
