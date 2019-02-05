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
#include "stdafx.h"

#include "ldmicro.h"
#include "intcode.h"
#include "freeze.h"

static struct {
    char name[MAX_NAME_LEN];
    bool powered;
} SingleBitItems[MAX_IO];
static int SingleBitItemsCount;

static struct {
    char   name[MAX_NAME_LEN];
    SDWORD val;
    char   valstr[MAX_COMMENT_LEN]; // value in simulation mode for STRING types.
    DWORD  usedFlags;
    int    initedRung;                 // Variable inited in rung.
    DWORD  initedOp;                   // Variable inited in Op number.
    char   usedRungs[MAX_COMMENT_LEN]; // Rungs, where variable is used.
} Variables[MAX_IO];
static int VariableCount;

DWORD CyclesCount; // Simulated

static struct {
    char  name[MAX_NAME_LEN];
    SWORD val;
} AdcShadows[MAX_IO];
static int AdcShadowsCount;

// clang-format off

#define VAR_FLAG_TON                  0x00000001
#define VAR_FLAG_TOF                  0x00000002
#define VAR_FLAG_RTO                  0x00000004
#define VAR_FLAG_RTL                  0x00000008
#define VAR_FLAG_TCY                  0x00000010
#define VAR_FLAG_THI                  0x00000020
#define VAR_FLAG_TLO                  0x00000040

#define VAR_FLAG_CTU                  0x00000100
#define VAR_FLAG_CTD                  0x00000200
#define VAR_FLAG_CTC                  0x00000400
#define VAR_FLAG_CTR                  0x00000800

#define VAR_FLAG_PWM                  0x00001000

#define VAR_FLAG_RES                  0x00010000
#define VAR_FLAG_TABLE                0x00100000
#define VAR_FLAG_ANY                  0x08000000

#define VAR_FLAG_OTHERWISE_FORGOTTEN  0x80000000

// clang-format on

// Schematic-drawing code needs to know whether we're in simulation mode or
// note, as that changes how everything is drawn; also UI code, to disable
// editing during simulation.
bool InSimulationMode;

// Don't want to redraw the screen unless necessary; track whether a coil
// changed state or a timer output switched to see if anything could have
// changed (not just coil, as we show the intermediate steps too).
static int NeedRedraw; // a->op used for debug
// Have to let the effects of a coil change in cycle k appear in cycle k+1,
// or set by the UI code to indicate that user manually changed an Xfoo
// input.
bool SimulateRedrawAfterNextCycle;

// Don't want to set a timer every 100 us to simulate a 100 us cycle
// time...but we can cycle multiple times per timer interrupt and it will
// be almost as good, as long as everything runs fast.
static int CyclesPerTimerTick;

// Program counter as we evaluate the intermediate code.
static uint32_t IntPc;

static FILE *fUART;
static FILE *fSPI;		///// Added by JG
static FILE *fI2C;		///// Added by JG

// A window to allow simulation with the UART stuff (insert keystrokes into
// the program, view the output, like a terminal window).
HWND     UartSimulationWindow = nullptr;			///// Modified by JG
static HWND     UartSimulationTextControl;
static LONG_PTR PrevUartTextProc;					///// Modified by JG

///// Added by JG
// A window to allow simulation with the SPI stuff (insert keystrokes into
// the program, view the output, like a terminal window).
HWND     SpiSimulationWindow = nullptr;
static HWND     SpiSimulationTextControl;
static LONG_PTR PrevSpiTextProc;

// A window to allow simulation with the I2C stuff (insert keystrokes into
// the program, view the output, like a terminal window).
HWND     I2cSimulationWindow = nullptr;
static HWND     I2cSimulationTextControl;
static LONG_PTR PrevI2cTextProc;
/////

static int QueuedUartCharacter = -1;
static int SimulateUartTxCountdown = 0; // 0 if UART ready to send;
                                        // 1 if UART busy
///// Added by JG
static DWORD TerminalX1 = 200, TerminalY1 = 200, TerminalW1 = 300, TerminalH1 = 150;
static DWORD TerminalX2 = 250, TerminalY2 = 250, TerminalW2 = 300, TerminalH2 = 150;
static DWORD TerminalX3 = 300, TerminalY3 = 350, TerminalW3 = 300, TerminalH3 = 150;

static int QueuedSpiCharacter = -1;
static int QueuedI2cCharacter = -1;
/////

/////	static void AppendToUartSimulationTextControl(BYTE b);
static void AppendToSimulationTextControl(BYTE b, HWND SimulationTextControl);				///// Modified by JG

static void        SimulateIntCode();
static const char *MarkUsedVariable(const char *name, DWORD flag);

//-----------------------------------------------------------------------------
int isVarInited(const char *name)
{
    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            break;
        }
    }
    if(i >= VariableCount)
        return -1;
    return Variables[i].initedRung;
}
//-----------------------------------------------------------------------------
DWORD isVarUsed(const char *name)
{
    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            break;
        }
    }
    if(i >= MAX_IO)
        return 0;

    return Variables[i].usedFlags;
}
//-----------------------------------------------------------------------------
// Query the state of a single-bit element (relay, digital in, digital out).
// Looks in the SingleBitItems list; if an item is not present then it is
// false by default.
//-----------------------------------------------------------------------------
static bool SingleBitOn(const char *name)
{
    for(int i = 0; i < SingleBitItemsCount; i++) {
        if(strcmp(SingleBitItems[i].name, name) == 0) {
            return SingleBitItems[i].powered;
        }
    }
    return false;
}

