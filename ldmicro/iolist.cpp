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
// Routines to maintain the processor I/O list. Whenever the user changes the
// name of an element, rebuild the I/O list from the PLC program, so that new
// assigned names are automatically reflected in the I/O list. Also keep a
// list of old I/Os that have been deleted, so that if the user deletes a
// a name and then recreates it the associated settings (e.g. pin number)
// will not be forgotten. Also the dialog box for assigning I/O pins.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"
//#include "display.h"

// I/O that we have seen recently, so that we don't forget pin assignments
// when we re-extract the list
#define MAX_IO_SEEN_PREVIOUSLY 1024
static struct {
    char         name[MAX_NAME_LEN];
    int          type;
    int          pin;
    ModbusAddr_t modbus;
} IoSeenPreviously[MAX_IO_SEEN_PREVIOUSLY];
static int IoSeenPreviouslyCount;

// stuff for the dialog box that lets you choose pin assignments

static HWND IoDialog;

static HWND PinList;
static HWND ModbusSlave;
static HWND ModbusRegister;

// stuff for the popup that lets you set the simulated value of an analog in
static HWND AnalogSliderMain;
static HWND AnalogSliderTrackbar;
static bool AnalogSliderDone;
static bool AnalogSliderCancel;

//-----------------------------------------------------------------------------
// Only this types can connect name to I/O pins.
//-----------------------------------------------------------------------------
int IsIoType(int type)
{
    // clang-format off
    if((type == IO_TYPE_INT_INPUT)
    || (type == IO_TYPE_DIG_INPUT)
    || (type == IO_TYPE_DIG_OUTPUT)
    || (type == IO_TYPE_READ_ADC)
    || (type == IO_TYPE_PWM_OUTPUT)
    || (type == IO_TYPE_SPI_MOSI)
    || (type == IO_TYPE_SPI_MISO)
    || (type == IO_TYPE_SPI_SCK)
    || (type == IO_TYPE_SPI__SS)
//  || (type == IO_TYPE_MODBUS_CONTACT) //???
//  || (type == IO_TYPE_MODBUS_COIL)    //???
    || (type == IO_TYPE_UART_TX)
    || (type == IO_TYPE_UART_RX))
        return type;
    return 0;
    // clang-format on
}
//-----------------------------------------------------------------------------
// Append an I/O to the I/O list if it is not in there already.
//-----------------------------------------------------------------------------
static void AppendIo(const char *name, int type)
{
    if(!name || !strlen(name))
        return;

    if(IsNumber(name))
        return;

    if(!IsNumber(name)) {
        if(strchr(name, '-')) {
            Error(_("Rename '%s': Replace the character '-' by the '_'."), name);
            return;
        }
    }

    if(name[0] == '#') {
        if(strspn(name, "#PORT") == 5)
            type = IO_TYPE_PORT_OUTPUT;
        else if(strspn(name, "#PIN") == 4)
            type = IO_TYPE_PORT_INPUT;
        else
            type = IO_TYPE_MCU_REG;
    }

    SetVariableType(name, type);

    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, name) == 0) {
            if((Prog.io.assignment[i].type == IO_TYPE_COUNTER) && (type == IO_TYPE_GENERAL)) {
                return;
            } else if((Prog.io.assignment[i].type == IO_TYPE_GENERAL) && (type == IO_TYPE_COUNTER)) {
                Prog.io.assignment[i].type = type; // replace // see compilecommon.cpp
            }
        }
        if((strcmp(Prog.io.assignment[i].name, name) == 0) && (Prog.io.assignment[i].type == type))
            return;
        /*
        if(strcmp(Prog.io.assignment[i].name, name)==0) {
            if(type != IO_TYPE_GENERAL && Prog.io.assignment[i].type ==
                IO_TYPE_GENERAL)
            {
                Prog.io.assignment[i].type = type;
            }
            // already in there
            return;
        }
        /**/
    }
    if(i < MAX_IO) {
        Prog.io.assignment[i].type = type;
        Prog.io.assignment[i].pin = NO_PIN_ASSIGNED;
        Prog.io.assignment[i].modbus.Slave = 0;
        Prog.io.assignment[i].modbus.Address = 0;
        strcpy(Prog.io.assignment[i].name, name);
        (Prog.io.count)++;
    }
}

//-----------------------------------------------------------------------------
// Move an I/O pin into the `seen previously' list. This means that if the
// user creates input Xasd, assigns it a pin, deletes, and then recreates it,
// then it will come back with the correct pin assigned.
//-----------------------------------------------------------------------------
static void AppendIoSeenPreviously(char *name, int type, int pin, ModbusAddr_t modbus)
{
    if(strcmp(name + 1, "new") == 0)
        return;

    int i;
    for(i = 0; i < IoSeenPreviouslyCount; i++) {
        if(strcmp(name, IoSeenPreviously[i].name) == 0 && type == IoSeenPreviously[i].type) {
            if(pin != NO_PIN_ASSIGNED) {
                IoSeenPreviously[i].pin = pin;
            }
            IoSeenPreviously[i].modbus = modbus;
            return;
        }
    }
    if(IoSeenPreviouslyCount >= MAX_IO_SEEN_PREVIOUSLY) {
        // maybe improve later; just throw away all our old information, and
        // the user might have to reenter the pin if they delete and recreate
        // things
        IoSeenPreviouslyCount = 0;
    }
    i = IoSeenPreviouslyCount;
    IoSeenPreviously[i].type = type;
    IoSeenPreviously[i].pin = pin;
    IoSeenPreviously[i].modbus = modbus;
    strcpy(IoSeenPreviously[i].name, name);
    IoSeenPreviouslyCount++;
}

static void AppendIoAutoType(char *name, int default_type)
{
    int type;

    // clang-format off
    switch (name[0]) {
    case 'X': type = IO_TYPE_DIG_INPUT;      break;
    case 'Y': type = IO_TYPE_DIG_OUTPUT;     break;
    case 'A': type = IO_TYPE_READ_ADC;       break;
  //case 'P': type = IO_TYPE_PWM_OUTPUT;     break;
  //case 'I': type = IO_TYPE_MODBUS_CONTACT; break; // conflict if IO_TYPE_GENERAL variable start with 'I'
    case 'M': type = IO_TYPE_MODBUS_COIL;    break;
    case 'H': type = IO_TYPE_MODBUS_HREG;    break;
    case 'G': type = IO_TYPE_GENERAL;        break;
    default:  type = default_type;
    };
    // clang-format on

    // type = default_type; // rewert

    AppendIo(name, type);
}
//-----------------------------------------------------------------------------
// Walk a subcircuit, calling ourselves recursively and extracting all the
// I/O names out of it.
//-----------------------------------------------------------------------------
static void ExtractNamesFromCircuit(int which, void *any)
{
    ElemLeaf *l = (ElemLeaf *)any;

    char str[MAX_NAME_LEN];

    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            for(int i = 0; i < p->count; i++) {
                ExtractNamesFromCircuit(p->contents[i].which, p->contents[i].data.any);
            }
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            for(int i = 0; i < s->count; i++) {
                ExtractNamesFromCircuit(s->contents[i].which, s->contents[i].data.any);
            }
            break;
        }
        case ELEM_CONTACTS:
            switch(l->d.contacts.name[0]) {
                case 'R':
                    AppendIo(l->d.contacts.name, IO_TYPE_INTERNAL_RELAY);
                    break;

                case 'Y':
                    AppendIo(l->d.contacts.name, IO_TYPE_DIG_OUTPUT);
                    break;

                case 'X':
                    AppendIo(l->d.contacts.name, IO_TYPE_DIG_INPUT);
                    break;

                case 'P':
                    AppendIo(l->d.contacts.name, IO_TYPE_PWM_OUTPUT);
                    break;

                case 'I':
                    AppendIo(l->d.contacts.name, IO_TYPE_MODBUS_CONTACT);
                    break;

                case 'M':
                    AppendIo(l->d.contacts.name, IO_TYPE_MODBUS_COIL);
                    break;

                default:
                    oops();
                    break;
            }
            break;

        case ELEM_COIL:
            switch(l->d.contacts.name[0]) {
                case 'R':
                    AppendIo(l->d.coil.name, IO_TYPE_INTERNAL_RELAY);
                    break;
                case 'Y':
                    AppendIo(l->d.coil.name, IO_TYPE_DIG_OUTPUT);
                    break;
                case 'M':
                    AppendIo(l->d.coil.name, IO_TYPE_MODBUS_COIL);
                    break;
                default:
                    oops();
                    break;
            }
            break;

        case ELEM_QUAD_ENCOD:
            /*
            switch(l->d.QuadEncod.inputA[0]) {
                case 'I':
                    AppendIo(l->d.QuadEncod.inputA, IO_TYPE_INT_INPUT);
                    break;
                default:
                    Error(_("Connect QUAD ENCOD input A to INTs input pin IqAn."));
                    break;
            }
            */
            switch(l->d.QuadEncod.inputA[0]) {
                case 'X':
                    AppendIo(l->d.QuadEncod.inputA, IO_TYPE_DIG_INPUT);
                    break;
                default:
                    Error(_("Connect QUAD ENCOD input A to input pin XqAn."));
                    break;
            }
            switch(l->d.QuadEncod.inputB[0]) {
                case 'X':
                    AppendIo(l->d.QuadEncod.inputB, IO_TYPE_DIG_INPUT);
                    break;
                default:
                    Error(_("Connect QUAD ENCOD input B to input pin XqBn."));
                    break;
            }
            if(strlen(l->d.QuadEncod.inputZ) > 0)
                switch(l->d.QuadEncod.inputZ[0]) {
                    case 'X':
                        AppendIo(l->d.QuadEncod.inputZ, IO_TYPE_DIG_INPUT);
                        break;
                    default:
                        Error(_("Connect QUAD ENCOD input Z to input pin XqZn."));
                        break;
                }
            if(strlen(l->d.QuadEncod.dir) > 0)
                switch(l->d.QuadEncod.dir[0]) {
                    case 'Y':
                        AppendIo(l->d.QuadEncod.dir, IO_TYPE_DIG_OUTPUT);
                        break;
                    default:
                        Error(_("Connect QUAD ENCOD dir flag to output pin YsomeName."));
                        break;
                }
