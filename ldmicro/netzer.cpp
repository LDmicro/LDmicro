//-----------------------------------------------------------------------------
// Copyright 2012-2013 Sven Schlender
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

#include "netzer.h"

#define NOT_LOCATED_YET (0x7FFFu)
#define MAPPED_TO_IO (0x8000u)

// The time epoch for 1.1.2000 (difference from 1.1.1970).
#define TIME_EPOCH (86400ul * (365ul * 30ul + 7ul))

typedef struct RegisterEntryTag {
    char Name[MAX_NAME_LEN];
    WORD Address;
} RegisterEntry;

typedef struct {
    WORD   op;
    WORD   name1;
    WORD   name2;
    WORD   name3;
    int32_t literal;
} BinOp;

static BinOp         OutProg[MAX_INT_OPS];
static RegisterEntry Variables[MAX_IO];
static int           VariablesCount;

static RegisterEntry Relays[MAX_IO];
static int           RelaysCount;

static char Strings[MAX_IO][MAX_NAME_LEN];
static int  StringsCount;

static void generateNetzerOpcodes(BinOp *Program, int MaxLabel, OpcodeMeta *pOpcodeMeta, FILE *f = nullptr);
static BYTE getInternalIntegerAddress(WORD Address);

static int GetLocalVariablesAsMetaTags(FILE *f = nullptr)
{
    int i;
    int metas = 0;
    for(i = 0; i < VariablesCount; i++) {
        if((Variables[i].Name[0] != '$') && !(Variables[i].Address & MAPPED_TO_IO)) {
            int name_len = strlen(Variables[i].Name);
            metas += name_len + 3;
            if(f) {
                // File handle is given. Write the tag directly to the file.
                fputc(MTT_LOCAL_VARIABLE, f); // Type tag.
                fputc(name_len + 1, f);       // Tag length.

                fputc(getInternalIntegerAddress(Variables[i].Address), f); // Write address of register.
                fwrite(Variables[i].Name, 1, name_len, f);                 // Write name.
            }
        }
    }

    return metas;
}

static int GetLocalRelaysAsMetaTags(FILE *f = nullptr)
{
    int i;
    int metas = 0;
    for(i = 0; i < RelaysCount; i++) {
        if((Relays[i].Name[0] != '$') && !(Relays[i].Address & MAPPED_TO_IO)) {
            int name_len = strlen(Relays[i].Name);
            metas += name_len + 3;
            if(f) {
                // File handle is given. Write the tag directly to the file.
                fputc(MTT_LOCAL_VARIABLE, f);           // Type tag.
                fputc(name_len + 1, f);                 // Tag length.
                fputc(Relays[i].Address & 0xFF, f);     // Write address of register.
                fwrite(Relays[i].Name, 1, name_len, f); // Write name.
            }
        }
    }

    return metas;
}

static WORD AddrForString(const NameArray &name)
{
    int i;
    for(i = 0; i < StringsCount; i++) {
        if((name == Strings[i])) {
            return i;
        }
    }
    strcpy(Strings[i], name.c_str());
    StringsCount++;
    return i;
}

static WORD AddrForRelay(const NameArray &name)
{
    // Look within the relays for an already known entry.
    int i;
    for(i = 0; i < RelaysCount; i++) {
        if((name == Relays[i].Name)) {
            return Relays[i].Address;
        }
    }

    // Prepare new entry.
    strcpy(Relays[i].Name, name.c_str());
    RelaysCount++;
    Relays[i].Address = NOT_LOCATED_YET; // Mark as not located yet.

    // Try to parse the given register name.

    // Locater included?
    auto n = strchr(name.c_str(), '@');
    if(n) {
        int bit = 0;
        n++; // Throw away the @ symbol.

        // No PAB mapping, maybe IO mapping.
        if(sscanf(n, "%x", &bit) == 1) {
            Relays[i].Address = bit | MAPPED_TO_IO;
        }
    }

    return Relays[i].Address;
}

static int GetPercentCharactersCount(const NameArray &name)
{
    int  found = 0;
    auto Search = name.c_str();
    while(Search[0]) {
        if(Search[0] == '%') {
            if(Search[1] == '%') {
                // Do not count double % (escaped.)
                Search++;
            } else {
                found++;
            }
        }
        Search++;
    }

    return found;
}

