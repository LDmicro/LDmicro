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
// Constants, structures, declarations etc. for the ladder logic compiler
//-----------------------------------------------------------------------------

#ifndef __ELEMENTS_H__
#define __ELEMENTS_H__

#include <cstdint>
#include <vector>
#include "ldconfig.h"

#define USE_SFR

//-----------------------------------------------
// Data structures for the actual ladder logic. A rung on the ladder
// is a series subcircuit. A series subcircuit contains elements or
// parallel subcircuits. A parallel subcircuit contains elements or series
// subcircuits. An element is a set of contacts (possibly negated) or a coil.

#define MAX_ELEMENTS_IN_SUBCKT  64

#define ELEM_NULL               0x00
#define ELEM_PLACEHOLDER        0x01
#define ELEM_SERIES_SUBCKT      0x02
#define ELEM_PARALLEL_SUBCKT    0x03
#define ELEM_PADDING            0x04
#define ELEM_COMMENT            0x05

#define ELEM_CONTACTS           0x10
#define ELEM_COIL               0x11
#define ELEM_TIME2COUNT         0x1210

#define ELEM_TCY                0x1201
#define ELEM_TON                0x12
#define ELEM_TOF                0x13
#define ELEM_RTO                0x14
#define ELEM_RTL                0x1401
#define ELEM_THI                0x1410
#define ELEM_TLO                0x1420

#define ELEM_RES                0x15
#define ELEM_ONE_SHOT_RISING    0x16
#define ELEM_ONE_SHOT_FALLING   0x17
#define ELEM_ONE_SHOT_LOW       0x1701
#define ELEM_MOVE               0x18
#define ELEM_BIN2BCD            0x1801
#define ELEM_BCD2BIN            0x1802
#define ELEM_SWAP               0x1803
#define ELEM_OPPOSITE           0x1804
#define ELEM_ADD                0x19
#define ELEM_SUB                0x1a
#define ELEM_MUL                0x1b
#define ELEM_DIV                0x1c
#define ELEM_MOD                0x1c01

#define ELEM_SET_BIT            0x1c81
#define ELEM_CLEAR_BIT          0x1c82
#define ELEM_COPY_BIT           0x1c83
#define ELEM_XOR_COPY_BIT       0x1c84

#define ELEM_AND                0x1c41
#define ELEM_OR                 0x1c42
#define ELEM_XOR                0x1c43
#define ELEM_NOT                0x1c44
#define ELEM_NEG                0x1c45
#define ELEM_RANDOM             0x1c46
#define ELEM_SEED_RANDOM        0x1c47

#define ELEM_SHL                0x1c21
#define ELEM_SHR                0x1c22
#define ELEM_SR0                0x1c23
#define ELEM_ROL                0x1c24
#define ELEM_ROR                0x1c25

#define ELEM_EQU                0x1d
#define ELEM_NEQ                0x1e
#define ELEM_GRT                0x1f
#define ELEM_GEQ                0x20
#define ELEM_LES                0x21
#define ELEM_LEQ                0x22
#define ELEM_IF_BIT_SET         0x2201
#define ELEM_IF_BIT_CLEAR       0x2202

#define ELEM_CTU                0x23
#define ELEM_CTD                0x24
#define ELEM_CTC                0x25
#define ELEM_CTR                0x2501

#define ELEM_SHORT              0x26
#define ELEM_OPEN               0x27
#define ELEM_READ_ADC           0x28
#define ELEM_SET_PWM            0x29
#define ELEM_SET_PWM_SOFT       0x2901
#define ELEM_UART_RECV          0x2a
#define ELEM_UART_RECVn         0x2a01
#define ELEM_UART_RECV_AVAIL    0x2a02
#define RUartRecvErrorFlag     "RUartRecvErrorFlag"
#define ELEM_UART_SEND          0x2b
#define ELEM_UART_SENDn         0x2b01
#define ELEM_UART_SEND_READY    0x2b02
#define RUartSendErrorFlag     "RUartSendErrorFlag"
#define ELEM_MASTER_RELAY       0x2c
#define ELEM_SLEEP              0x2c01
#define ELEM_CLRWDT             0x2c02
#define ELEM_LOCK               0x2c03
#define ELEM_DELAY              0x2c04
#define ELEM_TIME2DELAY         0x2c05
#define ELEM_LABEL              0x2c20 // rung label // operate with rung only
#define ELEM_GOTO               0x2c21 // goto rung  // operate with rung only
#define ELEM_SUBPROG            0x2c22 // call rung  // operate with rung only
#define ELEM_RETURN             0x2c23
#define ELEM_ENDSUB             0x2c24
#define ELEM_GOSUB              0x2c25
#define ELEM_SHIFT_REGISTER     0x2d
#define ELEM_LOOK_UP_TABLE      0x2e
#define ELEM_FORMATTED_STRING   0x2f
#define ELEM_PERSIST            0x30
#define ELEM_PIECEWISE_LINEAR   0x31