/*
            AppendIo("ELEM_QUAD_ENCOD", IO_TYPE_TABLE_IN_FLASH);
            SetSizeOfVar("ELEM_QUAD_ENCOD", 16);
*/
            break;

        case ELEM_NPULSE:
            AppendIo(l->d.Npulse.counter, IO_TYPE_GENERAL);
            switch(l->d.Npulse.coil[0]) {
                case 'Y':
                    AppendIo(l->d.Npulse.coil, IO_TYPE_DIG_OUTPUT);
                    break;
                default:
                    Error(_("Connect NPULSE to output pin YsomeName."));
                    break;
            }
            break;

        case ELEM_STEPPER:
            sprintf(str, "C%s%s", l->d.stepper.name, "Dec");
            AppendIo(str, IO_TYPE_COUNTER);

            if(IsNumber(l->d.stepper.P) && (CheckMakeNumber(l->d.stepper.P) > 1) //
               || (l->d.stepper.graph > 0)                                       // &&(l->d.stepper.n>1)
               || (!IsNumber(l->d.stepper.P))) {
                sprintf(str, "C%s%s", l->d.stepper.name, "Inc");
                AppendIo(str, IO_TYPE_COUNTER);

                sprintf(str, "C%s%s", l->d.stepper.name, "P");
                AppendIo(str, IO_TYPE_COUNTER);
            }
            switch(l->d.stepper.coil[0]) {
                case 'Y':
                    AppendIo(l->d.stepper.coil, IO_TYPE_DIG_OUTPUT);
                    break;
                default:
                    Error(_("Connect STEPPER coil to output pin YsomeName."));
                    break;
            }
            break;

        case ELEM_PULSER:
            switch(l->d.pulser.busy[0]) {
                case 'R':
                    AppendIo(l->d.pulser.busy, IO_TYPE_INTERNAL_RELAY);
                    break;
                case 'Y':
                    AppendIo(l->d.pulser.busy, IO_TYPE_DIG_OUTPUT);
                    break;
                default:
                    Error(_("Connect PULSER busy flag to output pin YsomeName or internal relay RsomeName."));
                    break;
            }
            break;

        case ELEM_TIME2DELAY:
        case ELEM_TIME2COUNT:
            AppendIo(l->d.timer.name, IO_TYPE_GENERAL);
            break;

        case ELEM_TCY:
            AppendIo(l->d.timer.name, IO_TYPE_TCY);
            break;

        case ELEM_TON:
            AppendIo(l->d.timer.name, IO_TYPE_TON);
            break;

        case ELEM_TOF:
            AppendIo(l->d.timer.name, IO_TYPE_TOF);
            break;

        case ELEM_RTO:
            AppendIo(l->d.timer.name, IO_TYPE_RTO);
            break;

        case ELEM_RTL:
            AppendIo(l->d.timer.name, IO_TYPE_RTL);
            break;

        case ELEM_THI:
            AppendIo(l->d.timer.name, IO_TYPE_THI);
            break;

        case ELEM_TLO:
            AppendIo(l->d.timer.name, IO_TYPE_TLO);
            break;

        case ELEM_BIN2BCD:
            AppendIo(l->d.move.dest, IO_TYPE_BCD);
            if(CheckForNumber(l->d.move.src) == false) {
                AppendIo(l->d.move.src, IO_TYPE_GENERAL);
            }
            break;

        case ELEM_BCD2BIN:
            AppendIo(l->d.move.dest, IO_TYPE_GENERAL);
            if(CheckForNumber(l->d.move.src) == false) {
                AppendIo(l->d.move.src, IO_TYPE_BCD);
            }
            break;

        case ELEM_SEED_RANDOM: {
            sprintf(str, "$seed_%s", l->d.move.dest);
            AppendIo(str, IO_TYPE_GENERAL);
            break;
        }

        case ELEM_SPI: {
            sprintf(str, "%s_MOSI", l->d.spi.name);
            AppendIo(str, IO_TYPE_SPI_MOSI);
            sprintf(str, "%s_MISO", l->d.spi.name);
            AppendIo(str, IO_TYPE_SPI_MISO);
            sprintf(str, "%s_SCK", l->d.spi.name);
            AppendIo(str, IO_TYPE_SPI_SCK);
            sprintf(str, "%s__SS", l->d.spi.name);
            AppendIo(str, IO_TYPE_SPI__SS);

            McuSpiInfo *spiInfo = GetMcuSpiInfo(l->d.spi.name);
            if(spiInfo) {
                if(spiInfo->MOSI) {
                    //     assign
                }
            }

            if(!CheckForNumber(l->d.spi.send)) {
                // Not need ???
                // Need if you add only one MOV or get erroneously other src name
                // then you can see l->d.move.src in IOlist
                AppendIo(l->d.spi.send, IO_TYPE_GENERAL);
            }
            if(!CheckForNumber(l->d.spi.recv)) {
                // Not need ???
                // Need if you add only one MOV or get erroneously other src name
                // then you can see l->d.move.src in IOlist
                AppendIo(l->d.spi.recv, IO_TYPE_GENERAL);
            }
            break;
        }
        case ELEM_OPPOSITE:
        case ELEM_SWAP:
        case ELEM_BUS:
        case ELEM_MOVE: {
            AppendIoAutoType(l->d.move.dest, IO_TYPE_GENERAL);
            if(CheckForNumber(l->d.move.src) == false) {
                // Not need ???
                // Need if you add only one MOV or get erroneously other src name
                // then you can see l->d.move.src in IOlist
                AppendIoAutoType(l->d.move.src, IO_TYPE_GENERAL);
            }
            break;
        }
            {
                int         n, n0;
                const char *nameTable;
                    // clang-format off
        case ELEM_7SEG: nameTable = "char7seg";  n = LEN7SEG;  n0=1; goto xseg;
        case ELEM_9SEG: nameTable = "char9seg";  n = LEN9SEG;  n0=2; goto xseg;
        case ELEM_14SEG:nameTable = "char14seg"; n = LEN14SEG; n0=2; goto xseg;
        case ELEM_16SEG:nameTable = "char16seg"; n = LEN16SEG; n0=3; goto xseg;
                    // clang-format on
                xseg:
                    AppendIoAutoType(l->d.segments.dest, IO_TYPE_GENERAL);
                    /*
            if (CheckForNumber(l->d.segments.src) == false) {
                AppendIo(l->d.segments.src, IO_TYPE_GENERAL); // not need ???
            }
            */
                    SetSizeOfVar(nameTable, n);
                    AppendIo(nameTable, IO_TYPE_TABLE_IN_FLASH);

                    sprintf(str, "%s[0]", nameTable);
                    SetSizeOfVar(str, n0);
                    AppendIo(str, IO_TYPE_VAL_IN_FLASH);
                    break;
            }

        case ELEM_SET_BIT:
        case ELEM_CLEAR_BIT:
            AppendIoAutoType(l->d.move.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_IF_BIT_SET:
        case ELEM_IF_BIT_CLEAR:
            AppendIoAutoType(l->d.move.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_SHL:
        case ELEM_SHR:
        case ELEM_SR0:
        case ELEM_ROL:
        case ELEM_ROR:
        case ELEM_AND:
        case ELEM_OR:
        case ELEM_XOR:
        case ELEM_NEG:
        case ELEM_NOT:
        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV:
        case ELEM_MOD:
            if(CheckForNumber(l->d.math.op1) == false) {
                AppendIoAutoType(l->d.math.op1, IO_TYPE_GENERAL);
            }
            if(which != ELEM_NOT)
                if(which != ELEM_NEG)
                    if(CheckForNumber(l->d.math.op2) == false) {
                        AppendIoAutoType(l->d.math.op2, IO_TYPE_GENERAL);
                    }
            AppendIoAutoType(l->d.math.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_STRING:
            if(strlen(l->d.fmtdStr.dest) > 0) {
                AppendIo(l->d.fmtdStr.dest, IO_TYPE_STRING);
            }
            if(strlen(l->d.fmtdStr.var) > 0) {
                AppendIo(l->d.fmtdStr.var, IO_TYPE_GENERAL);
            }
            break;

        case ELEM_FORMATTED_STRING:
            if(strlen(l->d.fmtdStr.var) > 0) {
                AppendIo(l->d.fmtdStr.var, IO_TYPE_UART_TX);
            }
            break;

        case ELEM_UART_SENDn:
        case ELEM_UART_SEND:
            AppendIo(l->d.uart.name, IO_TYPE_GENERAL);
            AppendIo(l->d.uart.name, IO_TYPE_UART_TX);
            break;

        case ELEM_UART_RECVn:
        case ELEM_UART_RECV:
            AppendIo(l->d.uart.name, IO_TYPE_GENERAL);
            AppendIo(l->d.uart.name, IO_TYPE_UART_RX);
            break;

        case ELEM_SET_PWM:
            AppendIo(l->d.setPwm.name, IO_TYPE_PWM_OUTPUT);
            if(!IsNumber(l->d.setPwm.duty_cycle))
                AppendIo(l->d.setPwm.duty_cycle, IO_TYPE_GENERAL);
            break;

        case ELEM_CTU:
        case ELEM_CTD:
        case ELEM_CTC:
        case ELEM_CTR:
            AppendIo(l->d.counter.name, IO_TYPE_COUNTER);
            if(CheckForNumber(l->d.counter.max) == false) {
                // Not need ??? See ELEM_MOV
                AppendIoAutoType(l->d.counter.max, IO_TYPE_GENERAL);
            }
            break;

        case ELEM_READ_ADC:
            AppendIo(l->d.readAdc.name, IO_TYPE_READ_ADC);
            break;

        case ELEM_RANDOM:
            AppendIo(l->d.readAdc.name, IO_TYPE_GENERAL);
            break;

        case ELEM_SHIFT_REGISTER: {
            int i;
            for(i = 0; i < l->d.shiftRegister.stages; i++) {
                sprintf(str, "%s%d", l->d.shiftRegister.name, i);
                AppendIo(str, IO_TYPE_GENERAL);
            }
            break;
        }
        case ELEM_LOOK_UP_TABLE: {
            if(l->d.lookUpTable.count > 0)
                MemForVariable(l->d.lookUpTable.name, nullptr, l->d.lookUpTable.count);
            AppendIo(l->d.lookUpTable.name, IO_TYPE_TABLE_IN_FLASH);

            sprintf(str, "%s[0]", l->d.lookUpTable.name);
            int sovElement = TestByteNeeded(l->d.lookUpTable.count, l->d.lookUpTable.vals);
            if(sovElement > 0)
                SetSizeOfVar(str, sovElement);
            AppendIo(str, IO_TYPE_VAL_IN_FLASH);

            AppendIo(l->d.lookUpTable.dest, IO_TYPE_GENERAL);
            AppendIo(l->d.lookUpTable.index, IO_TYPE_GENERAL);
            break;
        }
        case ELEM_PIECEWISE_LINEAR:
            AppendIo(l->d.piecewiseLinear.dest, IO_TYPE_GENERAL);
            AppendIo(l->d.lookUpTable.index, IO_TYPE_GENERAL);
            break;

        case ELEM_PLACEHOLDER:
        case ELEM_COMMENT:
        case ELEM_SHORT:
        case ELEM_OPEN:
        case ELEM_MASTER_RELAY:
        case ELEM_SLEEP:
        case ELEM_DELAY:
        case ELEM_CLRWDT:
        case ELEM_LOCK:
        case ELEM_GOTO:
        case ELEM_GOSUB:
        case ELEM_LABEL:
        case ELEM_SUBPROG:
        case ELEM_RETURN:
        case ELEM_ENDSUB:
        case ELEM_ONE_SHOT_RISING:
        case ELEM_ONE_SHOT_FALLING:
        case ELEM_ONE_SHOT_LOW:
        case ELEM_OSC:
        case ELEM_UART_SEND_READY:
        case ELEM_UART_RECV_AVAIL:
        case ELEM_EQU:
        case ELEM_NEQ:
        case ELEM_GRT:
        case ELEM_GEQ:
        case ELEM_LES:
        case ELEM_LEQ:
        case ELEM_RES:
#ifdef USE_SFR
        case ELEM_RSFR:
        case ELEM_WSFR:
        case ELEM_SSFR:
        case ELEM_CSFR:
        case ELEM_TSFR:
        case ELEM_T_C_SFR:
#endif
        // case ELEM_PWM_OFF:
        case ELEM_NPULSE_OFF:
            break;

        case ELEM_PADDING:
            break;

        case ELEM_PERSIST:
            AppendIo(l->d.persist.var, IO_TYPE_PERSIST);
            break;

        default:
            ooops("which=%d", which);
    }
}

//-----------------------------------------------------------------------------
// Compare function to qsort() the I/O list. Group by type, then
// alphabetically within each section.
//-----------------------------------------------------------------------------
static int CompareIo(const void *av, const void *bv)
{
    PlcProgramSingleIo *a = (PlcProgramSingleIo *)av;
    PlcProgramSingleIo *b = (PlcProgramSingleIo *)bv;

    if(a->type != b->type) {
        return a->type - b->type;
    }
    /*
    if(a->pin == NO_PIN_ASSIGNED && b->pin != NO_PIN_ASSIGNED) return  1;
    if(b->pin == NO_PIN_ASSIGNED && a->pin != NO_PIN_ASSIGNED) return -1;
*/
    return strcmp(a->name, b->name);
}

//-----------------------------------------------------------------------------
// Wipe the I/O list and then re-extract it from the PLC program, taking
// care not to forget the pin assignments. Gets passed the selected item
// as an index into the list; modifies the list, so returns the new selected
// item as an index into the new list.
//-----------------------------------------------------------------------------
int GenerateIoList(int prevSel)
{
    int i, j;

    char selName[MAX_NAME_LEN];
    if(prevSel >= 0) {
        strcpy(selName, Prog.io.assignment[prevSel].name);
    }

    if(IoSeenPreviouslyCount > MAX_IO_SEEN_PREVIOUSLY / 2) {
        // flush it so there's lots of room, and we don't run out and
        // forget important things
        IoSeenPreviouslyCount = 0;
    }

    // remember the pin assignments
    for(i = 0; i < Prog.io.count; i++) {
        AppendIoSeenPreviously(Prog.io.assignment[i].name,
                               Prog.io.assignment[i].type,
                               Prog.io.assignment[i].pin,
                               Prog.io.assignment[i].modbus);
    }
    // wipe the list
    Prog.io.count = 0;
    // extract the new list so that it must be up to date
    for(i = 0; i < Prog.numRungs; i++) {
        ExtractNamesFromCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    }
    // AppendIo("ROverflowFlagV", IO_TYPE_INTERNAL_RELAY);

    if(Prog.cycleDuty) {
        AppendIo(YPlcCycleDuty, IO_TYPE_DIG_OUTPUT);
    }
    for(i = 0; i < Prog.io.count; i++) {
        // clang-format off
        if(Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT ||
           Prog.io.assignment[i].type == IO_TYPE_INT_INPUT ||
           Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_PWM_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_CONTACT ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_COIL ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_HREG ||
           Prog.io.assignment[i].type == IO_TYPE_SPI_MOSI ||
           Prog.io.assignment[i].type == IO_TYPE_SPI_MISO ||
           Prog.io.assignment[i].type == IO_TYPE_SPI_SCK  ||
           Prog.io.assignment[i].type == IO_TYPE_SPI__SS  ||
           Prog.io.assignment[i].type == IO_TYPE_READ_ADC)
        {
            // clang-format on
            for(j = 0; j < IoSeenPreviouslyCount; j++) {
                if(strcmp(Prog.io.assignment[i].name, IoSeenPreviously[j].name) == 0) {
                    Prog.io.assignment[i].pin = IoSeenPreviously[j].pin;
                    Prog.io.assignment[i].modbus = IoSeenPreviously[j].modbus;
                    break;
                }
            }
        }
    }

    qsort(Prog.io.assignment, Prog.io.count, sizeof(PlcProgramSingleIo), CompareIo);

    if(prevSel >= 0) {
        for(i = 0; i < Prog.io.count; i++) {
            if(strlen(IoListSelectionName)) {
                if(strcmp(Prog.io.assignment[i].name, IoListSelectionName) == 0)
                    return i;
            } else if(strcmp(Prog.io.assignment[i].name, selName) == 0)
                return i;
        }
        if(i < Prog.io.count)
            return i;
    }
    // no previous, or selected was deleted
    return -1;
}

//-----------------------------------------------------------------------------
// Load the I/O list from a file. Since we are just loading pin assignments,
// put it into IoSeenPreviously so that it will get used on the next
// extraction.
//-----------------------------------------------------------------------------
bool LoadIoListFromFile(FILE *f)
{
    char         line[MAX_NAME_LEN];
    char         name[MAX_NAME_LEN];
    char         pinName[MAX_NAME_LEN];
    int          pin;
    ModbusAddr_t modbus;
    while(fgets(line, sizeof(line), f)) {
        if(!strlen(strspace(line)))
            continue;
        if(strcmp(line, "END") == 0) {
            return true;
        }
        modbus.Slave = 0;
        modbus.Address = 0;
        int type = 0;
        if(strstr(line, "_MOSI")) {
            type = IO_TYPE_SPI_MOSI;
        } else if(strstr(line, "_MISO")) {
            type = IO_TYPE_SPI_MISO;
        } else if(strstr(line, "_SCK")) {
            type = IO_TYPE_SPI_SCK;
        } else if(strstr(line, "__SS")) {
            type = IO_TYPE_SPI__SS;
        } else {
            switch(strspace(line)[0]) {
                    //case 'I': type = IO_TYPE_INT_INPUT; break;
                case 'X':
                    type = IO_TYPE_DIG_INPUT;
                    break;
                case 'Y':
                    type = IO_TYPE_DIG_OUTPUT;
                    break;
                case 'A':
                    type = IO_TYPE_READ_ADC;
                    break;
                case 'P':
                    type = IO_TYPE_PWM_OUTPUT;
                    break;
                case 'I':
                    type = IO_TYPE_MODBUS_CONTACT;
                    break;
                case 'M':
                    type = IO_TYPE_MODBUS_COIL;
                    break;
                case 'C':
                    type = IO_TYPE_COUNTER;
                    break;
                case 'H':
                    type = IO_TYPE_MODBUS_HREG;
                    break;
                default:
                    oops();
            }
        }
        char *s = strstr(line, " at ");
        if(isdigit(s[4])) {
            // Don't internationalize this! It's the file format, not UI.
            if(sscanf(line, " %s at %d %hhd %hd", name, &pin, &modbus.Slave, &modbus.Address) >= 2) {
                AppendIoSeenPreviously(name, type, pin, modbus);
            }
        } else {
            // Don't internationalize this! It's the file format, not UI.
            if(sscanf(line, " %s at %s", name, pinName) == 2) {
                // PC ports
                pin = NameToPin(pinName);
                AppendIoSeenPreviously(name, type, pin, modbus);
            }
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// Write the I/O list to a file. Since everything except the pin assignment
// can be extracted from the schematic, just write the Xs and Ys.
//-----------------------------------------------------------------------------
void SaveIoListToFile(FILE *f)
{
    int i, j1 = 0, j2 = 0;
    for(i = 0; i < Prog.io.count; i++) {
        // clang-format off
        if(Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT  ||
           Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_INT_INPUT  ||
           Prog.io.assignment[i].type == IO_TYPE_PWM_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_CONTACT ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_COIL ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_HREG ||
           Prog.io.assignment[i].type == IO_TYPE_SPI_MOSI ||
           Prog.io.assignment[i].type == IO_TYPE_SPI_MISO ||
           Prog.io.assignment[i].type == IO_TYPE_SPI_SCK  ||
           Prog.io.assignment[i].type == IO_TYPE_SPI__SS  ||
           Prog.io.assignment[i].type == IO_TYPE_READ_ADC)
        {
            // clang-format on
            j1++;
            if((strcmp(Prog.LDversion, "0.1") == 0)      //
               && (Prog.io.assignment[i].name[0] != 'X') //
               && (Prog.io.assignment[i].name[0] != 'Y') //
               && (Prog.io.assignment[i].name[0] != 'A'))
                continue;
            j2++;
            // Don't internationalize this! It's the file format, not UI.
            if(Prog.mcu && (Prog.mcu->whichIsa == ISA_PC) && (Prog.io.assignment[i].pin))
                fprintf(f, "    %s at %s\n", Prog.io.assignment[i].name, PinToName(Prog.io.assignment[i].pin));
            else
                fprintf(f,
                        "    %s at %d %d %d\n",
                        Prog.io.assignment[i].name,
                        Prog.io.assignment[i].pin,
                        Prog.io.assignment[i].modbus.Slave,
                        Prog.io.assignment[i].modbus.Address);
        }
    }
    if(j1 != j2) {
        Error(" %s%s", "Not all I/O pins are saved! Use menu:\n", _("File->Save LDmicro0.2 file format"));
    }
}

//-----------------------------------------------------------------------------
// Dialog proc for the popup that lets you set the value of an analog input for
// simulation.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK AnalogSliderDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            AnalogSliderDone = true;
            AnalogSliderCancel = true;
            return 1;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

//-----------------------------------------------------------------------------
// A little toolbar-style window that pops up to allow the user to set the
// simulated value of an ADC pin.
//-----------------------------------------------------------------------------
void ShowAnalogSliderPopup(char *name)
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);

    wc.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = (WNDPROC)AnalogSliderDialogProc;
    wc.hInstance = Instance;
    wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
    wc.lpszClassName = "LDmicroAnalogSlider";
    wc.lpszMenuName = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wc);

    POINT pt;
    GetCursorPos(&pt);

    SWORD currentVal = GetAdcShadow(name);

    SWORD maxVal;
    if(Prog.mcu) {
        maxVal = Prog.mcu->adcMax;
    } else {
        maxVal = 1023;
    }
    if(maxVal == 0) {
        Error(_("No ADC or ADC not supported for selected micro."));
        return;
    }

    int left = pt.x - 70;
    // try to put the slider directly under the cursor (though later we might
    // realize that that would put the popup off the screen)
    int top = pt.y - (15 + (73 * currentVal) / maxVal);

    RECT r;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);

    if(top + 110 + 28 >= r.bottom) {
        top = r.bottom - 110 - 28;
    }
    if(top < 0)
        top = 0;

    AnalogSliderMain = CreateWindowClient(WS_EX_TOOLWINDOW | WS_EX_APPWINDOW,
                                          "LDmicroAnalogSlider",
                                          "ADC Pin",
                                          WS_CAPTION | WS_VISIBLE | WS_POPUP | WS_DLGFRAME,
                                          left,
                                          top,
                                          30 + 15,
                                          100 + 28,
                                          nullptr,
                                          nullptr,
                                          Instance,
                                          nullptr);

    AnalogSliderTrackbar =
        CreateWindowEx(0,
                       TRACKBAR_CLASS,
                       "",
                       WS_CHILD | TBS_AUTOTICKS | TBS_VERT | TBS_TOOLTIPS | WS_CLIPSIBLINGS | WS_VISIBLE,
                       0,
                       0,
                       30 + 15,
                       100 + 28,
                       AnalogSliderMain,
                       nullptr,
                       Instance,
                       nullptr);
    SendMessage(AnalogSliderTrackbar, TBM_SETRANGE, false, MAKELONG(0, maxVal));
    SendMessage(AnalogSliderTrackbar, TBM_SETTICFREQ, (maxVal + 1) / 8, 0);
    SendMessage(AnalogSliderTrackbar, TBM_SETPOS, true, currentVal);

    SendMessage(AnalogSliderTrackbar, TBM_SETPAGESIZE, 0, 10);
    SendMessage(AnalogSliderTrackbar, TBM_SETLINESIZE, 0, 1);

    EnableWindow(MainWindow, false);
    ShowWindow(AnalogSliderMain, true);
    SetFocus(AnalogSliderTrackbar);

    DWORD ret;
    MSG   msg;
    AnalogSliderDone = false;
    AnalogSliderCancel = false;

    SWORD orig = GetAdcShadow(name);

    while(!AnalogSliderDone && (ret = GetMessage(&msg, nullptr, 0, 0))) {
        SWORD v = (SWORD)SendMessage(AnalogSliderTrackbar, TBM_GETPOS, 0, 0);

        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_RETURN) {
                AnalogSliderDone = true;
                break;
            } else if(msg.wParam == VK_ESCAPE) {
                AnalogSliderDone = true;
                AnalogSliderCancel = true;
                break;
            }
        } else if(msg.message == WM_RBUTTONDOWN) {
            AnalogSliderDone = true;
        }
        SetAdcShadow(name, v);

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!AnalogSliderCancel) {
        SWORD v = (SWORD)SendMessage(AnalogSliderTrackbar, TBM_GETPOS, 0, 0);
        SetAdcShadow(name, v);
    }

    EnableWindow(MainWindow, true);
    SetFocus(MainWindow);
    DestroyWindow(AnalogSliderMain);
    ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
}

