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
#include <windows.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

#define _ITERATOR_DEBUG_LEVEL 0

#include <string>
#include <map>
using namespace std;

#include "ldmicro.h"
#include "intcode.h"
#include "xinterpreter.h"

class dictionary : public map<string, int>
{
public:
	int AppendAndGet(string k) 
	{
		if (this->find(k) == end()) (*this)[k] = (int)this->size();
		return (*this)[k];
	}
};

static dictionary InternalVariables;
static dictionary InternalRelays;
static dictionary ModbusRelays;
static map<string, PlcProgramSingleIo> PlcIos;

static BYTE OutProg[MAX_INT_OPS];

static int CheckRange(int value, char *name)
{
	if (value < 0 || value > 255) {
		char msg[80];
		sprintf(msg, _("%s=%d: out of range for 8bits target"), name, value);
		Error(msg);
	}

	return value;
}

static BYTE GetArduinoPinNumber(PlcProgramSingleIo *io)
{
	int pin = io->pin;
	for (int i = 0; i < Prog.mcu->pinCount; i++) {
		if (Prog.mcu->pinInfo[i].pin == pin)
			return Prog.mcu->pinInfo[i].ArduinoPin;
	}
	return 0;
}

static BYTE AddrForBit(char *name)
{
	PlcProgramSingleIo io = PlcIos[name];
	int addr = 0;

	switch (io.type) {
	case IO_TYPE_DIG_INPUT:
	case IO_TYPE_DIG_OUTPUT:
		addr = GetArduinoPinNumber(&io);
		break;
	case IO_TYPE_MODBUS_COIL:
	case IO_TYPE_MODBUS_CONTACT:
		addr = XMODBUS_OFFSET + ModbusRelays.AppendAndGet(name);
		break;
	default:
		addr = XINTERNAL_OFFSET + InternalRelays.AppendAndGet(name);
		break;
	}

	return CheckRange(addr, name);
}

static BYTE AddrForVariable(char *name)
{
	PlcProgramSingleIo io = PlcIos[name];
	int addr = 0;

	switch (io.type) {
	case IO_TYPE_PWM_OUTPUT:
	case IO_TYPE_READ_ADC:
		addr = GetArduinoPinNumber(&io);
		break;
	case IO_TYPE_MODBUS_COIL:
	case IO_TYPE_MODBUS_CONTACT:
		addr = XMODBUS_OFFSET + ModbusRelays.AppendAndGet(name);
		break;
	default:
		addr = XINTERNAL_OFFSET + InternalVariables.AppendAndGet(name);
		break;
	}

	return CheckRange(addr, name);
}

void CompileXInterpreted(char *outFile)
{
    FILE *f = fopen(outFile, "w");
    if(!f) {
        Error(_("Couldn't write to '%s'"), outFile);
        return;
    }

	InternalRelays.clear();
	InternalVariables.clear();
	ModbusRelays.clear();

	// Create a map off io assignment
	PlcIos.clear();
	for (int i = 0; i < Prog.io.count; i++) {
		PlcIos[Prog.io.assignment[i].name] = Prog.io.assignment[i];
	}

    fprintf(f, "$$LDcode\n");

    int ipc;
    int outPc;

    // Convert the if/else structures in the intermediate code to absolute
    // conditional jumps, to make life a bit easier for the interpreter.
#define MAX_IF_NESTING      32
    int ifDepth = 0;
    // PC for the if(...) instruction, which we will complete with the
    // 'jump to if false' address (which is either the ELSE+1 or the ENDIF+1)
    int ifOpIf[MAX_IF_NESTING];
    // PC for the else instruction, which we will complete with the
    // 'jump to if reached' address (which is the ENDIF+1)
    int ifOpElse[MAX_IF_NESTING];

    outPc = 0;
    for(ipc = 0; ipc < IntCodeLen; ipc++) {
        switch(IntCode[ipc].op) {
            case INT_CLEAR_BIT:
            case INT_SET_BIT:
				OutProg[outPc++] = IntCode[ipc].op;
				OutProg[outPc++] = AddrForBit(IntCode[ipc].name1);
                break;

            case INT_COPY_BIT_TO_BIT:
				OutProg[outPc++] = IntCode[ipc].op;
				OutProg[outPc++] = AddrForBit(IntCode[ipc].name1);
				OutProg[outPc++] = AddrForBit(IntCode[ipc].name2);
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
				OutProg[outPc++] = IntCode[ipc].op;
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name1);
				OutProg[outPc++] = IntCode[ipc].literal & 0xFF;
				OutProg[outPc++] = IntCode[ipc].literal >> 8;
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
				OutProg[outPc++] = IntCode[ipc].op; 
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name1);
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name2);
                break;

            case INT_INCREMENT_VARIABLE:
				OutProg[outPc++] = IntCode[ipc].op; 
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name1);
                break;

            case INT_SET_VARIABLE_ADD:
            case INT_SET_VARIABLE_SUBTRACT:
            case INT_SET_VARIABLE_MULTIPLY:
            case INT_SET_VARIABLE_DIVIDE:
				OutProg[outPc++] = IntCode[ipc].op; 
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name1);
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name2);
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name3);
                break;
			
			case INT_SET_PWM:
				OutProg[outPc++] = IntCode[ipc].op; 
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name1);
				OutProg[outPc++] = IntCode[ipc].literal & 0xFF;
				OutProg[outPc++] = IntCode[ipc].literal >> 8;
				break;

			case INT_READ_ADC:
				OutProg[outPc++] = IntCode[ipc].op; 
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name1);
				break;

            case INT_IF_BIT_SET:
            case INT_IF_BIT_CLEAR:
				OutProg[outPc++] = IntCode[ipc].op; 
				OutProg[outPc++] = AddrForBit(IntCode[ipc].name1);
                goto finishIf;
            case INT_IF_VARIABLE_LES_LITERAL:
				OutProg[outPc++] = IntCode[ipc].op; 
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name1);
				OutProg[outPc++] = IntCode[ipc].literal & 0xFF;
				OutProg[outPc++] = IntCode[ipc].literal >> 8;
                goto finishIf;
            case INT_IF_VARIABLE_EQUALS_VARIABLE:
            case INT_IF_VARIABLE_GRT_VARIABLE:
				OutProg[outPc++] = IntCode[ipc].op; 
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name1);
				OutProg[outPc++] = AddrForVariable(IntCode[ipc].name2);
                goto finishIf;