template <size_t N> static WORD AddrForVariable(const StringArray<N> &name)
{
    // Look within the variables for an already known entry.
    int i;
    for(i = 0; i < VariablesCount; i++) {
        if((name == Variables[i].Name)) {
            return Variables[i].Address;
        }
    }

    // Prepare new entry.
    strcpy(Variables[i].Name, name.c_str());
    VariablesCount++;
    Variables[i].Address = NOT_LOCATED_YET; // Mark as not located yet.

    // Try to parse the given register name.

    // Locater included?
    auto n = strchr(name.c_str(), '@');
    if(n) {
        int reg = 0;
        n++; // Throw away the @ symbol.

        // No PAB mapping, maybe IO mapping.
        if(sscanf(n, "%x", &reg) == 1) {
            Variables[i].Address = reg | MAPPED_TO_IO;
        }
    }

    return Variables[i].Address;
}

static WORD AddrForVariable(gsl::cstring_span name)
{
    NameArray na(name.data());
    return AddrForVariable(na);
}

static void MapNotLocatedElements()
{
    int address = 0;

    // First we look for not already mapped relays.
    // The addressing scheme is linear, here are some examples:
    // Address 0 is register 0 bit 0 (0.0).
    // Address 15 is register 0 bit 15 (0.15).
    // Address 16 is register 1 bit 0 (1.0).

    int i;
    for(i = 0; i < RelaysCount; i++) {
        if(Relays[i].Address != NOT_LOCATED_YET) {
            // Relay is already mapped to address (common or io PAB).
            continue;
        }

        // Now we have to find a free address.
        int j;

        // Only relais which follow the current relay are considered here.
        for(j = i + 1; j < RelaysCount; j++) {
            if((Relays[j].Address == NOT_LOCATED_YET) || (Relays[j].Address & MAPPED_TO_IO)) {
                continue;
            } else if(Relays[j].Address == address) {
                // This bit address is already occupied.
                address++;
            }
        }

        Relays[i].Address = address++;
    }

    // Bring next free address to register alignment and leave as register address.
    address = (address / 16) + 1;

    for(i = 0; i < VariablesCount; i++) {
        if(Variables[i].Address != NOT_LOCATED_YET) {
            continue;
        }

        int j;
        for(j = i + 1; j < VariablesCount; j++) {
            if((Variables[j].Address == NOT_LOCATED_YET) || (Variables[j].Address & MAPPED_TO_IO)) {
                continue;
            } else if(Variables[j].Address == address) {
                // This register address is already occupied (take the next register address).
                address++;
            }
        }

        Variables[i].Address = address++;
    }
}

static void locateRegister()
{
    for(uint32_t ipc = 0; ipc < IntCode.size(); ipc++) {
        switch(IntCode[ipc].op) {
            case INT_CLEAR_BIT:
            case INT_SET_BIT:
            case INT_IF_BIT_SET:
            case INT_IF_BIT_CLEAR:
                AddrForRelay(IntCode[ipc].name1);
                break;

            case INT_COPY_BIT_TO_BIT:
                AddrForRelay(IntCode[ipc].name1);
                AddrForRelay(IntCode[ipc].name2);
                break;

            case INT_DECREMENT_VARIABLE:
            case INT_INCREMENT_VARIABLE:
            case INT_SET_VARIABLE_TO_LITERAL:
            case INT_IF_VARIABLE_LES_LITERAL:
                AddrForVariable(IntCode[ipc].name1); // dest
                break;

            case INT_WRITE_STRING:
                AddrForVariable(IntCode[ipc].name1); // dest
                AddrForVariable(IntCode[ipc].name2); // src
                break;

            case INT_SET_BIN2BCD:
            case INT_SET_BCD2BIN:
            case INT_SET_OPPOSITE:
            case INT_SET_VARIABLE_TO_VARIABLE:
            case INT_SET_SWAP:
                AddrForVariable(IntCode[ipc].name1);
                AddrForVariable(IntCode[ipc].name2);
                break;

            case INT_SET_VARIABLE_NOT:
            case INT_SET_VARIABLE_NEG:
                if(AddrForVariable(IntCode[ipc].name1) & MAPPED_TO_IO) {
                    AddrForVariable("$dummy1");
                }
                if(AddrForVariable(IntCode[ipc].name2) & MAPPED_TO_IO) {
                    AddrForVariable("$dummy2");
                }
                break;

            case INT_SET_VARIABLE_SHL:
            case INT_SET_VARIABLE_SHR:
            case INT_SET_VARIABLE_ROL:
            case INT_SET_VARIABLE_ROR:
            case INT_SET_VARIABLE_AND:
            case INT_SET_VARIABLE_OR:
            case INT_SET_VARIABLE_XOR:
            case INT_SET_VARIABLE_MOD:
            case INT_SET_VARIABLE_SR0:
            case INT_SET_VARIABLE_ADD:
            case INT_SET_VARIABLE_SUBTRACT:
            case INT_SET_VARIABLE_MULTIPLY:
            case INT_SET_VARIABLE_DIVIDE:
                // Some opcodes do not support direct PAB IO register access.
                // The interpreter can use dummy register to work around this issue.
                // The IO registers are loaded into the dummy registers before the operation.
                // The result is written back after the operation (if needed).
                if(AddrForVariable(IntCode[ipc].name1) & MAPPED_TO_IO) {
                    AddrForVariable("$dummy1");
                }
                if(AddrForVariable(IntCode[ipc].name2) & MAPPED_TO_IO) {
                    AddrForVariable("$dummy2");
                }
                if(AddrForVariable(IntCode[ipc].name3) & MAPPED_TO_IO) {
                    AddrForVariable("$dummy3");
                }
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
            case INT_IF_VARIABLE_GRT_VARIABLE:
                if(AddrForVariable(IntCode[ipc].name1) & MAPPED_TO_IO) {
                    AddrForVariable("$dummy1");
                }
                if(AddrForVariable(IntCode[ipc].name2) & MAPPED_TO_IO) {
                    AddrForVariable("$dummy2");
                }
                break;
        }
    }

    MapNotLocatedElements();
}