#ifdef USE_SFR
#define ELEM_RSFR               0x32    // Element read from SFR
#define ELEM_WSFR               0x33    // Element write to  SFR
#define ELEM_SSFR               0x34    // Element set bit in SFR
#define ELEM_CSFR               0x35    // Element clear bit in SFR
#define ELEM_TSFR               0x36    // Element test if set bit in SFR
#define ELEM_T_C_SFR            0x37    // Element test if clear bit in SFR
#endif

#define ELEM_CPRINTF            0x4c01
#define ELEM_SPRINTF            0x4c02
#define ELEM_FPRINTF            0x4c03
#define ELEM_PRINTF             0x4c04
#define ELEM_I2C_CPRINTF        0x4c05
#define ELEM_ISP_CPRINTF        0x4c06
#define ELEM_UART_CPRINTF       0x4c07

#define ELEM_STRING             0x3f
#define ELEM_OSC                0x4001
#define ELEM_STEPPER            0x4002   //
#define ELEM_PULSER             0x4003   //
#define ELEM_NPULSE             0x4004   // N pulse generator use timer0 for generate meander
#define ELEM_NPULSE_OFF         0x4005
//#define ELEM_PWM_OFF          0x4006
#define ELEM_PWM_OFF_SOFT       0x4007
#define ELEM_QUAD_ENCOD         0x4009

#define ELEM_SPI                0x6001

#define ELEM_BUS                0x7001
#define ELEM_7SEG               0x7007
#define ELEM_9SEG               0x7009
#define ELEM_14SEG              0x7014
#define ELEM_16SEG              0x7016

// clang-format on

