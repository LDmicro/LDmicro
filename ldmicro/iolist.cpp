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
// Routines to maintain the processor I/O list. Whenever the user changes the
// name of an element, rebuild the I/O list from the PLC program, so that new
// assigned names are automatically reflected in the I/O list. Also keep a
// list of old I/Os that have been deleted, so that if the user deletes a
// a name and then recreates it the associated settings (e.g. pin number)
// will not be forgotten. Also the dialog box for assigning I/O pins.
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>

#include "ldmicro.h"
#include "display.h"

// I/O that we have seen recently, so that we don't forget pin assignments
// when we re-extract the list
#define MAX_IO_SEEN_PREVIOUSLY 1024
static struct {
    char    name[MAX_NAME_LEN];
    int     type;
    int     pin;
    ModbusAddr_t modbus;
} IoSeenPreviously[MAX_IO_SEEN_PREVIOUSLY];
static int IoSeenPreviouslyCount;

// stuff for the dialog box that lets you choose pin assignments
static BOOL DialogDone;
static BOOL DialogCancel;

static HWND IoDialog;

static HWND PinList;
static HWND OkButton;
static HWND CancelButton;
static HWND ModbusSlave;
static HWND ModbusRegister;

// stuff for the popup that lets you set the simulated value of an analog in
static HWND AnalogSliderMain;
static HWND AnalogSliderTrackbar;
static BOOL AnalogSliderDone;
static BOOL AnalogSliderCancel;

