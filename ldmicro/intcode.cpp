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
#include <windows.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>

#include "ldmicro.h"
#include "intcode.h"

//-----------------------------------------------------------------------------
#define parThis_parOut_optimize
//-----------------------------------------------------------------------------
int int_comment_level  = 3;
//                       0 - no comments
//                       1 = Release 2.3 comments
//                       2 - more comments
//                     * 3 - ELEM_XXX comments added
//-----------------------------------------------------------------------------

IntOp IntCode[MAX_INT_OPS];
int IntCodeLen = 0;
int ProgWriteP = 0;
int rungNow = -INT_MAX;
static int whichNow = -INT_MAX;

static DWORD GenSymCountParThis;
static DWORD GenSymCountParOut;
static DWORD GenSymCountOneShot;
static DWORD GenSymCountFormattedString;
static DWORD GenSymCountStepper;

static WORD EepromAddrFree;

//-----------------------------------------------------------------------------
// Pretty-print the intermediate code to a file, for debugging purposes.
//-----------------------------------------------------------------------------
void IntDumpListing(char *outFile)
{
    FILE *f = fopen(outFile, "w");
    if(!f) {
        Error("Couldn't dump intermediate code to '%s'.", outFile);
    }

    int i;
    int indent = 0;
    for(i = 0; i < IntCodeLen; i++) {

        if(IntCode[i].op == INT_END_IF) indent--;
        if(IntCode[i].op == INT_ELSE) indent--;
        if(indent < 0) indent = 0;

        if(int_comment_level == 1) {
            fprintf(f, "%3d:", i);
        } else {
        if (IntCode[i].op != INT_SIMULATE_NODE_STATE)
        fprintf(f, "%4d:", i);
        }
        int j;
        if (IntCode[i].op != INT_SIMULATE_NODE_STATE)
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

            case INT_SET_VARIABLE_TO_LITERAL:
                fprintf(f, "let var '%s' := %d", IntCode[i].name1,
                    IntCode[i].literal);
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                fprintf(f, "let var '%s' := '%s'", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_SET_VARIABLE_ADD:
                fprintf(f, "let var '%s' := '%s' + '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_SUBTRACT:
                fprintf(f, "let var '%s' := '%s' - '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_MULTIPLY:
                fprintf(f, "let var '%s' := '%s' * '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_SET_VARIABLE_DIVIDE:
                fprintf(f, "let var '%s' := '%s' / '%s'", IntCode[i].name1,
                    IntCode[i].name2, IntCode[i].name3);
                break;

            case INT_INCREMENT_VARIABLE:
                fprintf(f, "increment '%s'", IntCode[i].name1);
                break;

            case INT_READ_ADC:
                fprintf(f, "read adc '%s'", IntCode[i].name1);
                break;

            case INT_SET_PWM:
                fprintf(f, "set pwm '%s' %s Hz", IntCode[i].name1,
                    IntCode[i].name2);
                break;

            case INT_EEPROM_BUSY_CHECK:
                fprintf(f, "set bit '%s' if EEPROM busy", IntCode[i].name1);
                break;

            case INT_EEPROM_READ:
                fprintf(f, "read EEPROM[%d,%d+1] into '%s'",
                    IntCode[i].literal, IntCode[i].literal, IntCode[i].name1);
                break;

            case INT_EEPROM_WRITE:
                fprintf(f, "write '%s' into EEPROM[%d,%d+1]",
                    IntCode[i].name1, IntCode[i].literal, IntCode[i].literal);
                break;

            case INT_UART_SEND:
                fprintf(f, "uart send from '%s', done? into '%s'",
                    IntCode[i].name1, IntCode[i].name2);
                break;

            case INT_UART_SEND_BUSY:
                fprintf(f, "'%s' = is uart busy to send ?",
                    IntCode[i].name1);
                break;

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

            default:
                ooops("IntCode[i].op=INT_%d",IntCode[i].op);
        }
        if((int_comment_level == 1)
        || (IntCode[i].op != INT_SIMULATE_NODE_STATE))
        fprintf(f, "\n");
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
    sprintf(dest, "$parThis_%04x", GenSymCountParThis);
    GenSymCountParThis++;
}
static void GenSymParOut(char *dest)
{
    sprintf(dest, "$parOut_%04x", GenSymCountParOut);
    GenSymCountParOut++;
}
static void GenSymOneShot(char *dest, char *name1, char *name2)
{
    if(int_comment_level == 1)
        sprintf(dest, "$oneShot_%04x", GenSymCountOneShot);
    else
    sprintf(dest, "$oneShot_%04x_%s_%s", GenSymCountOneShot, name1, name2);
    GenSymCountOneShot++;
}
static void GenSymOneShot(char *dest)
{
    GenSymOneShot(dest, "", "");
}
static void GenSymFormattedString(char *dest, char *name)
{
    sprintf(dest, "$fmtdStr_%04x_%s", GenSymCountFormattedString, name);
    GenSymCountFormattedString++;
}
static void GenSymFormattedString(char *dest)
{
    GenSymFormattedString(dest, "");
}
static void GenSymStepper(char *dest, char *name)
{
    sprintf(dest, "$stepper_%04x_%s", GenSymCountStepper, name);
    GenSymCountStepper++;
}

//-----------------------------------------------------------------------------
// Compile an instruction to the program.
//-----------------------------------------------------------------------------
static void Op(int op, char *name1, char *name2, char *name3, SWORD lit, SWORD lit2)
{
    IntCode[IntCodeLen].op = op;
    if(name1) strcpy(IntCode[IntCodeLen].name1, name1);
    if(name2) strcpy(IntCode[IntCodeLen].name2, name2);
    if(name3) strcpy(IntCode[IntCodeLen].name3, name3);
    IntCode[IntCodeLen].literal = lit;
    IntCode[IntCodeLen].literal2 = lit2;
    IntCode[IntCodeLen].rung = rungNow;
    IntCode[IntCodeLen].which = whichNow;
    IntCodeLen++;
    if(IntCodeLen >= MAX_INT_OPS) {
        Error(_("Internal limit exceeded (MAX_INT_OPS)"));
        CompileError();
    }
}
static void Op(int op, char *name1, char *name2, char *name3, SWORD lit)
{
    Op(op, name1, name2, name3, lit, 0);
}
static void Op(int op, char *name1, char *name2, SWORD lit)
{
    Op(op, name1, name2, NULL, lit, 0);
}
static void Op(int op, char *name1, SWORD lit, SWORD lit2)
{
    Op(op, name1, NULL, NULL, lit, lit2);
}
static void Op(int op, char *name1, SWORD lit)
{
    Op(op, name1, NULL, NULL, lit, 0);
}
static void Op(int op, char *name1, char *name2)
{
    Op(op, name1, name2, NULL, 0, 0);
}
static void Op(int op, char *name1)
{
    Op(op, name1, NULL, NULL, 0, 0);
}
static void Op(int op)
{
    Op(op, NULL, NULL, NULL, 0, 0);
}

//-----------------------------------------------------------------------------
// Compile the instruction that the simulator uses to keep track of which
// nodes are energized (so that it can display which branches of the circuit
// are energized onscreen). The MCU code generators ignore this, of course.
//-----------------------------------------------------------------------------
static void SimState(BOOL *b, char *name)
{
    IntCode[IntCodeLen].op = INT_SIMULATE_NODE_STATE;
    strcpy(IntCode[IntCodeLen].name1, name);
    IntCode[IntCodeLen].poweredAfter = b;
    IntCode[IntCodeLen].rung = rungNow;
    IntCode[IntCodeLen].which = whichNow;
    IntCodeLen++;
    if(IntCodeLen >= MAX_INT_OPS) {
        Error(_("Internal limit exceeded (MAX_INT_OPS)"));
        CompileError();
    }
}

//-----------------------------------------------------------------------------
// printf-like comment function
//-----------------------------------------------------------------------------
static void Comment(char *str, ...)
{
    if(strlen(str)>=MAX_NAME_LEN)
      str[MAX_NAME_LEN-1]='\0';
    va_list f;
    char buf[MAX_NAME_LEN];
    va_start(f, str);
    vsprintf(buf, str, f);
    Op(INT_COMMENT, buf);
}

//-----------------------------------------------------------------------------
// Calculate the period in scan units from the period in microseconds, and
// raise an error if the given period is unachievable.
//-----------------------------------------------------------------------------
static int TimerPeriod(ElemLeaf *l)
{
    int period = (l->d.timer.delay / Prog.cycleTime) - 1;

    if(period < 1)  {
        Error(_("Timer period too short (needs faster cycle time)."));
        CompileError();
    }
    if(period >= (1 << 15)) {
        Error(_("Timer period too long (max 32767 times cycle time); use a "
            "slower cycle time."));
        CompileError();
    }

    return period;
}

//-----------------------------------------------------------------------------
// Is an expression that could be either a variable name or a number a number?
//-----------------------------------------------------------------------------
BOOL IsNumber(char *str)
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

        if(*start_ptr == '0')
             if(toupper(start_ptr[1]) == 'B')
                 radix = 2;
             else if(toupper(start_ptr[1]) == 'O')
                 radix = 8;

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
// Report an error if a constant doesn't fit in 16 bits.
//-----------------------------------------------------------------------------
void CheckConstantInRange(int v)
{
    if(v < -32768 || v > 32767) {
        Error(_("Constant %d out of range: -32768 to 32767 inclusive."), v);
        CompileError();
    }
}

//-----------------------------------------------------------------------------
// Try to turn a string into a 16-bit constant, and raise an error if
// something bad happens when we do so (e.g. out of range).
//-----------------------------------------------------------------------------
SWORD CheckMakeNumber(char *str)
{
    int val;

    if(*str == '\'') {
        val = str[1];
    } else {
        val = atoi(str);
    }

    CheckConstantInRange(val);

    return (SWORD)val;
}

//-----------------------------------------------------------------------------
// Return an integer power of ten.
//-----------------------------------------------------------------------------
static int TenToThe(int x)
{
    int i;
    int r = 1;
    for(i = 0; i < x; i++) {
        r *= 10;
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
                if(!CheckStaySameElem(s->contents[i].which, s->contents[i].d.any))
                    return FALSE;
            }
            return TRUE;
        }
        case ELEM_PARALLEL_SUBCKT: {
            int i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(i = 0; i < p->count; i++) {
                if(!CheckStaySameElem(p->contents[i].which, p->contents[i].d.any))
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

            return CheckEndOfRungElem(s->contents[s->count-1].which,s->contents[s->count-1].d.any);
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            int i;
            for(i = 0; i < p->count; i++) {
                if(CheckEndOfRungElem(p->contents[i].which, p->contents[i].d.any))
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
                if(CheckCanChangeOutputElem(s->contents[i].which, s->contents[i].d.any))
                    return TRUE;
            }
            return FALSE;
        }
        case ELEM_PARALLEL_SUBCKT: {
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            int i;
            for(i = 0; i < p->count; i++) {
                if(CheckCanChangeOutputElem(p->contents[i].which, p->contents[i].d.any))
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
      Op(INT_SET_VARIABLE_TO_LITERAL, op1, (SDWORD)atoi(op2));
    else
      Op(INT_SET_VARIABLE_TO_VARIABLE, op1, op2);
}
//-----------------------------------------------------------------------------
// Compile code to evaluate the given bit of ladder logic. The rung input
// state is in stateInOut before calling and will be in stateInOut after
// calling.
//-----------------------------------------------------------------------------
static char *VarFromExpr(char *expr, char *tempName)
{
    if(IsNumber(expr)) {
        Op(INT_SET_VARIABLE_TO_LITERAL, tempName, CheckMakeNumber(expr));
        return tempName;
    } else {
        return expr;
    }
}
static void IntCodeFromCircuit(int which, void *any, char *stateInOut, int rung)
{
    ElemLeaf *l = (ElemLeaf *)any;
    whichNow = which;
    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            int i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)any;

            Comment("start series [");
            for(i = 0; i < s->count; i++) {
                IntCodeFromCircuit(s->contents[i].which, s->contents[i].d.any,
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

            BOOL ExistEnd;
            BOOL CanChange;
            #ifdef parThis_parOut_optimize
            ExistEnd = FALSE; //FALSE indicates that it is NEED to calculate the parOut
            for(i = 0; i < p->count; i++) {
              if(CheckEndOfRungElem(p->contents[i].which, p->contents[i].d.any)){
                ExistEnd = TRUE; // TRUE indicates that it is NOT NEED to calculate the parOut
                break;
              }
            }
            CanChange = FALSE; // FALSE indicates that it is NOT NEED to calculate the parThis
            for(i = 0; i < p->count; i++) {
              if(!CheckStaySameElem(p->contents[i].which, p->contents[i].d.any)){
                CanChange = TRUE; // TRUE indicates that it is NEED to calculate the parThis
                break;
              }
            }
            #else
            // Return to default algorithm
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
              #ifdef parThis_parOut_optimize
              if(CheckStaySameElem(p->contents[i].which, p->contents[i].d.any))
                  IntCodeFromCircuit(p->contents[i].which, p->contents[i].d.any, stateInOut, rung);
              else
              #endif
              {
                Op(INT_COPY_BIT_TO_BIT, parThis, stateInOut);

                IntCodeFromCircuit(p->contents[i].which, p->contents[i].d.any,
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
            if(l->d.contacts.negated) {
                Op(INT_IF_BIT_SET, l->d.contacts.name);
            } else {
                Op(INT_IF_BIT_CLEAR, l->d.contacts.name);
            }
            Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_COIL: {
            if(l->d.coil.negated) {
                Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_CLEAR_BIT, l->d.contacts.name);
                Op(INT_ELSE);
                Op(INT_SET_BIT, l->d.contacts.name);
                Op(INT_END_IF);
            } else if(l->d.coil.setOnly) {
                Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_SET_BIT, l->d.contacts.name);
                Op(INT_END_IF);
            } else if(l->d.coil.resetOnly) {
                Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_CLEAR_BIT, l->d.contacts.name);
                Op(INT_END_IF);
            } else {
                Op(INT_COPY_BIT_TO_BIT, l->d.contacts.name, stateInOut);
            }
            break;
        }
        case ELEM_RTO: {
            int period = TimerPeriod(l);

            Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);

            Op(INT_IF_BIT_SET, stateInOut);
            Op(INT_INCREMENT_VARIABLE, l->d.timer.name);
            Op(INT_END_IF);
            Op(INT_CLEAR_BIT, stateInOut);

            Op(INT_ELSE);

            Op(INT_SET_BIT, stateInOut);

            Op(INT_END_IF);

            break;
        }
        case ELEM_RES:
            Op(INT_IF_BIT_SET, stateInOut);
            Op(INT_SET_VARIABLE_TO_LITERAL, l->d.reset.name);
            Op(INT_END_IF);
            break;

        case ELEM_TON: {
            int period = TimerPeriod(l);

            Op(INT_IF_BIT_SET, stateInOut);
              Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);
                Op(INT_INCREMENT_VARIABLE, l->d.timer.name);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name);
            Op(INT_END_IF);

            break;
        }
        case ELEM_TOF: {
            int period = TimerPeriod(l);

            // All variables start at zero by default, so by default the
            // TOF timer would start out with its output forced HIGH, until
            // it finishes counting up. This does not seem to be what
            // people expect, so add a special case to fix that up.
            char antiGlitchName[MAX_NAME_LEN];
            sprintf(antiGlitchName, "$%s_antiglitch", l->d.timer.name);
            Op(INT_IF_BIT_CLEAR, antiGlitchName);
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name, period);
            Op(INT_END_IF);
            Op(INT_SET_BIT, antiGlitchName);

            Op(INT_IF_BIT_CLEAR, stateInOut);

            Op(INT_IF_VARIABLE_LES_LITERAL, l->d.timer.name, period);

            Op(INT_INCREMENT_VARIABLE, l->d.timer.name);
            Op(INT_SET_BIT, stateInOut);
            Op(INT_END_IF);

            Op(INT_ELSE);

            Op(INT_SET_VARIABLE_TO_LITERAL, l->d.timer.name);

            Op(INT_END_IF);
            break;
        }
        case ELEM_CTU: {
            CheckConstantInRange(l->d.counter.max);
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName);

            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_IF_BIT_CLEAR, storeName);
                    Op(INT_INCREMENT_VARIABLE, l->d.counter.name);
                Op(INT_END_IF);
            Op(INT_END_IF);
            Op(INT_COPY_BIT_TO_BIT, storeName, stateInOut);

            Op(INT_IF_VARIABLE_LES_LITERAL, l->d.counter.name,
                l->d.counter.max);
                Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_ELSE);
                Op(INT_SET_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_CTD: {
            CheckConstantInRange(l->d.counter.max);
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName);

            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_IF_BIT_CLEAR, storeName);
                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", 1);
                    Op(INT_SET_VARIABLE_SUBTRACT, l->d.counter.name,
                        l->d.counter.name, "$scratch", 0);
                Op(INT_END_IF);
            Op(INT_END_IF);
            Op(INT_COPY_BIT_TO_BIT, storeName, stateInOut);

            Op(INT_IF_VARIABLE_LES_LITERAL, l->d.counter.name,
                l->d.counter.max);
                Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_ELSE);
                Op(INT_SET_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_CTC: {
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName);

            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_CLEAR_BIT, stateInOut);
                Op(INT_IF_BIT_CLEAR, storeName);
                    Op(INT_SET_BIT, storeName); // This line1
                    Op(INT_INCREMENT_VARIABLE, l->d.counter.name);
                    Op(INT_IF_VARIABLE_LES_LITERAL, l->d.counter.name,
                        l->d.counter.max+1);
                    Op(INT_ELSE);
                        Op(INT_SET_VARIABLE_TO_LITERAL, l->d.counter.name,
                            (SWORD)0);
                        Op(INT_SET_BIT, stateInOut); // overload impulse
                    Op(INT_END_IF);
                Op(INT_END_IF);
            Op(INT_ELSE);
              Op(INT_CLEAR_BIT, storeName); // This line2
            Op(INT_END_IF);