template <size_t N> static bool SingleBitOn(const StringArray<N> &name)
{
    for(int i = 0; i < SingleBitItemsCount; i++) {
        if(name == SingleBitItems[i].name) {
            return SingleBitItems[i].powered;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// Set the state of a single-bit item. Adds it to the list if it is not there
// already.
//-----------------------------------------------------------------------------
static void SetSingleBit(const char *name, bool state)
{
    int i;
    for(i = 0; i < SingleBitItemsCount; i++) {
        if(strcmp(SingleBitItems[i].name, name) == 0) {
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

template <size_t N> static void SetSingleBit(const StringArray<N> &name, bool state)
{
    int i;
    for(i = 0; i < SingleBitItemsCount; i++) {
        if(name == SingleBitItems[i].name) {
            SingleBitItems[i].powered = state;
            return;
        }
    }
    if(i < MAX_IO) {
        strcpy(SingleBitItems[i].name, name.c_str());
        SingleBitItems[i].powered = state;
        SingleBitItemsCount++;
    }
}

bool GetSingleBit(char *name)
{
    return SingleBitOn(name);
}

//-----------------------------------------------------------------------------
long convert_to_int24_t(long v)
{
    long val;
    memcpy(&val, &v, 3); // only 3 bytes
    return val;
}

//-----------------------------------------------------------------------------
long OverflowToVarSize(long val, int sov)
{
    if(sov < 1)
        oops();
    if(sov > 4)
        oops();
    if(sov < byteNeeded(val)) {
        if(sov == 1)
            val = (int8_t)val;
        else if(sov == 2)
            val = (int16_t)val;
        else if(sov == 3)
            val = convert_to_int24_t(val);
        else if(sov == 4)
            val = (int32_t)val;
        else
            val = val;
    }
    return val;
}

//-----------------------------------------------------------------------------
// Count a timer up (i.e. increment its associated count by 1). Must already
// exist in the table.
//-----------------------------------------------------------------------------
static void Increment(const char *name, const char *overlap, const char *overflow)
{
    /*
    int    sov = SizeOfVar(name);
    SDWORD signMask = 1 << (sov * 8 - 1);
    SDWORD signBefore, signAfter;
    int    i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            signBefore = Variables[i].val & signMask;
            (Variables[i].val)++;
            signAfter = Variables[i].val & signMask;
            if((signBefore == 0) && (signAfter != 0)) {
                SetSingleBit(overflow, true);
                Variables[i].val &= (1 << (8 * sov)) - 1;
            }

            SetSingleBit(overlap, Variables[i].val == 0); // OVERLAP 11...11 -> 00...00
            return;
        }
    }
    ooops(name);
*/
    int sov = SizeOfVar(name);
    for(int i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {

            (Variables[i].val)++;

            if(sov < byteNeeded(Variables[i].val)) {
                SetSingleBit(overflow, true);
                Variables[i].val = OverflowToVarSize(Variables[i].val, sov);
            }
            SetSingleBit(overlap, Variables[i].val == 0); // OVERLAP 11...11 -> 00...00 // -1 -> 0
            return;
        }
    }
    ooops(name);
}

static void Increment(const NameArray &name, const char *overlap, const char *overflow)
{
    Increment(name.c_str(), overlap, overflow);
}

static void Increment(const NameArray &name, const NameArray &overlap, const char *overflow)
{
    Increment(name.c_str(), overlap.c_str(), overflow);
}

//-----------------------------------------------------------------------------
static void Decrement(const char *name, const char *overlap, const char *overflow)
{
    /*
    int    sov = SizeOfVar(name);
    SDWORD signMask = 1 << (sov * 8 - 1);
    SDWORD signBefore, signAfter;
    int    i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            SetSingleBit(overlap, Variables[i].val == 0); // OVERLAP 00...00 -> 11...11

            signBefore = Variables[i].val & signMask;
            (Variables[i].val)--;
            signAfter = Variables[i].val & signMask;

            if((signBefore != 0) && (signAfter == 0)) {
                SetSingleBit(overflow, true);
                Variables[i].val &= (1 << (8 * sov)) - 1;
            }
            return;
        }
    }
    ooops(name);
*/
    int sov = SizeOfVar(name);
    for(int i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            SetSingleBit(overlap, Variables[i].val == 0); // OVERLAP 00...00 -> 11...11 // 0 -> -1

            (Variables[i].val)--;

            if(sov < byteNeeded(Variables[i].val)) {
                SetSingleBit(overflow, true);
                Variables[i].val = OverflowToVarSize(Variables[i].val, sov);
            }
            return;
        }
    }
    ooops(name);
}

static void Decrement(const NameArray &name, const NameArray &overlap, const char *overflow)
{
    Decrement(name.c_str(), overlap.c_str(), overflow);
}

//-----------------------------------------------------------------------------
static SDWORD AddVariable(const NameArray &name1, const NameArray &name2, const NameArray &name3, const char *overflow)
{
    long long int ret = (long long int)GetSimulationVariable(name2) + (long long int)GetSimulationVariable(name3);
    int           sov = SizeOfVar(name1);
    SDWORD        signMask = 1 << (sov * 8 - 1);
    SDWORD        sign2 = GetSimulationVariable(name2) & signMask;
    SDWORD        sign3 = GetSimulationVariable(name3) & signMask;
    SDWORD        signr = (SDWORD)(ret & signMask);
    if((sign2 == sign3) && (signr != sign3))
        SetSingleBit(overflow, true);
    return (SDWORD)ret;
}

//-----------------------------------------------------------------------------
static SDWORD SubVariable(const NameArray &name1, const NameArray &name2, const NameArray &name3, const char *overflow)
{
    long long int ret = (long long int)GetSimulationVariable(name2) - (long long int)GetSimulationVariable(name3);
    int           sov = SizeOfVar(name1);
    SDWORD        signMask = 1 << (sov * 8 - 1);
    SDWORD        sign2 = GetSimulationVariable(name2) & signMask;
    SDWORD        sign3 = GetSimulationVariable(name3) & signMask;
    SDWORD        signr = (SDWORD)(ret & signMask);
    //  if((sign2 != sign3)
    //  && (signr != sign2))
    if((sign2 != sign3) && (signr == sign3))
        SetSingleBit(overflow, true);
    return (SDWORD)ret;
}

//-----------------------------------------------------------------------------
// Set a variable to a value.
//-----------------------------------------------------------------------------
void SetSimulationVariable(const NameArray &name, SDWORD val)
{
    SetSimulationVariable(name.c_str(), val);
}

void SetSimulationVariable(const char *name, SDWORD val)
{
    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
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
SDWORD GetSimulationVariable(const char *name, bool forIoList)
{
    if(IsNumber(name)) {
        return CheckMakeNumber(name);
    }
    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            return Variables[i].val;
        }
    }
    if(forIoList)
        return 0;
    MarkUsedVariable(name, VAR_FLAG_OTHERWISE_FORGOTTEN);
    return GetSimulationVariable(name);
}

SDWORD GetSimulationVariable(const char *name)
{
    return GetSimulationVariable(name, false);
}

SDWORD GetSimulationVariable(const NameArray &name)
{
    return GetSimulationVariable(name.c_str());
}

//-----------------------------------------------------------------------------
// Set a variable to a value.
//-----------------------------------------------------------------------------
void SetSimulationStr(const char *name, const char *val)
{
    for(int i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            strcpy(Variables[i].valstr, val);
            return;
        }
    }
    MarkUsedVariable(name, VAR_FLAG_OTHERWISE_FORGOTTEN);
    SetSimulationStr(name, val);
}

//-----------------------------------------------------------------------------
// Read a variable's value.
//-----------------------------------------------------------------------------
char *GetSimulationStr(const char *name)
{
    for(int i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            return Variables[i].valstr;
        }
    }
    MarkUsedVariable(name, VAR_FLAG_OTHERWISE_FORGOTTEN);
    return GetSimulationStr(name);
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
        if(strcmp(AdcShadows[i].name, name) == 0) {
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
SWORD GetAdcShadow(const char *name)
{
    int i;
    for(i = 0; i < AdcShadowsCount; i++) {
        if(strcmp(AdcShadows[i].name, name) == 0) {
            return AdcShadows[i].val;
        }
    }
    return 0;
}

SWORD GetAdcShadow(const NameArray &name)
{
    return GetAdcShadow(name.c_str());
}

//-----------------------------------------------------------------------------
// https://en.m.wikipedia.org/wiki/Linear_congruential_generator
// X[n+1] = (a * X[n] + c) mod m
// VMS's MTH$RANDOM, old versions of glibc
// a = 69069 ( 0x10DCD ) (1 00001101 11001101b)
// c = 1
// m = 2 ** 32
static unsigned long long seed = 1;
//
SDWORD MthRandom()
{
    // seed = (seed * 69069 + 1) % 4294967296;
    seed = (seed * 69069 + 1) & 0xFFFFffff;
    return (SDWORD)seed;
}

SDWORD GetRandom(const NameArray &name)
{
    int    sov = SizeOfVar(name);
    SDWORD seed = MthRandom();
    char   seedName[MAX_NAME_LEN];
    sprintf(seedName, "$seed_%s", name.c_str());
    SetSimulationVariable(seedName, seed);
    if(sov == 1)
        return (signed char)(seed >> (8 * (4 - sov)));
    else if(sov == 2)
        return (SWORD)(seed >> (8 * (4 - sov)));
    else if(sov >= 3)
        return (SDWORD)(seed >> (8 * (4 - sov)));
    else {
        oops();
        return 0;
    }
}

//-----------------------------------------------------------------------------
static const char *Check(const char *name, DWORD flag, int i)
{
    switch(flag) {
        case VAR_FLAG_PWM:
            if(Variables[i].usedFlags & ~(VAR_FLAG_RES | VAR_FLAG_PWM))
                return _("PWM: variable can only be used for RES elsewhere");
            break;

        case VAR_FLAG_TOF:
            if(Variables[i].usedFlags & ~VAR_FLAG_RES)
                return _("TOF: variable can only be used for RES elsewhere");
            break;

        case VAR_FLAG_TON:
            if(Variables[i].usedFlags & ~VAR_FLAG_RES)
                return _("TON: variable can only be used for RES elsewhere");
            break;

        case VAR_FLAG_TCY:
            if(Variables[i].usedFlags & ~VAR_FLAG_RES)
                return _("TCY: variable can only be used for RES elsewhere");
            break;

        case VAR_FLAG_RTO:
            if(Variables[i].usedFlags & ~VAR_FLAG_RES)
                return _("RTO: variable can only be used for RES elsewhere");
            break;

        case VAR_FLAG_RTL:
            if(Variables[i].usedFlags & ~VAR_FLAG_RES)
                return _("RTL: variable can only be used for RES elsewhere");
            break;

        case VAR_FLAG_THI:
            if(Variables[i].usedFlags & ~VAR_FLAG_RES)
                return _("THI: variable can only be used for RES elsewhere");
            break;

        case VAR_FLAG_TLO:
            if(Variables[i].usedFlags & ~VAR_FLAG_RES)
                return _("TLO: variable can only be used for RES elsewhere");
            break;

        case VAR_FLAG_RES:
        case VAR_FLAG_CTU:
        case VAR_FLAG_CTD:
        case VAR_FLAG_CTC:
        case VAR_FLAG_CTR:
            break;

        case VAR_FLAG_TABLE:
            if(Variables[i].usedFlags & ~VAR_FLAG_TABLE)
                return _("TABLE: variable can be used only with TABLE");
            break;

        case VAR_FLAG_ANY:
            if(Variables[i].usedFlags & VAR_FLAG_PWM)
                return _("PWM: variable can only be used for RES elsewhere");

            if(Variables[i].usedFlags & VAR_FLAG_TOF)
                return _("TOF: variable can only be used for RES elsewhere");

            if(Variables[i].usedFlags & VAR_FLAG_TON)
                return _("TON: variable can only be used for RES elsewhere");

            if(Variables[i].usedFlags & VAR_FLAG_TCY)
                return _("TCY: variable can only be used for RES elsewhere");
            /*
            if(Variables[i].usedFlags & VAR_FLAG_RTO)
                return _("RTO: variable can only be used for RES elsewhere");
            */
            if(Variables[i].usedFlags & VAR_FLAG_RTL)
                return _("RTL: variable can only be used for RES elsewhere");

            if(Variables[i].usedFlags & VAR_FLAG_THI)
                return _("THI: variable can only be used for RES elsewhere");

            if(Variables[i].usedFlags & VAR_FLAG_TLO)
                return _("TLO: variable can only be used for RES elsewhere");

            break;

        case VAR_FLAG_OTHERWISE_FORGOTTEN:
            if((name[0] != '$') && (name[0] != '#')) {
                Error(_("Variable '%s' not assigned to, e.g. with a "
                        "MOV statement, an ADD statement, etc.\r\n\r\n"
                        "This is probably a programming error; now it "
                        "will always be zero."),
                      name);
            }
            break;

        default:
            oops();
    }
    return nullptr;
}
//-----------------------------------------------------------------------------
// Mark how a variable is used; a series of flags that we can OR together,
// then we can check to make sure that only valid combinations have been used
// (e.g. just a TON, an RTO with its reset, etc.). Returns nullptr for success,
// else an error string.
//-----------------------------------------------------------------------------
static const char *rungsUsed = ""; // local store var for message

static const char *MarkUsedVariable(const char *name, DWORD flag)
{
    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            break;
        }
    }
    if(i >= MAX_IO)
        return "";

    if(i == VariableCount) {
        strcpy(Variables[i].name, name);
        Variables[i].usedFlags = 0;
        Variables[i].val = 0;
        Variables[i].initedRung = -2; // rungNow;
        strcpy(Variables[i].usedRungs, "");
        VariableCount++;
    }

    char srungNow[MAX_NAME_LEN];
    sprintf(srungNow, "%d ", rungNow + 1);
    if(!strstr(Variables[i].usedRungs, srungNow))
        strcat(Variables[i].usedRungs, srungNow);

    rungsUsed = Variables[i].usedRungs;

    const char *s = Check(name, flag, i);
    if(s)
        return s;

//    if(Variables[i].initedRung < 0)
  //      Variables[i].initedRung = rungNow;
    Variables[i].usedFlags |= flag;
    return nullptr;
}

void MarkInitedVariable(const char *name)
{
    int i;
    for(i = 0; i < VariableCount; i++) {
        if(strcmp(Variables[i].name, name) == 0) {
            break;
        }
    }
    if(i >= MAX_IO)
        oops();

    if(i == VariableCount) {
        strcpy(Variables[i].name, name);
        Variables[i].usedFlags = 0;
        Variables[i].val = 0;
        Variables[i].initedRung = -2; //rungNow;
        //Variables[i].initedOp = opNow;
        strcpy(Variables[i].usedRungs, "");
        VariableCount++;
    }
    if(Variables[i].initedRung < 0)
        Variables[i].initedRung = rungNow;
}

//-----------------------------------------------------------------------------
static void CheckMsg(const char *name, const char *s, int i)
{
    if(s) {
        Error(_("Rung %d: Variable '%s' incorrectly assigned.\n%s.\nSee rungs:%s"), rungNow + 1, name, s, rungsUsed);
    }
}
//-----------------------------------------------------------------------------
// Check for duplicate uses of a single variable. For example, there should
// not be two TONs with the same name. On the other hand, it would be okay
// to have an RTO with the same name as its reset; in fact, verify that
// there must be a reset for each RTO.
//-----------------------------------------------------------------------------
static void MarkWithCheck(const char *name, int flag)
{
    const char *s = MarkUsedVariable(name, flag);
    CheckMsg(name, s, -1);
}
//-----------------------------------------------------------------------------
static void CheckVariableNamesCircuit(int which, void *elem)
{
    ElemLeaf *l = (ElemLeaf *)elem;
    char *    name = nullptr;
    DWORD     flag;
    char      str[MAX_NAME_LEN];

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            int               i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(i = 0; i < s->count; i++) {
                CheckVariableNamesCircuit(s->contents[i].which, s->contents[i].data.any);
            }
            break;
        }

        case ELEM_PARALLEL_SUBCKT: {
            int                 i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(i = 0; i < p->count; i++) {
                CheckVariableNamesCircuit(p->contents[i].which, p->contents[i].data.any);
            }
            break;
        }

        case ELEM_RTL:
        case ELEM_RTO:
        case ELEM_TOF:
        case ELEM_TON:
        case ELEM_TCY:
        case ELEM_THI:
        case ELEM_TLO:
            if(which == ELEM_RTO)
                flag = VAR_FLAG_RTO;
            else if(which == ELEM_RTL)
                flag = VAR_FLAG_RTL;
            else if(which == ELEM_TOF)
                flag = VAR_FLAG_TOF;
            else if(which == ELEM_TON)
                flag = VAR_FLAG_TON;
            else if(which == ELEM_TCY)
                flag = VAR_FLAG_TCY;
            else if(which == ELEM_THI)
                flag = VAR_FLAG_THI;
            else if(which == ELEM_TLO)
                flag = VAR_FLAG_TLO;
            else
                oops();

            MarkWithCheck(l->d.timer.name, flag);

            break;

        case ELEM_CTU:
        case ELEM_CTD:
        case ELEM_CTC:
        case ELEM_CTR:
            if(which == ELEM_CTU)
                flag = VAR_FLAG_CTU;
            else if(which == ELEM_CTD)
                flag = VAR_FLAG_CTD;
            else if(which == ELEM_CTC)
                flag = VAR_FLAG_CTC;
            else if(which == ELEM_CTR)
                flag = VAR_FLAG_CTR;
            else
                oops();

            MarkWithCheck(l->d.counter.name, flag);

            break;

        case ELEM_SET_PWM:
            MarkWithCheck(l->d.setPwm.name, VAR_FLAG_PWM);
            break;

        case ELEM_RES:
            MarkWithCheck(l->d.reset.name, VAR_FLAG_RES);
            break;

        case ELEM_TIME2DELAY:
        case ELEM_TIME2COUNT:
        case ELEM_MOVE:
            MarkWithCheck(l->d.move.dest, VAR_FLAG_ANY);
            break;

        case ELEM_GOTO:
        case ELEM_GOSUB:
        case ELEM_LABEL:
        case ELEM_SUBPROG:
        case ELEM_ENDSUB:
            MarkWithCheck(l->d.doGoto.label, VAR_FLAG_ANY);
            break;

        case ELEM_RETURN:
            break;

        case ELEM_NPULSE_OFF:
            // case ELEM_PWM_OFF:
            break;

        case ELEM_QUAD_ENCOD:
            MarkWithCheck(l->d.QuadEncod.counter, VAR_FLAG_ANY);

            char ticks[MAX_NAME_LEN];
            sprintf(ticks, "$ticks_%s", l->d.QuadEncod.counter);
            MarkWithCheck(ticks, VAR_FLAG_ANY);

            char dir[MAX_NAME_LEN];
            sprintf(dir, "$dir_%s", l->d.QuadEncod.counter);
            MarkWithCheck(dir, VAR_FLAG_ANY);

            char revol[MAX_NAME_LEN];
            sprintf(revol, "$revol_%s", l->d.QuadEncod.counter);
            MarkWithCheck(revol, VAR_FLAG_ANY);

            MarkWithCheck("ELEM_QUAD_ENCOD", VAR_FLAG_ANY);
            break;

        case ELEM_NPULSE:
        case ELEM_PULSER:
        case ELEM_STEPPER:
            sprintf(str, "%s%s", l->d.stepper.name, ""); // "LutElement");
            MarkWithCheck(str, VAR_FLAG_TABLE);
            //MarkWithCheck(l->d.stepper.name, VAR_FLAG_TABLE);

            //MarkWithCheck(l->d.stepper.coil, VAR_FLAG_TABLE);
            //MarkWithCheck(l->d.stepper.name, VAR_FLAG_ANY);

            //if(IsNumber(l->d.stepper.P)&&(CheckMakeNumber(l->d.stepper.P)>1)
            //||(!IsNumber(l->d.stepper.P))){
            sprintf(str, "C%s%s", l->d.stepper.name, "Dec");
            MarkWithCheck(str, VAR_FLAG_ANY);

            sprintf(str, "C%s%s", l->d.stepper.name, "Inc");
            MarkWithCheck(str, VAR_FLAG_ANY);

            sprintf(str, "C%s%s", l->d.stepper.name, "P");
            MarkWithCheck(str, VAR_FLAG_ANY);

            //}
            //MarkWithCheck(l->d.stepper.counter, VAR_FLAG_ANY);
            //MarkWithCheck(l->d.stepper.P, VAR_FLAG_ANY);
            //MarkWithCheck(l->d.stepper.accel, VAR_FLAG_ANY);
            break;

        case ELEM_BIN2BCD:
        case ELEM_BCD2BIN:
        case ELEM_SWAP:
        case ELEM_OPPOSITE:
        case ELEM_BUS:
            MarkWithCheck(l->d.move.dest, VAR_FLAG_ANY);
            break;

            // clang-format off
        const char *s;
        case ELEM_7SEG:  s = "char7seg";  goto xseg;
        case ELEM_9SEG:  s = "char9seg";  goto xseg;
        case ELEM_14SEG: s = "char14seg"; goto xseg;
        case ELEM_16SEG: s = "char16seg"; goto xseg;
        xseg:
            // clang-format on
            MarkWithCheck(s, VAR_FLAG_ANY);
            MarkWithCheck(l->d.segments.dest, VAR_FLAG_ANY);
            if(!IsNumber(l->d.segments.src))
                MarkWithCheck(l->d.segments.src, VAR_FLAG_ANY);
            break;

        case ELEM_LOOK_UP_TABLE:
            sprintf(str, "%s%s", l->d.lookUpTable.name, ""); // "LutElement");
            MarkWithCheck(str, VAR_FLAG_TABLE);
            MarkWithCheck(l->d.lookUpTable.dest, VAR_FLAG_ANY);
            if(!IsNumber(l->d.lookUpTable.index))
                MarkWithCheck(l->d.lookUpTable.index, VAR_FLAG_ANY);
            break;

        case ELEM_PIECEWISE_LINEAR:
            sprintf(str, "%s%s", l->d.piecewiseLinear.name, "");
            MarkWithCheck(str, VAR_FLAG_TABLE);
            MarkWithCheck(l->d.piecewiseLinear.dest, VAR_FLAG_ANY);
            if(!IsNumber(l->d.lookUpTable.index))
                MarkWithCheck(l->d.lookUpTable.index, VAR_FLAG_ANY); // not tested
            break;

        case ELEM_READ_ADC:
            MarkWithCheck(l->d.readAdc.name, VAR_FLAG_ANY);
            break;

        case ELEM_ADD:
        case ELEM_SUB:
        case ELEM_MUL:
        case ELEM_DIV:
        case ELEM_MOD:
        case ELEM_SHL:
        case ELEM_SHR:
        case ELEM_SR0:
        case ELEM_ROL:
        case ELEM_ROR:
        case ELEM_SET_BIT:
        case ELEM_CLEAR_BIT:
        case ELEM_IF_BIT_SET:
        case ELEM_IF_BIT_CLEAR:
        case ELEM_AND:
        case ELEM_OR:
        case ELEM_XOR:
        case ELEM_NOT:
        case ELEM_NEG:
        case ELEM_RANDOM:
            MarkWithCheck(l->d.math.dest, VAR_FLAG_ANY);
            break;

        case ELEM_UART_RECV:
        case ELEM_UART_RECVn:
            MarkWithCheck(l->d.uart.name, VAR_FLAG_ANY);
            break;

        case ELEM_SHIFT_REGISTER: {
            int i;
            for(i = 1; i < l->d.shiftRegister.stages; i++) {
                char str[MAX_NAME_LEN + 10];
                sprintf(str, "%s%d", l->d.shiftRegister.name, i);
                MarkWithCheck(str, VAR_FLAG_ANY);
            }
            break;
        }

        case ELEM_STRING:
        case ELEM_FORMATTED_STRING: {
            break;
        }
        case ELEM_SEED_RANDOM:
        case ELEM_PERSIST:
        case ELEM_MASTER_RELAY:
        case ELEM_DELAY:
        case ELEM_SLEEP:
        case ELEM_CLRWDT:
        case ELEM_LOCK:
        case ELEM_UART_SEND:
        case ELEM_UART_SENDn:
        case ELEM_UART_SEND_READY:
        case ELEM_UART_RECV_AVAIL:
		case ELEM_SPI:					///// Added by JG
		case ELEM_SPI_WR:				///// Added by JG
		case ELEM_I2C_RD:				///// Added by JG
		case ELEM_I2C_WR:				///// Added by JG
        case ELEM_PLACEHOLDER:
        case ELEM_COMMENT:
        case ELEM_OPEN:
        case ELEM_SHORT:
        case ELEM_COIL:
        case ELEM_CONTACTS:
        case ELEM_ONE_SHOT_RISING:
        case ELEM_ONE_SHOT_FALLING:
        case ELEM_ONE_SHOT_LOW:
        case ELEM_OSC:
        case ELEM_EQU:
        case ELEM_NEQ:
        case ELEM_GRT:
        case ELEM_GEQ:
        case ELEM_LES:
        case ELEM_LEQ:
#ifdef USE_SFR
        case ELEM_RSFR:
        case ELEM_WSFR:
        case ELEM_SSFR:
        case ELEM_CSFR:
        case ELEM_TSFR:
        case ELEM_T_C_SFR:
#endif
            break;

        default:
            ooops("ELEM_0x%X", which);
    }
}
//-----------------------------------------------------------------------------
void CheckVariableNames()
{
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        rungNow = i; // Ok
        CheckVariableNamesCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    }

    // reCheck
    rungNow++;

    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_RES)
            if((Variables[i].usedFlags & ~VAR_FLAG_RES) == 0)
                Error(_("Rung %d: Variable '%s' incorrectly assigned.\n%s."),
                      Variables[i].initedRung + 1,
                      Variables[i].name,
                      _("RES: Variable is not assigned to COUNTER or TIMER or PWM.\r\n"
                        "You must assign a variable."));
    return;