int GenerateIntOpcodes()
{
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
                op.name1 = AddrForRelay(IntCode[ipc].name1);
                break;

            case INT_COPY_BIT_TO_BIT:
                op.name1 = AddrForRelay(IntCode[ipc].name1);
                op.name2 = AddrForRelay(IntCode[ipc].name2);
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                op.literal = IntCode[ipc].literal;
                break;

            case INT_SET_BIN2BCD:
            case INT_SET_BCD2BIN:
            case INT_SET_OPPOSITE:
            case INT_SET_VARIABLE_NOT:
            case INT_SET_SWAP:
            case INT_SET_VARIABLE_NEG:
            case INT_SET_VARIABLE_TO_VARIABLE:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                op.name2 = AddrForVariable(IntCode[ipc].name2);
                break;

            case INT_DECREMENT_VARIABLE:
            case INT_INCREMENT_VARIABLE:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                break;

            case INT_SET_VARIABLE_SHL:
            case INT_SET_VARIABLE_SHR:
            case INT_SET_VARIABLE_AND:
            case INT_SET_VARIABLE_OR:
            case INT_SET_VARIABLE_XOR:
            case INT_SET_VARIABLE_MOD:
            case INT_SET_VARIABLE_SR0:
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
                op.name1 = AddrForRelay(IntCode[ipc].name1);
                goto finishIf;

            case INT_IF_VARIABLE_LES_LITERAL:
                op.name1 = AddrForVariable(IntCode[ipc].name1);
                op.literal = IntCode[ipc].literal;
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

            case INT_WRITE_STRING:
                op.name1 = AddrForVariable(IntCode[ipc].name1); // dest var
                op.name2 = AddrForVariable(IntCode[ipc].name2); // source var
                op.name3 = AddrForString(IntCode[ipc].name3);   // source string

                if(!(op.name1 & MAPPED_TO_IO)) {
                    THROW_COMPILER_EXCEPTION(_("Dest variable of write string instruction must be located at IO register."));
                    return -1;
                }

                // Check whether only one % sign is included!
                if(GetPercentCharactersCount(IntCode[ipc].name3) > 1) {
                    THROW_COMPILER_EXCEPTION(_("Maximal one format placeholder is allowed in write string instruction."));
                    return -1;
                }

                break;

            case INT_SIMULATE_NODE_STATE:
            case INT_COMMENT:
                // Don't care; ignore, and don't generate an instruction.
                continue;

            case INT_EEPROM_BUSY_CHECK:
            case INT_EEPROM_READ:
            case INT_EEPROM_WRITE:
            case INT_READ_ADC:
            case INT_SET_PWM:
            case INT_UART_SEND:
            case INT_UART_SEND1:
            case INT_UART_SENDn:
            case INT_UART_RECV:
            case INT_UART_RECV_AVAIL:
            case INT_UART_SEND_READY:
            case INT_UART_SEND_BUSY:
            default:
                THROW_COMPILER_EXCEPTION(_("Unsupported op (anything ADC, PWM, UART, EEPROM) for Netzer target."));
                return -1;
        }

        memcpy(&OutProg[outPc], &op, sizeof(op));
        outPc++;
    }

    memset(&op, 0, sizeof(op));
    op.op = INT_END_OF_PROGRAM;
    memcpy(&OutProg[outPc++], &op, sizeof(op));

    return outPc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static WORD calculateCRC(FILE *f, WORD Size)
{
    WORD crc = 0xffff;

    while(Size--) {
        BYTE tmp = (BYTE)fgetc(f);
        crc = (BYTE)(crc >> 8) | (crc << 8);
        crc ^= tmp;
        crc ^= (BYTE)(crc & 0xff) >> 4;
        crc ^= crc << 12;
        crc ^= (crc & 0xff) << 5;
    }
    return crc;
}