//-----------------------------------------------------------------------------
// Window proc for the contacts dialog box
//-----------------------------------------------------------------------------
static LRESULT CALLBACK IoDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_COMMAND: {
            HWND h = (HWND)lParam;
            if(h == OkButton && wParam == BN_CLICKED) {
                DialogDone = true;
            } else if(h == CancelButton && wParam == BN_CLICKED) {
                DialogDone = true;
                DialogCancel = true;
            } else if(h == PinList && HIWORD(wParam) == LBN_DBLCLK) {
                DialogDone = true;
            }
            break;
        }

        case WM_CLOSE:
        case WM_DESTROY:
            DialogDone = true;
            DialogCancel = true;
            return 1;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Create our window class; nothing exciting.
//-----------------------------------------------------------------------------
static ATOM MakeWindowClass()
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);

    wc.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = (WNDPROC)IoDialogProc;
    wc.hInstance = Instance;
    wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
    wc.lpszClassName = "LDmicroIo";
    wc.lpszMenuName = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000), IMAGE_ICON, 32, 32, 0);
    wc.hIconSm = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000), IMAGE_ICON, 16, 16, 0);

    return RegisterClassEx(&wc);
}

#define AddX 300
#define AddY 50
static void MakeControls()
{
    HWND textLabel =
        CreateWindowEx(0,
                       WC_STATIC,
                       ((Prog.mcu) && (Prog.mcu->whichIsa == ISA_AVR))
                           ? _("Pin#:   MCU pin name:                                       Arduino pin name:")
                           : _("Pin#:   MCU pin name:"),
                       WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                       6,
                       1,
                       400,
                       17,
                       IoDialog,
                       nullptr,
                       Instance,
                       nullptr);
    NiceFont(textLabel);

    PinList = CreateWindowEx(WS_EX_CLIENTEDGE,
                             WC_LISTBOX,
                             "",
                             WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                             6,
                             18,
                             95 + AddX,
                             320 + AddY,
                             IoDialog,
                             nullptr,
                             Instance,
                             nullptr);
    FixedFont(PinList);

    OkButton = CreateWindowEx(0,
                              WC_BUTTON,
                              _("OK"),
                              WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
                              6,
                              325 + AddY + 4,
                              95,
                              23,
                              IoDialog,
                              nullptr,
                              Instance,
                              nullptr);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Cancel"),
                                  WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  6,
                                  356 + AddY + 2,
                                  95,
                                  23,
                                  IoDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(CancelButton);
}