#ifdef _LOT_OF_EXPERIMENTS_ // Never define!
    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_PWM) {
            //Variables[i].usedFlags &= ~VAR_FLAG_PWM;
            CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_PWM, i), i);
        }
    //--
    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_TCY) {
            //Variables[i].usedFlags &= ~VAR_FLAG_TCY;
            CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_TCY, i), i);
        }
    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_TON)
            CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_TON, i), i);

    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_TOF)
            CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_TOF, i), i);
    /*
    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_RTO)
             CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_RTO, i));
*/
    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_RTL)
            CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_RTL, i), i);

    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_THI)
            CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_THI, i), i);

    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_TLO)
            CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_TLO, i), i);
    //--
    /*
    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_CTU)
             CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_CTU, i));

    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_CTD)
             CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_CTD, i));

    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_CTC)
             CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_CTC, i));

    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_CTR)
             CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_CTR, i));
*/
    //--
    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_TABLE)
            CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_TABLE, i), i);
            /*
    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_ANY)
             CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_ANY, i), i);

    for(i = 0; i < VariableCount; i++)
        if(Variables[i].usedFlags & VAR_FLAG_OTHERWISE_FORGOTTEN)
             CheckMsg(Variables[i].name, Check(Variables[i].name, VAR_FLAG_OTHERWISE_FORGOTTEN, i), i);
*/
#endif
}
//-----------------------------------------------------------------------------
// Set normal closed inputs to 1 before simulating.
//-----------------------------------------------------------------------------
static void CheckSingleBitNegateCircuit(int which, void *elem)
{
    ElemLeaf *l = (ElemLeaf *)elem;
    char *    name = nullptr;

    switch(which) {
        case ELEM_SERIES_SUBCKT: {
            int               i;
            ElemSubcktSeries *s = (ElemSubcktSeries *)elem;
            for(i = 0; i < s->count; i++) {
                CheckSingleBitNegateCircuit(s->contents[i].which, s->contents[i].data.any);
            }
            break;
        }

        case ELEM_PARALLEL_SUBCKT: {
            int                 i;
            ElemSubcktParallel *p = (ElemSubcktParallel *)elem;
            for(i = 0; i < p->count; i++) {
                CheckSingleBitNegateCircuit(p->contents[i].which, p->contents[i].data.any);
            }
            break;
        }
        case ELEM_CONTACTS: {
            if((l->d.contacts.name[0] == 'X') && (l->d.contacts.set1))
                SetSingleBit(l->d.contacts.name, true); // Set HI level inputs before simulating
            break;
        }

        default:;
    }
}