finishIf:
                ifOpIf[ifDepth] = outPc++;
                ifOpElse[ifDepth] = 0;
                ifDepth++;
                // jump target will be filled in later
                break;

            case INT_ELSE:
				OutProg[outPc++] = IntCode[ipc].op; 
				ifOpElse[ifDepth-1] = outPc++;
                // jump target will be filled in later
                break;

            case INT_END_IF:
                --ifDepth;
                if(ifOpElse[ifDepth] == 0) {
                    // There is no else; if should jump straight to the
                    // instruction after this one if the condition is false.
                    OutProg[ifOpIf[ifDepth]] = CheckRange(outPc-1 - ifOpIf[ifDepth], "pc");
                } else {
                    // There is an else clause; if the if is false then jump
                    // just past the else, and if the else is reached then
                    // jump to the endif.
                    OutProg[ifOpIf[ifDepth]] = CheckRange(ifOpElse[ifDepth] - ifOpIf[ifDepth], "pc");
                    OutProg[ifOpElse[ifDepth]]= CheckRange(outPc-1 - ifOpElse[ifDepth], "pc");
                }
                // But don't generate an instruction for this.
                continue;

            case INT_SIMULATE_NODE_STATE:
            case INT_COMMENT:
                // Don't care; ignore, and don't generate an instruction.
                continue;

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

            case INT_EEPROM_BUSY_CHECK:
            case INT_EEPROM_READ:
            case INT_EEPROM_WRITE:
            case INT_UART_SEND:
            case INT_UART_RECV:
            case INT_WRITE_STRING:
            default:
                Error(_("Unsupported op (anything UART, EEPROM, SFR..) for "
                    "interpretable target."));
                fclose(f);
                return;
        }
    }
	
	OutProg[outPc++] = INT_END_OF_PROGRAM;

    for(int i = 0; i < outPc; i++) {
        fprintf(f, "%02X", OutProg[i]);
		if ( (i % 16) == 15 || i == outPc-1) fprintf(f, "\n");
    }

    fprintf(f, "$$bits\n");
	for (auto const &ent1 : InternalRelays) {
		if (ent1.first[0] == '$') continue;
		fprintf(f, "%s:%d\n", ent1.first.c_str(), ent1.second);
	}

	fprintf(f, "$$int16s\n");
	for (auto const &ent1 : InternalVariables) {
		if (ent1.first[0] == '$') continue;
		fprintf(f, "%s:%d\n", ent1.first.c_str(), ent1.second);
	}

	fprintf(f, "$$pins\n");
	for (int i = 0; i < Prog.io.count; i++) {
		if (Prog.io.assignment[i].type != IO_TYPE_DIG_INPUT && Prog.io.assignment[i].type != IO_TYPE_DIG_OUTPUT) continue;
		int pin = Prog.io.assignment[i].pin;
		for (int j = 0; j < Prog.mcu->pinCount; j++) {
			if (Prog.mcu->pinInfo[j].pin == pin) {
				fprintf(f, "%s:%d\n", Prog.mcu->pinInfo[j].pinName, Prog.mcu->pinInfo[j].ArduinoPin);
				break;
			}
		}
	}

	fprintf(f, "$$pin16s\n");
	for (int i = 0; i < Prog.io.count; i++) {
		if (Prog.io.assignment[i].type != IO_TYPE_READ_ADC && Prog.io.assignment[i].type != IO_TYPE_PWM_OUTPUT) continue;
		int pin = Prog.io.assignment[i].pin;
		for (int j = 0; j < Prog.mcu->pinCount; j++) {
			if (Prog.mcu->pinInfo[j].pin == pin) {
				fprintf(f, "%s:%d\n", Prog.mcu->pinInfo[j].pinName, Prog.mcu->pinInfo[j].ArduinoPin);
				break;
			}
		}
	}

	fprintf(f, "$$modbus_pins\n");
	for (auto const &ent1 : ModbusRelays) {
		PlcProgramSingleIo io = PlcIos[ent1.first];
		fprintf(f, "%s:%d %d:%05d\n", ent1.first.c_str(), ent1.second, io.modbus.Slave, io.modbus.Register);
	}

    fprintf(f, "$$cycle %d us\n", Prog.cycleTime);

    fclose(f);

    char str[MAX_PATH+500];
    sprintf(str,
      _("Compile successful; wrote interpretable code to '%s'.\r\n\r\n"
        "You probably have to adapt the interpreter to your application. See "
        "the documentation."), outFile);
    CompileSuccessfulMessage(str);
}
