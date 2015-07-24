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
// Routines to simulate the logic interactively, for testing purposes. We can
// simulate in real time, triggering off a Windows timer, or we can 
// single-cycle it. The GUI acts differently in simulation mode, to show the
// status of all the signals graphically, show how much time is left on the
// timers, etc.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "ldmicro.h"
#include "intcode.h"
#include "freeze.h"

static struct {
    char name[MAX_NAME_LEN];
    BOOL powered;
} SingleBitItems[MAX_IO];
static int SingleBitItemsCount;

static struct {
    char    name[MAX_NAME_LEN];
    SWORD   val;
    DWORD   usedFlags;
} Variables[MAX_IO];
static int VariablesCount;

static struct {
    char    name[MAX_NAME_LEN];
    SWORD   val;
} AdcShadows[MAX_IO];
static int AdcShadowsCount;

#define VAR_FLAG_TON  0x00000001
#define VAR_FLAG_TOF  0x00000002
#define VAR_FLAG_RTO  0x00000004
#define VAR_FLAG_CTU  0x00000008
#define VAR_FLAG_CTD  0x00000010
#define VAR_FLAG_CTC  0x00000020
#define VAR_FLAG_RES  0x00000040
#define VAR_FLAG_ANY  0x00000080

#define VAR_FLAG_OTHERWISE_FORGOTTEN  0x80000000


// Schematic-drawing code needs to know whether we're in simulation mode or
// note, as that changes how everything is drawn; also UI code, to disable
// editing during simulation.
BOOL InSimulationMode;

// Don't want to redraw the screen unless necessary; track whether a coil
// changed state or a timer output switched to see if anything could have
// changed (not just coil, as we show the intermediate steps too).
static BOOL NeedRedraw;
// Have to let the effects of a coil change in cycle k appear in cycle k+1,
// or set by the UI code to indicate that user manually changed an Xfoo
// input.
BOOL SimulateRedrawAfterNextCycle;

// Don't want to set a timer every 100 us to simulate a 100 us cycle
// time...but we can cycle multiple times per timer interrupt and it will
// be almost as good, as long as everything runs fast.
static int CyclesPerTimerTick;

// Program counter as we evaluate the intermediate code.
static int IntPc;

// A window to allow simulation with the UART stuff (insert keystrokes into
// the program, view the output, like a terminal window).
static HWND UartSimulationWindow;
static HWND UartSimulationTextControl;
static LONG_PTR PrevTextProc;

static int QueuedUartCharacter = -1;
static int SimulateUartTxCountdown = 0;

static void AppendToUartSimulationTextControl(BYTE b);

static void SimulateIntCode(void);
static char *MarkUsedVariable(char *name, DWORD flag);