static void CheckSingleBitNegate()
{
    int i;
    for(i = 0; i < Prog.numRungs; i++) {
        rungNow = i; // Ok
        CheckSingleBitNegateCircuit(ELEM_SERIES_SUBCKT, Prog.rungs[i]);
    }
}
//-----------------------------------------------------------------------------
// The IF condition is true. Execute the body, up until the ELSE or the
// END IF, and then skip the ELSE if it is present. Called with PC on the
// IF, returns with PC on the END IF.
//-----------------------------------------------------------------------------
static void IfConditionTrue()
{
    IntPc++;
    // now PC is on the first statement of the IF body
    SimulateIntCode();
    if(IntPc < IntCode.size()) {
        // now PC is on the ELSE or the END IF
        if(IntCode[IntPc].op == INT_ELSE) {
            int nesting = 1;
            for(; IntPc < IntCode.size(); IntPc++) {
                if(IntPc >= IntCode.size())
                    oops();

                if(IntCode[IntPc].op == INT_END_IF) {
                    nesting--;
                } else if(INT_IF_GROUP(IntCode[IntPc].op)) {
                    nesting++;
                }
                if(nesting == 0)
                    break;
            }
        } else if(IntCode[IntPc].op == INT_END_IF) {
            return;
        } else {
            if(!GotoGosubUsed())
                oops();
        }
    }
}