//-----------------------------------------------------------------------------
void ShowIoDialog(int item)
{
    int  type = Prog.io.assignment[item].type;
    char name[MAX_NAME_LEN];
    strcpy(name, Prog.io.assignment[item].name);

    if(!IsNumber(name)) {
        if(strchr(name, '-')) {
            Error(_("Rename '%s': Replace the character '-' by the '_'."), name);
            return;
        }
    }

    switch(type) {
        case IO_TYPE_PORT_INPUT:
        case IO_TYPE_PORT_OUTPUT:
        case IO_TYPE_MCU_REG:
            // //       return; // used for set value of port
        case IO_TYPE_GENERAL:
        case IO_TYPE_PERSIST:
        case IO_TYPE_BCD:
        case IO_TYPE_STRING:
        case IO_TYPE_COUNTER:
        case IO_TYPE_UART_TX:
        case IO_TYPE_UART_RX:
        case IO_TYPE_TCY:
        case IO_TYPE_TON:
        case IO_TYPE_TOF:
        case IO_TYPE_RTO:
        case IO_TYPE_RTL:
        case IO_TYPE_THI:
        case IO_TYPE_TLO:
            ShowSizeOfVarDialog(&Prog.io.assignment[item]);
            SetFocus(IoList);
            return;
        case IO_TYPE_INTERNAL_RELAY:
            // nothing;
            return;
    }
    if(!Prog.mcu) {
        MessageBox(MainWindow,
                   _("No microcontroller has been selected. You must select a "
                     "microcontroller before you can assign I/O pins.\r\n\r\n"
                     "Select a microcontroller under the Settings menu and try "
                     "again."),
                   _("I/O Pin Assignment"),
                   MB_OK | MB_ICONWARNING);
        return;
    }

    if(Prog.mcu->core == NOTHING) {
        if(Prog.io.assignment[item].pin) {
            int i;
            for(i = 0; i < IoSeenPreviouslyCount; i++) {
                if(strcmp(IoSeenPreviously[i].name, Prog.io.assignment[item].name) == 0) {
                    IoSeenPreviously[i].pin = NO_PIN_ASSIGNED;
                }
            }
            Prog.io.assignment[item].pin = NO_PIN_ASSIGNED;
            return;
        }
    }
    /*
    if(Prog.mcu->whichIsa == ISA_ANSIC) {
        Error(_("Can't specify I/O assignment for ANSI C target; compile and "
            "see comments in generated source code."));
        return;
    }
    */
    if(Prog.mcu->whichIsa == ISA_INTERPRETED) {
        Error(
            _("Can't specify I/O assignment for interpretable target; see "
              "comments in reference implementation of interpreter."));
        return;
    }

    if(Prog.mcu->whichIsa == ISA_NETZER) {
        Error(_("Can't specify I/O assignment for Netzer!"));
        return;
    }

    switch(Prog.io.assignment[item].type) {
        case IO_TYPE_READ_ADC:
        case IO_TYPE_PWM_OUTPUT:
        case IO_TYPE_INT_INPUT:
        case IO_TYPE_DIG_INPUT:
        case IO_TYPE_DIG_OUTPUT:
        case IO_TYPE_SPI_MOSI:
        case IO_TYPE_SPI_MISO:
        case IO_TYPE_SPI_SCK:
        case IO_TYPE_SPI__SS:
        case IO_TYPE_UART_TX:
        case IO_TYPE_UART_RX:
        case IO_TYPE_MODBUS_CONTACT:
        case IO_TYPE_MODBUS_COIL:
            break;
        default: {
            Error(
                _("Can only assign pin number to input/output pins (Xname or "
                  "Yname or Aname or Pname)."));
            return;
        }
    }

    if((Prog.io.assignment[item].type == IO_TYPE_READ_ADC) && (Prog.mcu->adcCount == 0)) {
        Error(_("No ADC or ADC not supported for this micro."));
        return;
    }

    if((Prog.io.assignment[item].type == IO_TYPE_PWM_OUTPUT) && (Prog.mcu->pwmCount == 0)
       && (Prog.mcu->pwmNeedsPin == 0)) {
        Error(_("No PWM or PWM not supported for this MCU."));
        return;
    }

    if(strcmp(Prog.io.assignment[item].name + 1, "new") == 0) {
        Error(_("Rename I/O from default name ('%s') before assigning "
                "MCU pin."),
              Prog.io.assignment[item].name);
        return;
    }

    MakeWindowClass();

    // We need the TOOLWINDOW style, or else the window will be forced to
    // a minimum width greater than our current width. And without the
    // APPWINDOW style, it becomes impossible to get the window back (by
    // Alt+Tab or taskbar).
    IoDialog = CreateWindowClient(WS_EX_TOOLWINDOW | WS_EX_APPWINDOW,
                                  "LDmicroIo",
                                  _("I/O Pin"),
                                  WS_OVERLAPPED | WS_SYSMENU,
                                  100,
                                  100,
                                  106 + AddX,
                                  387 + AddY,
                                  nullptr,
                                  nullptr,
                                  Instance,
                                  nullptr);

    MakeControls();

    SendMessage(PinList, LB_ADDSTRING, 0, (LPARAM)_("(no pin)"));
    int  Index = 0;
    char buf[MAX_NAME_LEN];
    char pinName[MAX_NAME_LEN];
    for(uint32_t i = 0; i < Prog.mcu->pinCount; i++) {
        for(int j = 0; j < Prog.io.count; j++) {
            if(j == item)
                continue;
            if(Prog.io.assignment[j].pin == Prog.mcu->pinInfo[i].pin) {
                goto cant_use_this_io;
            }
        }

        if(Prog.mcu->pinInfo[i].ioType) {
            if((type == IO_TYPE_DIG_INPUT) && (Prog.mcu->pinInfo[i].ioType != IO_TYPE_DIG_INPUT)) {
                goto cant_use_this_io;
            }
            if((type == IO_TYPE_DIG_OUTPUT) && (Prog.mcu->pinInfo[i].ioType != IO_TYPE_DIG_OUTPUT)) {
                goto cant_use_this_io;
            }
        }

        if(type == IO_TYPE_SPI_MOSI) {
            char *c = strchr(name, '_');
            if(c)
                *c = '\0';
            McuSpiInfo *iop = GetMcuSpiInfo(name);
            if(iop)
                if(iop->MOSI == Prog.mcu->pinInfo[i].pin)
                    ; // okay; we know how to connect it up to the SPI
                else
                    goto cant_use_this_io;
            else
                goto cant_use_this_io;
        } else if(type == IO_TYPE_SPI_MISO) {
            char *c = strchr(name, '_');
            if(c)
                *c = '\0';
            McuSpiInfo *iop = GetMcuSpiInfo(name);
            if(iop)
                if(iop->MISO == Prog.mcu->pinInfo[i].pin)
                    ; // okay; we know how to connect it up to the SPI
                else
                    goto cant_use_this_io;
            else
                goto cant_use_this_io;
        } else if(type == IO_TYPE_SPI_SCK) {
            char *c = strchr(name, '_');
            if(c)
                *c = '\0';
            McuSpiInfo *iop = GetMcuSpiInfo(name);
            if(iop)
                if(iop->SCK == Prog.mcu->pinInfo[i].pin)
                    ; // okay; we know how to connect it up to the SPI
                else
                    goto cant_use_this_io;
            else
                goto cant_use_this_io;
        } else if(type == IO_TYPE_SPI__SS) {
            char *c = strchr(name, '_');
            if(c)
                *c = '\0';
            McuSpiInfo *iop = GetMcuSpiInfo(name);
            if(iop)
                if(iop->_SS == Prog.mcu->pinInfo[i].pin)
                    ; // okay; we know how to connect it up to the SPI
                else
                    goto cant_use_this_io;
            else
                goto cant_use_this_io;
        }

        if(UartFunctionUsed() && Prog.mcu
           && ((Prog.mcu->pinInfo[i].pin == Prog.mcu->uartNeeds.rxPin)
               || (Prog.mcu->pinInfo[i].pin == Prog.mcu->uartNeeds.txPin))) {
            goto cant_use_this_io;
        }

#if 0
        if(PwmFunctionUsed() &&
            Prog.mcu->pinInfo[i].pin == Prog.mcu->pwmNeedsPin)
        {
            goto cant_use_this_io;
        }
#endif
        if((type == IO_TYPE_INT_INPUT) && (!IsExtIntPin(Prog.mcu->pinInfo[i].pin))) {
            goto cant_use_this_io;
        }

        if(Prog.io.assignment[item].type == IO_TYPE_READ_ADC) {
            McuAdcPinInfo *iop = AdcPinInfo(Prog.mcu->pinInfo[i].pin);
            if(iop)
                ; // okay; we know how to connect it up to the ADC
            else {
                goto cant_use_this_io;
            }
        }
        if(Prog.io.assignment[item].type == IO_TYPE_PWM_OUTPUT) {
            if(Prog.mcu->pwmCount) {
                McuPwmPinInfo *iop = PwmPinInfo(Prog.mcu->pinInfo[i].pin, Prog.cycleTimer);
                if(!iop)
                    goto cant_use_this_io;
                if(/*(Prog.mcu->whichIsa == ISA_AVR) && */(iop->timer == Prog.cycleTimer))
                    goto cant_use_this_io;
                // okay; we know how to connect it up to the PWM
            } else {
                if(Prog.mcu->pwmNeedsPin == Prog.mcu->pinInfo[i].pin)
                    ; // okay; we know how to connect it up to the PWM
                else {
                    goto cant_use_this_io;
                }
            }
        }

        if(Prog.mcu->pinInfo[i].pin == Prog.io.assignment[item].pin) {
            Index = SendMessage(PinList, LB_GETCOUNT, 0, 0);
            if(Index == LB_ERR)
                Index = 0;
        };

        GetPinName(Prog.mcu->pinInfo[i].pin, pinName);
        sprintf(buf, "%3d  %-30s %s", Prog.mcu->pinInfo[i].pin, pinName, ArduinoPinName(Prog.mcu->pinInfo[i].pin));

        SendMessage(PinList, LB_ADDSTRING, 0, (LPARAM)buf);
    cant_use_this_io:;
    }

    for(uint32_t j = 0; j < Prog.mcu->adcCount; j++) {
        if(Prog.io.assignment[item].type == IO_TYPE_READ_ADC) {
            for(uint32_t i = 0; i < Prog.mcu->pinCount; i++) {
                if(Prog.mcu->adcInfo[j].pin == Prog.mcu->pinInfo[i].pin) {
                    // okay; we know how to connect it up to the ADC
                    // break;
                    goto cant_use_this_io_adc;
                }
            }
            if(j == Prog.mcu->adcCount) {
                goto cant_use_this_io_adc;
            } else {
                sprintf(buf, "%3d  ADC%d", Prog.mcu->adcInfo[j].pin, Prog.mcu->adcInfo[j].muxRegValue);
            }
            SendMessage(PinList, LB_ADDSTRING, 0, (LPARAM)buf);
        }
    cant_use_this_io_adc:;
    }

    EnableWindow(MainWindow, false);
    ShowWindow(IoDialog, true);
    SetFocus(PinList);

    SendMessage(PinList, LB_SETCURSEL, (WPARAM)Index, 0);

    MSG   msg;
    DWORD ret;
    DialogDone = false;
    DialogCancel = false;
    while((ret = GetMessage(&msg, nullptr, 0, 0)) && !DialogDone) {
        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_RETURN) {
                DialogDone = true;
                break;
            } else if(msg.wParam == VK_ESCAPE) {
                DialogDone = true;
                DialogCancel = true;
                break;
            }
        }

        if(IsDialogMessage(IoDialog, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        int  sel = SendMessage(PinList, LB_GETCURSEL, 0, 0);
        char pin[MAX_NAME_LEN];
        SendMessage(PinList, LB_GETTEXT, (WPARAM)sel, (LPARAM)pin);
        if(strcmp(pin, _("(no pin)")) == 0) {
            int i;
            for(i = 0; i < IoSeenPreviouslyCount; i++) {
                if(strcmp(IoSeenPreviously[i].name, Prog.io.assignment[item].name) == 0) {
                    IoSeenPreviously[i].pin = NO_PIN_ASSIGNED;
                }
            }
            Prog.io.assignment[item].pin = NO_PIN_ASSIGNED;
        } else {
            Prog.io.assignment[item].pin = atoi(pin);
            // Only one name can be bound to each pin; make sure that there's
            // not another entry for this pin in the IoSeenPreviously list,
            // that might get used if the user creates a new pin with that
            // name.
            int i;
            for(i = 0; i < IoSeenPreviouslyCount; i++) {
                if(IoSeenPreviously[i].pin == atoi(pin)) {
                    IoSeenPreviously[i].pin = NO_PIN_ASSIGNED;
                }
            }
        }
    }

    EnableWindow(MainWindow, true);
    DestroyWindow(IoDialog);
    SetFocus(IoList);
    strcpy(IoListSelectionName, Prog.io.assignment[item].name);
    return;
}