#define CASE_LEAF \
        case ELEM_PLACEHOLDER: \
        case ELEM_COMMENT: \
        case ELEM_COIL: \
        case ELEM_CONTACTS: \
        case ELEM_TIME2COUNT: \
        case ELEM_TIME2DELAY: \
        case ELEM_TON: \
        case ELEM_TOF: \
        case ELEM_RTO: \
        case ELEM_RTL: \
        case ELEM_THI: \
        case ELEM_TLO: \
        case ELEM_TCY: \
        case ELEM_CTD: \
        case ELEM_CTU: \
        case ELEM_CTC: \
        case ELEM_CTR: \
        case ELEM_RES: \
        case ELEM_ONE_SHOT_RISING: \
        case ELEM_ONE_SHOT_FALLING: \
        case ELEM_ONE_SHOT_LOW: \
        case ELEM_OSC: \
        case ELEM_STEPPER: \
        case ELEM_PULSER: \
        case ELEM_NPULSE: \
        case ELEM_NPULSE_OFF: \
        case ELEM_PWM_OFF_SOFT: \
        case ELEM_SPI: \
        case ELEM_BUS: \
        case ELEM_7SEG: \
        case ELEM_9SEG: \
        case ELEM_14SEG: \
        case ELEM_16SEG: \
        case ELEM_EQU: \
        case ELEM_NEQ: \
        case ELEM_GRT: \
        case ELEM_GEQ: \
        case ELEM_LES: \
        case ELEM_LEQ: \
        case ELEM_IF_BIT_SET: \
        case ELEM_IF_BIT_CLEAR: \
        case ELEM_SET_BIT: \
        case ELEM_CLEAR_BIT: \
        case ELEM_COPY_BIT: \
        case ELEM_XOR_COPY_BIT: \
        case ELEM_SHL: \
        case ELEM_SHR: \
        case ELEM_ROL: \
        case ELEM_ROR: \
        case ELEM_SR0: \
        case ELEM_AND: \
        case ELEM_OR: \
        case ELEM_XOR: \
        case ELEM_NOT: \
        case ELEM_NEG: \
        case ELEM_RANDOM: \
        case ELEM_SEED_RANDOM: \
        case ELEM_ADD: \
        case ELEM_SUB: \
        case ELEM_MUL: \
        case ELEM_DIV: \
        case ELEM_MOD: \
        case ELEM_BIN2BCD: \
        case ELEM_BCD2BIN: \
        case ELEM_SWAP: \
        case ELEM_OPPOSITE: \
        case ELEM_MOVE: \
        case ELEM_SHORT: \
        case ELEM_OPEN: \
        case ELEM_READ_ADC: \
        case ELEM_SET_PWM: \
        case ELEM_SET_PWM_SOFT: \
        case ELEM_QUAD_ENCOD: \
        case ELEM_UART_SEND: \
        case ELEM_UART_SENDn: \
        case ELEM_UART_SEND_READY: \
        case ELEM_UART_RECV: \
        case ELEM_UART_RECVn: \
        case ELEM_UART_RECV_AVAIL: \
        case ELEM_MASTER_RELAY: \
        case ELEM_SLEEP: \
        case ELEM_CLRWDT: \
        case ELEM_LOCK: \
        case ELEM_LABEL: \
        case ELEM_GOTO: \
        case ELEM_SUBPROG: \
        case ELEM_RETURN: \
        case ELEM_ENDSUB: \
        case ELEM_GOSUB: \
        case ELEM_DELAY: \
        case ELEM_SHIFT_REGISTER: \
        case ELEM_LOOK_UP_TABLE: \
        case ELEM_PIECEWISE_LINEAR: \
        case ELEM_STRING: \
        case ELEM_CPRINTF: \
        case ELEM_SPRINTF: \
        case ELEM_FPRINTF: \
        case ELEM_PRINTF: \
        case ELEM_I2C_CPRINTF: \
        case ELEM_ISP_CPRINTF: \
        case ELEM_UART_CPRINTF: \
        case ELEM_FORMATTED_STRING: \
        case ELEM_PERSIST: \
        case ELEM_RSFR: \
        case ELEM_WSFR: \
        case ELEM_SSFR: \
        case ELEM_CSFR: \
        case ELEM_TSFR: \
        case ELEM_T_C_SFR:

struct ElemSubcktParallel;
struct ElemSubcktSeries;

typedef struct ElemCommentTag {
    char    str[MAX_COMMENT_LEN];
} ElemComment;

typedef struct ElemContactsTag {
    char    name[MAX_NAME_LEN]; // All named "name[]" fields must be in first position in typedef structs!!!
    bool    negated;
    bool    set1; // set HI input level before Simnlation mode
} ElemContacts;

typedef struct ElemCoilTag {
    char    name[MAX_NAME_LEN];
    bool    negated;
    bool    setOnly;
    bool    resetOnly;
    bool    ttrigger;
} ElemCoil;

typedef struct ElemTimeTag {
    char    name[MAX_NAME_LEN];
  //SDWORD  delay; // us
    char    delay[MAX_NAME_LEN]; // us
    int     adjust; // adjust timer delay, default = 0, typical = -1
    // timer_delay = delay + adjust * PLC_cycle_time
} ElemTimer;

typedef struct ElemResetTag {
    char    name[MAX_NAME_LEN];
} ElemReset;

typedef struct ElemMoveTag {
    char    dest[MAX_NAME_LEN];
    char    src[MAX_NAME_LEN];
} ElemMove;

typedef struct ElemCmpTag {
    char    op1[MAX_NAME_LEN];
    char    op2[MAX_NAME_LEN];
} ElemCmp;

typedef struct ElemSfrTag {
    char    sfr[MAX_NAME_LEN];
    char    op[MAX_NAME_LEN];
} ElemSfr;

#define COMMON_CATHODE ('C')
#define COMMON_ANODE   ('A')
#define PCBbit_LEN 17

