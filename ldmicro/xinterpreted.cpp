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

static std::vector<uint8_t> OutProg;

#define MAX_PLCIO 256

static const char *PlcIos[MAX_PLCIO];
static int         PlcIos_size = 0;

int PlcIos_AppendAndGet(const char *name)
{
    for(int i = 0; i < PlcIos_size; i++) {
        if(strcmp(PlcIos[i], name) == 0)
            return i;
    }

    if(PlcIos_size == MAX_PLCIO)
        return -1;
    PlcIos[PlcIos_size++] = name;
    return PlcIos_size - 1;
}

static int CheckRange(int value, const char *name)
{
    if(value < 0 || value > 255) {
        char msg[80];
        sprintf(msg, _("%s=%d: out of range for 8bits target"), name, value);
        THROW_COMPILER_EXCEPTION(msg, 0);
    }

    return value;
}

static BYTE GetArduinoPinNumber(int pin)
{
    if(Prog.mcu())
        for(uint32_t i = 0; i < Prog.mcu()->pinCount; i++) {
            if(Prog.mcu()->pinInfo[i].pin == pin)
                return Prog.mcu()->pinInfo[i].ArduinoPin;
        }
    return 0;
}

static BYTE AddrForBit(const char *name)
{
    return CheckRange(PlcIos_AppendAndGet(name), name);
}

static BYTE AddrForVariable(const char *name)
{
    return CheckRange(PlcIos_AppendAndGet(name), name);
}