//-----------------------------------------------------------------------------
// Query the state of a single-bit element (relay, digital in, digital out).
// Looks in the SingleBitItems list; if an item is not present then it is
// FALSE by default.
//-----------------------------------------------------------------------------
static BOOL SingleBitOn(char *name)
{
    int i;
    for(i = 0; i < SingleBitItemsCount; i++) {
        if(strcmp(SingleBitItems[i].name, name)==0) {
            return SingleBitItems[i].powered;
        }
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
// Set the state of a single-bit item. Adds it to the list if it is not there
// already.
//-----------------------------------------------------------------------------
static void SetSingleBit(char *name, BOOL state)
{
    int i;
    for(i = 0; i < SingleBitItemsCount; i++) {
        if(strcmp(SingleBitItems[i].name, name)==0) {
            SingleBitItems[i].powered = state;
            return;
        }
    }
    if(i < MAX_IO) {
        strcpy(SingleBitItems[i].name, name);
        SingleBitItems[i].powered = state;
        SingleBitItemsCount++;
    }
}

//-----------------------------------------------------------------------------
// Count a timer up (i.e. increment its associated count by 1). Must already
// exist in the table.
//-----------------------------------------------------------------------------
static void IncrementVariable(char *name)
{
    int i;
    for(i = 0; i < VariablesCount; i++) {
        if(strcmp(Variables[i].name, name)==0) {
            (Variables[i].val)++;
            return;
        }
    }
    oops();
}

//-----------------------------------------------------------------------------
// Set a variable to a value.
//-----------------------------------------------------------------------------
static void SetSimulationVariable(char *name, SWORD val)
{
    int i;
    for(i = 0; i < VariablesCount; i++) {
        if(strcmp(Variables[i].name, name)==0) {
            Variables[i].val = val;
            return;
        }
    }
    MarkUsedVariable(name, VAR_FLAG_OTHERWISE_FORGOTTEN);
    SetSimulationVariable(name, val);
}

//-----------------------------------------------------------------------------
// Read a variable's value.
//-----------------------------------------------------------------------------
SWORD GetSimulationVariable(char *name)
{
    int i;
    for(i = 0; i < VariablesCount; i++) {
        if(strcmp(Variables[i].name, name)==0) {
            return Variables[i].val;
        }
    }
    MarkUsedVariable(name, VAR_FLAG_OTHERWISE_FORGOTTEN);
    return GetSimulationVariable(name);
}

//-----------------------------------------------------------------------------
// Set the shadow copy of a variable associated with a READ ADC operation. This
// will get committed to the real copy when the rung-in condition to the
// READ ADC is true.
//-----------------------------------------------------------------------------
void SetAdcShadow(char *name, SWORD val)
{
    int i;
    for(i = 0; i < AdcShadowsCount; i++) {
        if(strcmp(AdcShadows[i].name, name)==0) {
            AdcShadows[i].val = val;
            return;
        }
    }
    strcpy(AdcShadows[i].name, name);
    AdcShadows[i].val = val;
    AdcShadowsCount++;
}

//-----------------------------------------------------------------------------
// Return the shadow value of a variable associated with a READ ADC. This is
// what gets copied into the real variable when an ADC read is simulated.
//-----------------------------------------------------------------------------
SWORD GetAdcShadow(char *name)
{
    int i;
    for(i = 0; i < AdcShadowsCount; i++) {
        if(strcmp(AdcShadows[i].name, name)==0) {
            return AdcShadows[i].val;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Mark how a variable is used; a series of flags that we can OR together,
// then we can check to make sure that only valid combinations have been used
// (e.g. just a TON, an RTO with its reset, etc.). Returns NULL for success,
// else an error string.
//-----------------------------------------------------------------------------
static char *MarkUsedVariable(char *name, DWORD flag)
{
    int i;
    for(i = 0; i < VariablesCount; i++) {
        if(strcmp(Variables[i].name, name)==0) {
            break;
        }
    }
    if(i >= MAX_IO) return "";

    if(i == VariablesCount) {
        strcpy(Variables[i].name, name);
        Variables[i].usedFlags = 0;
        Variables[i].val = 0;
        VariablesCount++;
    }

    switch(flag) {
        case VAR_FLAG_TOF:
            if(Variables[i].usedFlags != 0) 
                return _("TOF: variable cannot be used elsewhere");
            break;

        case VAR_FLAG_TON:
            if(Variables[i].usedFlags != 0)
                return _("TON: variable cannot be used elsewhere");
            break;
        
        case VAR_FLAG_RTO:
            if(Variables[i].usedFlags & ~VAR_FLAG_RES)
                return _("RTO: variable can only be used for RES elsewhere");
            break;

        case VAR_FLAG_CTU:
        case VAR_FLAG_CTD:
        case VAR_FLAG_CTC:
        case VAR_FLAG_RES:
        case VAR_FLAG_ANY:
            break;

        case VAR_FLAG_OTHERWISE_FORGOTTEN:
            if(name[0] != '$') {
                Error(_("Variable '%s' not assigned to, e.g. with a "
                    "MOV statement, an ADD statement, etc.\r\n\r\n"
                    "This is probably a programming error; now it "
                    "will always be zero."), name);
            }
            break;

        default:
            oops();
    }

    Variables[i].usedFlags |= flag;
    return NULL;
}

//-----------------------------------------------------------------------------
// Check for duplicate uses of a single variable. For example, there should
// not be two TONs with the same name. On the other hand, it would be okay
// to have an RTO with the same name as its reset; in fact, verify that
// there must be a reset for each RTO.
//-----------------------------------------------------------------------------
static void MarkWithCheck(char *name, int flag)
{
    char *s = MarkUsedVariable(name, flag);
    if(s) {
        Error(_("Variable for '%s' incorrectly assigned: %s."), name, s);
    }
}
static void CheckVariableNamesCircuit(int which, void *elem)
{
    ElemLeaf *l = (ElemLeaf *)elem;
    char *name = NULL;
    DWORD flag;

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            int i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(i = 0; i < s->count; i++) {
                CheckVariableNamesCircuit(s->contents[i].which,
                    s->contents[i].d.any);
            }
            break;
        }

        case ELEM_PARALLEL_SUBCKT: {
            int i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(i = 0; i < p->count; i++) {
                CheckVariableNamesCircuit(p->contents[i].which,
                    p->contents[i].d.any);
            }
            break;
        }
        
        case ELEM_RTO:
        case ELEM_TOF:
        case ELEM_TON:
            if(which == ELEM_RTO)
                flag = VAR_FLAG_RTO;
            else if(which == ELEM_TOF)
                flag = VAR_FLAG_TOF;
            else if(which == ELEM_TON)
                flag = VAR_FLAG_TON;
            else oops();

            MarkWithCheck(l->d.timer.name, flag);

            break;

        case ELEM_CTU:
        case ELEM_CTD:
        case ELEM_CTC:
            if(which == ELEM_CTU)
                flag = VAR_FLAG_CTU;
            else if(which == ELEM_CTD)
                flag = VAR_FLAG_CTD;
            else if(which == ELEM_CTC)
                flag = VAR_FLAG_CTC;
            else oops();

            MarkWithCheck(l->d.counter.name, flag);

            break;

        case ELEM_RES:
            MarkWithCheck(l->d.reset.name, VAR_FLAG_RES);
            break;

        case ELEM_MOVE:
            MarkWithCheck(l->d.move.dest, VAR_FLAG_ANY);
            break;

        case ELEM_LOOK_UP_TABLE:
            MarkWithCheck(l->d.lookUpTable.dest, VAR_FLAG_ANY);
            break;

        case ELEM_PIECEWISE_LINEAR:
            MarkWithCheck(l->d.piecewiseLinear.dest, VAR_FLAG_ANY);
            break;

        case ELEM_READ_ADC:
            MarkWithCheck(l->d.readAdc.name, VAR_FLAG_ANY);
            break;

        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV:
            MarkWithCheck(l->d.math.dest, VAR_FLAG_ANY);
            break;

        case ELEM_UART_RECV:
            MarkWithCheck(l->d.uart.name, VAR_FLAG_ANY);
            break;

        case ELEM_SHIFT_REGISTER: {
            int i;
            for(i = 1; i < l->d.shiftRegister.stages; i++) {
                char str[MAX_NAME_LEN+10];
                sprintf(str, "%s%d", l->d.shiftRegister.name, i);
                MarkWithCheck(str, VAR_FLAG_ANY);
            }
            break;
        }

        case ELEM_PERSIST:
        case ELEM_FORMATTED_STRING:
        case ELEM_SET_PWM:
        case ELEM_MASTER_RELAY:
        case ELEM_UART_SEND:
        case ELEM_PLACEHOLDER:
        case ELEM_COMMENT:
        case ELEM_OPEN:
        case ELEM_SHORT:
        case ELEM_COIL:
        case ELEM_CONTACTS:
        case ELEM_ONE_SHOT_RISING:
        case ELEM_ONE_SHOT_FALLING:
        case ELEM_EQU:
        case ELEM_NEQ:
        case ELEM_GRT:
        case ELEM_GEQ:
        case ELEM_LES:
        case ELEM_LEQ:
            break;

        default:
            oops();
    }
}
static void CheckVariableNames(void)
{
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        CheckVariableNamesCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    }
}

//-----------------------------------------------------------------------------
// The IF condition is true. Execute the body, up until the ELSE or the
// END IF, and then skip the ELSE if it is present. Called with PC on the
// IF, returns with PC on the END IF.
//-----------------------------------------------------------------------------
static void IfConditionTrue(void)
{
    IntPc++;
    // now PC is on the first statement of the IF body
    SimulateIntCode();
    // now PC is on the ELSE or the END IF
    if(IntCode[IntPc].op == INT_ELSE) {
        int nesting = 1;
        for(; ; IntPc++) {
            if(IntPc >= IntCodeLen) oops();

            if(IntCode[IntPc].op == INT_END_IF) {
                nesting--;
            } else if(INT_IF_GROUP(IntCode[IntPc].op)) {
                nesting++;
            }
            if(nesting == 0) break;
        }
    } else if(IntCode[IntPc].op == INT_END_IF) {
        return;
    } else {
        oops();
    }
}

//-----------------------------------------------------------------------------
// The IF condition is false. Skip the body, up until the ELSE or the END
// IF, and then execute the ELSE if it is present. Called with PC on the IF,
// returns with PC on the END IF.
//-----------------------------------------------------------------------------
static void IfConditionFalse(void)
{
    int nesting = 0;
    for(; ; IntPc++) {
        if(IntPc >= IntCodeLen) oops();

        if(IntCode[IntPc].op == INT_END_IF) {
            nesting--;
        } else if(INT_IF_GROUP(IntCode[IntPc].op)) {
            nesting++;
        } else if(IntCode[IntPc].op == INT_ELSE && nesting == 1) {
            break;
        }
        if(nesting == 0) break;
    }

    // now PC is on the ELSE or the END IF
    if(IntCode[IntPc].op == INT_ELSE) {
        IntPc++;
        SimulateIntCode();
    } else if(IntCode[IntPc].op == INT_END_IF) {
        return;
    } else {
        oops();
    }
}

//-----------------------------------------------------------------------------
// Evaluate a circuit, calling ourselves recursively to evaluate if/else
// constructs. Updates the on/off state of all the leaf elements in our
// internal tables. Returns when it reaches an end if or an else construct,
// or at the end of the program.
//-----------------------------------------------------------------------------
static void SimulateIntCode(void)
{
    for(; IntPc < IntCodeLen; IntPc++) {
        IntOp *a = &IntCode[IntPc];
        switch(a->op) {
            case INT_SIMULATE_NODE_STATE:
                if(*(a->poweredAfter) != SingleBitOn(a->name1))
                    NeedRedraw = TRUE;
                *(a->poweredAfter) = SingleBitOn(a->name1);
                break;

            case INT_SET_BIT:
                SetSingleBit(a->name1, TRUE);
                break;

            case INT_CLEAR_BIT:
                SetSingleBit(a->name1, FALSE);
                break;

            case INT_COPY_BIT_TO_BIT:
                SetSingleBit(a->name1, SingleBitOn(a->name2));
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                if(GetSimulationVariable(a->name1) !=
                    a->literal && a->name1[0] != '$')
                {
                    NeedRedraw = TRUE;
                }
                SetSimulationVariable(a->name1, a->literal);
                break;

            case INT_SET_VARIABLE_TO_VARIABLE:
                if(GetSimulationVariable(a->name1) != 
                    GetSimulationVariable(a->name2))
                {
                    NeedRedraw = TRUE;
                }
                SetSimulationVariable(a->name1,
                    GetSimulationVariable(a->name2));
                break;

            case INT_INCREMENT_VARIABLE:
                IncrementVariable(a->name1);
                break;

            {
                SWORD v;
                case INT_SET_VARIABLE_ADD:
                    v = GetSimulationVariable(a->name2) +
                        GetSimulationVariable(a->name3);
                    goto math;
                case INT_SET_VARIABLE_SUBTRACT:
                    v = GetSimulationVariable(a->name2) -
                        GetSimulationVariable(a->name3);
                    goto math;
                case INT_SET_VARIABLE_MULTIPLY:
                    v = GetSimulationVariable(a->name2) *
                        GetSimulationVariable(a->name3);
                    goto math;
                case INT_SET_VARIABLE_DIVIDE:
                    if(GetSimulationVariable(a->name3) != 0) {
                        v = GetSimulationVariable(a->name2) /
                            GetSimulationVariable(a->name3);
                    } else {
                        v = 0;
                        Error(_("Division by zero; halting simulation"));
                        StopSimulation();
                    }
                    goto math;
math:
                    if(GetSimulationVariable(a->name1) != v) {
                        NeedRedraw = TRUE;
                        SetSimulationVariable(a->name1, v);
                    }
                    break;
            }

#define IF_BODY \
    { \
        IfConditionTrue(); \
    } else { \
        IfConditionFalse(); \
    }
            case INT_IF_BIT_SET:
                if(SingleBitOn(a->name1))
                    IF_BODY
                break;

            case INT_IF_BIT_CLEAR:
                if(!SingleBitOn(a->name1))
                    IF_BODY
                break;

            case INT_IF_VARIABLE_LES_LITERAL:
                if(GetSimulationVariable(a->name1) < a->literal)
                    IF_BODY
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                if(GetSimulationVariable(a->name1) ==
                    GetSimulationVariable(a->name2))
                    IF_BODY
                break;

            case INT_IF_VARIABLE_GRT_VARIABLE:
                if(GetSimulationVariable(a->name1) >
                    GetSimulationVariable(a->name2))
                    IF_BODY
                break;

            case INT_SET_PWM:
                // Dummy call will cause a warning if no one ever assigned
                // to that variable.
                (void)GetSimulationVariable(a->name1);
                break;

            // Don't try to simulate the EEPROM stuff: just hold the EEPROM
            // busy all the time, so that the program never does anything
            // with it.
            case INT_EEPROM_BUSY_CHECK:
                SetSingleBit(a->name1, TRUE);
                break;

            case INT_EEPROM_READ:
            case INT_EEPROM_WRITE:
                oops();
                break;

            case INT_READ_ADC:
                // Keep the shadow copies of the ADC variables because in
                // the real device they will not be updated until an actual
                // read is performed, which occurs only for a true rung-in
                // condition there.
                SetSimulationVariable(a->name1, GetAdcShadow(a->name1));
                break;

            case INT_UART_SEND:
                if(SingleBitOn(a->name2) && (SimulateUartTxCountdown == 0)) {
                    SimulateUartTxCountdown = 2;
                    AppendToUartSimulationTextControl(
                        (BYTE)GetSimulationVariable(a->name1));
                }
                if(SimulateUartTxCountdown == 0) {
                    SetSingleBit(a->name2, FALSE);
                } else {
                    SetSingleBit(a->name2, TRUE);
                }
                break;

            case INT_UART_RECV:
                if(QueuedUartCharacter >= 0) {
                    SetSingleBit(a->name2, TRUE);
                    SetSimulationVariable(a->name1, (SWORD)QueuedUartCharacter);
                    QueuedUartCharacter = -1;
                } else {
                    SetSingleBit(a->name2, FALSE);
                }
                break;

            case INT_END_IF:
            case INT_ELSE:
                return;

            case INT_COMMENT:
                break;
            
            default:
                oops();
                break;
        }
    }
}

//-----------------------------------------------------------------------------
// Called by the Windows timer that triggers cycles when we are running
// in real time.
//-----------------------------------------------------------------------------
void CALLBACK PlcCycleTimer(HWND hwnd, UINT msg, UINT_PTR id, DWORD time)
{
    int i;
    for(i = 0; i < CyclesPerTimerTick; i++) {
        SimulateOneCycle(FALSE);
    }
}

//-----------------------------------------------------------------------------
// Simulate one cycle of the PLC. Update everything, and keep track of whether
// any outputs have changed. If so, force a screen refresh. If requested do
// a screen refresh regardless.
//-----------------------------------------------------------------------------
void SimulateOneCycle(BOOL forceRefresh)
{
    // When there is an error message up, the modal dialog makes its own
    // event loop, and there is risk that we would go recursive. So let
    // us fix that. (Note that there are no concurrency issues; we really
    // would get called recursively, not just reentrantly.)
    static BOOL Simulating = FALSE;

    if(Simulating) return;
    Simulating = TRUE;

    NeedRedraw = FALSE;

    if(SimulateUartTxCountdown > 0) {
        SimulateUartTxCountdown--;
    } else {
        SimulateUartTxCountdown = 0;
    }

    IntPc = 0;
    SimulateIntCode();

    if(NeedRedraw || SimulateRedrawAfterNextCycle || forceRefresh) {
        InvalidateRect(MainWindow, NULL, FALSE);
        ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
    }

    SimulateRedrawAfterNextCycle = FALSE;
    if(NeedRedraw) SimulateRedrawAfterNextCycle = TRUE;

    Simulating = FALSE;
}

//-----------------------------------------------------------------------------
// Start the timer that we use to trigger PLC cycles in approximately real
// time. Independently of the given cycle time, just go at 40 Hz, since that
// is about as fast as anyone could follow by eye. Faster timers will just
// go instantly.
//-----------------------------------------------------------------------------
void StartSimulationTimer(void)
{
    int p = Prog.cycleTime/1000;
    if(p < 5) {
        SetTimer(MainWindow, TIMER_SIMULATE, 10, PlcCycleTimer);
        CyclesPerTimerTick = 10000 / Prog.cycleTime;
    } else {
        SetTimer(MainWindow, TIMER_SIMULATE, p, PlcCycleTimer);
        CyclesPerTimerTick = 1;
    }
}

//-----------------------------------------------------------------------------
// Clear out all the parameters relating to the previous simulation.
//-----------------------------------------------------------------------------
void ClearSimulationData(void)
{
    VariablesCount = 0;
    SingleBitItemsCount = 0;
    AdcShadowsCount = 0;
    QueuedUartCharacter = -1;
    SimulateUartTxCountdown = 0;

    CheckVariableNames();

    SimulateRedrawAfterNextCycle = TRUE;

    if(!GenerateIntermediateCode()) {
        ToggleSimulationMode();
        return;
    }

    SimulateOneCycle(TRUE);
}

//-----------------------------------------------------------------------------
// Provide a description for an item (Xcontacts, Ycoil, Rrelay, Ttimer,
// or other) in the I/O list.
//-----------------------------------------------------------------------------
void DescribeForIoList(char *name, char *out)
{
    switch(name[0]) {
        case 'R':
        case 'X':
        case 'Y':
            sprintf(out, "%d", SingleBitOn(name));
            break;

        case 'T': {
            double dtms = GetSimulationVariable(name) *
                (Prog.cycleTime / 1000.0);
            if(dtms < 1000) {
                sprintf(out, "%.2f ms", dtms);
            } else {
                sprintf(out, "%.3f s", dtms / 1000);
            }
            break;
        }
        default: {
            SWORD v = GetSimulationVariable(name);
            sprintf(out, "%hd (0x%04hx)", v, v);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
// Toggle the state of a contact input; for simulation purposes, so that we
// can set the input state of the program.
//-----------------------------------------------------------------------------
void SimulationToggleContact(char *name)
{
    SetSingleBit(name, !SingleBitOn(name));
    ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
}

//-----------------------------------------------------------------------------
// Dialog proc for the popup that lets you interact with the UART stuff.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK UartSimulationProc(HWND hwnd, UINT msg,
    WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_DESTROY:
            DestroyUartSimulationWindow();
            break;

        case WM_CLOSE:
            break;

        case WM_SIZE:
            MoveWindow(UartSimulationTextControl, 0, 0, LOWORD(lParam),
                HIWORD(lParam), TRUE);
            break;

        case WM_ACTIVATE:
            if(wParam != WA_INACTIVE) {
                SetFocus(UartSimulationTextControl);
            }
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 1;
}

//-----------------------------------------------------------------------------
// Intercept WM_CHAR messages that to the terminal simulation window so that
// we can redirect them to the PLC program.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK UartSimulationTextProc(HWND hwnd, UINT msg, 
    WPARAM wParam, LPARAM lParam)
{
    if(msg == WM_CHAR) {
        QueuedUartCharacter = (BYTE)wParam;
        return 0;
    }

    return CallWindowProc((WNDPROC)PrevTextProc, hwnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// Pop up the UART simulation window; like a terminal window where the
// characters that you type go into UART RECV instruction and whatever
// the program puts into UART SEND shows up as text.
//-----------------------------------------------------------------------------
void ShowUartSimulationWindow(void)
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);

    wc.style            = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC |
                            CS_DBLCLKS;
    wc.lpfnWndProc      = (WNDPROC)UartSimulationProc;
    wc.hInstance        = Instance;
    wc.hbrBackground    = (HBRUSH)COLOR_BTNSHADOW;
    wc.lpszClassName    = "LDmicroUartSimulationWindow";
    wc.lpszMenuName     = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wc);

    DWORD TerminalX = 200, TerminalY = 200, TerminalW = 300, TerminalH = 150;

    ThawDWORD(TerminalX);
    ThawDWORD(TerminalY);
    ThawDWORD(TerminalW);
    ThawDWORD(TerminalH);

    if(TerminalW > 800) TerminalW = 100;
    if(TerminalH > 800) TerminalH = 100;

    RECT r;
    GetClientRect(GetDesktopWindow(), &r);
    if(TerminalX >= (DWORD)(r.right - 10)) TerminalX = 100;
    if(TerminalY >= (DWORD)(r.bottom - 10)) TerminalY = 100;

    UartSimulationWindow = CreateWindowClient(WS_EX_TOOLWINDOW |
        WS_EX_APPWINDOW, "LDmicroUartSimulationWindow",
        "UART Simulation (Terminal)", WS_VISIBLE | WS_SIZEBOX,
        TerminalX, TerminalY, TerminalW, TerminalH,
        NULL, NULL, Instance, NULL);

    UartSimulationTextControl = CreateWindowEx(0, WC_EDIT, "", WS_CHILD |
        WS_CLIPSIBLINGS | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE |
        WS_VSCROLL, 0, 0, TerminalW, TerminalH, UartSimulationWindow, NULL,
        Instance, NULL);

    HFONT fixedFont = CreateFont(14, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_DONTCARE, "Lucida Console");
    if(!fixedFont)
        fixedFont = (HFONT)GetStockObject(SYSTEM_FONT);

    SendMessage((HWND)UartSimulationTextControl, WM_SETFONT, (WPARAM)fixedFont,
        TRUE);

    PrevTextProc = SetWindowLongPtr(UartSimulationTextControl,
        GWLP_WNDPROC, (LONG_PTR)UartSimulationTextProc);

    ShowWindow(UartSimulationWindow, TRUE);
    SetFocus(MainWindow);
}

//-----------------------------------------------------------------------------
// Get rid of the UART simulation terminal-type window.
//-----------------------------------------------------------------------------
void DestroyUartSimulationWindow(void)
{
    // Try not to destroy the window if it is already destroyed; that is
    // not for the sake of the window, but so that we don't trash the
    // stored position.
    if(UartSimulationWindow == NULL) return;

    DWORD TerminalX, TerminalY, TerminalW, TerminalH;
    RECT r;

    GetClientRect(UartSimulationWindow, &r);
    TerminalW = r.right - r.left;
    TerminalH = r.bottom - r.top;

    GetWindowRect(UartSimulationWindow, &r);
    TerminalX = r.left;
    TerminalY = r.top;

    FreezeDWORD(TerminalX);
    FreezeDWORD(TerminalY);
    FreezeDWORD(TerminalW);
    FreezeDWORD(TerminalH);

    DestroyWindow(UartSimulationWindow);
    UartSimulationWindow = NULL;
}

//-----------------------------------------------------------------------------
// Append a received character to the terminal buffer.
//-----------------------------------------------------------------------------
static void AppendToUartSimulationTextControl(BYTE b)
{
    char append[5];

    if((isalnum(b) || strchr("[]{};':\",.<>/?`~ !@#$%^&*()-=_+|", b) ||
        b == '\r' || b == '\n') && b != '\0')
    {
        append[0] = b;
        append[1] = '\0';
    } else {
        sprintf(append, "\\x%02x", b);
    }

#define MAX_SCROLLBACK 256
    char buf[MAX_SCROLLBACK];

    SendMessage(UartSimulationTextControl, WM_GETTEXT, (WPARAM)sizeof(buf),
        (LPARAM)buf);

    int overBy = (strlen(buf) + strlen(append) + 1) - sizeof(buf);
    if(overBy > 0) {
        memmove(buf, buf + overBy, strlen(buf));
    }
    strcat(buf, append);

    SendMessage(UartSimulationTextControl, WM_SETTEXT, 0, (LPARAM)buf);
    SendMessage(UartSimulationTextControl, EM_LINESCROLL, 0, (LPARAM)INT_MAX);
}
