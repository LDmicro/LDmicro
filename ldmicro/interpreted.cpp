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
// A crunched-down version of the intermediate code (e.g. assigning addresses
// to all the variables instead of just working with their names), suitable
// for interpretation.
// Jonathan Westhues, Aug 2005
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
#include "intcode.h"

static char Variables[MAX_IO][MAX_NAME_LEN];
static int  VariablesCount;

static char InternalRelays[MAX_IO][MAX_NAME_LEN];
static int  InternalRelaysCount;

typedef struct {
    int32_t op;
    int32_t name1;
    int32_t name2;
    int32_t name3;
    int32_t literal1;
} BinOp;

static BinOp OutProg[MAX_INT_OPS];

template <size_t N> static int32_t AddrForInternalRelay(const StringArray<N> &name)
{
    int32_t i;
    for(i = 0; i < InternalRelaysCount; i++) {
        if(name == InternalRelays[i]) {
            return i;
        }
    }
    strcpy(InternalRelays[i], name.c_str());
    InternalRelaysCount++;
    return i;
}

template <size_t N> static int32_t AddrForVariable(const StringArray<N> &name)
{
    int32_t i;
    for(i = 0; i < VariablesCount; i++) {
        if((name == Variables[i])) {
            return i;
        }
    }
    strcpy(Variables[i], name.c_str());
    VariablesCount++;
    return i;
}

static void Write(FileTracker &f, BinOp *op)
{
    uint8_t *b = (uint8_t *)op;
    for(uint32_t i = 0; i < sizeof(BinOp); i++) {
        fprintf(f, "%02x", b[i]);
    }
    fprintf(f, "\n");
}

void CompileInterpreted(const char *outFile)
{
    FileTracker f(outFile, "w");
    if(!f) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't write to '%s'"), outFile);
        return;
    }

    InternalRelaysCount = 0;
    VariablesCount = 0;

    fprintf(f, "$$LDcode\n");

    int   outPc;
    BinOp op;

    // Convert the if/else structures in the intermediate code to absolute
    // conditional jumps, to make life a bit easier for the interpreter.