///////////////////////////////////////////////////////////////////////////////

static WORD calculateJumpLabel(WORD DestinationLabel)
{
    OpcodeMeta opcodeMeta;
    opcodeMeta.BytesConsumed = 0;
    opcodeMeta.Opcodes = 0;

    generateNetzerOpcodes(OutProg, DestinationLabel + 1, &opcodeMeta);
    return (WORD)opcodeMeta.BytesConsumed;
}

///////////////////////////////////////////////////////////////////////////////

static BYTE getIOAddress(WORD Address)
{
    return (BYTE)(Address);
}

///////////////////////////////////////////////////////////////////////////////

static void clearBit(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f, "%c%c", OP_BIT_CLEAR_IO, getIOAddress(Op->name1));
        pMeta->BytesConsumed += 2;
    } else {
        if(f)
            fprintf(f, "%c%c%c", OP_BIT_CLEAR, Op->name1 / 8, 1 << (Op->name1 % 8));
        pMeta->BytesConsumed += 3;
    }

    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void setBit(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f, "%c%c", OP_BIT_SET_IO, getIOAddress(Op->name1));
        pMeta->BytesConsumed += 2;
    } else {
        if(f)
            fprintf(f, "%c%c%c", OP_BIT_SET, Op->name1 / 8, 1 << (Op->name1 % 8));
        pMeta->BytesConsumed += 3;
    }

    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void copyBit(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    int src_relay = Op->name2;
    int dst_relay = Op->name1;

    if(((src_relay & MAPPED_TO_IO) == 0) && ((dst_relay & MAPPED_TO_IO) == 0)) {
        if((src_relay / 8) == (dst_relay / 8)) {
            if(f)
                fprintf(f,
                        "%c%c%c%c",
                        OP_COPY_BITS_SAME_REGISTER,
                        src_relay / 8,         // Register (byte address)
                        1 << (src_relay % 8),  // Source mask
                        1 << (dst_relay % 8)); // Destination mask
            pMeta->BytesConsumed += 4;
        } else {
            if(f)
                fprintf(f,
                        "%c%c%c%c%c",
                        OP_COPY_BITS,
                        src_relay / 8,         // Source register (byte address)
                        1 << (src_relay % 8),  // Source mask
                        dst_relay / 8,         // Destination register (byte address)
                        1 << (dst_relay % 8)); // Destination mask
            pMeta->BytesConsumed += 5;
        }
    } else if((src_relay & MAPPED_TO_IO) && (dst_relay & MAPPED_TO_IO)) {
        if(f)
            fprintf(f, "%c%c%c", OP_COPY_BITS_IO, getIOAddress(src_relay), getIOAddress(dst_relay));
        pMeta->BytesConsumed += 3;
    } else if(((src_relay & MAPPED_TO_IO) == 0) && (dst_relay & MAPPED_TO_IO)) {
        if(f)
            fprintf(f,
                    "%c%c%c%c",
                    OP_COPY_BIT_TO_IO,
                    src_relay / 8,            // Source register (byte address)
                    1 << (src_relay % 8),     // Source mask
                    getIOAddress(dst_relay)); // destination io
        pMeta->BytesConsumed += 4;
    } else if((src_relay & MAPPED_TO_IO) && ((dst_relay & MAPPED_TO_IO) == 0)) {
        if(f)
            fprintf(f,
                    "%c%c%c%c",
                    OP_COPY_BIT_FROM_IO,
                    getIOAddress(src_relay), // Source io
                    dst_relay / 8,           // Destination register (byte address)
                    1 << (dst_relay % 8));   // Destination mask
        pMeta->BytesConsumed += 4;
    }

    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void ifBitSet(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    WORD labelAddress;
    if(f)
        labelAddress = calculateJumpLabel(Op->name3);
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f,
                    "%c%c%c%c",
                    OP_IF_BIT_SET_IO,
                    getIOAddress(Op->name1),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
        pMeta->BytesConsumed += 4;
    } else {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c",
                    OP_IF_BIT_SET,
                    Op->name1 / 8,        // Register (byte address)
                    1 << (Op->name1 % 8), // Mask
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
        pMeta->BytesConsumed += 5;
    }

    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void ifBitCleared(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    WORD labelAddress;
    if(f)
        labelAddress = calculateJumpLabel(Op->name3);
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f,
                    "%c%c%c%c",
                    OP_IF_BIT_CLEARED_IO,
                    getIOAddress(Op->name1),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
        pMeta->BytesConsumed += 4;
    } else {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c",
                    OP_IF_BIT_CLEARED,
                    Op->name1 / 8,        // Register (byte address)
                    1 << (Op->name1 % 8), // Mask
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
        pMeta->BytesConsumed += 5;
    }

    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static BYTE getInternalIntegerAddress(WORD Address)
{
    if(Address * 2 >= 256) {
        oops();
    }

    return (BYTE)(Address * 2);
}

///////////////////////////////////////////////////////////////////////////////

static void setVariableToLiteral(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    BYTE           address;
    NetzerIntCodes op;

    if(Op->name1 & MAPPED_TO_IO) {
        op = OP_SET_VARIABLE_TO_LITERAL_IO;
        address = getIOAddress(Op->name1);
    } else {
        op = OP_SET_VARIABLE_TO_LITERAL;
        address = getInternalIntegerAddress(Op->name1);
    }

    // Write register address and literal
    if(f)
        fprintf(f, "%c%c%c%c", op, address, (BYTE)Op->literal, (BYTE)(Op->literal >> 8));
    pMeta->BytesConsumed += 4;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void setVariableToVariable(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    int src_var = Op->name2;
    int dst_var = Op->name1;

    if((src_var & MAPPED_TO_IO) && (dst_var & MAPPED_TO_IO)) {
        if(f)
            fprintf(f, "%c%c%c", OP_SET_VARIABLE_IO_TO_VARIABLE_IO, getIOAddress(src_var), getIOAddress(dst_var));
    } else if((src_var & MAPPED_TO_IO) && ((dst_var & MAPPED_TO_IO) == 0)) {
        if(f)
            fprintf(
                f, "%c%c%c", OP_SET_VARIABLE_TO_VARIABLE_IO, getIOAddress(src_var), getInternalIntegerAddress(dst_var));
    } else if(((src_var & MAPPED_TO_IO) == 0) && (dst_var & MAPPED_TO_IO)) {
        if(f)
            fprintf(
                f, "%c%c%c", OP_SET_VARIABLE_IO_TO_VARIABLE, getInternalIntegerAddress(src_var), getIOAddress(dst_var));
    } else if(((src_var & MAPPED_TO_IO) == 0) && ((dst_var & MAPPED_TO_IO) == 0)) {
        if(f)
            fprintf(f,
                    "%c%c%c",
                    OP_SET_VARIABLE_TO_VARIABLE,
                    getInternalIntegerAddress(src_var),
                    getInternalIntegerAddress(dst_var));
    }
    pMeta->BytesConsumed += 3;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void incrementVariable(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f, "%c%c", OP_INCREMENT_VARIABLE_IO, getIOAddress(Op->name1));
    } else {
        if(f)
            fprintf(f, "%c%c", OP_INCREMENT_VARIABLE, getInternalIntegerAddress(Op->name1));
    }
    pMeta->BytesConsumed += 2;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void decrementVariable(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f, "%c%c", OP_DECREMENT_VARIABLE_IO, getIOAddress(Op->name1));
    } else {
        if(f)
            fprintf(f, "%c%c", OP_DECREMENT_VARIABLE, getInternalIntegerAddress(Op->name1));
    }
    pMeta->BytesConsumed += 2;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void ifVariableLesLiteral(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    WORD labelAddress;
    if(f)
        labelAddress = calculateJumpLabel(Op->name3);
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c%c",
                    OP_IF_VARIABLE_IO_LES_LITERAL,
                    getIOAddress(Op->name1),
                    (BYTE)(Op->literal),
                    (BYTE)(Op->literal >> 8),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
    } else {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c%c",
                    OP_IF_VARIABLE_LES_LITERAL,
                    getInternalIntegerAddress(Op->name1),
                    (BYTE)(Op->literal),
                    (BYTE)(Op->literal >> 8),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
    }

    pMeta->BytesConsumed += 6;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void ifVariableGeqLiteral(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    WORD labelAddress;
    if(f)
        labelAddress = calculateJumpLabel(Op->name3);
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c%c",
                    OP_IF_VARIABLE_IO_GRT_LITERAL,
                    getIOAddress(Op->name1),
                    (BYTE)(Op->literal),
                    (BYTE)(Op->literal >> 8),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
    } else {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c%c",
                    OP_IF_VARIABLE_GRT_LITERAL,
                    getInternalIntegerAddress(Op->name1),
                    (BYTE)(Op->literal),
                    (BYTE)(Op->literal >> 8),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
    }

    pMeta->BytesConsumed += 6;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void ifVariableEquLiteral(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    WORD labelAddress;
    if(f)
        labelAddress = calculateJumpLabel(Op->name3);
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c%c",
                    OP_IF_VARIABLE_IO_EQU_LITERAL,
                    getIOAddress(Op->name1),
                    (BYTE)(Op->literal),
                    (BYTE)(Op->literal >> 8),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
    } else {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c%c",
                    OP_IF_VARIABLE_EQU_LITERAL,
                    getInternalIntegerAddress(Op->name1),
                    (BYTE)(Op->literal),
                    (BYTE)(Op->literal >> 8),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
    }

    pMeta->BytesConsumed += 6;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void ifVariableNeqLiteral(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    WORD labelAddress;
    if(f)
        labelAddress = calculateJumpLabel(Op->name3);
    if(Op->name1 & MAPPED_TO_IO) {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c%c",
                    OP_IF_VARIABLE_IO_NEQ_LITERAL,
                    getIOAddress(Op->name1),
                    (BYTE)(Op->literal),
                    (BYTE)(Op->literal >> 8),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
    } else {
        if(f)
            fprintf(f,
                    "%c%c%c%c%c%c",
                    OP_IF_VARIABLE_NEQ_LITERAL,
                    getInternalIntegerAddress(Op->name1),
                    (BYTE)(Op->literal),
                    (BYTE)(Op->literal >> 8),
                    (BYTE)(labelAddress),
                    (BYTE)(labelAddress >> 8));
    }

    pMeta->BytesConsumed += 6;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void math(NetzerIntCodes Opcode, BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    int dst = Op->name1;
    int src1 = Op->name2;
    int src2 = Op->name3;

    if(src1 & MAPPED_TO_IO) {
        BinOp load;
        load.name1 = AddrForVariable("$dummy2");
        load.name2 = src1;
        setVariableToVariable(&load, pMeta, f);
        src1 = load.name1;
    }

    if(src2 & MAPPED_TO_IO) {
        BinOp load;
        load.name1 = AddrForVariable("$dummy3");
        load.name2 = src2;
        setVariableToVariable(&load, pMeta, f);
        src2 = load.name1;
    }

    if(dst & MAPPED_TO_IO) {
        BinOp load;
        if(f)
            fprintf(f,
                    "%c%c%c%c",
                    Opcode,
                    getInternalIntegerAddress(src1),
                    getInternalIntegerAddress(src2),
                    getInternalIntegerAddress(AddrForVariable("$dummy1")));

        // Replay the calculated value.
        load.name1 = dst;
        load.name2 = AddrForVariable("$dummy1");
        setVariableToVariable(&load, pMeta, f);
    } else {
        if(f)
            fprintf(f,
                    "%c%c%c%c",
                    Opcode,
                    getInternalIntegerAddress(src1),
                    getInternalIntegerAddress(src2),
                    getInternalIntegerAddress(dst));
    }

    pMeta->BytesConsumed += 4;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void ifVariable_X_Variable(BinOp *Op, BYTE NetzerOp, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    int src1 = Op->name1;
    int src2 = Op->name2;

    if(src1 & MAPPED_TO_IO) {
        BinOp load;
        load.name1 = AddrForVariable("$dummy1");
        load.name2 = src1;
        setVariableToVariable(&load, pMeta, f);
        src1 = load.name1;
    }

    if(src2 & MAPPED_TO_IO) {
        BinOp load;
        load.name1 = AddrForVariable("$dummy2");
        load.name2 = src2;
        setVariableToVariable(&load, pMeta, f);
        src2 = load.name1;
    }

    if(f) {
        WORD labelAddress = calculateJumpLabel(Op->name3);
        fprintf(f,
                "%c%c%c%c%c",
                NetzerOp,
                getInternalIntegerAddress(src1),
                getInternalIntegerAddress(src2),
                (BYTE)(labelAddress),
                (BYTE)(labelAddress >> 8));
    }

    pMeta->BytesConsumed += 5;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static void elseOp(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    if(f) {
        WORD labelAddress = calculateJumpLabel(Op->name3);
        fprintf(f, "%c%c%c", OP_ELSE, (BYTE)(labelAddress), (BYTE)(labelAddress >> 8));
    }

    pMeta->BytesConsumed += 3;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

static int normalizeString(char *pString, FILE *f = nullptr)
{
    int len = 0;

    return len;
}

///////////////////////////////////////////////////////////////////////////////

static void writeStringOp(BinOp *Op, OpcodeMeta *pMeta, FILE *f = nullptr)
{
    BYTE           address;
    NetzerIntCodes op;

    if(Op->name2 & MAPPED_TO_IO) {
        op = OP_WRITE_STRING_IO;
        address = getIOAddress(Op->name2);
    } else {
        op = OP_WRITE_STRING;
        address = getInternalIntegerAddress(Op->name2);
    }

    int len = strlen(Strings[Op->name3]);
    if(f) {
        fprintf(f, "%c%c%c%c", op, len + 4, address, getIOAddress(Op->name1));
        fprintf(f, "%s", Strings[Op->name3]);
        fputc(0, f); // Terminate string.
    }

    // Now normalize string for embedding it into image.
    pMeta->BytesConsumed += 4 + len + 1;
    pMeta->Opcodes += 1; // One opcode generated.
}

///////////////////////////////////////////////////////////////////////////////

void CompileNetzer(const char *outFile)
{
    char                    projectname[MAX_PROJECTNAME_LENGTH + 1];
    NetzerMetaInformation_t meta;
    int                     opcodes;

    memset((void *)&meta, 0, sizeof(meta));

    // Prepare projectname.
    {
        int   i;
        const char *lastslash = strrchr(outFile, '/');
        const char *lastbslash = strrchr(outFile, '\\');

        const char *copy = lastslash > lastbslash ? lastslash : lastbslash;
        if(copy == nullptr) {
            copy = outFile;
        } else {
            // Omit separator.
            copy++;
        }

        for(i = 0; i < MAX_PROJECTNAME_LENGTH; i++) {
            if((copy[i] == '.') || (copy[i] == 0)) {
                break;
            }
            projectname[i] = copy[i];
        }

        projectname[i] = 0; // Terminate string
    }

    // Prepare variables and relays.
    RelaysCount = 0;
    VariablesCount = 0;
    locateRegister();

    // Generate interpretable code.
    opcodes = GenerateIntOpcodes();
    if(opcodes == 0) {
        Error(_("No opcodes found."));
        return;
    } else if(opcodes == -1) {
        // Errors found, better return without doing anything here.
        return;
    }

    FileTracker f(outFile, "w+b");
    if(!f) {
        Error(_("Couldn't write to '%s'"), outFile);
        return;
    }

    // Add space for meta informations.
    {
        int meta_size = sizeof(meta) + strlen(projectname);

        // Add space for meta tags.
        meta_size++; // At least the end of head tag is needed.
        meta_size += GetLocalRelaysAsMetaTags();
        meta_size += GetLocalVariablesAsMetaTags();

        // Seek after.
        fseek(f, meta_size, SEEK_SET);
    }

    OpcodeMeta opcodeMeta;
    opcodeMeta.BytesConsumed = 0;
    opcodeMeta.Opcodes = 0;

    generateNetzerOpcodes(OutProg, opcodes, &opcodeMeta, f);

    // Complete and write meta informations.
    meta.StartTag[0] = START_TAG_BYTE1;
    meta.StartTag[1] = START_TAG_BYTE2;
    meta.StartTag[2] = START_TAG_BYTE3;
    meta.StartTag[3] = START_TAG_BYTE4;
    meta.Opcodes = opcodeMeta.Opcodes;
    meta.ImageLength = (WORD)(ftell(f));

    if(RunningInTestMode) {
        // Do not generate a time stamp in test mode (for comparing with expected results).
        meta.TimeStamp = 0;
    } else {
        time_t rawtime;
        time(&rawtime);
        meta.TimeStamp = (uint32_t)(rawtime)-TIME_EPOCH;
    }

    meta.CycleTime = uint32_t(Prog.cycleTime / 1000);
    meta.ProjectnameLength = strlen(projectname);
    meta.Flags.FormatVersion = CURRENT_FORMAT_VERSION;
    meta.Flags.IsCompiled = false;
    meta.ProjectID = PROJECT_ID_IO; // Only IO project is supported in the moment.

    fseek(f, 0, SEEK_SET);
    fwrite((const void *)&meta, 1, sizeof(meta), f);
    fprintf(f, "%s", projectname);

    // Write meta tags to file:
    GetLocalRelaysAsMetaTags(f);
    GetLocalVariablesAsMetaTags(f);
    fputc(MTT_END_OF_HEADER, f);

    // Calculate image CRC and write it to file.
    fseek(f, offsetof(NetzerMetaInformation_t, ImageLength), SEEK_SET);
    meta.ImageCRC = calculateCRC(f, meta.ImageLength - offsetof(NetzerMetaInformation_t, ImageLength));
    fseek(f, offsetof(NetzerMetaInformation_t, ImageCRC), SEEK_SET);
    fwrite((const void *)&meta.ImageCRC, 1, sizeof(meta.ImageCRC), f);

    // And ready.
    char str[MAX_PATH + 500];
    sprintf(str, _("Compile successful!\r\nWrote Netzer code to '%s' (CRC: 0x%.4x)."), outFile, meta.ImageCRC);
    CompileSuccessfulMessage(str);
}

///////////////////////////////////////////////////////////////////////////////

static void generateNetzerOpcodes(BinOp *Program, int MaxLabel, OpcodeMeta *pOpcodeMeta, FILE *f)
{
    int idx;

    for(idx = 0; idx < MaxLabel; idx++) {
        switch(Program[idx].op) {
            case INT_CLEAR_BIT:
                clearBit(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_BIT:
                setBit(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_COPY_BIT_TO_BIT:
                copyBit(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                setVariableToLiteral(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                setVariableToVariable(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_INCREMENT_VARIABLE:
                incrementVariable(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_DECREMENT_VARIABLE:
                decrementVariable(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_SR0:
                math(OP_SET_VARIABLE_SR0, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_SHL:
                math(OP_SET_VARIABLE_SHL, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_SHR:
                math(OP_SET_VARIABLE_SHR, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_AND:
                math(OP_SET_VARIABLE_AND, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_OR:
                math(OP_SET_VARIABLE_OR, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_XOR:
                math(OP_SET_VARIABLE_XOR, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_NOT:
                math(OP_SET_VARIABLE_NOT, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_NEG:
                math(OP_SET_VARIABLE_NEG, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_ADD:
                math(OP_SET_VARIABLE_ADD, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_SUBTRACT:
                math(OP_SET_VARIABLE_SUB, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_MULTIPLY:
                math(OP_SET_VARIABLE_MUL, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_DIVIDE:
                math(OP_SET_VARIABLE_DIV, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_SET_VARIABLE_MOD:
                math(OP_SET_VARIABLE_MOD, &Program[idx], pOpcodeMeta, f);
                break;

            case INT_IF_BIT_SET:
                ifBitSet(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_IF_BIT_CLEAR:
                ifBitCleared(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_IF_VARIABLE_LES_LITERAL:
                ifVariableLesLiteral(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                ifVariable_X_Variable(&Program[idx], OP_IF_VARIABLE_EQUALS_VARIABLE, pOpcodeMeta, f);
                break;

            case INT_IF_VARIABLE_GRT_VARIABLE:
                ifVariable_X_Variable(&Program[idx], OP_IF_VARIABLE_GRT_VARIABLE, pOpcodeMeta, f);
                break;

            case INT_ELSE:
                elseOp(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_WRITE_STRING:
                writeStringOp(&Program[idx], pOpcodeMeta, f);
                break;

            case INT_END_OF_PROGRAM:
                if(f)
                    fputc(OP_END_OF_PROGRAM, f);
                pOpcodeMeta->BytesConsumed++;
                pOpcodeMeta->Opcodes++;
                break;

            default:
                oops();
        } // switch(Program[idx].op)
    }
}

///////////////////////////////////////////////////////////////////////////////