void CompileXInterpreted(const char *outFile)
{
    FileTracker f(outFile, "w");
    if(!f) {
        THROW_COMPILER_EXCEPTION_FMT(_("Couldn't write to '%s'"), outFile);
        return;
    }

    // Preload physical IOs in the table
    PlcIos_size = 0;

    for(int i = 0; i < Prog.io.count; i++) {
        PlcIos[PlcIos_size++] = Prog.io.assignment[i].name;
    }
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

    OutProg.clear();
    OutProg.reserve(IntCode.size() * 2);
    for(uint32_t ipc = 0; ipc < IntCode.size(); ipc++) {
        switch(IntCode[ipc].op) {
            case INT_CLEAR_BIT:
            case INT_SET_BIT:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForBit(IntCode[ipc].name1.c_str()));
                break;

            case INT_COPY_BIT_TO_BIT:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForBit(IntCode[ipc].name1.c_str()));
                OutProg.push_back(AddrForBit(IntCode[ipc].name2.c_str()));
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForVariable(IntCode[ipc].name1.c_str()));
                OutProg.push_back((uint8_t)(IntCode[ipc].literal & 0xFF));
                OutProg.push_back((uint8_t)((IntCode[ipc].literal >> 8) & 0xFF));
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForVariable(IntCode[ipc].name1.c_str()));
                OutProg.push_back(AddrForVariable(IntCode[ipc].name2.c_str()));
                break;

            case INT_DECREMENT_VARIABLE:
            case INT_INCREMENT_VARIABLE:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForVariable(IntCode[ipc].name1.c_str()));
                break;

            case INT_SET_VARIABLE_ADD:
            case INT_SET_VARIABLE_SUBTRACT:
            case INT_SET_VARIABLE_MULTIPLY:
            case INT_SET_VARIABLE_DIVIDE:
            case INT_SET_VARIABLE_MOD:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForVariable(IntCode[ipc].name1.c_str()));
                OutProg.push_back(AddrForVariable(IntCode[ipc].name2.c_str()));
                OutProg.push_back(AddrForVariable(IntCode[ipc].name3.c_str()));
                break;

            case INT_SET_PWM:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForVariable(IntCode[ipc].name1.c_str()));
                OutProg.push_back((uint8_t)(IntCode[ipc].literal & 0xFF));
                OutProg.push_back((uint8_t)((IntCode[ipc].literal >> 8) & 0xFF));
                break;

            case INT_READ_ADC:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForVariable(IntCode[ipc].name1.c_str()));
                break;

            case INT_IF_BIT_SET:
            case INT_IF_BIT_CLEAR:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForBit(IntCode[ipc].name1.c_str()));
                goto finishIf;
            case INT_IF_VARIABLE_LES_LITERAL:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForVariable(IntCode[ipc].name1.c_str()));
                OutProg.push_back((uint8_t)(IntCode[ipc].literal & 0xFF));
                OutProg.push_back((uint8_t)((IntCode[ipc].literal >> 8) & 0xFF));
                goto finishIf;
            case INT_IF_VARIABLE_EQUALS_VARIABLE:
            case INT_IF_VARIABLE_GRT_VARIABLE:
                OutProg.push_back(IntCode[ipc].op);
                OutProg.push_back(AddrForVariable(IntCode[ipc].name1.c_str()));
                OutProg.push_back(AddrForVariable(IntCode[ipc].name2.c_str()));
                goto finishIf;
            finishIf:
                ifOpIf[ifDepth] = OutProg.size();
                OutProg.push_back(0);
                ifOpElse[ifDepth] = 0;
                ifDepth++;
                // jump target will be filled in later
                break;

            case INT_ELSE:
                OutProg.push_back(IntCode[ipc].op);
                ifOpElse[ifDepth - 1] = OutProg.size();
                OutProg.push_back(0);
                // jump target will be filled in later
                break;

            case INT_END_IF:
                --ifDepth;
                if(ifOpElse[ifDepth] == 0) {
                    // There is no else; if should jump straight to the
                    // instruction after this one if the condition is false.
                    OutProg[ifOpIf[ifDepth]] = CheckRange(OutProg.size() - 1 - ifOpIf[ifDepth], "pc");
                } else {
                    // There is an else clause; if the if is false then jump
                    // just past the else, and if the else is reached then
                    // jump to the endif.
                    OutProg[ifOpIf[ifDepth]] = CheckRange(ifOpElse[ifDepth] - ifOpIf[ifDepth], "pc");
                    OutProg[ifOpElse[ifDepth]] = CheckRange(OutProg.size() - 1 - ifOpElse[ifDepth], "pc");
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
            case INT_SPI:
            case INT_SPI_WRITE:         ///// Added by JG
            case INT_I2C_READ:          /////
            case INT_I2C_WRITE:         /////
            case INT_UART_SEND:
            case INT_UART_SEND1:
            case INT_UART_SENDn:
            case INT_UART_RECV:
            case INT_UART_SEND_READY:
            case INT_UART_SEND_BUSY:
            case INT_UART_RECV_AVAIL:
            case INT_WRITE_STRING:
            default:
                THROW_COMPILER_EXCEPTION_FMT(
                    _("Unsupported op (Peripheral) for interpretable target.\nINT_%d"), IntCode[ipc].op);
                return;
        }
    }

    OutProg.push_back(INT_END_OF_PROGRAM);

    // Create a map of io and internal variables
    // $$IO nb_named_IO total_nb_IO
    fprintf(f, "$$IO %d %d\n", Prog.io.count, PlcIos_size);

    for(int i = 0; i < Prog.io.count; i++) {
        PlcProgramSingleIo io = Prog.io.assignment[i];
        fprintf(f,
                "%2d %20s %2d %2d %2d %05d\n",
                i,
                io.name,
                io.type,
                GetArduinoPinNumber(io.pin),
                io.modbus.Slave,
                io.modbus.Address);
    }

    // $$LDcode program_size
    fprintf(f, "$$LDcode %d\n", OutProg.size());
    for(uint32_t i = 0; i < OutProg.size(); i++) {
        fprintf(f, "%02X", OutProg[i]);
        if((i % 16) == 15 || i == OutProg.size() - 1)
            fprintf(f, "\n");
    }

    fprintf(f, "$$cycle %lld us\n", Prog.cycleTime);

    ///// Added by JG
    if(CompileFailure) return;
    /////

    char str[MAX_PATH + 500];
    sprintf(str,
            _("Compile successful; wrote interpretable code to '%s'.\r\n\r\n"
              "You probably have to adapt the interpreter to your application. See "
              "the documentation."),
            outFile);
    CompileSuccessfulMessage(str);
}