//-----------------------------------------------------------------------------
// Append an I/O to the I/O list if it is not in there already.
//-----------------------------------------------------------------------------
static void AppendIo(char *name, int type)
{
    if(!name || !strlen(name))
        return;
    SetVariableType(name, type);
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(strcmp(Prog.io.assignment[i].name, name)==0) {
            if(type != IO_TYPE_GENERAL && Prog.io.assignment[i].type ==
                IO_TYPE_GENERAL)
            {
                Prog.io.assignment[i].type = type;
            }
            // already in there
            return;
        }
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
    if(strcmp(name+1, "new")==0) return;

    int i;
    for(i = 0; i < IoSeenPreviouslyCount; i++) {
        if(strcmp(name, IoSeenPreviously[i].name)==0 &&
            type == IoSeenPreviously[i].type)
        {
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

    switch (name[0]) {
    case 'X': type = IO_TYPE_DIG_INPUT; break;
    case 'Y': type = IO_TYPE_DIG_OUTPUT; break;
    case 'A': type = IO_TYPE_READ_ADC; break;
  //case 'P': type = IO_TYPE_PWM_OUTPUT; break;
    case 'I': type = IO_TYPE_MODBUS_CONTACT; break;
    case 'M': type = IO_TYPE_MODBUS_COIL; break;
    case 'H': type = IO_TYPE_MODBUS_HREG; break;
    case 'G': type = IO_TYPE_GENERAL; break;
    default: type = default_type;
    };

    AppendIo(name, type);
}

static void AppendIoAutoTypePortGeneral(char *name)
{
    if(name[0] == '#')
        AppendIo(name, IO_TYPE_PORT_OUTPUT);
    else
        AppendIoAutoType(name, IO_TYPE_GENERAL);
}

static void AppendIoAutoTypePinGeneral(char *name)
{
    if(name[0] == '#')
        AppendIo(name, IO_TYPE_PORT_INPUT);
    else
        AppendIoAutoType(name, IO_TYPE_GENERAL);
}

//-----------------------------------------------------------------------------
/*
static void ExtractPortsFromMcu()
{
    char str[MAX_NAME_LEN];
    if(Prog.mcu) {
        int i;
        for(i=0; i<MAX_IO_PORTS; i++) {
            if((Prog.mcu->inputRegs[i] != 0) && (Prog.mcu->inputRegs[i] != 0xff)) {
                 sprintf(str,"%s%c", "#PIN", 'A'+i);
                 AppendIo(str, IO_TYPE_PORT_INPUT);

            }
            if((Prog.mcu->outputRegs[i] != 0) && (Prog.mcu->outputRegs[i] != 0xff)) {
                 sprintf(str,"%s%c", "#PORT", 'A'+i);
                 AppendIo(str, IO_TYPE_PORT_OUTPUT);

            }
        }
    }
}
*/
//-----------------------------------------------------------------------------
// Walk a subcircuit, calling ourselves recursively and extracting all the
// I/O names out of it.
//-----------------------------------------------------------------------------
static void ExtractNamesFromCircuit(int which, void *any)
{
    ElemLeaf *l = (ElemLeaf *)any;

    switch(which) {
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)any;
            int i;
            for(i = 0; i < p->count; i++) {
                ExtractNamesFromCircuit(p->contents[i].which,
                    p->contents[i].d.any);
            }
            break;
        }
        case ELEM_SERIES_SUBCKT: {
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;
            int i;
            for(i = 0; i < s->count; i++) {
                ExtractNamesFromCircuit(s->contents[i].which,
                    s->contents[i].d.any);
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
            switch (l->d.contacts.name[0]) {
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
            switch(l->d.QuadEncod.contactA[0]) {
                case 'I':
                    AppendIo(l->d.QuadEncod.contactA, IO_TYPE_INT_INPUT);
                    break;
                default:
                    Error(_("Connect QUAD ENCOD input A to INTs input pin IqAn."));
                    break;
            }
            switch(l->d.QuadEncod.contactB[0]) {
                case 'X':
                    AppendIo(l->d.QuadEncod.contactB, IO_TYPE_DIG_INPUT);
                    break;
                default:
                    Error(_("Connect QUAD ENCOD input B to input pin XqBn."));
                    break;
            }
            if(strlen(l->d.QuadEncod.contactZ)>0)
            switch(l->d.QuadEncod.contactZ[0]) {
                case 'X':
                    AppendIo(l->d.QuadEncod.contactZ, IO_TYPE_DIG_INPUT);
                    break;
                default:
                    Error(_("Connect QUAD ENCOD input Z to input pin XqZn."));
                    break;
            }
            if(strlen(l->d.QuadEncod.zero)>0)
            switch(l->d.QuadEncod.zero[0]) {
                case 'Y':
                    AppendIo(l->d.QuadEncod.zero, IO_TYPE_DIG_OUTPUT);
                    break;
                default:
                    Error(_("Connect QUAD ENCOD zero flag to output pin YsomeName."));
                    break;
            }
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
            char str[MAX_NAME_LEN];
            sprintf(str, "C%s%s", l->d.stepper.name, "Dec");
            AppendIo(str, IO_TYPE_COUNTER);

            if(IsNumber(l->d.stepper.P)&&(CheckMakeNumber(l->d.stepper.P)>1)
            ||(l->d.stepper.graph>0)//&&(l->d.stepper.n>1)
            ||(!IsNumber(l->d.stepper.P))){
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

        case ELEM_BIN2BCD:
        case ELEM_BCD2BIN:
        case ELEM_SWAP:
        case ELEM_BUS:
        case ELEM_MOVE:
            AppendIoAutoTypePortGeneral(l->d.move.dest);
            if(CheckForNumber(l->d.move.src) == FALSE) {
                // Not need ???
                // Need if you add only one MOV or get erroneously other src name
                // then you can see l->d.move.src in IOlist 
                AppendIoAutoTypePinGeneral(l->d.move.src);
            }
            break;
        {
        int n;
        char *nameTable;
        case ELEM_7SEG: nameTable = "char7seg";  n = LEN7SEG;  goto xseg;
        case ELEM_9SEG: nameTable = "char9seg";  n = LEN9SEG;  goto xseg;
        case ELEM_14SEG:nameTable = "char14seg"; n = LEN14SEG; goto xseg;
        case ELEM_16SEG:nameTable = "char16seg"; n = LEN16SEG; goto xseg;
        xseg:
            AppendIoAutoTypePortGeneral(l->d.segments.dest);
            /*
            if (CheckForNumber(l->d.segments.src) == FALSE) {
                AppendIo(l->d.segments.src, IO_TYPE_GENERAL); // not need ???
            }
            */
            AppendIo(nameTable, IO_TYPE_TABLE);
            SetSizeOfVar(nameTable, n);
            break;
        }

        case ELEM_SET_BIT     :
        case ELEM_CLEAR_BIT   :
            AppendIoAutoTypePortGeneral(l->d.move.dest);
            break;

        case ELEM_IF_BIT_SET  :
        case ELEM_IF_BIT_CLEAR:
            AppendIoAutoTypePinGeneral(l->d.move.dest);
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
            if (CheckForNumber(l->d.math.op1) == FALSE) {
                AppendIoAutoType(l->d.math.op1, IO_TYPE_GENERAL);
            }
            if (which != ELEM_NOT)
            if (which != ELEM_NEG)
            if (CheckForNumber(l->d.math.op2) == FALSE) {
                AppendIoAutoType(l->d.math.op2, IO_TYPE_GENERAL);
            }
            AppendIoAutoType(l->d.math.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_STRING:
            if(strlen(l->d.fmtdStr.dest) > 0) {
                AppendIo(l->d.fmtdStr.dest, IO_TYPE_GENERAL);
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

        case ELEM_UART_SEND:
            AppendIo(l->d.uart.name, IO_TYPE_UART_TX);
            break;

        case ELEM_UART_RECV:
            AppendIo(l->d.uart.name, IO_TYPE_UART_RX);
            break;

        case ELEM_SET_PWM:
            AppendIo(l->d.setPwm.name, IO_TYPE_PWM_OUTPUT);
            AppendIo(l->d.setPwm.duty_cycle, IO_TYPE_GENERAL);
            break;

        case ELEM_CTU:
        case ELEM_CTD:
        case ELEM_CTC:
        case ELEM_CTR:
            AppendIo(l->d.counter.name, IO_TYPE_COUNTER);
            if(CheckForNumber(l->d.counter.max) == FALSE) {
                // Not need ??? See ELEM_MOV
                AppendIoAutoTypePinGeneral(l->d.counter.max);
            }
            break;

        case ELEM_READ_ADC:
            AppendIo(l->d.readAdc.name, IO_TYPE_READ_ADC);
            break;

        case ELEM_SHIFT_REGISTER: {
            int i;
            for(i = 0; i < l->d.shiftRegister.stages; i++) {
                char str[MAX_NAME_LEN+10];
                sprintf(str, "%s%d", l->d.shiftRegister.name, i);
                AppendIo(str, IO_TYPE_GENERAL);
            }
            break;
        }

        case ELEM_LOOK_UP_TABLE:
            AppendIo(l->d.lookUpTable.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_PIECEWISE_LINEAR:
            AppendIo(l->d.piecewiseLinear.dest, IO_TYPE_GENERAL);
            break;

        case ELEM_PLACEHOLDER:
        case ELEM_COMMENT:
        case ELEM_SHORT:
        case ELEM_OPEN:
        case ELEM_MASTER_RELAY:
        case ELEM_ONE_SHOT_RISING:
        case ELEM_ONE_SHOT_FALLING:
        case ELEM_OSC:
        case ELEM_EQU:
        case ELEM_NEQ:
        case ELEM_GRT:
        case ELEM_GEQ:
        case ELEM_LES:
        case ELEM_LEQ:
        case ELEM_RES:
        case ELEM_RSFR:
        case ELEM_WSFR:
        case ELEM_SSFR:
        case ELEM_CSFR:
        case ELEM_TSFR:
        case ELEM_T_C_SFR:
        case ELEM_PWM_OFF:
        case ELEM_NPULSE_OFF:
            break;

        case ELEM_PADDING:
            break;

        case ELEM_PERSIST:
            AppendIo(l->d.persist.var, IO_TYPE_PERSIST);
            break;

        default:
            ooops("which=%d",which);
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

    if(a->pin == NO_PIN_ASSIGNED && b->pin != NO_PIN_ASSIGNED) return  1;
    if(b->pin == NO_PIN_ASSIGNED && a->pin != NO_PIN_ASSIGNED) return -1;
/*
    if(a->pin != NO_PIN_ASSIGNED && b->pin != NO_PIN_ASSIGNED) {
        if((a->type == IO_TYPE_DIG_INPUT)
         ||(a->type == IO_TYPE_DIG_OUTPUT)
         ||(a->type == IO_TYPE_INT_INPUT)
         ||(a->type == IO_TYPE_READ_ADC)) {

            char PinName[MAX_NAME_LEN] = "";
            char apin[MAX_NAME_LEN] = "";
            char bpin[MAX_NAME_LEN] = "";
            char aPortName[MAX_NAME_LEN] = "";
            char bPortName[MAX_NAME_LEN] = "";

            PinNumberForIo(apin, a, aPortName, PinName);
            PinNumberForIo(bpin, b, bPortName, PinName);

            return strcmp(aPortName, bPortName);
        }
    }
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

    if(IoSeenPreviouslyCount > MAX_IO_SEEN_PREVIOUSLY/2) {
        // flush it so there's lots of room, and we don't run out and
        // forget important things
        IoSeenPreviouslyCount = 0;
    }

    // remember the pin assignments
    for(i = 0; i < Prog.io.count; i++) {
        AppendIoSeenPreviously(Prog.io.assignment[i].name,
            Prog.io.assignment[i].type, Prog.io.assignment[i].pin, Prog.io.assignment[i].modbus);
    }
    // wipe the list
    Prog.io.count = 0;
    // extract the new list so that it must be up to date
    for(i = 0; i < Prog.numRungs; i++) {
        ExtractNamesFromCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    }
    if(Prog.cycleDuty) {
        AppendIo(YPlcCycleDuty, IO_TYPE_DIG_OUTPUT);
    }
    for(i = 0; i < Prog.io.count; i++) {
        if(Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT ||
           Prog.io.assignment[i].type == IO_TYPE_INT_INPUT ||
           Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_PWM_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_CONTACT ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_COIL ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_HREG ||
           Prog.io.assignment[i].type == IO_TYPE_READ_ADC)
        {
            for(j = 0; j < IoSeenPreviouslyCount; j++) {
                if(strcmp(Prog.io.assignment[i].name,
                    IoSeenPreviously[j].name)==0)
                {
                    Prog.io.assignment[i].pin = IoSeenPreviously[j].pin;
                    Prog.io.assignment[i].modbus = IoSeenPreviously[j].modbus;
                    break;
                }
            }
        }
    }

    qsort(Prog.io.assignment, Prog.io.count, sizeof(PlcProgramSingleIo),
        CompareIo);

    if(prevSel >= 0) {
        for(i = 0; i < Prog.io.count; i++) {
            if (strlen(IoListSelectionName)) {
               if(strcmp(Prog.io.assignment[i].name, IoListSelectionName)==0)
                   return i;
            } else
                if(strcmp(Prog.io.assignment[i].name, selName)==0)
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
BOOL LoadIoListFromFile(FILE *f)
{
    char line[MAX_NAME_LEN];
    char name[MAX_NAME_LEN];
    int pin;
    ModbusAddr_t modbus;
    while(fgets(line, sizeof(line), f)) {
        if(!strlen(strspace(line))) continue;
        if(strcmp(line, "END\n")==0) {
            return TRUE;
        }
        // Don't internationalize this! It's the file format, not UI.
        modbus.Slave = 0;
        modbus.Address = 0;
        if(sscanf(line, "    %s at %d %hhd %hd", name, &pin, &modbus.Slave, &modbus.Address)>=2) {
            int type;
            switch(name[0]) {
                case 'X': type = IO_TYPE_DIG_INPUT; break;
                case 'Y': type = IO_TYPE_DIG_OUTPUT; break;
                case 'A': type = IO_TYPE_READ_ADC; break;
                case 'P': type = IO_TYPE_PWM_OUTPUT; break;
                case 'I': type = IO_TYPE_MODBUS_CONTACT; break;
                case 'M': type = IO_TYPE_MODBUS_COIL; break;
                case 'C': type = IO_TYPE_COUNTER; break;
                case 'H': type = IO_TYPE_MODBUS_HREG; break;
                default: oops();
            }
            AppendIoSeenPreviously(name, type, pin, modbus);
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Write the I/O list to a file. Since everything except the pin assignment
// can be extracted from the schematic, just write the Xs and Ys.
//-----------------------------------------------------------------------------
void SaveIoListToFile(FILE *f)
{
    int i;
    for(i = 0; i < Prog.io.count; i++) {
        if(Prog.io.assignment[i].type == IO_TYPE_DIG_INPUT  ||
           Prog.io.assignment[i].type == IO_TYPE_DIG_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_INT_INPUT  ||
           Prog.io.assignment[i].type == IO_TYPE_PWM_OUTPUT ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_CONTACT ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_COIL ||
           Prog.io.assignment[i].type == IO_TYPE_MODBUS_HREG ||
           Prog.io.assignment[i].type == IO_TYPE_READ_ADC)
        {
            // Don't internationalize this! It's the file format, not UI.
            fprintf(f, "    %s at %d %d %d\n",
                Prog.io.assignment[i].name,
                Prog.io.assignment[i].pin,
                Prog.io.assignment[i].modbus.Slave,
                Prog.io.assignment[i].modbus.Address);
        }
    }
}

//-----------------------------------------------------------------------------
// Dialog proc for the popup that lets you set the value of an analog input for
// simulation.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK AnalogSliderDialogProc(HWND hwnd, UINT msg,
    WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            AnalogSliderDone = TRUE;
            AnalogSliderCancel = TRUE;
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

    wc.style            = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC |
                            CS_DBLCLKS;
    wc.lpfnWndProc      = (WNDPROC)AnalogSliderDialogProc;
    wc.hInstance        = Instance;
    wc.hbrBackground    = (HBRUSH)COLOR_BTNSHADOW;
    wc.lpszClassName    = "LDmicroAnalogSlider";
    wc.lpszMenuName     = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);

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

    int left = pt.x - 10;
    // try to put the slider directly under the cursor (though later we might
    // realize that that would put the popup off the screen)
    int top = pt.y - (15 + (73*currentVal)/maxVal);

    RECT r;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);

    if(top + 110  +28 >= r.bottom) {
        top = r.bottom - 110 -28;
    }
    if(top < 0) top = 0;

    AnalogSliderMain = CreateWindowClient(0, "LDmicroAnalogSlider", "I/O Pin",
        WS_VISIBLE | WS_POPUP | WS_DLGFRAME,
        left, top, 30 +5, 100 +28, NULL, NULL, Instance, NULL);

    AnalogSliderTrackbar = CreateWindowEx(0, TRACKBAR_CLASS, "", WS_CHILD |
        TBS_AUTOTICKS | TBS_VERT | TBS_TOOLTIPS | WS_CLIPSIBLINGS | WS_VISIBLE,
        0, 0, 30 +5, 100 +28, AnalogSliderMain, NULL, Instance, NULL);
    SendMessage(AnalogSliderTrackbar, TBM_SETRANGE, FALSE,
        MAKELONG(0, maxVal));
    SendMessage(AnalogSliderTrackbar, TBM_SETTICFREQ, (maxVal + 1)/8, 0);
    SendMessage(AnalogSliderTrackbar, TBM_SETPOS, TRUE, currentVal);

    SendMessage(AnalogSliderTrackbar, TBM_SETPAGESIZE, 0, 10);
    SendMessage(AnalogSliderTrackbar, TBM_SETLINESIZE, 0, 1);

    EnableWindow(MainWindow, FALSE);
    ShowWindow(AnalogSliderMain, TRUE);
    SetFocus(AnalogSliderTrackbar);

    DWORD ret;
    MSG msg;
    AnalogSliderDone = FALSE;
    AnalogSliderCancel = FALSE;

    SWORD orig = GetAdcShadow(name);

    while(!AnalogSliderDone && (ret = GetMessage(&msg, NULL, 0, 0))) {
        SWORD v = (SWORD)SendMessage(AnalogSliderTrackbar, TBM_GETPOS, 0, 0);

        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_RETURN) {
                AnalogSliderDone = TRUE;
                break;
            } else if(msg.wParam == VK_ESCAPE) {
                AnalogSliderDone = TRUE;
                AnalogSliderCancel = TRUE;
                break;
            }
        } else if(msg.message == WM_LBUTTONUP) {
            if(v != orig) {
// //            AnalogSliderDone = TRUE; // this line not allow a kyboard UP DN
            }
        }
        SetAdcShadow(name, v);

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!AnalogSliderCancel) {
        SWORD v = (SWORD)SendMessage(AnalogSliderTrackbar, TBM_GETPOS, 0, 0);
        SetAdcShadow(name, v);
    }

    EnableWindow(MainWindow, TRUE);
    DestroyWindow(AnalogSliderMain);
    ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
}

//-----------------------------------------------------------------------------
// Window proc for the contacts dialog box
//-----------------------------------------------------------------------------
static LRESULT CALLBACK IoDialogProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
    switch (msg) {
        case WM_COMMAND: {
            HWND h = (HWND)lParam;
            if(h == OkButton && wParam == BN_CLICKED) {
                DialogDone = TRUE;
            } else if(h == CancelButton && wParam == BN_CLICKED) {
                DialogDone = TRUE;
                DialogCancel = TRUE;
            } else if(h == PinList && HIWORD(wParam) == LBN_DBLCLK) {
                DialogDone = TRUE;
            }
            break;
        }

        case WM_CLOSE:
        case WM_DESTROY:
            DialogDone = TRUE;
            DialogCancel = TRUE;
            return 1;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 1;
}

//-----------------------------------------------------------------------------
// Create our window class; nothing exciting.
//-----------------------------------------------------------------------------
static BOOL MakeWindowClass()
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);

    wc.style            = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC |
                            CS_DBLCLKS;
    wc.lpfnWndProc      = (WNDPROC)IoDialogProc;
    wc.hInstance        = Instance;
    wc.hbrBackground    = (HBRUSH)COLOR_BTNSHADOW;
    wc.lpszClassName    = "LDmicroIo";
    wc.lpszMenuName     = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon            = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
                            IMAGE_ICON, 32, 32, 0);
    wc.hIconSm          = (HICON)LoadImage(Instance, MAKEINTRESOURCE(4000),
                            IMAGE_ICON, 16, 16, 0);

    return RegisterClassEx(&wc);
}

#define AddX 200
#define AddY 50
static void MakeControls(void)
{
    HWND textLabel = CreateWindowEx(0, WC_STATIC, _("Assign:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        6, 1, 80, 17, IoDialog, NULL, Instance, NULL);
    NiceFont(textLabel);

    PinList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTBOX, "",
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | WS_VSCROLL |
        LBS_NOTIFY, 6, 18, 95+AddX, 320+AddY, IoDialog, NULL, Instance, NULL);
    FixedFont(PinList);

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        6, 325+AddY+4, 95, 23, IoDialog, NULL, Instance, NULL);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        6, 356+AddY+2, 95, 23, IoDialog, NULL, Instance, NULL);
    NiceFont(CancelButton);
}

//-----------------------------------------------------------------------------
void ShowIoDialog(int item)
{
    int type = Prog.io.assignment[item].type;
    switch(type) {
        case IO_TYPE_GENERAL:
        case IO_TYPE_PERSIST:
        case IO_TYPE_STRING:
        case IO_TYPE_RTO:
        case IO_TYPE_COUNTER:
        case IO_TYPE_UART_TX:
        case IO_TYPE_UART_RX:
        case IO_TYPE_TCY:
        case IO_TYPE_TON:
        case IO_TYPE_TOF:
            ShowSizeOfVarDialog(&Prog.io.assignment[item]);
            return;
    }
    if(!Prog.mcu) {
        MessageBox(MainWindow,
            _("No microcontroller has been selected. You must select a "
            "microcontroller before you can assign I/O pins.\r\n\r\n"
            "Select a microcontroller under the Settings menu and try "
            "again."), _("I/O Pin Assignment"), MB_OK | MB_ICONWARNING);
        return;
    }

    if(Prog.mcu->whichIsa == ISA_INTERPRETED) {
        Error(_("Can't specify I/O assignment for interpretable target; see "
            "comments in reference implementation of interpreter."));
        return;
    }

    if(Prog.mcu->whichIsa == ISA_NETZER) {
        Error(_("Can't specify I/O assignment for Netzer!"));
        return;
    }

    if(Prog.io.assignment[item].name[0] != 'X' &&
       Prog.io.assignment[item].name[0] != 'Y' &&
       Prog.io.assignment[item].name[0] != 'A' &&
       Prog.io.assignment[item].name[0] != 'P')
    {
        Error(_("Can only assign pin number to input/output pins (Xname or "
            "Yname or Aname or Pname)."));
        return;
    }

    if((Prog.io.assignment[item].type == IO_TYPE_READ_ADC) && (Prog.mcu->adcCount == 0)) {
        Error(_("No ADC or ADC not supported for this micro."));
        return;
    }

    if((Prog.io.assignment[item].type == IO_TYPE_PWM_OUTPUT) && (Prog.mcu->pwmCount == 0) && (Prog.mcu->pwmNeedsPin == 0)) {
        Error(_("No PWM or PWM not supported for this MCU."));
        return;
    }

    if(strcmp(Prog.io.assignment[item].name+1, "new")==0) {
        Error(_("Rename I/O from default name ('%s') before assigning "
            "MCU pin."), Prog.io.assignment[item].name);
        return;
    }

    MakeWindowClass();

    // We need the TOOLWINDOW style, or else the window will be forced to
    // a minimum width greater than our current width. And without the
    // APPWINDOW style, it becomes impossible to get the window back (by
    // Alt+Tab or taskbar).
    IoDialog = CreateWindowClient(WS_EX_TOOLWINDOW | WS_EX_APPWINDOW,
        "LDmicroIo", _("I/O Pin"),
        WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 106+AddX, 387+AddY, NULL, NULL, Instance, NULL);

    MakeControls();

    SendMessage(PinList, LB_ADDSTRING, 0, (LPARAM)_("(no pin)"));
    int Index = 0;
    char buf[MAX_NAME_LEN];
    int j;
    int i;
    for(i = 0; i < Prog.mcu->pinCount; i++) {
        for(j = 0; j < Prog.io.count; j++) {
            if(j == item) continue;
            if(Prog.io.assignment[j].pin == Prog.mcu->pinInfo[i].pin) {
                goto cant_use_this_io;
            }
        }

        if(UartFunctionUsed() && Prog.mcu &&
                ((Prog.mcu->pinInfo[i].pin == Prog.mcu->uartNeeds.rxPin) ||
                 (Prog.mcu->pinInfo[i].pin == Prog.mcu->uartNeeds.txPin)))
        {
            goto cant_use_this_io;
        }

#if 0
        if(PwmFunctionUsed() &&
            Prog.mcu->pinInfo[i].pin == Prog.mcu->pwmNeedsPin)
        {
            goto cant_use_this_io;
        }
#endif
        int type = Prog.io.assignment[item].type;
        if((type == IO_TYPE_INT_INPUT) && (!IsInterruptPin(Prog.mcu->pinInfo[i].pin)))
            goto cant_use_this_io;

        if(Prog.io.assignment[item].type == IO_TYPE_READ_ADC) {
            McuAdcPinInfo *iop = AdcPinInfo(Prog.mcu->pinInfo[i].pin);
            if(iop)
                ; // okay; we know how to connect it up to the ADC
            else
                goto cant_use_this_io;

        }
        if(Prog.io.assignment[item].type == IO_TYPE_PWM_OUTPUT) {
            McuPwmPinInfo *iop = PwmPinInfo(Prog.mcu->pinInfo[i].pin);
            if((iop)&&(iop->timer != Prog.cycleTimer))
                ; // okay; we know how to connect it up to the PWM
            else
                goto cant_use_this_io;

        }

        char pinName[MAX_NAME_LEN];
        GetPinName(Prog.mcu->pinInfo[i].pin, pinName);
        sprintf(buf, "%3d  %s", Prog.mcu->pinInfo[i].pin, pinName);

        if(Prog.mcu->pinInfo[i].pin == Prog.io.assignment[item].pin) {
            Index = SendMessage(PinList, LB_GETCOUNT, 0, 0);
            if(Index == LB_ERR) Index = 0;
        };

        SendMessage(PinList, LB_ADDSTRING, 0, (LPARAM)buf);
    cant_use_this_io:;
    }

    for(j = 0; j < Prog.mcu->adcCount; j++) {
      //if(Prog.io.assignment[item].name[0] == 'A') {
        if(Prog.io.assignment[item].type == IO_TYPE_READ_ADC) {
            for(i = 0; i < Prog.mcu->pinCount; i++) {
                if(Prog.mcu->adcInfo[j].pin == Prog.mcu->pinInfo[i].pin) {
                    // okay; we know how to connect it up to the ADC
                    // break;
                    goto cant_use_this_io_adc;
                }
            }
            if(j == Prog.mcu->adcCount) {
                goto cant_use_this_io_adc;
            } else {
                sprintf(buf, "%3d  ADC%d", Prog.mcu->adcInfo[j].pin,
                    Prog.mcu->adcInfo[j].muxRegValue);
            }
            SendMessage(PinList, LB_ADDSTRING, 0, (LPARAM)buf);
        }
    cant_use_this_io_adc:;
    }

    EnableWindow(MainWindow, FALSE);
    ShowWindow(IoDialog, TRUE);
    SetFocus(PinList);

    SendMessage(PinList, LB_SETCURSEL, (WPARAM)Index, 0);

    MSG msg;
    DWORD ret;
    DialogDone = FALSE;
    DialogCancel = FALSE;
    while((ret = GetMessage(&msg, NULL, 0, 0)) && !DialogDone) {
        if(msg.message == WM_KEYDOWN) {
            if(msg.wParam == VK_RETURN) {
                DialogDone = TRUE;
                break;
            } else if(msg.wParam == VK_ESCAPE) {
                DialogDone = TRUE;
                DialogCancel = TRUE;
                break;
            }
        }

        if(IsDialogMessage(IoDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        int sel = SendMessage(PinList, LB_GETCURSEL, 0, 0);
        char pin[MAX_NAME_LEN];
        SendMessage(PinList, LB_GETTEXT, (WPARAM)sel, (LPARAM)pin);
        if(strcmp(pin, _("(no pin)"))==0) {
            int i;
            for(i = 0; i < IoSeenPreviouslyCount; i++) {
                if(strcmp(IoSeenPreviously[i].name,
                    Prog.io.assignment[item].name)==0)
                {
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

    EnableWindow(MainWindow, TRUE);
    DestroyWindow(IoDialog);
    SetFocus(IoList);
    strcpy(IoListSelectionName, Prog.io.assignment[item].name);
    return;
}

//-----------------------------------------------------------------------------
static void MakeModbusControls(void)
{
    HWND textLabel2 = CreateWindowEx(0, WC_STATIC, _("Slave ID:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        6, 1, 70, 21, IoDialog, NULL, Instance, NULL);
    NiceFont(textLabel2);

    ModbusSlave = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        80, 1, 30, 21, IoDialog, NULL, Instance, NULL);
    FixedFont(ModbusSlave);

    HWND textLabel3 = CreateWindowEx(0, WC_STATIC, _("Register:"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        6, 24, 70, 21, IoDialog, NULL, Instance, NULL);
    NiceFont(textLabel3);

    ModbusRegister = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        80, 24, 80, 21, IoDialog, NULL, Instance, NULL);
    FixedFont(ModbusRegister);

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        6, 48, 50, 23, IoDialog, NULL, Instance, NULL);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        56, 48, 50, 23, IoDialog, NULL, Instance, NULL);
    NiceFont(CancelButton);
}

void ShowModbusDialog(int item)
{
    MakeWindowClass();

    IoDialog = CreateWindowClient(WS_EX_TOOLWINDOW | WS_EX_APPWINDOW,
        "LDmicroIo", _("Modbus Address"),
        WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 170, 80, NULL, NULL, Instance, NULL);

    MakeModbusControls();

    char txtModbusSlave[10];
    char txtModbusRegister[20];
    sprintf(txtModbusSlave, "%d", Prog.io.assignment[item].modbus.Slave);
    sprintf(txtModbusRegister, "%05d", Prog.io.assignment[item].modbus.Address);

    SendMessage(ModbusSlave, WM_SETTEXT, 0, (LPARAM)txtModbusSlave);
    SendMessage(ModbusRegister, WM_SETTEXT, 0, (LPARAM)txtModbusRegister);

    EnableWindow(MainWindow, FALSE);
    ShowWindow(IoDialog, TRUE);

    MSG msg;
    DWORD ret;
    DialogDone = FALSE;
    DialogCancel = FALSE;
    while ((ret = GetMessage(&msg, NULL, 0, 0)) && !DialogDone) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_RETURN) {
                DialogDone = TRUE;
                break;
            }
            else if (msg.wParam == VK_ESCAPE) {
                DialogDone = TRUE;
                DialogCancel = TRUE;
                break;
            }
        }

        if (IsDialogMessage(IoDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (!DialogCancel) {
        SendMessage(ModbusSlave, WM_GETTEXT, (WPARAM)sizeof(txtModbusSlave), (LPARAM)txtModbusSlave);
        SendMessage(ModbusRegister, WM_GETTEXT, (WPARAM)sizeof(txtModbusRegister), (LPARAM)txtModbusRegister);
        Prog.io.assignment[item].modbus.Slave = atoi(txtModbusSlave);
        Prog.io.assignment[item].modbus.Address = atoi(txtModbusRegister);
    }

    EnableWindow(MainWindow, TRUE);
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
            if(!((i->item.mask & LVIF_TEXT) &&
                 (i->item.pszText) &&
                 (i->item.cchTextMax > 200)))
            {
                // This test didn't used to be present, and Windows 10 now
                // sends an LVN_GETDISPINFO that fails it, which would
                // otherwise cause us to write to a null pointer.
                break;
            }
            strcpy(i->item.pszText, "");
            int item = i->item.iItem;
            char *name = Prog.io.assignment[item].name;
            int   type = Prog.io.assignment[item].type;
            switch(i->item.iSubItem) {
                case LV_IO_PIN:
                    // Don't confuse people by displaying bogus pin assignments
                    // for the target.
                    if(Prog.mcu && (Prog.mcu->whichIsa == ISA_NETZER ||
                                    Prog.mcu->whichIsa == ISA_XINTERPRETED ||
                                    Prog.mcu->whichIsa == ISA_INTERPRETED) )
                    {
                        strcpy(i->item.pszText, "");
                        break;
                    }

                    PinNumberForIo(i->item.pszText,
                        &(Prog.io.assignment[item]));
                    break;

                case LV_IO_TYPE: {
                    char *s = IoTypeToString(Prog.io.assignment[item].type);
                    strcpy(i->item.pszText, s);
                    break;
                }

                case LV_IO_NAME:
                    strcpy(i->item.pszText, Prog.io.assignment[item].name);
                    break;

                case LV_IO_RAM_ADDRESS: {
                    DWORD addr = 0;
                    int bit = 0;
                    if((type == IO_TYPE_PORT_INPUT      )
                    || (type == IO_TYPE_PORT_OUTPUT     )
                    ) {
                        if(!InSimulationMode) {
                            MemForVariable(name, &addr);
                            if(addr)
                                sprintf(i->item.pszText, "0x%x", addr);
                             else
                                sprintf(i->item.pszText, "Not a PORT!");
                        }
                    } else
                    if((type == IO_TYPE_GENERAL)
                    || (type == IO_TYPE_PERSIST)
                    || (type == IO_TYPE_STRING)
                    || (type == IO_TYPE_RTO)
                    || (type == IO_TYPE_TCY)
                    || (type == IO_TYPE_TON)
                    || (type == IO_TYPE_TOF)
                    || (type == IO_TYPE_COUNTER)
                    ) {
                        if(!InSimulationMode && AllocOfVar(name)) {
                            MemForVariable(name, &addr);
                            sprintf(i->item.pszText, "0x%x", addr);
                        }
                    } else
                    if((type == IO_TYPE_INTERNAL_RELAY)
                    ) {
                        if(!InSimulationMode) {
                            MemForSingleBit(name, TRUE, &addr, &bit);
                            if(addr)
                                sprintf(i->item.pszText, "0x%02x (BIT%d)", addr, bit);
                        }
                    } else
                    if((type == IO_TYPE_DIG_INPUT)
                    || (type == IO_TYPE_DIG_OUTPUT)
                    ) {
                        if(!InSimulationMode) {
                            if(SingleBitAssigned(name))
                                MemForSingleBit(name, TRUE, &addr, &bit);
                            if(addr)
                                sprintf(i->item.pszText, "0x%02x (BIT%d)", addr, bit);
                        }
                    }
                    break;
                }

                case LV_IO_SISE_OF_VAR:
                    if((type == IO_TYPE_GENERAL         )
                    || (type == IO_TYPE_PERSIST         )
                    || (type == IO_TYPE_PORT_INPUT      )
                    || (type == IO_TYPE_PORT_OUTPUT     )
                    || (type == IO_TYPE_STRING          )
                    || (type == IO_TYPE_RTO             )
                    || (type == IO_TYPE_COUNTER         )
                    || (type == IO_TYPE_UART_TX         )
                    || (type == IO_TYPE_UART_RX         )
                    || (type == IO_TYPE_TABLE           )
                    || (type == IO_TYPE_TCY             )
                    || (type == IO_TYPE_TON             )
                    || (type == IO_TYPE_TOF             )) {
                        int sov = SizeOfVar(name);
                        sprintf(i->item.pszText, sov==1 ? "%d byte" : "%d bytes", sov);
                    } else
                    if((type == IO_TYPE_DIG_INPUT)
                    || (type == IO_TYPE_DIG_OUTPUT)
                    || (type == IO_TYPE_INTERNAL_RELAY)
                    || (type == IO_TYPE_UART_TX)
                    || (type == IO_TYPE_UART_RX)
                    || (type == IO_TYPE_READ_ADC)
                    || (type == IO_TYPE_PWM_OUTPUT)) {
                        sprintf(i->item.pszText, "1 bit");
                    }
                    break;

                case LV_IO_PORT: {
                case LV_IO_PINNAME:
                    char pin[MAX_NAME_LEN];
                    char poptName[MAX_NAME_LEN];
                    char pinName[MAX_NAME_LEN];
                    PinNumberForIo(pin,
                        &(Prog.io.assignment[item]), poptName, pinName);
                    if(i->item.iSubItem == LV_IO_PORT)
                        strcpy(i->item.pszText, poptName);
                    else
                        strcpy(i->item.pszText, pinName);
                    break;
                }

                case LV_IO_STATE: {
                    if(TRUE || InSimulationMode) {
                        DescribeForIoList(name, type, i->item.pszText);
                    } else {
                        strcpy(i->item.pszText, "");
                    }
                    break;
                }

                case LV_IO_MODBUS: {
                    if (type != IO_TYPE_MODBUS_COIL &&
                        type != IO_TYPE_MODBUS_CONTACT &&
                        type != IO_TYPE_MODBUS_HREG)
                    {
                        strcpy(i->item.pszText, "");
                        break;
                    }

                    if (Prog.io.assignment[item].modbus.Slave == 0) {
                        sprintf(i->item.pszText, _("(not assigned)"));
                    }
                    else {
                        sprintf(i->item.pszText, "%d:%05d",
                            Prog.io.assignment[item].modbus.Slave,
                            Prog.io.assignment[item].modbus.Address);
                    }
                    break;
                }

            }
            break;
        }
        case LVN_ITEMACTIVATE: {
            NMITEMACTIVATE *i = (NMITEMACTIVATE *)h;
            char *name = Prog.io.assignment[i->iItem].name;
            int   type = Prog.io.assignment[i->iItem].type;
            if(InSimulationMode) {
                switch (type) {
                    case IO_TYPE_STRING:
                    case IO_TYPE_GENERAL:
                    case IO_TYPE_PERSIST:
                    case IO_TYPE_RTO:
                    case IO_TYPE_COUNTER:
                    case IO_TYPE_READ_ADC:
                    case IO_TYPE_UART_TX:
                    case IO_TYPE_UART_RX:
                    case IO_TYPE_PWM_OUTPUT:
                    case IO_TYPE_TCY:
                    case IO_TYPE_TON:
                    case IO_TYPE_TOF: {
                        ShowIoDialog(i->iItem);
                        InvalidateRect(MainWindow, NULL, FALSE);
                        ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
                        break;
                    }
                    default: {
                        if(name[0] == 'X') {
                            SimulationToggleContact(name);
                        } else if(type == IO_TYPE_READ_ADC) {
                            ShowAnalogSliderPopup(name);
                        }
                    }
                }
            } else {
                UndoRemember();
                switch (type) {
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
                InvalidateRect(MainWindow, NULL, FALSE);
            }
            break;
        }
    }
}