//          Op(INT_COPY_BIT_TO_BIT, storeName, stateInOut); // This line3 equivalently line1 + line2
            break;
        }
        // Special Function
        case ELEM_RSFR:
            if(IsNumber(l->d.move.dest)) {
                Error(_("Read SFR instruction: '%s' not a valid destination."),
                    l->d.move.dest);
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.src)) {
                Op(INT_READ_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
            } else {
                Op(INT_READ_SFR_VARIABLE, l->d.move.src, l->d.move.dest,0);
            }
            Op(INT_END_IF);
            break;
        case ELEM_WSFR:
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.dest)) {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_WRITE_SFR_LITERAL_L,NULL,NULL,NULL, CheckMakeNumber(l->d.move.src), CheckMakeNumber(l->d.move.dest));
                } else {
                    Op(INT_WRITE_SFR_VARIABLE_L,l->d.move.src, CheckMakeNumber(l->d.move.dest));
                }
            }
            else {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_WRITE_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
                } else {
                    Op(INT_WRITE_SFR_VARIABLE, l->d.move.src, l->d.move.dest,0);
                }
            }
            Op(INT_END_IF);
            break;
        case ELEM_SSFR:
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
                    Op(INT_SET_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
                } else {
                    Op(INT_SET_SFR_VARIABLE, l->d.move.src, l->d.move.dest,0);
                }
            }
            Op(INT_END_IF);
            break;
        case ELEM_CSFR:
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
                    Op(INT_CLEAR_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
                } else {
                    Op(INT_CLEAR_SFR_VARIABLE, l->d.move.src, l->d.move.dest,0);
                }
            }
            Op(INT_END_IF);
            break;
        case ELEM_TSFR: {
             if(IsNumber(l->d.move.dest)) {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_TEST_SFR_LITERAL_L,NULL,NULL,NULL, CheckMakeNumber(l->d.move.src), CheckMakeNumber(l->d.move.dest));
                    Op(INT_ELSE);
                } else {
                    Op(INT_TEST_SFR_VARIABLE_L,l->d.move.src, CheckMakeNumber(l->d.move.dest));
                    Op(INT_ELSE);
                }
            }
            else {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_TEST_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
                    Op(INT_ELSE);
                } else {
                    Op(INT_TEST_SFR_VARIABLE, l->d.move.src, l->d.move.dest,0);
                    Op(INT_ELSE);
                }
            }
            Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_T_C_SFR: {
             if(IsNumber(l->d.move.dest)) {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_TEST_C_SFR_LITERAL_L,NULL,NULL,NULL, CheckMakeNumber(l->d.move.src), CheckMakeNumber(l->d.move.dest));
                    Op(INT_ELSE);
                } else {
                    Op(INT_TEST_C_SFR_VARIABLE_L,l->d.move.src, CheckMakeNumber(l->d.move.dest));
                    Op(INT_ELSE);
                }
            }
            else {
                if(IsNumber(l->d.move.src)) {
                    Op(INT_TEST_C_SFR_LITERAL, l->d.move.dest, CheckMakeNumber(l->d.move.src));
                    Op(INT_ELSE);
                } else {
                    Op(INT_TEST_C_SFR_VARIABLE, l->d.move.src, l->d.move.dest,0);
                    Op(INT_ELSE);
                }
            }
            Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        // Special function
        case ELEM_GRT:
        case ELEM_GEQ:
        case ELEM_LES:
        case ELEM_LEQ:
        case ELEM_NEQ:
        case ELEM_EQU: {
            char *op1 = VarFromExpr(l->d.cmp.op1, "$scratch");
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

            Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            break;
        }
        case ELEM_ONE_SHOT_RISING: {
            /*
                     ___
            INPUT __/
                     _
            OUTPUT__/ \_

                    | |
                     Tcycle
            */
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName);

            Op(INT_COPY_BIT_TO_BIT, "$scratch", stateInOut);
            Op(INT_IF_BIT_SET, storeName);
              Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);
            Op(INT_COPY_BIT_TO_BIT, storeName, "$scratch");
            break;
        }
        case ELEM_ONE_SHOT_FALLING: {
            /*
                  __
            INPUT   \___
                     _
            OUTPUT__/ \_

                    | |
                     Tcycle
            */
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName);

            Op(INT_COPY_BIT_TO_BIT, "$scratch", stateInOut);

            Op(INT_IF_BIT_CLEAR, stateInOut);
            Op(INT_IF_BIT_SET, storeName);
            Op(INT_SET_BIT, stateInOut);
            Op(INT_END_IF);
            Op(INT_ELSE);
            Op(INT_CLEAR_BIT, stateInOut);
            Op(INT_END_IF);

            Op(INT_COPY_BIT_TO_BIT, storeName, "$scratch");
            break;
        }
        case ELEM_MOVE: {
            if(IsNumber(l->d.move.dest)) {
                Error(_("Move instruction: '%s' not a valid destination."),
                    l->d.move.dest);
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);
            if(IsNumber(l->d.move.src)) {
                CheckVarInRange(l->d.move.dest, CheckMakeNumber(l->d.move.src));
                Op(INT_SET_VARIABLE_TO_LITERAL, l->d.move.dest,
                    CheckMakeNumber(l->d.move.src));
            } else {
                Op(INT_SET_VARIABLE_TO_VARIABLE, l->d.move.dest, l->d.move.src,
                    0);
            }
            Op(INT_END_IF);
            break;
        }

        case ELEM_STRING: {
            Op(INT_IF_BIT_SET, stateInOut);
            Op(INT_WRITE_STRING, l->d.fmtdStr.dest, l->d.fmtdStr.var, l->d.fmtdStr.string, 0);
            Op(INT_END_IF);
            break;
        }

        // These ELEM's are highly processor-dependent; the int code op does
        // most of the highly specific work
        {
            case ELEM_READ_ADC:
                Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_READ_ADC, l->d.readAdc.name);
                Op(INT_END_IF);
                break;

            case ELEM_SET_PWM: {
                Op(INT_IF_BIT_SET, stateInOut);
                // ugh; need a >16 bit literal though, could be >64 kHz
				Op(INT_SET_PWM, l->d.readAdc.name, l->d.setPwm.targetFreq);
                Op(INT_END_IF);
                break;
            }
            case ELEM_PERSIST: {
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
                Op(INT_END_IF);

                // While running, continuously compare the EEPROM copy of
                // the variable against the RAM one; if they are different,
                // write the RAM one to EEPROM.
                Op(INT_CLEAR_BIT, "$scratch");
                Op(INT_EEPROM_BUSY_CHECK, "$scratch");
                Op(INT_IF_BIT_CLEAR, "$scratch");
                    Op(INT_EEPROM_READ, "$scratch", EepromAddrFree);
                    Op(INT_IF_VARIABLE_EQUALS_VARIABLE, "$scratch",
                        l->d.persist.var);
                    Op(INT_ELSE);
                        Op(INT_EEPROM_WRITE, l->d.persist.var, EepromAddrFree);
                    Op(INT_END_IF);
                Op(INT_END_IF);

                Op(INT_END_IF);

                EepromAddrFree += 2;
                break;
            }
            case ELEM_UART_SEND:
                // Why in this place do not controlled stateInOut, as in the ELEM_UART_RECV ?
                // 1. It's need in Simulation Mode.
                // 2. It's need for Arduino.
                Op(INT_IF_BIT_SET, stateInOut);
                  Op(INT_UART_SEND, l->d.uart.name, stateInOut);
                Op(INT_END_IF);
                break;

            case ELEM_UART_RECV:
                Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_UART_RECV, l->d.uart.name, stateInOut);
                Op(INT_END_IF);
                break;
        }

        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV: {
            if(IsNumber(l->d.math.dest)) {
                Error(_("Math instruction: '%s' not a valid destination."),
                    l->d.math.dest);
                CompileError();
            }
            Op(INT_IF_BIT_SET, stateInOut);

            char *op1 = VarFromExpr(l->d.math.op1, "$scratch");
            char *op2 = VarFromExpr(l->d.math.op2, "$scratch2");

            int intOp;
            if(which == ELEM_ADD) {
                intOp = INT_SET_VARIABLE_ADD;
            } else if(which == ELEM_SUB) {
                intOp = INT_SET_VARIABLE_SUBTRACT;
            } else if(which == ELEM_MUL) {
                intOp = INT_SET_VARIABLE_MULTIPLY;
            } else if(which == ELEM_DIV) {
                intOp = INT_SET_VARIABLE_DIVIDE;
            } else oops();

            Op(intOp, l->d.math.dest, op1, op2, 0);

            Op(INT_END_IF);
            break;
        }
        case ELEM_MASTER_RELAY:
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
            char storeName[MAX_NAME_LEN];
            GenSymOneShot(storeName);
            Op(INT_IF_BIT_SET, stateInOut);
                Op(INT_IF_BIT_CLEAR, storeName);
                    int i;
                    for(i = (l->d.shiftRegister.stages-2); i >= 0; i--) {
                        char dest[MAX_NAME_LEN+10], src[MAX_NAME_LEN+10];
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
            break;
        }
        case ELEM_PIECEWISE_LINEAR: {
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
                Op(INT_SET_VARIABLE_SUBTRACT, "$scratch", t->index,
                    "$scratch", 0);
                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch2", thisDx);
                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch3", thisDy);
                Op(INT_SET_VARIABLE_MULTIPLY, t->dest, "$scratch", "$scratch3",
                    0);
                Op(INT_SET_VARIABLE_DIVIDE, t->dest, t->dest, "$scratch2", 0);

                Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch",
                    t->vals[(i-1)*2 + 1]);
                Op(INT_SET_VARIABLE_ADD, t->dest, t->dest, "$scratch", 0);
                Op(INT_END_IF);
            }
            Op(INT_END_IF);
            break;
        }
        case ELEM_FORMATTED_STRING: {
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
            int steps;

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
                    Op(INT_SET_VARIABLE_TO_LITERAL, seq, (SWORD)0);
                    Op(INT_SET_BIT, doSend);
                Op(INT_END_IF);
            Op(INT_ELSE); //v2
                Op(INT_CLEAR_BIT, oneShot); //v2
            Op(INT_END_IF);
            //Op(INT_COPY_BIT_TO_BIT, oneShot, stateInOut); //v1

            // Everything that involves seqScratch is a terrible hack to
            // avoid an if statement with a big body, which is the risk
            // factor for blowing up on PIC16 page boundaries.

            char *seqScratch = "$seqScratch";

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
                Op(INT_CLEAR_BIT, "$scratch");
                Op(INT_UART_SEND, "$scratch", "$scratch");
                Op(INT_IF_BIT_SET, "$scratch");
                    Op(INT_SET_VARIABLE_TO_LITERAL, seqScratch, -1);
                Op(INT_END_IF);
                */
                Op(INT_UART_SEND_BUSY, "$scratch");
                Op(INT_IF_BIT_SET, "$scratch");
                    Op(INT_SET_VARIABLE_TO_LITERAL, seqScratch, -1);
                Op(INT_END_IF);
            Op(INT_END_IF);

            // So we transmit this cycle, so check out which character.
            int i;
            int digit = 0;
            for(i = 0; i < steps; i++) {
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
                        "$scratch", 0);
                    Op(INT_SET_VARIABLE_MULTIPLY, "$scratch", "$scratch",
                        "$charToUart", 0);
                    Op(INT_SET_VARIABLE_SUBTRACT, convertState,
                        convertState, "$scratch", 0);
                    Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch", '0');
                    Op(INT_SET_VARIABLE_ADD, "$charToUart", "$charToUart",
                        "$scratch", 0);

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
                        Op(INT_SET_VARIABLE_TO_VARIABLE, convertState, var);
                        Op(INT_SET_VARIABLE_TO_LITERAL, "$charToUart", ' ');
                        Op(INT_IF_VARIABLE_LES_LITERAL, var, (SWORD)0);
                            Op(INT_SET_VARIABLE_TO_LITERAL, "$charToUart", '-');
                            Op(INT_SET_VARIABLE_TO_LITERAL, "$scratch",
                                (SWORD)0);
                            Op(INT_SET_VARIABLE_SUBTRACT, convertState,
                                "$scratch", var, 0);
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

            Op(INT_IF_VARIABLE_LES_LITERAL, seqScratch, (SWORD)0);
            Op(INT_ELSE);
              Op(INT_IF_BIT_SET, doSend);
                Op(INT_SET_BIT, "$scratch");
                Op(INT_UART_SEND, "$charToUart", "$scratch");
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
            Op(INT_CLEAR_BIT, stateInOut);
            break;

        case ELEM_SHORT:
            // goes straight through
            break;

        case ELEM_PLACEHOLDER:
            Error(
              _("Empty row; delete it or add instructions before compiling."));
            CompileError();
            break;

        default:
            ooops("ELEM_0x%X", which);
            break;
    }

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
                CheckMasterCircuit(s->contents[i].which,
                    s->contents[i].d.any);
            }
            break;
        }

        case ELEM_PARALLEL_SUBCKT: {
            int i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(i = 0; i < p->count; i++) {
                CheckMasterCircuit(p->contents[i].which,
                    p->contents[i].d.any);
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
    //ProgramChangedNotSaved = TRUE;

    GenSymCountParThis = 0;
    GenSymCountParOut = 0;
    GenSymCountOneShot = 0;
    GenSymCountFormattedString = 0;
    GenSymCountStepper = 0;

    // The EEPROM addresses for the `Make Persistent' op are assigned at
    // int code generation time.
    EepromAddrFree = 0;

    rungNow = -100;//INT_MAX;

    WipeIntMemory   ();

    AllocStart();

    CheckVariableNames();

    rungNow++;
    BOOL ExistMasterRelay = CheckMasterRelay();
    if(int_comment_level == 1) {
        ExistMasterRelay = TRUE; // Comment this for optimisation
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
        Prog.OpsInRung[rung] = 0;
        Prog.HexInRung[rung] = 0;
        #ifdef NEW_FEATURE
        Op(INT_AllocFwdAddr, NULL, NULL, NULL, (SDWORD)rung);
        #endif
    }

    for(rung = 0; rung < Prog.numRungs; rung++) {
        rungNow = rung;
        if(int_comment_level != 1) {
            Comment("");
            Comment("======= START RUNG %d =======", rung+1);
        }

        if(Prog.rungs[rung]->count == 1 &&
            Prog.rungs[rung]->contents[0].which == ELEM_COMMENT)
        {
            // nothing to do for this one
            // Yes, I do! Push comment into interpretable OP code for C and PASCAL.
            l=(ElemLeaf *) Prog.rungs[rung]->contents[0].d.any;
            AnsiToOem(l->d.comment.str,s1);
            s2 = s1;
            for(; *s2; s2++) {
                if(*s2 == '\r'){
                    *s2 = '\0';
                    s2++;
                    if(*s2 == '\n')
                        s2++;
                    break;
                }
            }
            if(int_comment_level>=2) {
                if(s1) Comment(s1);
                if(s2) Comment(s2);
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