typedef struct ElemSegmentsTag {
    char    dest[MAX_NAME_LEN];
    char    src[MAX_NAME_LEN];
    char    common;
    int     which;
} ElemSegments;

typedef struct ElemSpiTag {
    char    name[MAX_NAME_LEN];
    char    send[MAX_NAME_LEN];
    char    recv[MAX_NAME_LEN];
    char    mode[MAX_NAME_LEN];
    char    bitrate[MAX_NAME_LEN];
    char    modes[MAX_NAME_LEN];
    char    size[MAX_NAME_LEN];
    char    first[MAX_NAME_LEN];
} ElemSpi;

typedef struct ElemBusTag {
    char    dest[MAX_NAME_LEN];
    char    src[MAX_NAME_LEN];
    int     PCBbit[PCBbit_LEN];
} ElemBus;

typedef struct ElemMathTag {
    char    dest[MAX_NAME_LEN];
    char    op1[MAX_NAME_LEN];
    char    op2[MAX_NAME_LEN];
} ElemMath;

typedef struct ElemGotoTag {
    char    label[MAX_NAME_LEN]; // rung number or rung symbol label
} ElemGoto;

typedef struct ElemCounterTag {
    char    name[MAX_NAME_LEN];
    char    max[MAX_NAME_LEN];
    char    init[MAX_NAME_LEN];
    char    inputKind; // '-' Direct static logic input. Active at external 1.
                       // 'o' Inverse static logic input. Logic negation at input. External 0 produces internal 1.
                       // '/' Dynamic input active on 0 to 1 transition for positive logic.
                       // '\' Dynamic input active on 1 to 0 transition for positive logic.

} ElemCounter;

typedef struct ElemAccelTag {
    //INPUT or OUTPUT DATAS
    //If s is input and ds=1 then t is output.
    //If t is input and dt=1 then s is output.
    double  s;      // пройденый путь от 0 s(t)
    int32_t  si;     // -/- целое от s
    double  ds;     // приращение пути от предыдущей точки ds=s[i]-s[i-1]
    int32_t  dsi;    // -/- целое от ds не int(s[i]-s[i-1])

    double  t;      // время разгона от 0 до v=1
    int32_t  ti;     // -/- целое от t
    double  dt;     // приращение времени от предыдущей точки dt=t[i]-t[i-1]
    int32_t  dti;    // -/- целое от dt
    int32_t  dtMul;  // dtMul = dt * mult;
    int32_t  dtShr;  // dtShr = dtMul >> shrt;
    int32_t  tdiscrete;//tdti;   // =summa(0..dti)
    //OUTPUT DATAS
    double  v,      // скорость в текущей точке разгона
            dv,     // приращение скорости в текущей точке разгона
            vdiscrete, // dsi/dti
            a,      // ускорение в текущей точке разгона
            da,     // приращение ускорения в текущей точке разгона
            e,      // энергия в текущей точке разгона
            de;     // приращение энергии в текущей точке разгона

} ElemAccel;//, *ElemAccelPointer; //структура и указательна структуру

typedef struct ResStepsTag {
    ElemAccel *T;
    int n;
    int Psum;
    int shrt; // mult = 2 ^ shrt
    int sovElement;
} ResSteps;

typedef struct ElemStepperTag {
    char    name[MAX_NAME_LEN]; // step counter down from counter limit to 0
    char    max[MAX_NAME_LEN];  // step counter limit
    char    P[MAX_NAME_LEN];
    int     nSize;              // Table size:
    int     n;                  // real accelaration/decelaratin table size
    int     graph;
    char    coil[MAX_NAME_LEN]; // short pulse on this pin
} ElemStepper;

typedef struct ElemPulserTag {
    char    counter[MAX_NAME_LEN];
    char    P1[MAX_NAME_LEN];
    char    P0[MAX_NAME_LEN];
    char    accel[MAX_NAME_LEN];
    char    busy[MAX_NAME_LEN];
} ElemPulser;

typedef struct ElemNPulseTag {
    char    counter[MAX_NAME_LEN];
    char    targetFreq[MAX_NAME_LEN];
    char    coil[MAX_NAME_LEN];
} ElemNPulse;