//-----------------------------------------------------------------------------
// The IF condition is false. Skip the body, up until the ELSE or the END
// IF, and then execute the ELSE if it is present. Called with PC on the IF,
// returns with PC on the END IF.
//-----------------------------------------------------------------------------
static void IfConditionFalse()
{
    int nesting = 0;
    for(; IntPc < IntCode.size(); IntPc++) {
        if(IntPc >= IntCode.size())
            oops();

        if(IntCode[IntPc].op == INT_END_IF) {
            nesting--;
        } else if(INT_IF_GROUP(IntCode[IntPc].op)) {
            nesting++;
        } else if(IntCode[IntPc].op == INT_ELSE && nesting == 1) {
            break;
        }
        if(nesting == 0)
            break;
    }

    // now PC is on the ELSE or the END IF
    if(IntCode[IntPc].op == INT_ELSE) {
        IntPc++;
        SimulateIntCode();
    } else if(IntCode[IntPc].op == INT_END_IF) {
        return;
    } else {
        if(!GotoGosubUsed())
            oops();
    }
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
long rol(long val, SDWORD n, int size, bool *state)
{
    char        MSB = 0;
    signed char i;
    for(i = 0; i < n; i++) {
        if(size == 1) {
            if(val & 0x80)
                MSB = 1;
        } else if(size == 2) {
            if(val & 0x8000)
                MSB = 1;
        } else if(size == 3) {
            if(val & 0x800000)
                MSB = 1;
        } else if(size == 4) {
            if(val & 0x80000000)
                MSB = 1;
        } else
            oops();

        val = val << 1;
        val |= MSB;
        *state = MSB != 0;

        if(size == 1)
            val &= 0xff;
        else if(size == 2)
            val &= 0xffff;
        else if(size == 3)
            val &= 0xFFffff;
        else if(size == 4)
            val &= 0xFFFFffff;
        else
            oops();
    }
    return val;
}
//-----------------------------------------------------------------------------
long ror(long val, SDWORD n, int size, bool *state)
{
    char        LSB = 0;
    signed char i;
    for(i = 0; i < n; i++) {
        LSB = (char)(val & 1);
        *state = LSB != 0;
        val = val >> 1;
        if(LSB) {
            if(size == 1)
                val |= 0x80;
            else if(size == 2)
                val |= 0x8000;
            else if(size == 3)
                val |= 0x800000;
            else if(size == 4)
                val |= 0x80000000;
            else
                oops();
        } else {
            if(size == 1)
                val &= 0x7f;
            else if(size == 2)
                val &= 0x7fff;
            else if(size == 3)
                val &= 0x7fffFF;
            else if(size == 4)
                val &= 0x7fffFFFF;
            else
                oops();
        }
        if(size == 1)
            val &= 0xff;
        else if(size == 2)
            val &= 0xffff;
        else if(size == 3)
            val &= 0xffffFF;
        else if(size == 4)
            val &= 0xffffFFFF;
        else
            oops();
    }
    return val;
}
//-----------------------------------------------------------------------------
long sr0(long val, SDWORD n, int size, bool *state)
{
    char        LSB = 0;
    signed char i;
    for(i = 0; i < n; i++) {
        LSB = (char)(val & 1);
        *state = LSB != 0;
        val = val >> 1;
        if(size == 1)
            val &= 0x7f;
        else if(size == 2)
            val &= 0x7fff;
        else if(size == 3)
            val &= 0x7fffFF;
        else if(size == 4)
            val &= 0x7fffFFFF;
        else
            oops();
    }
    return val;
}
//-----------------------------------------------------------------------------
long shr(signed long int val, SDWORD n, int size, bool *state)
{
    signed long int MSB = 0;
    char            LSB = 0;
    signed char     i;
    for(i = 0; i < n; i++) {
        if(size == 1) {
            MSB = val & 0x80;
        } else if(size == 2) {
            MSB = val & 0x8000;
        } else if(size == 3) {
            MSB = val & 0x800000;
        } else if(size == 4) {
            MSB = val & 0x80000000;
        } else
            oops();

        LSB = (char)(val & 1);
        *state = LSB != 0;
        val = val >> 1;
        val |= MSB;
    }
    return val;
}
//-----------------------------------------------------------------------------
long shl(long val, SDWORD n, int size, bool *state)
{
    char        MSB = 0;
    signed char i;
    for(i = 0; i < n; i++) {
        if(size == 1) {
            if(val & 0x80)
                MSB = 1;
        } else if(size == 2) {
            if(val & 0x8000)
                MSB = 1;
        } else if(size == 3) {
            if(val & 0x800000)
                MSB = 1;
        } else if(size == 4) {
            if(val & 0x80000000)
                MSB = 1;
        } else
            oops();

        val = val << 1;
        *state = MSB != 0;

        if(size == 1)
            val &= 0xff;
        else if(size == 2)
            val &= 0xffff;
        else if(size == 3)
            val &= 0xFFffff;
        else if(size == 4)
            val &= 0xFFFFffff;
        else
            oops();
    }
    return val;
}
//-----------------------------------------------------------------------------
//Binary to unpacked BCD
int bin2bcd(int val)
{
    int sign = 1;
    if(val < 0) {
        sign = -1;
        Error(" Value 'val'=%d < 0", val);
    }
    if(val >= TenToThe(sizeof(val)))
        Error("Value 'val'=%d overflow output range %d.", val, sizeof(val) - 1);
    int ret = val % 10;
    val /= 10;
    ret |= (val % 10) << 8;
    val /= 10;
    ret |= (val % 10) << 16;
    val /= 10;
    ret |= (val % 10) << 24;
    //  ret |= sign<0?0x80000000:0*/;
    return ret;
}
//-----------------------------------------------------------------------------
//Unpacked BCD to binary
int bcd2bin(int val)
{
    // clang-format off
    if( (val & 0x000000f)        > 9
    || ((val & 0x0000f00) >>  8) > 9
    || ((val & 0x00f0000) >> 16) > 9
    || ((val & 0xf000000) >> 24) > 9 )
        Error("Value 'val'=0x%x not in unpacked BCD format.", val);
    return (val & 0x000000f)
        + ((val & 0x0000f00) >>  8) * 10
        + ((val & 0x00f0000) >> 16) * 100
        + ((val & 0xf000000) >> 24) * 1000;
    // clang-format on
}
//-----------------------------------------------------------------------------
//Packed BCD to binary
int packedbcd2bin(int val)
{
    // clang-format off
    return (val & 0x0000000f)
        + ((val & 0x000000f0) >>  4) * 10
        + ((val & 0x00000f00) >>  8) * 100
        + ((val & 0x0000f000) >> 12) * 1000
        + ((val & 0x000f0000) >> 16) * 10000
        + ((val & 0x00f00000) >> 20) * 100000
        + ((val & 0x0f000000) >> 24) * 1000000
        + ((val & 0xf0000000) >> 28) * 10000000;
    // clang-format on
}
//-----------------------------------------------------------------------------
int opposite(int val, int sov)
{
    int ret = 0;
    int i;
    for(i = 0; i < sov * 8; i++) {
        ret = ret << 1;
        ret |= val & 1;
        val = val >> 1;
    }
    return ret;
}
//-----------------------------------------------------------------------------
int swap(int val, int sov)
{
    int ret = 0;
    if(sov == 1) {
        ret = ((val & 0x0f) << 4) | ((val & 0xf0) >> 4);
    } else if(sov == 2) {
        ret = ((val & 0xff) << 8) | ((val & 0xff00) >> 8);
    } else if(sov == 3) {
        ret = ((val & 0xff) << 16) | ((val & 0xff0000) >> 16) | (val & 0xff00);
    } else if(sov == 4) {
        ret = ((val & 0xff) << 20) | ((val & 0xff000000) >> 20) | ((val & 0xff0000) >> 8) | ((val & 0xff00) << 8);
    } else
        oops();
    return ret;
}
//-----------------------------------------------------------------------------
#define STACK_LEN 8 // hardware limit
static int stack[STACK_LEN];
static int stackCount = 0;
//-----------------------------------------------------------------------------
void PushStack(int IntPc)
{
    if(stackCount < STACK_LEN) {
        stack[stackCount] = IntPc;
        stackCount++;
    } else {
        Error(_("Stack size %d overflow"), STACK_LEN);
    }
}

//-----------------------------------------------------------------------------
int PopStack()
{
    if(stackCount > 0) {
        stackCount--;
        return stack[stackCount];
    } else {
        Error(_("Stack size %d underflow"), STACK_LEN);
        return -1;
    }
}

//-----------------------------------------------------------------------------
int FindOpRung(int op, int rung)
{
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if((IntCode[i].op == op) && (IntCode[i].rung == rung)) {
            //dbp("i=%d INT_%d r=%d ELEM_0x%X", i, IntCode[i].op, IntCode[i].rung, IntCode[i].which);
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
int FindOpName(int op, const NameArray &name1)
{
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if((IntCode[i].op == op) && (IntCode[i].name1 == name1)) {
            //dbp("i=%d INT_%d r=%d ELEM_0x%X", i, IntCode[i].op, IntCode[i].rung, IntCode[i].which);
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
int FindOpName(int op, const NameArray &name1, const NameArray &name2)
{
    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if((IntCode[i].op == op) && (IntCode[i].name1 == name1) && (IntCode[i].name2 == name2)) {
            //dbp("i=%d INT_%d r=%d ELEM_0x%X", i, IntCode[i].op, IntCode[i].rung, IntCode[i].which);
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
int FindOpNameLast(int op, const NameArray &name1)
{
    for(int i = IntCode.size() - 1; i >= 0; i--) {
        if((IntCode[i].op == op) && (IntCode[i].name1 == name1)) {
            //dbp("i=%d INT_%d r=%d ELEM_0x%X", i, IntCode[i].op, IntCode[i].rung, IntCode[i].which);
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
int FindOpNameLast(int op, const NameArray &name1, const NameArray &name2)
{
    for(int i = IntCode.size() - 1; i >= 0; i--) {
        if((IntCode[i].op == op) && ((IntCode[i].name1 == name1)) && ((IntCode[i].name2 == name2))) {
            //dbp("i=%d INT_%d r=%d ELEM_0x%X", i, IntCode[i].op, IntCode[i].rung, IntCode[i].which);
            return i;
        }
    }
    return -1;
}

//-----------------------------------------------------------------------------
// Evaluate a circuit, calling ourselves recursively to evaluate if/else
// constructs. Updates the on/off state of all the leaf elements in our
// internal tables. Returns when it reaches an end if or an else construct,
// or at the end of the program.
//-----------------------------------------------------------------------------
static void SimulateIntCode()
{
    long v;
    bool state;
    int  sov;
    for(; IntPc < IntCode.size(); IntPc++) {
        IntCode[IntPc].simulated = true;
        IntOp *a = &IntCode[IntPc];
        switch(a->op) {
            case INT_SIMULATE_NODE_STATE:
                if(*(a->poweredAfter) != SingleBitOn(a->name1)) {
                    NeedRedraw = a->op;
                    *(a->poweredAfter) = SingleBitOn(a->name1);
                }

                if(a->name2.size())
                    if(*(a->workingNow) != SingleBitOn(a->name2)) {
                        NeedRedraw = a->op;
                        *(a->workingNow) = SingleBitOn(a->name2);
                    }
                break;

            case INT_SET_BIT:
                SetSingleBit(a->name1, true);
                break;

            case INT_CLEAR_BIT:
                SetSingleBit(a->name1, false);
                break;

            case INT_COPY_BIT_TO_BIT:
                SetSingleBit(a->name1, SingleBitOn(a->name2));
                break;

            case INT_COPY_NOT_BIT_TO_BIT:
                SetSingleBit(a->name1, !SingleBitOn(a->name2));
                break;

            case INT_SET_BIT_AND_BIT:
                SetSingleBit(a->name1, (SingleBitOn(a->name2) & SingleBitOn(a->name3)) & 1);
                break;

            case INT_SET_BIT_OR_BIT:
                SetSingleBit(a->name1, (SingleBitOn(a->name2) | SingleBitOn(a->name3)) & 1);
                break;

            case INT_SET_BIT_XOR_BIT:
                SetSingleBit(a->name1, (SingleBitOn(a->name2) ^ SingleBitOn(a->name3)) & 1);
                break;

            case INT_COPY_VAR_BIT_TO_VAR_BIT:
                if(GetSimulationVariable(a->name2) & (1 << a->literal2))
                    SetSimulationVariable(a->name1, GetSimulationVariable(a->name1) | (1 << a->literal));
                else
                    SetSimulationVariable(a->name1, GetSimulationVariable(a->name1) & ~(1 << a->literal));
                break;

            case INT_SET_VARIABLE_TO_LITERAL:
                if(GetSimulationVariable(a->name1) != a->literal && a->name1[0] != '$') {
                    NeedRedraw = a->op;
                }
                SetSimulationVariable(a->name1, a->literal);
                break;

#ifdef USE_SFR
            case INT_READ_SFR_LITERAL:
                SetSimulationVariable(a->name1, GetAdcShadow(a->name1));
                break;

            case INT_READ_SFR_VARIABLE:
                SetSimulationVariable(a->name2, GetAdcShadow(a->name2));
                break;

            case INT_WRITE_SFR_LITERAL:
            case INT_SET_SFR_LITERAL:
            case INT_CLEAR_SFR_LITERAL:
            case INT_TEST_SFR_LITERAL:
            case INT_WRITE_SFR_VARIABLE:
            case INT_SET_SFR_VARIABLE:
            case INT_CLEAR_SFR_VARIABLE:
            case INT_TEST_SFR_VARIABLE:
            case INT_TEST_C_SFR_LITERAL:
            case INT_WRITE_SFR_LITERAL_L:
            case INT_WRITE_SFR_VARIABLE_L:
            case INT_SET_SFR_LITERAL_L:
            case INT_SET_SFR_VARIABLE_L:
            case INT_CLEAR_SFR_LITERAL_L:
            case INT_CLEAR_SFR_VARIABLE_L:
            case INT_TEST_SFR_LITERAL_L:
            case INT_TEST_SFR_VARIABLE_L:
            case INT_TEST_C_SFR_VARIABLE:
            case INT_TEST_C_SFR_LITERAL_L:
            case INT_TEST_C_SFR_VARIABLE_L:
                break;
#endif

            case INT_SET_BIN2BCD: {
                int var2 = bin2bcd(GetSimulationVariable(a->name2));
                if(GetSimulationVariable(a->name1) != var2) {
                    NeedRedraw = a->op;
                    SetSimulationVariable(a->name1, var2);
                }
                break;
            }

            case INT_SET_BCD2BIN: {
                int var2 = bcd2bin(GetSimulationVariable(a->name2));
                if(GetSimulationVariable(a->name1) != var2) {
                    NeedRedraw = a->op;
                    SetSimulationVariable(a->name1, var2);
                }
                break;
            }

            case INT_SET_OPPOSITE: {
                int var2 = opposite(GetSimulationVariable(a->name2), SizeOfVar(a->name2));
                if(GetSimulationVariable(a->name1) != var2) {
                    NeedRedraw = a->op;
                    SetSimulationVariable(a->name1, var2);
                }
                break;
            }

            case INT_SET_SWAP: {
                int var2 = swap(GetSimulationVariable(a->name2), SizeOfVar(a->name2));
                if(GetSimulationVariable(a->name1) != var2) {
                    NeedRedraw = a->op;
                    SetSimulationVariable(a->name1, var2);
                }
                break;
            }

            case INT_SET_VARIABLE_TO_VARIABLE:
                if(GetSimulationVariable(a->name1) != GetSimulationVariable(a->name2)) {
                    NeedRedraw = a->op;
                }
                SetSimulationVariable(a->name1, GetSimulationVariable(a->name2));
                break;

            case INT_INCREMENT_VARIABLE:
                Increment(a->name1, a->name2, "ROverflowFlagV");
                NeedRedraw = a->op;
                break;

            case INT_DECREMENT_VARIABLE:
                Decrement(a->name1, a->name2, "ROverflowFlagV");
                NeedRedraw = a->op;
                break;

            case INT_SET_VARIABLE_SR0:
                v = sr0(GetSimulationVariable(a->name2), GetSimulationVariable(a->name3), SizeOfVar(a->name2), &state);
                SetSingleBit(a->name4, state);
                goto math;
            case INT_SET_VARIABLE_ROL:
                v = rol(GetSimulationVariable(a->name2), GetSimulationVariable(a->name3), SizeOfVar(a->name2), &state);
                SetSingleBit(a->name4, state);
                goto math;
            case INT_SET_VARIABLE_ROR:
                v = ror(GetSimulationVariable(a->name2), GetSimulationVariable(a->name3), SizeOfVar(a->name2), &state);
                SetSingleBit(a->name4, state);
                goto math;
            case INT_SET_VARIABLE_SHL:
                v = shl(GetSimulationVariable(a->name2), GetSimulationVariable(a->name3), SizeOfVar(a->name2), &state);
                SetSingleBit(a->name4, state);
                goto math;
            case INT_SET_VARIABLE_SHR:
                v = shr(GetSimulationVariable(a->name2), GetSimulationVariable(a->name3), SizeOfVar(a->name2), &state);
                SetSingleBit(a->name4, state);
                goto math;
            case INT_SET_VARIABLE_AND:
                v = GetSimulationVariable(a->name2) & GetSimulationVariable(a->name3);
                goto math;
            case INT_SET_VARIABLE_OR:
                v = GetSimulationVariable(a->name2) | GetSimulationVariable(a->name3);
                goto math;
            case INT_SET_VARIABLE_XOR:
                v = GetSimulationVariable(a->name2) ^ GetSimulationVariable(a->name3);
                goto math;
            case INT_SET_VARIABLE_NOT:
                v = ~GetSimulationVariable(a->name2);
                goto math;
            case INT_SET_VARIABLE_RANDOM:
                v = GetRandom(a->name1);
                goto math;
            case INT_SET_SEED_RANDOM:
                v = GetSimulationVariable(a->name1);
                goto math;
            case INT_SET_VARIABLE_NEG:
                v = -GetSimulationVariable(a->name2);
                goto math;
            case INT_SET_VARIABLE_ADD:
                v = AddVariable(a->name1, a->name2, a->name3, "ROverflowFlagV");
                goto math;
            case INT_SET_VARIABLE_SUBTRACT:
                v = SubVariable(a->name1, a->name2, a->name3, "ROverflowFlagV");
                goto math;
            case INT_SET_VARIABLE_MULTIPLY:
                v = GetSimulationVariable(a->name2) * GetSimulationVariable(a->name3);
                goto math;
            case INT_SET_VARIABLE_MOD:
            case INT_SET_VARIABLE_DIVIDE:
                if(GetSimulationVariable(a->name3) != 0) {
                    if(a->op == INT_SET_VARIABLE_DIVIDE)
                        v = GetSimulationVariable(a->name2) / GetSimulationVariable(a->name3);
                    else
                        v = GetSimulationVariable(a->name2) % GetSimulationVariable(a->name3);
                } else {
                    v = 0;
                    Error(_("Division by zero; halting simulation"));
                    StopSimulation();
                }
                goto math;
            math:
                sov = SizeOfVar(a->name1);
                v = OverflowToVarSize(v, sov);
                if(GetSimulationVariable(a->name1) != v) {
                    NeedRedraw = a->op;
                    SetSimulationVariable(a->name1, v);
                }
                break;
//vvv
#define IF_BODY             \
    {                       \
        IfConditionTrue();  \
    }                       \
    else                    \
    {                       \
        IfConditionFalse(); \
    }
                //^^^
            case INT_IF_BIT_SET:
                if(SingleBitOn(a->name1))
                    IF_BODY
                break;

            case INT_IF_BIT_CLEAR:
                if(!SingleBitOn(a->name1))
                    IF_BODY
                break;

            case INT_IF_BIT_EQU_BIT:
                if(SingleBitOn(a->name1) == SingleBitOn(a->name2))
                    IF_BODY
                break;

            case INT_IF_BIT_NEQ_BIT:
                if(SingleBitOn(a->name1) != SingleBitOn(a->name2))
                    IF_BODY
                break;

            case INT_VARIABLE_SET_BIT:
            case INT_VARIABLE_CLEAR_BIT: {
                SDWORD v1, v2;
                v1 = GetSimulationVariable(a->name1);
                if(IsNumber(a->name2))
                    v2 = hobatoi(a->name2.c_str());
                else
                    v2 = GetSimulationVariable(a->name2);
                if(a->op == INT_VARIABLE_SET_BIT)
                    v1 |= 1 << v2;
                else if(a->op == INT_VARIABLE_CLEAR_BIT)
                    v1 &= ~(1 << v2);
                else
                    oops();
                if(GetSimulationVariable(a->name1) != v1) {
                    SetSimulationVariable(a->name1, v1);
                    NeedRedraw = a->op;
                }
                break;
            }

            case INT_IF_BIT_SET_IN_VAR: {
                SDWORD v1, v2;
                v1 = GetSimulationVariable(a->name1);
                if(IsNumber(a->name2))
                    v2 = hobatoi(a->name2.c_str());
                else
                    v2 = GetSimulationVariable(a->name2);
                if(v1 & (1 << v2))
                    IF_BODY
                break;
            }
            case INT_IF_BIT_CLEAR_IN_VAR: {
                SDWORD v1, v2;
                v1 = GetSimulationVariable(a->name1);
                if(IsNumber(a->name2))
                    v2 = hobatoi(a->name2.c_str());
                else
                    v2 = GetSimulationVariable(a->name2);
                if((v1 & (1 << v2)) == 0)
                    IF_BODY
                break;
            }
            case INT_IF_BITS_SET_IN_VAR:
                if((GetSimulationVariable(a->name1) & hobatoi(a->name2.c_str())) == hobatoi(a->name2.c_str()))
                    IF_BODY
                break;

            case INT_IF_BITS_CLEAR_IN_VAR:
                if((GetSimulationVariable(a->name1) & hobatoi(a->name2.c_str())) == 0)
                    IF_BODY
                break;

#ifdef NEW_CMP
            case INT_IF_GRT:
                if(GetSimulationVariable(a->name1) > GetSimulationVariable(a->name2))
                    IF_BODY
                break;

            case INT_IF_GEQ:
                if(GetSimulationVariable(a->name1) >= GetSimulationVariable(a->name2))
                    IF_BODY
                break;

            case INT_IF_LES:
                if(GetSimulationVariable(a->name1) < GetSimulationVariable(a->name2))
                    IF_BODY
                break;

            case INT_IF_LEQ:
                if(GetSimulationVariable(a->name1) <= GetSimulationVariable(a->name2))
                    IF_BODY
                break;

            case INT_IF_NEQ:
                if(GetSimulationVariable(a->name1) != GetSimulationVariable(a->name2))
                    IF_BODY
                break;

            case INT_IF_EQU:
                if(GetSimulationVariable(a->name1) == GetSimulationVariable(a->name2))
                    IF_BODY
                break;
#endif

#ifndef NEW_CMP
            case INT_IF_VARIABLE_LES_LITERAL:
                if(GetSimulationVariable(a->name1) < a->literal)
                    IF_BODY
                break;

            case INT_IF_VARIABLE_EQUALS_VARIABLE:
                if(GetSimulationVariable(a->name1) == GetSimulationVariable(a->name2))
                    IF_BODY
                break;

            case INT_IF_VARIABLE_GRT_VARIABLE:
                if(GetSimulationVariable(a->name1) > GetSimulationVariable(a->name2))
                    IF_BODY
                break;
#endif

            case INT_QUAD_ENCOD:
            case INT_SET_NPULSE:
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
                SetSingleBit(a->name1, true);
                break;

            case INT_EEPROM_READ:
            case INT_EEPROM_WRITE:
                oops();
                break;

            case INT_READ_ADC: {
                // Keep the shadow copies of the ADC variables because in
                // the real device they will not be updated until an actual
                // read is performed, which occurs only for a true rung-in
                // condition there.
                SDWORD tmp = GetSimulationVariable(a->name1);
                SetSimulationVariable(a->name1, GetAdcShadow(a->name1));
                if(tmp != GetSimulationVariable(a->name1)) {
                    NeedRedraw = a->op;
                }
                break;
            }
            case INT_UART_SEND1:
                if(SimulateUartTxCountdown == 0) {
                    SimulateUartTxCountdown = 2;
					/////	AppendToSimulationTextControl((BYTE)GetSimulationVariable(a->name1);		///// Modified by JG
                    AppendToSimulationTextControl((BYTE)GetSimulationVariable(a->name1), UartSimulationTextControl);		
                }
                break;
            case INT_UART_SEND:
                if(SingleBitOn(a->name2) && (SimulateUartTxCountdown == 0)) {
                    SimulateUartTxCountdown = 2;
                    /////	AppendToSimulationTextControl((BYTE)GetSimulationVariable(a->name1));		///// Modified by JG
                    AppendToSimulationTextControl((BYTE)GetSimulationVariable(a->name1), UartSimulationTextControl);		
                }
                if(SimulateUartTxCountdown > 0) {
                    SetSingleBit(a->name2, true); // busy
                } else {
                    SetSingleBit(a->name2, false); // not busy
                }
                break;
            case INT_UART_SEND_READY:
                if(SimulateUartTxCountdown == 0) {
                    SetSingleBit(a->name1, true); // ready
                } else {
                    SetSingleBit(a->name1, false); // not ready, busy
                }
                break;

            case INT_UART_SEND_BUSY:
                if(SimulateUartTxCountdown != 0) {
                    SetSingleBit(a->name1, true); // busy
                } else {
                    SetSingleBit(a->name1, false); // not busy, ready
                }
                break;

            case INT_UART_RECV:
                if(QueuedUartCharacter >= 0) {
                    SetSingleBit(a->name2, true);
                    SetSimulationVariable(a->name1, (SWORD)QueuedUartCharacter);
                    QueuedUartCharacter = -1;
                } else {
                    SetSingleBit(a->name2, false);
                }
                break;

            case INT_UART_RECV_AVAIL:
                if(QueuedUartCharacter >= 0) {
                    SetSingleBit(a->name1, true);
                } else {
                    SetSingleBit(a->name1, false);
                }
                break;

            case INT_END_IF:
            case INT_ELSE:
                return;

            case INT_COMMENT:
                break;

            case INT_AllocKnownAddr:
            case INT_AllocFwdAddr:
            case INT_FwdAddrIsNow:
                break;

            case INT_GOTO:
                if(a->poweredAfter) {
                    if(*(a->poweredAfter)) {
                        //IntPc = FindOpRung(INT_FwdAddrIsNow, a->literal);
                        if(a->literal)
                            IntPc = FindOpName(INT_AllocKnownAddr, a->name1);
                        else
                            IntPc = FindOpName(INT_FwdAddrIsNow, a->name1);
                    }
                }
                break;

            case INT_GOSUB:
                if(a->poweredAfter) {
                    if(*(a->poweredAfter)) {
                        PushStack(IntPc + 1);
                        //IntPc = FindOpRung(INT_FwdAddrIsNow, a->literal);
                        if(a->literal)
                            IntPc = FindOpName(INT_AllocKnownAddr, a->name1);
                        else
                            IntPc = FindOpName(INT_FwdAddrIsNow, a->name1);
                    }
                }
                break;

            case INT_RETURN:
                if(a->poweredAfter) {
                    if(*(a->poweredAfter)) {
                        IntPc = PopStack();
                    }
                }
                break;

#ifdef NEW_FEATURE
            case INT_PRINTF_STRING:
                break;
#endif

#define SPINTF(buffer, format, args) sprintf(buffer, format, #args);
            case INT_WRITE_STRING: {
                break;
            }
#ifdef TABLE_IN_FLASH
            case INT_FLASH_INIT:
                if(!GetSimulationVariable(a->name1)) {
                    SetSimulationVariable(a->name1, (SDWORD) & (a->data[0]));
                }
                break;

            case INT_FLASH_READ: {
                SDWORD *adata;
                adata = (SDWORD *)GetSimulationVariable(a->name2);
                if(adata == nullptr) {
                    Error("TABLE %s is not initialized.", a->name2.c_str());
                    StopSimulation();
                    ToggleSimulationMode(false);
                    break;
                }
                int index = GetSimulationVariable(a->name3);
                if((index < 0) || (a->literal < index)) {
                    Error("Index=%d out of range for TABLE %s[0..%d]", index, a->name2.c_str(), a->literal-1);
                    index = a->literal;
                    StopSimulation();
                    ToggleSimulationMode(false);
                    break;
                }
                SDWORD d = adata[index];
                if(GetSimulationVariable(a->name1) != d) {
                    SetSimulationVariable(a->name1, d);
                    NeedRedraw = a->op;
                }
            } break;

            case INT_RAM_READ: {
                int index = GetSimulationVariable(a->name3);
                if((index < 0) || (a->literal <= index)) {
                    Error("Index=%d out of range for string %s[%d]", index, a->name1.c_str(), a->literal);
                    index = a->literal;
                    StopSimulation();
                }
                //dbps(GetSimulationStr(a->name1.c_str()))
                char d = GetSimulationStr(a->name1.c_str())[index];
                if(GetSimulationVariable(a->name2) != d) {
                    SetSimulationVariable(a->name2, d);
                    NeedRedraw = a->op;
                }
            } break;
#endif

            case INT_DELAY:
            case INT_SLEEP:
            case INT_CLRWDT:
            case INT_LOCK:
            case INT_PWM_OFF:
                break;

			///// Added by JG
			case INT_SPI:	
				AppendToSimulationTextControl((BYTE)GetSimulationVariable(a->name2), SpiSimulationTextControl);
				if(QueuedSpiCharacter >= 0) 
				{
                    SetSimulationVariable(a->name3, (SWORD)QueuedSpiCharacter);
                    QueuedSpiCharacter = -1;
                } 
				break;

			case INT_SPI_WRITE:
				for (int i= 0 ; i < a->name2.size() ; i++)				// send text to terminal window
					AppendToSimulationTextControl(a->name2[i], SpiSimulationTextControl);		
				break;

			case INT_I2C_READ:	
				if(QueuedI2cCharacter >= 0) 
				{
                    SetSimulationVariable(a->name2, (SWORD)QueuedI2cCharacter);
                    QueuedI2cCharacter = -1;
                } 
				break;

			case INT_I2C_WRITE:	
				AppendToSimulationTextControl((BYTE)GetSimulationVariable(a->name2), I2cSimulationTextControl);

				break;
				/////

            default:
                ooops("op=%d", a->op);
                break;
        }
    }
} // SimulateIntCode()

//-----------------------------------------------------------------------------
// Called by the Windows timer that triggers cycles when we are running
// in real time.
//-----------------------------------------------------------------------------
static int updateWindow = -1;
void CALLBACK PlcCycleTimer(HWND hwnd, UINT msg, UINT_PTR id, DWORD time)
{
    int i;
    for(i = 0; i < CyclesPerTimerTick; i++) {
        SimulateOneCycle(false);
        if(CyclesPerTimerTick > 1) {
            if(updateWindow < 0) {
                updateWindow = CyclesPerTimerTick * rand() / RAND_MAX + (rand() & 1);
            } else
                updateWindow--;
        }
    }
}

//-----------------------------------------------------------------------------
// Simulate one cycle of the PLC. Update everything, and keep track of whether
// any outputs have changed. If so, force a screen refresh. If requested do
// a screen refresh regardless.
//-----------------------------------------------------------------------------
void SimulateOneCycle(bool forceRefresh)
{
    // When there is an error message up, the modal dialog makes its own
    // event loop, and there is risk that we would go recursive. So let
    // us fix that. (Note that there are no concurrency issues; we really
    // would get called recursively, not just reentrantly.)
    static bool Simulating = false;

    if(Simulating)
        return;
    Simulating = true;

    NeedRedraw = 0;

    if(SimulateUartTxCountdown > 0) {
        SimulateUartTxCountdown--;
    } else {
        SimulateUartTxCountdown = 0;
    }

    std::for_each(std::begin(IntCode), std::end(IntCode), [](IntOp &op) { op.simulated = false; });
    for(int i = 0; i < Prog.numRungs; i++) {
        Prog.rungSimulated[i] = false;
    }

    IntPc = 0;
    SimulateIntCode();

    for(uint32_t i = 0; i < IntCode.size(); i++) {
        if((IntCode[i].op != INT_AllocFwdAddr) && (IntCode[i].simulated)) {
            if((IntCode[i].rung >= 0) && (IntCode[i].rung < Prog.numRungs)) {
                Prog.rungSimulated[IntCode[i].rung] = true;
            }
        }
    }
    for(int i = 0; i < Prog.numRungs; i++) {
        if(!Prog.rungSimulated[i]) {
            Prog.rungPowered[i] = false;
            NeedRedraw = true;
        }
    }

    CyclesCount++;

    if(NeedRedraw || SimulateRedrawAfterNextCycle || forceRefresh) {
        if((updateWindow == 0) && (forceRefresh == false)) {
            UpdateWindow(MainWindow);
            updateWindow--;
        } else {
            InvalidateRect(MainWindow, nullptr, false);
        }
        ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
    }
    RefreshStatusBar();

    SimulateRedrawAfterNextCycle = false;
    if(NeedRedraw)
        SimulateRedrawAfterNextCycle = true;

    Simulating = false;
}

//-----------------------------------------------------------------------------
// Start the timer that we use to trigger PLC cycles in approximately real
// time. Independently of the given cycle time, just go at 40 Hz, since that
// is about as fast as anyone could follow by eye. Faster timers will just
// go instantly.
//-----------------------------------------------------------------------------
void StartSimulationTimer()
{
    int p = (int)(Prog.cycleTime / 1000);
    if(p < 5) {
        SetTimer(MainWindow, TIMER_SIMULATE, 10, PlcCycleTimer);
        if(Prog.cycleTime > 0)
            CyclesPerTimerTick = (int)(10000 / Prog.cycleTime);
        else
            CyclesPerTimerTick = 1;
    } else {
        SetTimer(MainWindow, TIMER_SIMULATE, p, PlcCycleTimer);
        CyclesPerTimerTick = 1;
    }
}

//-----------------------------------------------------------------------------
// Clear out all the parameters relating to the previous simulation.
//-----------------------------------------------------------------------------
void ClrSimulationData()
{
    seed = 1;
    int i;
    for(i = 0; i < VariableCount; i++) {
        Variables[i].val = 0;
        Variables[i].usedFlags = 0;
        Variables[i].initedRung = -2;
        Variables[i].initedOp = 0;
        strcpy(Variables[i].usedRungs, "");
    }
}

bool ClearSimulationData()
{
    ClrSimulationData();
    SingleBitItemsCount = 0;
    AdcShadowsCount = 0;
    QueuedUartCharacter = -1;
    SimulateUartTxCountdown = 0;
	///// Added by JG
	QueuedSpiCharacter = -1;
	QueuedI2cCharacter = -1;
	/////

    VariableCount = 0;
    CheckVariableNames(); // ??? moved to GenerateIntermediateCode()
    CyclesCount = 0;

    CheckSingleBitNegate(); // Set normal closed inputs to 1 before simulating

    SimulateRedrawAfterNextCycle = true;

    if(!GenerateIntermediateCode()) {
        ToggleSimulationMode();
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Provide a description for an item (Xcontacts, Ycoil, Rrelay, Ttimer,
// or other) in the I/O list.
//-----------------------------------------------------------------------------
void DescribeForIoList(const char *name, int type, char *out)
{
    strcpy(out, "");

    switch(type) {
        case IO_TYPE_INT_INPUT:
        case IO_TYPE_DIG_INPUT:
        case IO_TYPE_DIG_OUTPUT:
        case IO_TYPE_INTERNAL_RELAY:
        case IO_TYPE_MODBUS_COIL:
        case IO_TYPE_MODBUS_CONTACT:
            sprintf(out, "%d", SingleBitOn(name));
            break;

        case IO_TYPE_SPI_MOSI:
        case IO_TYPE_SPI_MISO:
        case IO_TYPE_SPI_SCK:
        case IO_TYPE_SPI__SS:
		case IO_TYPE_I2C_SCL:			///// Added by JG
		case IO_TYPE_I2C_SDA:			/////
            break;

        case IO_TYPE_PWM_OUTPUT:
            char s[MAX_NAME_LEN];
            sprintf(s, "$%s", name);
            sprintf(out, "%s", SingleBitOn(s) ? "ON" : "OFF");
            break;

        case IO_TYPE_STRING:
            sprintf(out, "\"%s\"", GetSimulationStr(name));
            break;

        case IO_TYPE_VAL_IN_FLASH:
        case IO_TYPE_TABLE_IN_FLASH: {
            break;
        }
        case IO_TYPE_TCY:
        case IO_TYPE_TON:
        case IO_TYPE_TOF:
        case IO_TYPE_THI:
        case IO_TYPE_TLO:
        case IO_TYPE_RTL:
        case IO_TYPE_RTO: {
            SDWORD v = GetSimulationVariable(name, true);
            double dtms = v * (Prog.cycleTime / 1000.0);
            int    sov = SizeOfVar(name);
            //v = OverflowToVarSize(v, sov);
            if(dtms < 1000) {
                if(sov == 1)
                    sprintf(out, "0x%02X = %d = %.6g ms", v & 0xff, v, dtms);
                else if(sov == 2)
                    sprintf(out, "0x%04X = %d = %.6g ms", v & 0xffff, v, dtms);
                else if(sov == 3)
                    sprintf(out, "0x%06X = %ld = %.6g ms", v & 0xFFffff, v, dtms);
                else if(sov == 4)
                    sprintf(out, "0x%08X = %ld = %.6g ms", v & 0xFFFFffff, v, dtms);
                else
                    oops();
            } else {
                if(sov == 1)
                    sprintf(out, "0x%02X = %d = %.6g s", v & 0xff, v, dtms / 1000);
                else if(sov == 2)
                    sprintf(out, "0x%04X = %d = %.6g s", v & 0xffff, v, dtms / 1000);
                else if(sov == 3)
                    sprintf(out, "0x%06X = %ld = %.6g s", v & 0xFFffff, v, dtms / 1000);
                else if(sov == 4)
                    sprintf(out, "0x%08X = %ld = %.6g s", v & 0xFFFFffff, v, dtms / 1000);
                else
                    oops();
            }
            break;
        }
        default: {
            long v = GetSimulationVariable(name, true);
            int  sov = SizeOfVar(name);
            //v = OverflowToVarSize(v, sov);
            if(sov == 1)
                sprintf(out, "0x%02X = %d = '%c'", v & 0xff, v, v);
            else if(sov == 2)
                sprintf(out, "0x%04X = %d", v & 0xffff, v);
            else if(sov == 3)
                sprintf(out, "0x%06X = %ld", v & 0xFFffff, v);
            else if(sov == 4)
                sprintf(out, "0x%08X = %ld", v & 0xFFFFffff, v);
            else {
                sprintf(out, "0x%X = %d", v, v);
            }
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
    if((name[0] == 'X') || (name[0] == 'Y')) {
      McuIoPinInfo *iop = PinInfoForName(name);
      if(iop) {
        DWORD addr = -1;
        int   bit = -1;
        MemForSingleBit(name, true, &addr, &bit);

        if((addr != -1) && (bit != -1)) {
            char s[MAX_NAME_LEN];
            if(name[0] == 'X')
                sprintf(s, "#PIN%c", 'A' + InputRegIndex(addr));
            else
                sprintf(s, "#PORT%c", 'A' + OutputRegIndex(addr));
            SDWORD v = GetSimulationVariable(s);
            if(SingleBitOn(name))
                v |= 1<<bit;
            else
                v &= ~(1<<bit);
            SetSimulationVariable(s, v);
        }
      }
    }
    ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
}

//-----------------------------------------------------------------------------
// Dialog proc for the popup that lets you interact with the UART stuff.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK UartSimulationProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_DESTROY:
            DestroySimulationWindow(UartSimulationWindow);		///// Modified by JG
            break;

        case WM_CLOSE:
            break;

        case WM_SIZE:
            MoveWindow(UartSimulationTextControl, 0, 0, LOWORD(lParam), HIWORD(lParam), true);
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
// Dialog proc for the popup that lets you interact with the SPI stuff.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK SpiSimulationProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_DESTROY:
            DestroySimulationWindow(SpiSimulationWindow);
            break;

        case WM_CLOSE:
            break;

        case WM_SIZE:
            MoveWindow(SpiSimulationTextControl, 0, 0, LOWORD(lParam), HIWORD(lParam), true);
            break;

        case WM_ACTIVATE:
            if(wParam != WA_INACTIVE) {
                SetFocus(SpiSimulationTextControl);
            }
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 1;
}

//-----------------------------------------------------------------------------
// Dialog proc for the popup that lets you interact with the I2C stuff.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK I2cSimulationProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_DESTROY:
            DestroySimulationWindow(I2cSimulationWindow);
            break;

        case WM_CLOSE:
            break;

        case WM_SIZE:
            MoveWindow(I2cSimulationTextControl, 0, 0, LOWORD(lParam), HIWORD(lParam), true);
            break;

        case WM_ACTIVATE:
            if(wParam != WA_INACTIVE) {
                SetFocus(I2cSimulationTextControl);
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
static LRESULT CALLBACK SimulationTextProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)		//// Modified by JG
{
    switch(msg) {
        case WM_KEYDOWN:
            // vvv copy-paste from ldmicro.cpp
            if(InSimulationMode) {
                switch(wParam) {
                        //                  key ' ',Enter-VK_RETURN must be available for simulation input
                        //                  case ' ':
                        //                      SimulateOneCycle(true);
                        //                      break;

                    case VK_F8:
                        StartSimulation();
                        break;

                    case 'R':
                        if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                            StartSimulation();
                        break;

                    case VK_F9:
                        StopSimulation();
                        break;

                    case 'H':
                        if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
                            StopSimulation();
                        break;

                        //                  case VK_RETURN:
                    case VK_ESCAPE:
                        ToggleSimulationMode();
                        break;
                }
                break;
            }
            // ^^^
    }

    if(msg == WM_CHAR) {
		///// Modified by JG
		if (hwnd == UartSimulationTextControl)
	        QueuedUartCharacter = (BYTE)wParam;
		if (hwnd == SpiSimulationTextControl)
	        QueuedSpiCharacter = (BYTE)wParam;
		if (hwnd == I2cSimulationTextControl)
	        QueuedI2cCharacter = (BYTE)wParam;
		/////
        return 0;
    }

	///// Modified by JG
	if (hwnd == UartSimulationTextControl)
		return CallWindowProc((WNDPROC)PrevUartTextProc, hwnd, msg, wParam, lParam);	
	if (hwnd == SpiSimulationTextControl)
		return CallWindowProc((WNDPROC)PrevSpiTextProc, hwnd, msg, wParam, lParam);	
	if (hwnd == I2cSimulationTextControl)
		return CallWindowProc((WNDPROC)PrevI2cTextProc, hwnd, msg, wParam, lParam);	
	/////
}

//-----------------------------------------------------------------------------
// Pop up the UART / SPI / I2C simulation window; like a terminal window where 
// the characters that you type go into UART / SPI / I2C RECV instruction and whatever
// the program puts into UART / SPI / I2C SEND shows up as text.
//-----------------------------------------------------------------------------
#define MAX_SCROLLBACK 0x10000 //256 // 0x10000
static char buf[MAX_SCROLLBACK] = "";

void ShowSimulationWindow(int sim)			///// Modified by JG
{
	HWND SimHwnd= nullptr;					///// Added by JG
	HWND SimCtrl= nullptr;

	///// Modified by JG
    if((sim == SIM_UART) && (UartSimulationWindow != nullptr))			
        oops();
	if((sim == SIM_SPI) && (SpiSimulationWindow != nullptr))			
        oops();
	if((sim == SIM_I2C) && (I2cSimulationWindow != nullptr))			
        oops();
	/////

    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);

    wc.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_OWNDC | CS_DBLCLKS;
	///// Modified by JG
	if (sim == SIM_UART)	
		wc.lpfnWndProc = (WNDPROC)UartSimulationProc;
	if (sim == SIM_SPI)	
		wc.lpfnWndProc = (WNDPROC)SpiSimulationProc;
	if (sim == SIM_I2C)	
		wc.lpfnWndProc = (WNDPROC)I2cSimulationProc;
	/////
    wc.hInstance = Instance;
    wc.hbrBackground = (HBRUSH)COLOR_BTNSHADOW;
	wc.lpszClassName = "LDmicroSimulationWindow";		///// Modified by JG
    wc.lpszMenuName = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassEx(&wc);
	
	///// Modified by JG
	DWORD TerminalX, TerminalY, TerminalW, TerminalH;
	
	if (sim == SIM_UART)	
	{
		ThawDWORD(TerminalX1);		// Restore window size from Windows Registry (if saved by Freeze)
		ThawDWORD(TerminalY1);
		ThawDWORD(TerminalW1);
		ThawDWORD(TerminalH1);

		TerminalX = TerminalX1, TerminalY = TerminalY1, TerminalW = TerminalW1, TerminalH = TerminalH1;
	}
	if (sim == SIM_SPI)	
	{
		ThawDWORD(TerminalX2);		// Restore window size from Windows Registry (if saved by Freeze)
		ThawDWORD(TerminalY2);
		ThawDWORD(TerminalW2);
		ThawDWORD(TerminalH2);

		TerminalX = TerminalX2, TerminalY = TerminalY2, TerminalW = TerminalW2, TerminalH = TerminalH2;
	}	
	if (sim == SIM_I2C)	
	{
		ThawDWORD(TerminalX3);		// Restore window size from Windows Registry (if saved by Freeze)
		ThawDWORD(TerminalY3);
		ThawDWORD(TerminalW3);
		ThawDWORD(TerminalH3);

		TerminalX = TerminalX3, TerminalY = TerminalY3, TerminalW = TerminalW3, TerminalH = TerminalH3;
	}	
	/////

    if(TerminalW > 800)
        TerminalW = 100;
    if(TerminalH > 800)
        TerminalH = 100;

    RECT r;
    GetClientRect(GetDesktopWindow(), &r);
    if(TerminalX >= (DWORD)(r.right - 10))
        TerminalX = 100;
    if(TerminalY >= (DWORD)(r.bottom - 10))
        TerminalY = 100;

	///// Modified by JG
	char WndName[100];
	if (sim == SIM_UART)	
		{
		fUART = fopen("uart.log", "w");
		strcpy(WndName, _("UART Simulation (Terminal)"));
		}
	if (sim == SIM_SPI)	
		{
		fSPI = fopen("spi.log", "w");
		strcpy(WndName, _("SPI Simulation (Terminal)"));
		}
	if (sim == SIM_I2C)	
		{
		fI2C = fopen("i2c.log", "w");
		strcpy(WndName, _("I2C Simulation (Terminal)"));
		}
	/////

    SimHwnd = CreateWindowClient(WS_EX_TOOLWINDOW | WS_EX_APPWINDOW,			///// Modified by JG
                                              "LDmicroSimulationWindow",			///// Common Class name
                                              WndName,								///// Window name modified by JG
                                              WS_VISIBLE | WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
                                              TerminalX,
                                              TerminalY,
                                              TerminalW,
                                              TerminalH,
                                              nullptr,
                                              nullptr,
                                              Instance,
                                              nullptr);

	///// Modified by JG
	if (sim == SIM_UART)	
		UartSimulationWindow = SimHwnd;	
	if (sim == SIM_SPI)	
		SpiSimulationWindow = SimHwnd;	
	if (sim == SIM_I2C)	
		I2cSimulationWindow = SimHwnd;	
	/////

    SimCtrl =							///// Modified by JG
        CreateWindowEx(0,
                       WC_EDIT,
                       "",
                       WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL,
                       0,
                       0,
                       TerminalW,
                       TerminalH,
                       SimHwnd,					///// Modified by JG
                       nullptr,
                       Instance,
                       nullptr);

	///// Modified by JG
	if (sim == SIM_UART)	
		UartSimulationTextControl = SimCtrl;	
	if (sim == SIM_SPI)	
		SpiSimulationTextControl = SimCtrl;	
	if (sim == SIM_I2C)	
		I2cSimulationTextControl = SimCtrl;	
	/////

    HFONT fixedFont = CreateFont(14,
                                 0,
                                 0,
                                 0,
                                 FW_REGULAR,
                                 false,
                                 false,
                                 false,
                                 ANSI_CHARSET,
                                 OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS,
                                 DEFAULT_QUALITY,
                                 FF_DONTCARE,
                                 "Lucida Console");
    if(!fixedFont)
        fixedFont = (HFONT)GetStockObject(SYSTEM_FONT);

	///// Modified by JG
	strcpy(buf, "");
	if (sim == SIM_UART)	
		{
		SendMessage((HWND)UartSimulationTextControl, WM_SETFONT, (WPARAM)fixedFont, true);		
		PrevUartTextProc = SetWindowLongPtr(UartSimulationTextControl, GWLP_WNDPROC, (LONG_PTR)SimulationTextProc);	///// Modified by JG
		SendMessage(UartSimulationTextControl, WM_SETTEXT, 0, (LPARAM)buf);
		SendMessage(UartSimulationTextControl, EM_LINESCROLL, 0, (LPARAM)INT_MAX);
		ShowWindow(UartSimulationWindow, true);
		}
	if (sim == SIM_SPI)	
		{
		SendMessage((HWND)SpiSimulationTextControl, WM_SETFONT, (WPARAM)fixedFont, true);		
		PrevSpiTextProc = SetWindowLongPtr(SpiSimulationTextControl, GWLP_WNDPROC, (LONG_PTR)SimulationTextProc);	///// Modified by JG
		SendMessage(SpiSimulationTextControl, WM_SETTEXT, 0, (LPARAM)buf);
		SendMessage(SpiSimulationTextControl, EM_LINESCROLL, 0, (LPARAM)INT_MAX);
		ShowWindow(SpiSimulationWindow, true);
		}
	if (sim == SIM_I2C)	
		{
		SendMessage((HWND)I2cSimulationTextControl, WM_SETFONT, (WPARAM)fixedFont, true);		
		PrevI2cTextProc = SetWindowLongPtr(I2cSimulationTextControl, GWLP_WNDPROC, (LONG_PTR)SimulationTextProc);	///// Modified by JG
		SendMessage(I2cSimulationTextControl, WM_SETTEXT, 0, (LPARAM)buf);
		SendMessage(I2cSimulationTextControl, EM_LINESCROLL, 0, (LPARAM)INT_MAX);
		ShowWindow(I2cSimulationWindow, true);
		}

	/////    
    
	/////    SetFocus(MainWindow);			///// Removed by JG to show simulation windows
}

//-----------------------------------------------------------------------------
// Get rid of the simulation terminal-type window.
//-----------------------------------------------------------------------------
void DestroySimulationWindow(HWND SimulationWindow)			///// Modified by JG
{
    // Try not to destroy the window if it is already destroyed; that is
    // not for the sake of the window, but so that we don't trash the
    // stored position.
    //if(SimulationWindow == nullptr) return;
    if(SimulationWindow != nullptr) {

        if((SimulationWindow == UartSimulationWindow) && (fUART))	///// Modified by JG
            fclose(fUART);
		if((SimulationWindow == SpiSimulationWindow) && (fSPI))		///// Added by JG
            fclose(fSPI);
		if((SimulationWindow == I2cSimulationWindow) && (fI2C))		///// Added by JG
            fclose(fI2C);

        DWORD TerminalX, TerminalY, TerminalW, TerminalH;
        RECT  r;

        GetClientRect(SimulationWindow, &r);
        TerminalW = r.right - r.left;
        TerminalH = r.bottom - r.top;

        GetWindowRect(SimulationWindow, &r);
        TerminalX = r.left;
        TerminalY = r.top;

		///// Modified by JG
		if(SimulationWindow == UartSimulationWindow)
		{
			TerminalX1 = TerminalX;        
			TerminalY1 = TerminalY;
			TerminalW1 = TerminalW; 
			TerminalH1 = TerminalH;

			FreezeDWORD(TerminalX1);				// Save window size to Windows Registry
			FreezeDWORD(TerminalY1);
			FreezeDWORD(TerminalW1);
			FreezeDWORD(TerminalH1);
		}
		if(SimulationWindow == SpiSimulationWindow)
		{
			TerminalX2 = TerminalX;        
			TerminalY2 = TerminalY;
			TerminalW2 = TerminalW; 
			TerminalH2 = TerminalH;

			FreezeDWORD(TerminalX2);				// Save window size to Windows Registry
			FreezeDWORD(TerminalY2);
			FreezeDWORD(TerminalW2);
			FreezeDWORD(TerminalH2);
		}
		if(SimulationWindow == I2cSimulationWindow)
		{
			TerminalX3 = TerminalX;        
			TerminalY3 = TerminalY;
			TerminalW3 = TerminalW; 
			TerminalH3 = TerminalH;

			FreezeDWORD(TerminalX3);				// Save window size to Windows Registry
			FreezeDWORD(TerminalY3);
			FreezeDWORD(TerminalW3);
			FreezeDWORD(TerminalH3);
		}

		/////
    }
	
	///// Modified by JG
	if(SimulationWindow == UartSimulationWindow)
		UartSimulationWindow = nullptr;		
	if(SimulationWindow == SpiSimulationWindow)
		SpiSimulationWindow = nullptr;		
	if(SimulationWindow == I2cSimulationWindow)
		I2cSimulationWindow = nullptr;		
	DestroyWindow(SimulationWindow);
	/////
}

//-----------------------------------------------------------------------------
// Append a received character to the terminal buffer.
//-----------------------------------------------------------------------------
static SDWORD bPrev = 0;
static void   AppendToSimulationTextControl(BYTE b, HWND SimulationTextControl)		///// Modifief by JG to fit UART / SPI / I2C
{
    char append[50];

    if((isalnum(b) || strchr("[]{};':\",.<>/?`~ !@#$%^&*()-=_+|", b) || b == '\r' || b == '\n' || b == '\b' || b == '\f'
        || b == '\t' || b == '\v' || b == '\a')
       && b != '\0') {
        append[0] = (char)b;
        append[1] = '\0';
    } else {
        sprintf(append, "\\x%02x", b);
    }

    if((SimulationTextControl == UartSimulationTextControl) && (fUART))
        fprintf(fUART, "%s", append);
	///// Added by JG
	if((SimulationTextControl == SpiSimulationTextControl) && (fSPI))
        fprintf(fSPI, "%s", append);
	if((SimulationTextControl == I2cSimulationTextControl) && (fI2C))
        fprintf(fI2C, "%s", append);
	/////

    SendMessage(SimulationTextControl, WM_GETTEXT, (WPARAM)(sizeof(buf) - 1), (LPARAM)buf);

    // vvv // This patch only for simulation mode and for WC_EDIT control.
    // Compared with Windows HyperTerminal and Putty.
    if(b == '\f') {
        // form feed as erase buf
        buf[0] = '\0';
        append[0] = '\0';
    } else if(b == '\v') {
        // vertical tab VT as LF in HyperTerminal and Putty,
        // but in simulation window "\r\n" more like as HyperTerminal and Putty.
        strcpy(append, "\r\n");
    } else if(b == '\b') {
        if(strlen(buf) > 0) {
            // backspace delete last char
            buf[strlen(buf) - 1] = '\0';
            append[0] = '\0';
        }
    } else if(b == '\r') {
        if(strlen(buf) > 0) {
            if(buf[strlen(buf) - 1] == '\n') {
                // LF CR -> CR LF
                // "\n\r" -> "\r\n"
                buf[strlen(buf) - 1] = '\0';
                strcpy(append, "\r\n");
                b = '\0';
            } else {
                append[0] = '\0'; // Now, at current cycle, '\r' is suppressed.
            }
        }
    }

    char *s;

    if(bPrev == '\r') { // Now, at the next cycle, '\r' is activated.
        if(strlen(buf) > 0) {
            if(b == '\n') {
                strcpy(append, "\r\n");
                b = '\0';
            } else {
                if(s = strrchr(buf, '\n')) {
                    s[1] = '\0';
                } else {
                    buf[0] = '\0';
                }
            }
        }
    }
    bPrev = b;
    // ^^^ // This patch only for simulation mode and for WC_EDIT control.

    int overBy = (strlen(buf) + strlen(append) + 1) - sizeof(buf);
    if(overBy > 0) {
        memmove(buf, buf + overBy, strlen(buf));
    }
    strcat(buf, append);

    SendMessage(SimulationTextControl, WM_SETTEXT, 0, (LPARAM)buf);
    SendMessage(SimulationTextControl, EM_LINESCROLL, 0, (LPARAM)INT_MAX);
}