//-----------------------------------------------------------------------------
static void MakeModbusControls()
{
    HWND textLabel2 = CreateWindowEx(0,
                                     WC_STATIC,
                                     _("Slave ID:"),
                                     WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                                     6,
                                     1,
                                     70,
                                     21,
                                     IoDialog,
                                     nullptr,
                                     Instance,
                                     nullptr);
    NiceFont(textLabel2);

    ModbusSlave = CreateWindowEx(WS_EX_CLIENTEDGE,
                                 WC_EDIT,
                                 "",
                                 WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                 80,
                                 1,
                                 30,
                                 21,
                                 IoDialog,
                                 nullptr,
                                 Instance,
                                 nullptr);
    FixedFont(ModbusSlave);

    HWND textLabel3 = CreateWindowEx(0,
                                     WC_STATIC,
                                     _("Register:"),
                                     WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
                                     6,
                                     24,
                                     70,
                                     21,
                                     IoDialog,
                                     nullptr,
                                     Instance,
                                     nullptr);
    NiceFont(textLabel3);

    ModbusRegister = CreateWindowEx(WS_EX_CLIENTEDGE,
                                    WC_EDIT,
                                    "",
                                    WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                    80,
                                    24,
                                    80,
                                    21,
                                    IoDialog,
                                    nullptr,
                                    Instance,
                                    nullptr);
    FixedFont(ModbusRegister);

    OkButton = CreateWindowEx(0,
                              WC_BUTTON,
                              _("OK"),
                              WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
                              6,
                              48,
                              50,
                              23,
                              IoDialog,
                              nullptr,
                              Instance,
                              nullptr);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Cancel"),
                                  WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  56,
                                  48,
                                  50,
                                  23,
                                  IoDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(CancelButton);
}