#define MAX_IF_NESTING 32
    int ifDepth = 0;
    // PC for the if(...) instruction, which we will complete with the
    // 'jump to if false' address (which is either the ELSE+1 or the ENDIF+1)
    int ifOpIf[MAX_IF_NESTING];
    // PC for the else instruction, which we will complete with the
    // 'jump to if reached' address (which is the ENDIF+1)
    int ifOpElse[MAX_IF_NESTING];

    outPc = 0;
    for(uint32_t ipc = 0; ipc < IntCode.size(); ipc++) {
        memset(&op, 0, sizeof(op));
        op.op = IntCode[ipc].op;

        switch(IntCode[ipc].op) {
            case INT_CLEAR_BIT:
            case INT_SET_BIT:
                op.name1 = AddrForInternalRelay(IntCode[ipc].name1);
                break;

            case INT_COPY_BIT_TO_BIT:
                op.name1 = AddrForInternalRelay(IntCode[ipc].name1);
                op.name2 = AddrForInternalRelay(IntCode[ipc].name2);
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                op.literal1 = IntCode[ipc].literal1;
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
            case INT_SET_SEED_RANDOM:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                op.name2 = AddrForVariable(IntCode[ipc].name2);
                break;

            case INT_DECREMENT_VARIABLE:
            case INT_INCREMENT_VARIABLE:
            case INT_SET_VARIABLE_RANDOM:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                break;

            case INT_SET_VARIABLE_SHL:
            case INT_SET_VARIABLE_SHR:
            case INT_SET_VARIABLE_ROL:
            case INT_SET_VARIABLE_ROR:
            case INT_SET_VARIABLE_AND:
            case INT_SET_VARIABLE_OR:
            case INT_SET_VARIABLE_NOT:
            case INT_SET_VARIABLE_MOD:
            case INT_SET_VARIABLE_XOR:
            case INT_SET_VARIABLE_SR0:
            case INT_SET_VARIABLE_NEG:
            case INT_SET_VARIABLE_ADD:
            case INT_SET_VARIABLE_SUBTRACT:
            case INT_SET_VARIABLE_MULTIPLY:
            case INT_SET_VARIABLE_DIVIDE:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                op.name2 = AddrForVariable(IntCode[ipc].name2);
                op.name3 = AddrForVariable(IntCode[ipc].name3);
                break;

            case INT_IF_BIT_SET:
            case INT_IF_BIT_CLEAR:
                op.name1 = AddrForInternalRelay(IntCode[ipc].name1);
                goto finishIf;

            case INT_IF_VARIABLE_LES_LITERAL:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                op.literal1 = IntCode[ipc].literal1;
                goto finishIf;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
            case INT_IF_VARIABLE_GRT_VARIABLE:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                op.name2 = AddrForVariable(IntCode[ipc].name2);
                goto finishIf;
            finishIf:
                ifOpIf[ifDepth] = outPc;
                ifOpElse[ifDepth] = 0;
                ifDepth++;
                // jump target will be filled in later
                break;

            case INT_ELSE:
                ifOpElse[ifDepth - 1] = outPc;
                // jump target will be filled in later
                break;

            case INT_END_IF:
                --ifDepth;
                if(ifOpElse[ifDepth] == 0) {
                    // There is no else; if should jump straight to the
                    // instruction after this one if the condition is false.
                    OutProg[ifOpIf[ifDepth]].name3 = outPc - 1;
                } else {
                    // There is an else clause; if the if is false then jump
                    // just past the else, and if the else is reached then
                    // jump to the endif.
                    OutProg[ifOpIf[ifDepth]].name3 = ifOpElse[ifDepth];
                    OutProg[ifOpElse[ifDepth]].name3 = outPc - 1;
                }
                // But don't generate an instruction for this.
                continue;

            case INT_SIMULATE_NODE_STATE:
            case INT_COMMENT:
                // Don't care; ignore, and don't generate an instruction.
                continue;

            case INT_AllocFwdAddr:
            case INT_AllocKnownAddr:
            case INT_FwdAddrIsNow:
            case INT_GOTO:
            case INT_GOSUB:
            case INT_RETURN:
                // TODO
                break;

            case INT_EEPROM_BUSY_CHECK:
            case INT_EEPROM_READ:
            case INT_EEPROM_WRITE:
            case INT_READ_ADC:
            case INT_SET_PWM:
            case INT_SPI:
            case INT_SPI_WRITE: ///// Added by JG
            case INT_I2C_READ:  /////
            case INT_I2C_WRITE: /////
                                //            case INT_UART_SEND:
            case INT_UART_SEND1:
                //            case INT_UART_SENDn:
                //            case INT_UART_RECV:
            case INT_UART_SEND_BUSY:
            case INT_UART_SEND_READY:
            case INT_UART_RECV_AVAIL:
            case INT_WRITE_STRING:
            case INT_STRING:
            default:
                THROW_COMPILER_EXCEPTION_FMT("%s INT_%d", _("Unsupported op (Peripheral) for interpretable target."), IntCode[ipc].op);
                return;
        }

        memcpy(&OutProg[outPc], &op, sizeof(op));
        outPc++;
    }

    for(int i = 0; i < outPc; i++) {
        Write(f, &OutProg[i]);
    }
    memset(&op, 0, sizeof(op));
    op.op = INT_END_OF_PROGRAM;
    Write(f, &op);

    fprintf(f, "$$bits\n");
    for(int i = 0; i < InternalRelaysCount; i++) {
        if(InternalRelays[i][0] != '$') {
            fprintf(f, "%s,%d\n", InternalRelays[i], i);
        }
    }
    fprintf(f, "$$int16s\n");
    for(int i = 0; i < VariablesCount; i++) {
        if(Variables[i][0] != '$') {
            fprintf(f, "%s,%d\n", Variables[i], i);
        }
    }

    fprintf(f, "$$cycle %lld us\n", Prog.cycleTime);

    char str[MAX_PATH + 500];
    sprintf(str, _("Compile successful; wrote interpretable code to '%s'.\r\n\r\nYou probably have to adapt the interpreter to your application. See the documentation."), outFile);
    CompileSuccessfulMessage(str);
}