typedef struct ElemReadAdcTag {
    char    name[MAX_NAME_LEN];
    int     refs; // REFS1:0 for AVR // PCFG3:0 for PIC
} ElemReadAdc;

typedef struct ElemSetPwmTag {
    char    duty_cycle[MAX_NAME_LEN];
    char    targetFreq[MAX_NAME_LEN];
    char    name[MAX_NAME_LEN]; // for IO pin
    char    resolution[MAX_NAME_LEN]; // 0-100% (6.7 bits), 0-256 (8 bits), 0-512 (9 bits), 0-1024 (10 bits)
} ElemSetPwm;

typedef struct ElemQuadEncodTag {
    char    counter[MAX_NAME_LEN];
    int     int01; // inputA
    char    inputA[MAX_NAME_LEN];
    char    inputB[MAX_NAME_LEN];
    char    inputZ[MAX_NAME_LEN];
    char    inputZKind; // '/\-o'
    int     countPerRevol; // counter ticks per revolution
    char    dir[MAX_NAME_LEN];
} ElemQuadEncod;

typedef struct ElemUartTag {
    char    name[MAX_NAME_LEN];
    int     bytes; // Number of bytes to transmit, default is 1 (backward compatible).
    bool    wait;  // Wait until all bytes are transmitted
                   //  in the same cycle of the PLC in which it started.
                   //  or transmit a one byte per a one PLC cycle,
                   // Default is false.
                   // The 'wait' parameter is not used when receiving the one byte.
} ElemUart;

typedef struct ElemShiftRegisterTag {
    char    name[MAX_NAME_LEN];
    int     stages;
} ElemShiftRegister;

typedef struct ElemLookUpTableTag {
    char    name[MAX_NAME_LEN];
    char    dest[MAX_NAME_LEN];
    char    index[MAX_NAME_LEN];
    int     count; // Table size
    bool    editAsString;
    int32_t  vals[MAX_LOOK_UP_TABLE_LEN];
} ElemLookUpTable;

typedef struct ElemPiecewiseLinearTag {
    char    name[MAX_NAME_LEN];
    char    dest[MAX_NAME_LEN];
    char    index[MAX_NAME_LEN];
    int     count;
    int32_t  vals[MAX_LOOK_UP_TABLE_LEN];
} ElemPiecewiseLinear;

typedef struct ElemFormattedStringTag {
    char    var[MAX_NAME_LEN]; // also a varsList
    char    string[MAX_STRING_LEN];
    char    dest[MAX_NAME_LEN]; // also a CHAR
    char    enable[MAX_NAME_LEN]; // bitVar
    char    error[MAX_NAME_LEN];  // bitVar
} ElemFormattedString;

typedef struct ElemPerisistTag {
    char    var[MAX_NAME_LEN];
} ElemPersist;

#define SELECTED_NONE       0
#define SELECTED_ABOVE      1
#define SELECTED_BELOW      2
#define SELECTED_RIGHT      3
#define SELECTED_LEFT       4
struct ElemLeaf {
    int     selectedState;
    bool    poweredAfter;
    bool    workingNow;
    union {
        ElemComment         comment;
        ElemContacts        contacts;
        ElemCoil            coil;
        ElemTimer           timer;
        ElemReset           reset;
        ElemMove            move;
        ElemMath            math;
        ElemCmp             cmp;
        ElemGoto            doGoto;
        ElemSfr             sfr;
        ElemSpi             spi;
        ElemBus             bus;
        ElemSegments        segments;
        ElemStepper         stepper;
        ElemPulser          pulser;
        ElemNPulse          Npulse;
        ElemQuadEncod       QuadEncod;
        ElemCounter         counter;
        ElemReadAdc         readAdc;
        ElemSetPwm          setPwm;
        ElemUart            uart;
        ElemShiftRegister   shiftRegister;
        ElemFormattedString fmtdStr;
        ElemLookUpTable     lookUpTable;
        ElemPiecewiseLinear piecewiseLinear;
        ElemPersist         persist;
    } d;
};