void ShowModbusDialog(int item)
{
    MakeWindowClass();

    IoDialog = CreateWindowClient(WS_EX_TOOLWINDOW | WS_EX_APPWINDOW,
                                  "LDmicroIo",
                                  _("Modbus Address"),
                                  WS_OVERLAPPED | WS_SYSMENU,
                                  100,
                                  100,
                                  170,
                                  80,
                                  nullptr,
                                  nullptr,
                                  Instance,
                                  nullptr);

    MakeModbusControls();

    char txtModbusSlave[10];
    char txtModbusRegister[20];
    sprintf(txtModbusSlave, "%d", Prog.io.assignment[item].modbus.Slave);
    sprintf(txtModbusRegister, "%05d", Prog.io.assignment[item].modbus.Address);

    SendMessage(ModbusSlave, WM_SETTEXT, 0, (LPARAM)txtModbusSlave);
    SendMessage(ModbusRegister, WM_SETTEXT, 0, (LPARAM)txtModbusRegister);

    EnableWindow(MainWindow, false);
    ShowWindow(IoDialog, true);

    MSG   msg;
    DWORD ret;
    DialogDone = false;
    DialogCancel = false;
    while((ret = GetMessage(&msg, nullptr, 0, 0)) && !DialogDone) {
        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_RETURN) {
                DialogDone = true;
                break;
            } else if(msg.wParam == VK_ESCAPE) {
                DialogDone = true;
                DialogCancel = true;
                break;
            }
        }

        if(IsDialogMessage(IoDialog, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        SendMessage(ModbusSlave, WM_GETTEXT, (WPARAM)sizeof(txtModbusSlave), (LPARAM)txtModbusSlave);
        SendMessage(ModbusRegister, WM_GETTEXT, (WPARAM)sizeof(txtModbusRegister), (LPARAM)txtModbusRegister);
        Prog.io.assignment[item].modbus.Slave = atoi(txtModbusSlave);
        Prog.io.assignment[item].modbus.Address = atoi(txtModbusRegister);
    }

    EnableWindow(MainWindow, true);
    SetFocus(MainWindow);
    DestroyWindow(IoDialog);
    return;
}
//-----------------------------------------------------------------------------
// Called in response to a notify for the listview. Handles click, text-edit
// operations etc., but also gets called to find out what text to display
// where (LPSTR_TEXTCALLBACK); that way we don't have two parallel copies of
// the I/O list to keep in sync.
//-----------------------------------------------------------------------------
void IoListProc(NMHDR *h)
{
    switch(h->code) {
        case LVN_GETDISPINFO: {
            NMLVDISPINFO *i = (NMLVDISPINFO *)h;
            if(!((i->item.mask & LVIF_TEXT) && (i->item.pszText) && (i->item.cchTextMax > 200))) {
                // This test didn't used to be present, and Windows 10 now
                // sends an LVN_GETDISPINFO that fails it, which would
                // otherwise cause us to write to a null pointer.
                break;
            }
            strcpy(i->item.pszText, "");
            int   item = i->item.iItem;
            char *name = Prog.io.assignment[item].name;
            int   type = Prog.io.assignment[item].type;
            switch(i->item.iSubItem) {
                case LV_IO_NAME:
                    strcpy(i->item.pszText, Prog.io.assignment[item].name);
                    break;

                case LV_IO_TYPE: {
                    const char *s = IoTypeToString(Prog.io.assignment[item].type);
                    strcpy(i->item.pszText, s);
                    break;
                }

                case LV_IO_STATE: {
                    if(true || InSimulationMode) {
                        DescribeForIoList(name, type, i->item.pszText);
                    } else {
                        strcpy(i->item.pszText, "");
                    }
                    break;
                }

                case LV_IO_PIN:
                case LV_IO_PORT:
                case LV_IO_PINNAME: {
                    if(!Prog.mcu)
                        break;
                    // Don't confuse people by displaying bogus pin assignments
                    // for the target.
                    if(Prog.mcu
                       && (Prog.mcu->whichIsa == ISA_NETZER || Prog.mcu->whichIsa == ISA_XINTERPRETED
                           || Prog.mcu->whichIsa == ISA_INTERPRETED)) {
                        strcpy(i->item.pszText, "");
                        break;
                    }
                    char poptName[MAX_NAME_LEN];
                    char pinName[MAX_NAME_LEN];

                    PinNumberForIo(i->item.pszText, // i->item.iSubItem == LV_IO_PIN
                                   &(Prog.io.assignment[item]),
                                   poptName,
                                   pinName);

                    if(i->item.iSubItem == LV_IO_PORT)
                        strcpy(i->item.pszText, poptName);
                    else if(i->item.iSubItem == LV_IO_PINNAME)
                        strcpy(i->item.pszText, pinName);
                    break;
                }

                case LV_IO_MODBUS: {
                    if(type != IO_TYPE_MODBUS_COIL && type != IO_TYPE_MODBUS_CONTACT && type != IO_TYPE_MODBUS_HREG) {
                        strcpy(i->item.pszText, "");
                        break;
                    }

                    if(Prog.io.assignment[item].modbus.Slave == 0) {
                        sprintf(i->item.pszText, _("(not assigned)"));
                    } else {
                        sprintf(i->item.pszText,
                                "%d:%05d",
                                Prog.io.assignment[item].modbus.Slave,
                                Prog.io.assignment[item].modbus.Address);
                    }
                    break;
                }

                case LV_IO_RAM_ADDRESS: {
                    if(!Prog.mcu)
                        break;
                    DWORD addr = 0;
                    int   bit = -1;
                    if((type == IO_TYPE_PORT_INPUT) || //
                       (type == IO_TYPE_PORT_OUTPUT)) {
                        MemForVariable(name, &addr);
                        if(addr > 0)
                            sprintf(i->item.pszText, "0x%X", addr);
                        else
                            sprintf(i->item.pszText, "Not a PORT!");
                    } else if(type == IO_TYPE_MCU_REG) {
                        if(IsAddrInVar(name)) {
                            char buf[MAX_NAME_LEN];
                            DescribeForIoList(&name[1], type, buf);
                            char *c = strchr(buf, '=');
                            if(c)
                                *c = '\0';
                            sprintf(i->item.pszText, "%s in %s", buf, &name[1]);
                        } else {
                            MemForVariable(name, &addr);
                            if(addr > 0)
                                sprintf(i->item.pszText, "0x%X", addr);
                            else
                                sprintf(i->item.pszText, "Not a PORT!");
                        }
                    } else if((type == IO_TYPE_GENERAL)    //
                              || (type == IO_TYPE_PERSIST) //
                              || (type == IO_TYPE_STRING)  //
                              || (type == IO_TYPE_RTL)     //
                              || (type == IO_TYPE_RTO)     //
                              || (type == IO_TYPE_TCY)     //
                              || (type == IO_TYPE_TON)     //
                              || (type == IO_TYPE_TOF)     //
                              || (type == IO_TYPE_THI)     //
                              || (type == IO_TYPE_TLO)     //
                              || (type == IO_TYPE_COUNTER)) {
                        MemForVariable(name, &addr);
                        if(addr > 0)
                            sprintf(i->item.pszText, "0x%X", addr);
                    } else if((type == IO_TYPE_INTERNAL_RELAY)) {
                        MemForSingleBit(name, true, &addr, &bit);
                        if(addr > 0 && bit >= 0)
                            sprintf(i->item.pszText, "0x%02X (BIT%d)", addr, bit);
                    } else if(type == IO_TYPE_UART_TX) {
                        if(Prog.mcu) {
                            AddrBitForPin(Prog.mcu->uartNeeds.txPin, &addr, &bit, false);
                            if(addr > 0 && bit >= 0)
                                sprintf(i->item.pszText, "0x%02X (BIT%d)", addr, bit);
                        }
                    } else if(type == IO_TYPE_UART_RX) {
                        if(Prog.mcu) {
                            AddrBitForPin(Prog.mcu->uartNeeds.rxPin, &addr, &bit, true);
                            if(addr > 0 && bit >= 0)
                                sprintf(i->item.pszText, "0x%02X (BIT%d)", addr, bit);
                        }
                    } else if((type == IO_TYPE_READ_ADC)    //
                              || (type == IO_TYPE_SPI_MOSI) //
                              || (type == IO_TYPE_SPI_MISO) //
                              || (type == IO_TYPE_SPI_SCK)  //
                              || (type == IO_TYPE_SPI__SS)  //
                    ) {
                        if(Prog.mcu) {
                            McuIoPinInfo *iop;
                            iop = PinInfoForName(name);
                            if(iop) {
                                AddrBitForPin(iop->pin, &addr, &bit, true);
                                if(addr > 0 && bit >= 0)
                                    sprintf(i->item.pszText, "0x%02X (BIT%d)", addr, bit);
                            }
                        }
                    } else if(type == IO_TYPE_TABLE_IN_FLASH) {
                        MemOfVar(name, &addr);
                        if(addr > 0)
                            sprintf(i->item.pszText, "0x%X", addr);
                    } else if((type == IO_TYPE_DIG_INPUT)     //
                              || (type == IO_TYPE_DIG_OUTPUT) //
                              || (type == IO_TYPE_PWM_OUTPUT)) {
                        if(SingleBitAssigned(name))
                            MemForSingleBit(name, true, &addr, &bit);
                        if(addr > 0 && bit >= 0)
                            sprintf(i->item.pszText, "0x%02X (BIT%d)", addr, bit);
                    }
                    break;
                }

                case LV_IO_SISE_OF_VAR:
                    if((type == IO_TYPE_GENERAL)         //
                       || (type == IO_TYPE_PERSIST)      //
                       || (type == IO_TYPE_PORT_INPUT)   //
                       || (type == IO_TYPE_PORT_OUTPUT)  //
                       || (type == IO_TYPE_MCU_REG)      //
                       || (type == IO_TYPE_BCD)          //
                       || (type == IO_TYPE_STRING)       //
                       || (type == IO_TYPE_COUNTER)      //
                       || (type == IO_TYPE_VAL_IN_FLASH) //
                       || (type == IO_TYPE_THI)          //
                       || (type == IO_TYPE_TLO)          //
                       || (type == IO_TYPE_RTL)          //
                       || (type == IO_TYPE_RTO)          //
                       || (type == IO_TYPE_TCY)          //
                       || (type == IO_TYPE_TON)          //
                       || (type == IO_TYPE_TOF)) {
                        int sov = SizeOfVar(name);
                        sprintf(i->item.pszText, sov == 1 ? "%d byte" : "%d bytes", sov);
                    } else if(type == IO_TYPE_TABLE_IN_FLASH) {
                        int sov = MemOfVar(name, nullptr); // ok
                        sprintf(i->item.pszText, sov == 1 ? "%d elem" : "%d elem's", sov);
                    } else if((type == IO_TYPE_DIG_INPUT)         //
                              || (type == IO_TYPE_DIG_OUTPUT)     //
                              || (type == IO_TYPE_INTERNAL_RELAY) //
                              || (type == IO_TYPE_SPI_MOSI)       //
                              || (type == IO_TYPE_SPI_MISO)       //
                              || (type == IO_TYPE_SPI_SCK)        //
                              || (type == IO_TYPE_SPI__SS)        //
                              || (type == IO_TYPE_MODBUS_COIL)    //
                              || (type == IO_TYPE_MODBUS_CONTACT)) {
                        sprintf(i->item.pszText, "1 bit");
                    } else if((type == IO_TYPE_UART_TX) //
                              || (type == IO_TYPE_UART_RX)) {
                        sprintf(i->item.pszText, "1 pin/1 byte");
                    } else if(type == IO_TYPE_PWM_OUTPUT) {
                        sprintf(i->item.pszText, "1 pin");
                    } else if(type == IO_TYPE_READ_ADC) {
                        sprintf(i->item.pszText, "1 pin/2 bytes");
                    } // else oops();
                    break;
            }
            break;
        }
        case LVN_ITEMACTIVATE: {
            NMITEMACTIVATE *i = (NMITEMACTIVATE *)h;
            char *          name = Prog.io.assignment[i->iItem].name;
            int             type = Prog.io.assignment[i->iItem].type;
            if(InSimulationMode) {
                switch(type) {
                    case IO_TYPE_STRING:
                    case IO_TYPE_GENERAL:
                    case IO_TYPE_PERSIST:
                    case IO_TYPE_RTL:
                    case IO_TYPE_RTO:
                    case IO_TYPE_THI:
                    case IO_TYPE_TLO:
                    case IO_TYPE_COUNTER:
                    case IO_TYPE_UART_TX:
                    case IO_TYPE_UART_RX:
                    case IO_TYPE_PORT_INPUT:
                    case IO_TYPE_PORT_OUTPUT:
                    case IO_TYPE_MCU_REG:
                    case IO_TYPE_TCY:
                    case IO_TYPE_TON:
                    case IO_TYPE_TOF: {
                        ShowIoDialog(i->iItem);
                        InvalidateRect(MainWindow, nullptr, false);
                        ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
                        break;
                    }
                    case IO_TYPE_READ_ADC: {
                        ShowAnalogSliderPopup(name);
                        break;
                    }
                    case IO_TYPE_DIG_INPUT:
                    case IO_TYPE_INTERNAL_RELAY:
                    case IO_TYPE_MODBUS_CONTACT: {
                        SimulationToggleContact(name);
                        break;
                    }
                    default: {
                    }
                }
            } else {
                UndoRemember();
                switch(type) {
                    case IO_TYPE_MODBUS_COIL:
                    case IO_TYPE_MODBUS_CONTACT:
                    case IO_TYPE_MODBUS_HREG:
                        ShowModbusDialog(i->iItem);
                        break;
                    default:
                        ShowIoDialog(i->iItem);
                        break;
                }
                ProgramChanged();
                InvalidateRect(MainWindow, nullptr, false);
            }
            break;
        }
    }
}