struct SeriesNode
{
    SeriesNode() : which(0), parent_(nullptr) {data.any = nullptr;}
    SeriesNode(int w, void* any, SeriesNode* parent = nullptr) : which(w), parent_(parent) {data.any = any;}
    int     which;
    union {
        void               *any;
        ElemSubcktParallel *parallel;
        ElemSubcktSeries   *series; // used in the Copy-Paste command
        ElemLeaf           *leaf;
    } data;
    SeriesNode* parent_;

    SeriesNode*       parent() {return parent_;}
    const SeriesNode* parent() const {return parent_;}
    ElemLeaf*         leaf  () {return data.leaf;}
    const ElemLeaf*   leaf  () const {return data.leaf;}
    void*             any   () {return data.any;}
    const void*       any   () const {return data.any;}

    ElemSubcktSeries*         series() {return data.series;}
    const ElemSubcktSeries*   series() const {return data.series;}
    ElemSubcktParallel*       parallel() {return data.parallel;}
    const ElemSubcktParallel* parallel() const {return data.parallel;}
};

struct ElemSubcktSeries {
    SeriesNode contents[MAX_ELEMENTS_IN_SUBCKT];
    int count;
} ;

struct ElemSubcktParallel {
    SeriesNode contents[MAX_ELEMENTS_IN_SUBCKT];
    int count;
};

void AddTimer(int which);
void AddCoil(int what);
void AddContact(int what);
void AddEmpty(int which);
void AddMove();
void AddBus(int which);
void AddBcd(int which);
void AddSegments(int which);
void AddStepper();
void AddPulser();
void AddNPulse();
void AddQuadEncod();
void AddSfr(int which);
void AddBitOps(int which);
void AddMath(int which);
void AddCmp(int which);
void AddReset();
void AddCounter(int which);
void AddReadAdc();
void AddRandom();
void AddSeedRandom();
void AddSetPwm();
void AddSpi(int which);
void AddUart(int which);
void AddPersist();
void AddComment(const char *text);
void AddShiftRegister();
void AddMasterRelay();
void AddSleep();
void AddClrWdt();
void AddDelay();
void AddLock();
void AddGoto(int which);
void AddLookUpTable();
void AddPiecewiseLinear();
void AddFormattedString();
void AddString();
void AddPrint(int code);
void DeleteSelectedFromProgram();
void DeleteSelectedRung();
bool CollapseUnnecessarySubckts(int which, void *any);
void InsertRung(bool afterCursor);
int RungContainingSelected();
bool ItemIsLastInCircuit(ElemLeaf *item);
int FindRung(int seek, char *name);
int FindRungLast(int seek, char *name);
int CountWhich(int seek1, int seek2, int seek3, char *name);
int CountWhich(int seek1, int seek2, char *name);
int CountWhich(int seek1, char *name);
int CountWhich(int seek1);
int AdcFunctionUsed();
int PwmFunctionUsed();
uint32_t QuadEncodFunctionUsed();
bool NPulseFunctionUsed();
bool EepromFunctionUsed();
bool SleepFunctionUsed();
bool DelayUsed();
bool TablesUsed();
void PushRungUp();
void PushRungDown();
void CopyRungDown();
void CutRung();
void CopyRung();
void CopyElem();
void PasteRung(int PasteTo);
void NewProgram();
ElemLeaf *AllocLeaf();
ElemSubcktSeries *AllocSubcktSeries();
ElemSubcktParallel *AllocSubcktParallel();
void FreeCircuit(int which, void *any);
void FreeEntireProgram();
ElemLeaf *ContainsWhich(int which, void *any, int seek1, int seek2, int seek3);
ElemLeaf *ContainsWhich(int which, void *any, int seek1, int seek2);
ElemLeaf *ContainsWhich(int which, void *any, int seek1);
void RenameSet1(int which, char *name, char *new_name, bool set1);
void *FindElem(int which, char *name);

//bool ContainsWhich(int which, void *any, int seek1, int seek2, int seek3);
//bool ContainsWhich(int which, void *any, int seek1, int seek2);
//bool ContainsWhich(int which, void *any, int seek1);

class Circuit
{
public:
    Circuit();
    ~Circuit();

public:

private:
    std::vector<SeriesNode> elements_;
};

#endif //__ELEMENTS_H__
