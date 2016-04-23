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
// Constants, structures, declarations etc. for the PIC ladder logic compiler
// Jonathan Westhues, Oct 2004
//-----------------------------------------------------------------------------

#ifndef __LDMICRO_H
#define __LDMICRO_H

#include <setjmp.h>
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <errno.h>

#include "current_function.hpp"

typedef signed short SWORD;
typedef signed long SDWORD;

#define _BV(bit) (1 << (bit))

//-----------------------------------------------
// `Configuration options.'

// The library that I use to do registry stuff.
#define FREEZE_SUBKEY "LDMicro"

// Size of the font that we will use to draw the ladder diagrams, in pixels
#define FONT_WIDTH   7
#define FONT_HEIGHT 13


//-----------------------------------------------
// Constants for the GUI. We have drop-down menus, a listview for the I/Os,
// etc.

// Menu IDs

#define MNU_NEW                 0x01
#define MNU_OPEN                0x02
#define MNU_SAVE                0x03
#define MNU_SAVE_AS             0x04
#define MNU_EXPORT              0x05
#define MNU_EXIT                0x06

#define MNU_NOTEPAD_LD          0x0700
#define MNU_NOTEPAD_PL          0x0701
#define MNU_NOTEPAD_ASM         0x0702
#define MNU_NOTEPAD_C           0x0703
#define MNU_NOTEPAD_H           0x0704
#define MNU_NOTEPAD_PAS         0x0705
#define MNU_NOTEPAD_TXT         0x070F

#define MNU_UNDO                0x10
#define MNU_REDO                0x11
#define MNU_PUSH_RUNG_UP        0x12
#define MNU_PUSH_RUNG_DOWN      0x13
#define MNU_INSERT_RUNG_BEFORE  0x14
#define MNU_INSERT_RUNG_AFTER   0x15
#define MNU_DELETE_ELEMENT      0x16
#define MNU_DELETE_RUNG         0x17

#define MNU_CAT_RUNG            0x1801
#define MNU_COPY_RUNG_DOWN      0x1802
#define MNU_COPY_RUNG           0x1803
#define MNU_COPY_ELEM           0x1804
#define MNU_PASTE_RUNG          0x1805
#define MNU_PASTE_INTO_RUNG     0x1806

#define MNU_SCROLL_DOWN         0x1901
#define MNU_SCROLL_UP           0x1902
#define MNU_SCROLL_PgDOWN       0x1903
#define MNU_SCROLL_PgUP         0x1904
#define MNU_ROLL_HOME           0x1905
#define MNU_ROLL_END            0x1906

#define MNU_INSERT_COMMENT      0x20
#define MNU_INSERT_CONTACTS     0x21
#define MNU_INSERT_COIL         0x22
#define MNU_INSERT_TON          0x23
#define MNU_INSERT_TOF          0x24
#define MNU_INSERT_RTO          0x25
#define MNU_INSERT_RES          0x26
#define MNU_INSERT_OSR          0x27
#define MNU_INSERT_OSF          0x28
#define MNU_INSERT_CTU          0x29
#define MNU_INSERT_CTD          0x2a
#define MNU_INSERT_CTC          0x2b
#define MNU_INSERT_CTR          0x2b01
#define MNU_INSERT_ADD          0x2c
#define MNU_INSERT_SUB          0x2d
#define MNU_INSERT_MUL          0x2e
#define MNU_INSERT_DIV          0x2f
#define MNU_INSERT_MOD          0x2f01
#define MNU_INSERT_AND          0x2f02
#define MNU_INSERT_OR           0x2f03
#define MNU_INSERT_XOR          0x2f04
#define MNU_INSERT_NOT          0x2f05
#define MNU_INSERT_NEG          0x2f06
#define MNU_INSERT_SHL          0x2f07
#define MNU_INSERT_SHR          0x2f08
#define MNU_INSERT_SR0          0x2f09
#define MNU_INSERT_ROL          0x2f0a
#define MNU_INSERT_ROR          0x2f0b
#define MNU_INSERT_MOV          0x30
#define MNU_INSERT_BIN2BCD      0x3001
#define MNU_INSERT_BCD2BIN      0x3002
#define MNU_INSERT_SWAP         0x3003
#define MNU_INSERT_READ_ADC     0x31
#define MNU_INSERT_SET_PWM      0x32
#define MNU_INSERT_SET_PWM_SOFT 0x3201
#define MNU_INSERT_UART_SEND    0x33
#define MNU_INSERT_UART_RECV    0x34
#define MNU_INSERT_EQU          0x35
#define MNU_INSERT_NEQ          0x36
#define MNU_INSERT_GRT          0x37
#define MNU_INSERT_GEQ          0x38
#define MNU_INSERT_LES          0x39
#define MNU_INSERT_LEQ          0x3a
#define MNU_INSERT_OPEN         0x3b
#define MNU_INSERT_SHORT        0x3c
#define MNU_INSERT_MASTER_RLY   0x3d
#define MNU_INSERT_SHIFT_REG    0x3e
#define MNU_INSERT_LUT          0x3f
#define MNU_INSERT_FMTD_STR     0x40
#define MNU_INSERT_PERSIST      0x41
#define MNU_MAKE_NORMAL         0x42
#define MNU_NEGATE              0x43
#define MNU_MAKE_SET_ONLY       0x44
#define MNU_MAKE_RESET_ONLY     0x45
#define MNU_INSERT_PWL          0x46

#define MNU_INSERT_SFR          0x47    // special function register read
#define MNU_INSERT_SFW          0x48    // special function register write
#define MNU_INSERT_SSFB         0x49    // set bit in special function register
#define MNU_INSERT_csFB         0x4a    // clear bit in special function register
#define MNU_INSERT_TSFB         0x4b    // test if bit is set in special function register
#define MNU_INSERT_T_C_SFB      0x4c    // test if bit is clear in special function register

#define MNU_INSERT_STRING       0x4d
#define MNU_INSERT_OSC          0x4f01
#define MNU_INSERT_STEPPER      0x4f02
#define MNU_INSERT_PULSER       0x4f03
#define MNU_INSERT_NPULSE       0x4f04
#define MNU_INSERT_NPULSE_OFF   0x4f05
#define MNU_INSERT_PWM_OFF      0x4f06
#define MNU_INSERT_PWM_OFF_SOFT 0x4f07
#define MNU_INSERT_UART_UDRE    0x4f08
#define MNU_INSERT_QUAD_ENCOD   0x4f09
#define MNU_INSERT_MASTER_BREAK 0x4f0a

#define MNU_MCU_SETTINGS        0x50
#define MNU_SPEC_FUNCTION       0x51
#define MNU_PROCESSOR_0         0xa0

#define MNU_SIMULATION_MODE     0x60
#define MNU_START_SIMULATION    0x61
#define MNU_STOP_SIMULATION     0x62
#define MNU_SINGLE_CYCLE        0x63

#define MNU_INSERT_7SEG         0x6507
#define MNU_INSERT_9SEG         0x6509
#define MNU_INSERT_14SEG        0x6514
#define MNU_INSERT_16SEG        0x6516

#define MNU_COMPILE             0x70
#define MNU_COMPILE_AS          0x71
#define MNU_COMPILE_ANSIC       0x72
#define MNU_COMPILE_IHEX        0x73
#define MNU_COMPILE_ASM         0x74
#define MNU_COMPILE_PASCAL      0x75
#define MNU_COMPILE_ARDUINO     0x76
#define MNU_COMPILE_CAVR        0x77
#define MNU_FLASH_BAT           0x7E
#define MNU_READ_BAT            0x7F

#define MNU_MANUAL              0x80
#define MNU_ABOUT               0x81
#define MNU_RELEASE             0x82

#define MNU_COMPILE_XINT        0x83	// Extended interpreter

// Columns within the I/O etc. listview.
#define LV_IO_NAME              0x00
#define LV_IO_TYPE              0x01
#define LV_IO_STATE             0x02
#define LV_IO_PIN               0x03
#define LV_IO_PORT              0x04
#define LV_IO_PINNAME           0x05
#define LV_IO_MODBUS            0x06

// Timer IDs associated with the main window.
#define TIMER_BLINK_CURSOR      1
#define TIMER_SIMULATE          2

//-----------------------------------------------
// Data structures for the actual ladder logic. A rung on the ladder
// is a series subcircuit. A series subcircuit contains elements or
// parallel subcircuits. A parallel subcircuit contains elements or series
// subcircuits. An element is a set of contacts (possibly negated) or a coil.

#define MAX_ELEMENTS_IN_SUBCKT  64

#define ELEM_PLACEHOLDER        0x01
#define ELEM_SERIES_SUBCKT      0x02
#define ELEM_PARALLEL_SUBCKT    0x03
#define ELEM_PADDING            0x04
#define ELEM_COMMENT            0x05

#define ELEM_CONTACTS           0x10
#define ELEM_COIL               0x11
#define ELEM_TON                0x12
#define ELEM_TOF                0x13
#define ELEM_RTO                0x14
#define ELEM_RES                0x15
#define ELEM_ONE_SHOT_RISING    0x16
#define ELEM_ONE_SHOT_FALLING   0x17
#define ELEM_MOVE               0x18
#define ELEM_BIN2BCD            0x1801
#define ELEM_BCD2BIN            0x1802
#define ELEM_SWAP               0x1803
#define ELEM_ADD                0x19
#define ELEM_SUB                0x1a
#define ELEM_MUL                0x1b
#define ELEM_DIV                0x1c
#define ELEM_MOD                0x1c01
#define ELEM_AND                0x1c02
#define ELEM_OR                 0x1c03
#define ELEM_XOR                0x1c04
#define ELEM_NOT                0x1c05
#define ELEM_NEG                0x1c06
#define ELEM_SHL                0x1c07
#define ELEM_SHR                0x1c08
#define ELEM_SR0                0x1c09
#define ELEM_ROL                0x1c0a
#define ELEM_ROR                0x1c0b
#define ELEM_EQU                0x1d
#define ELEM_NEQ                0x1e
#define ELEM_GRT                0x1f
#define ELEM_GEQ                0x20
#define ELEM_LES                0x21
#define ELEM_LEQ                0x22
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
#define ELEM_UART_SEND          0x2b
#define ELEM_MASTER_RELAY       0x2c
#define ELEM_SHIFT_REGISTER     0x2d
#define ELEM_LOOK_UP_TABLE      0x2e
#define ELEM_FORMATTED_STRING   0x2f
#define ELEM_PERSIST            0x30
#define ELEM_PIECEWISE_LINEAR   0x31

#define ELEM_RSFR               0x32    // Element read from SFR
#define ELEM_WSFR               0x33    // Element write to  SFR
#define ELEM_SSFR               0x34    // Element set bit in SFR
#define ELEM_CSFR               0x35    // Element clear bit in SFR
#define ELEM_TSFR               0x36    // Element test if set bit in SFR
#define ELEM_T_C_SFR            0x37    // Element test if clear bit in SFR

#define ELEM_STRING             0x3f
#define ELEM_MASTER_BREAK       0x4000
#define ELEM_OSC                0x4001
#define ELEM_STEPPER            0x4002   //
#define ELEM_PULSER             0x4003   //
#define ELEM_NPULSE             0x4004   // N pulse generator use timer0 for generate meander
#define ELEM_NPULSE_OFF         0x4005
#define ELEM_PWM_OFF            0x4006
#define ELEM_PWM_OFF_SOFT       0x4007
#define ELEM_UART_UDRE          0x4008
#define ELEM_QUAD_ENCOD         0x4009

#define ELEM_7SEG               0x7007
#define ELEM_9SEG               0x7009
#define ELEM_14SEG              0x7014
#define ELEM_16SEG              0x7016

#define CASE_LEAF \
        case ELEM_PLACEHOLDER: \
        case ELEM_COMMENT: \
        case ELEM_COIL: \
        case ELEM_CONTACTS: \
        case ELEM_TON: \
        case ELEM_TOF: \
        case ELEM_RTO: \
        case ELEM_CTD: \
        case ELEM_CTU: \
        case ELEM_CTC: \
        case ELEM_CTR: \
        case ELEM_RES: \
        case ELEM_ONE_SHOT_RISING: \
        case ELEM_ONE_SHOT_FALLING: \
        case ELEM_OSC: \
        case ELEM_STEPPER: \
        case ELEM_PULSER: \
        case ELEM_NPULSE: \
        case ELEM_NPULSE_OFF: \
        case ELEM_PWM_OFF: \
        case ELEM_PWM_OFF_SOFT: \
        case ELEM_RSFR: \
        case ELEM_WSFR: \
        case ELEM_SSFR: \
        case ELEM_CSFR: \
        case ELEM_TSFR: \
        case ELEM_T_C_SFR: \
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
        case ELEM_SHL: \
        case ELEM_SHR: \
        case ELEM_SR0: \
        case ELEM_ROL: \
        case ELEM_ROR: \
        case ELEM_AND: \
        case ELEM_OR: \
        case ELEM_XOR: \
        case ELEM_NOT: \
        case ELEM_NEG: \
        case ELEM_ADD: \
        case ELEM_SUB: \
        case ELEM_MUL: \
        case ELEM_DIV: \
        case ELEM_MOD: \
        case ELEM_BIN2BCD: \
        case ELEM_BCD2BIN: \
        case ELEM_SWAP: \
        case ELEM_MOVE: \
        case ELEM_SHORT: \
        case ELEM_OPEN: \
        case ELEM_READ_ADC: \
        case ELEM_SET_PWM: \
        case ELEM_SET_PWM_SOFT: \
        case ELEM_QUAD_ENCOD: \
        case ELEM_UART_UDRE: \
        case ELEM_UART_SEND: \
        case ELEM_UART_RECV: \
        case ELEM_MASTER_RELAY: \
        case ELEM_MASTER_BREAK: \
        case ELEM_SHIFT_REGISTER: \
        case ELEM_LOOK_UP_TABLE: \
        case ELEM_PIECEWISE_LINEAR: \
        case ELEM_STRING: \
        case ELEM_FORMATTED_STRING: \
        case ELEM_PERSIST:

#define MAX_NAME_LEN                128
#define MAX_COMMENT_LEN             384
#define MAX_LOOK_UP_TABLE_LEN        64
#define MAX_SHIFT_REGISTER_STAGES   256
#define MAX_STRING_LEN              256

typedef struct ElemSubckParallelTag ElemSubcktParallel;

typedef struct ElemCommentTag {
    char    str[MAX_COMMENT_LEN];
} ElemComment;

typedef struct ElemContactsTag {
    char    name[MAX_NAME_LEN];
    BOOL    negated;
} ElemContacts;

typedef struct ElemCoilTag {
    char    name[MAX_NAME_LEN];
    BOOL    negated;
    BOOL    setOnly;
    BOOL    resetOnly;
} ElemCoil;

typedef struct ElemTimeTag {
    char    name[MAX_NAME_LEN];
    int     delay; // us
} ElemTimer;

typedef struct ElemResetTag {
    char    name[MAX_NAME_LEN];
} ElemReset;

typedef struct ElemMoveTag {
    char    src[MAX_NAME_LEN];
    char    dest[MAX_NAME_LEN];
} ElemMove;

typedef struct ElemMathTag {
    char    op1[MAX_NAME_LEN];
    char    op2[MAX_NAME_LEN];
    char    dest[MAX_NAME_LEN];
} ElemMath;

typedef struct ElemCmpTag {
    char    op1[MAX_NAME_LEN];
    char    op2[MAX_NAME_LEN];
} ElemCmp;

typedef struct ElemCounterTag {
    char    name[MAX_NAME_LEN];
    int     max;
} ElemCounter;

typedef struct ElemReadAdcTag {
    char    name[MAX_NAME_LEN];
} ElemReadAdc;

typedef struct ElemSetPwmTag {
    char    name[MAX_NAME_LEN];
    int     targetFreq;
} ElemSetPwm;

typedef struct ElemUartTag {
    char    name[MAX_NAME_LEN];
} ElemUart;

typedef struct ElemShiftRegisterTag {
    char    name[MAX_NAME_LEN];
    int     stages;
} ElemShiftRegister;

typedef struct ElemLookUpTableTag {
    char    dest[MAX_NAME_LEN];
    char    index[MAX_NAME_LEN];
    int     count;
    BOOL    editAsString;
    SWORD   vals[MAX_LOOK_UP_TABLE_LEN];
} ElemLookUpTable;

typedef struct ElemPiecewiseLinearTag {
    char    dest[MAX_NAME_LEN];
    char    index[MAX_NAME_LEN];
    int     count;
    SWORD   vals[MAX_LOOK_UP_TABLE_LEN];
} ElemPiecewiseLinear;

typedef struct ElemFormattedStringTag {
    char    dest[MAX_NAME_LEN];
    char    var[MAX_NAME_LEN];
    char    string[MAX_LOOK_UP_TABLE_LEN];
} ElemFormattedString;

typedef struct ElemPerisistTag {
    char    var[MAX_NAME_LEN];
} ElemPersist;

#define SELECTED_NONE       0
#define SELECTED_ABOVE      1
#define SELECTED_BELOW      2
#define SELECTED_RIGHT      3
#define SELECTED_LEFT       4
typedef struct ElemLeafTag {
    int     selectedState;
    BOOL    poweredAfter;
    union {
        ElemComment         comment;
        ElemContacts        contacts;
        ElemCoil            coil;
        ElemTimer           timer;
        ElemReset           reset;
        ElemMove            move;
        ElemMath            math;
        ElemCmp             cmp;
        ElemCounter         counter;
        ElemReadAdc         readAdc;
        ElemSetPwm          setPwm;
        ElemUart            uart;
        ElemShiftRegister   shiftRegister;
        ElemFormattedString fmtdStr;
        ElemLookUpTable     lookUpTable;
        ElemPiecewiseLinear piecewiseLinear;
        ElemPersist         persist;
    }       d;
} ElemLeaf;

typedef struct ElemSubcktSeriesTag {
    struct {
        int     which;
        union {
            void                   *any;
            ElemSubcktParallel     *parallel;
            ElemLeaf               *leaf;
        } d;
    } contents[MAX_ELEMENTS_IN_SUBCKT];
    int count;
} ElemSubcktSeries;

typedef struct ElemSubckParallelTag {
    struct {
        int     which;
        union {
            void                   *any;
            ElemSubcktSeries       *series;
            ElemLeaf               *leaf;
        } d;
    } contents[MAX_ELEMENTS_IN_SUBCKT];
    int count;
} ElemSubcktParallel;

typedef struct McuIoInfoTag McuIoInfo;

typedef struct ModbusAddr {
	unsigned char Slave;
	unsigned short Register;
} ModbusAddr_t;

typedef struct PlcProgramSingleIoTag {
    char        name[MAX_NAME_LEN];
#define IO_TYPE_PENDING         0

/*
#define IO_TYPE_DIG_INPUT       1
#define IO_TYPE_DIG_OUTPUT      2
#define IO_TYPE_READ_ADC        3
#define IO_TYPE_UART_TX         4
#define IO_TYPE_UART_RX         5
#define IO_TYPE_PWM_OUTPUT      6
#define IO_TYPE_INTERNAL_RELAY  7
#define IO_TYPE_TON             8
#define IO_TYPE_TOF             9
#define IO_TYPE_RTO             10
#define IO_TYPE_COUNTER         11
#define IO_TYPE_GENERAL         12
*/
/*More convenient sort order in IOlist*/
#define IO_TYPE_GENERAL         1
#define IO_TYPE_PERSIST         2
#define IO_TYPE_STRING          3
#define IO_TYPE_RTO             4
#define IO_TYPE_COUNTER         5
#define IO_TYPE_INT_INPUT       6
#define IO_TYPE_DIG_INPUT       7
#define IO_TYPE_DIG_OUTPUT      8
#define IO_TYPE_READ_ADC        9
#define IO_TYPE_UART_TX         10
#define IO_TYPE_UART_RX         11
#define IO_TYPE_PWM_OUTPUT      12
#define IO_TYPE_INTERNAL_RELAY  13
#define IO_TYPE_TON             14
#define IO_TYPE_TOF             15
#define IO_TYPE_MODBUS_CONTACT  16
#define IO_TYPE_MODBUS_COIL     17
    int         type;
#define NO_PIN_ASSIGNED         0
    int         pin;
	ModbusAddr modbus;
} PlcProgramSingleIo;

#define MAX_IO 1024
typedef struct PlcProgramTag {
    struct {
        PlcProgramSingleIo  assignment[MAX_IO];
        int                 count;
    }           io;
    McuIoInfo  *mcu;
    int         cycleTime; // us
    int         mcuClock;  // Hz
    int         baudRate;  // Hz
    char        LDversion[512];

#define MAX_RUNGS 9999
    ElemSubcktSeries *rungs[MAX_RUNGS];
    BOOL              rungPowered[MAX_RUNGS];
    int               numRungs;
    char              rungSelected[MAX_RUNGS];
    DWORD             OpsInRung[MAX_RUNGS];
    DWORD             HexInRung[MAX_RUNGS];
} PlcProgram;

//-----------------------------------------------
// For actually drawing the ladder logic on screen; constants that determine
// how the boxes are laid out in the window, need to know that lots of
// places for figuring out if a mouse click is in a box etc.

// dimensions, in characters, of the area reserved for 1 leaf element
#define POS_WIDTH   17
#define POS_HEIGHT  3

// offset from the top left of the window at which we start drawing, in pixels
#define X_PADDING    35
#define Y_PADDING    14

typedef struct PlcCursorTag {
    int left;
    int top;
    int width;
    int height;
} PlcCursor;

//-----------------------------------------------
// The syntax highlighting style colours; a structure for the palette.

typedef struct SyntaxHighlightingColoursTag {
    COLORREF    bg;             // background
    COLORREF    def;            // default foreground
    COLORREF    selected;       // selected element
    COLORREF    op;             // `op code' (like OSR, OSF, ADD, ...)
    COLORREF    punct;          // punctuation, like square or curly braces
    COLORREF    lit;            // a literal number
    COLORREF    name;           // the name of an item
    COLORREF    rungNum;        // rung numbers
    COLORREF    comment;        // user-written comment text

    COLORREF    bus;            // the `bus' at the right and left of screen

    COLORREF    simBg;          // background, simulation mode
    COLORREF    simRungNum;     // rung number, simulation mode
    COLORREF    simOff;         // de-energized element, simulation mode
    COLORREF    simOn;          // energzied element, simulation mode
    COLORREF    simBusLeft;     // the `bus,' can be different colours for
    COLORREF    simBusRight;    // right and left of the screen
} SyntaxHighlightingColours;
extern SyntaxHighlightingColours HighlightColours;

//-----------------------------------------------
//See Atmel AVR Instruction set inheritance table
//https://en.wikipedia.org/wiki/Atmel_AVR_instruction_set#Instruction_set_inheritance
typedef enum AvrFamilyTag {
    NOT_AVR,
    MinimalCore,
    ClassicCore8K,
    ClassicCore128K,
    EnhancedCore8K,
    EnhancedCore128K,
    EnhancedCore4M,
    XMEGAcore,
    ReducedCore
} AvrFamily;

//-----------------------------------------------
// Processor definitions. These tables tell us where to find the I/Os on
// a processor, what bit in what register goes with what pin, etc. There
// is one master SupportedMcus table, which contains entries for each
// supported microcontroller.

typedef struct McuIoPinInfoTag {
    char    port;
    int     bit;
    int     pin;
    char    pinName[MAX_NAME_LEN];
	int		ArduinoPin;
} McuIoPinInfo;

typedef struct McuAdcPinInfoTag {
    int     pin;
    BYTE    muxRegValue;
} McuAdcPinInfo;

#define ISA_AVR             0x00
#define ISA_PIC16           0x01
#define ISA_ANSIC           0x02
#define ISA_INTERPRETED     0x03
#define ISA_AVR1            ISA_AVR // 0x04
#define ISA_NETZER          0x05
#define ISA_PASCAL          0x06
#define ISA_ARDUINO         0x07
#define ISA_CAVR            0x08
#define ISA_XINTERPRETED	0x09	// Extended interpeter
#define ISA_C_MAXI_INTER	0x10	// Controllino Maxi with extended interpreter

#define MAX_IO_PORTS        13
#define MAX_RAM_SECTIONS    5

typedef struct McuIoInfoTag {
    char            *mcuName;
    char             portPrefix;
    DWORD            inputRegs[MAX_IO_PORTS];         // A is 0, J is 9
    DWORD            outputRegs[MAX_IO_PORTS];
    DWORD            dirRegs[MAX_IO_PORTS];
    DWORD            flashWords;
    struct {
        DWORD            start;
        int              len;
    }                ram[MAX_RAM_SECTIONS];
    McuIoPinInfo    *pinInfo;
    int              pinCount;
    McuAdcPinInfo   *adcInfo;
    int              adcCount;
    int              adcMax;
    struct {
        int             rxPin;
        int             txPin;
    }                uartNeeds;
    int              pwmNeedsPin;
    int              whichIsa;
    AvrFamily        Family;         // BOOL             avrUseIjmp;
    
    DWORD            configurationWord;
    struct {
        int             int0; // The pin can serve as an External Interrupt source 0.
        //int           pol0PinB; // The pin can polling in QuadEncod routines.
        // PinA and PinB should be in the same register.
        int             int1; // The pin can serve as an External Interrupt source 1.
        //int           pol1PinB; // The pin can polling in QuadEncod routines.
        // PinA and PinB should be in the same register.
    }                IntNeeds;
} McuIoInfo;

#define NUM_SUPPORTED_MCUS 29

//-----------------------------------------------
// Function prototypes

// ldmicro.cpp
#define CHANGING_PROGRAM(x) { \
        UndoRemember(); \
        x; \
        ProgramChanged(); \
    }
void ProgramChanged(void);
void SetMenusEnabled(BOOL canNegate, BOOL canNormal, BOOL canResetOnly,
    BOOL canSetOnly, BOOL canDelete, BOOL canInsertEnd, BOOL canInsertOther,
    BOOL canPushRungDown, BOOL canPushRungUp, BOOL canInsertComment);
void SetUndoEnabled(BOOL undoEnabled, BOOL redoEnabled);
void RefreshScrollbars(void);
extern HINSTANCE Instance;
extern HWND MainWindow;
extern HDC Hdc;
extern PlcProgram Prog;
extern char CurrentSaveFile[MAX_PATH];
extern char CurrentCompileFile[MAX_PATH];
extern McuIoInfo SupportedMcus[]; // NUM_SUPPORTED_MCUS
// memory debugging, because I often get careless; ok() will check that the
// heap used for all the program storage is not yet corrupt, and oops() if
// it is
void CheckHeap(char *file, int line);
#define ok() CheckHeap(__FILE__, __LINE__)
char *ExtractFilePath(char *buffer);
char *ExtractFileName(char *src); // with .ext
char *GetFileName(char *dest, char *src); // without .ext
char *SetExt(char *dest, const char *src, const char *ext);
extern char CurrentLdPath[MAX_PATH];

// maincontrols.cpp
void MakeMainWindowControls(void);
HMENU MakeMainWindowMenus(void);
void VscrollProc(WPARAM wParam);
void HscrollProc(WPARAM wParam);
void GenerateIoListDontLoseSelection(void);
void RefreshControlsToSettings(void);
void MainWindowResized(void);
void ToggleSimulationMode(void);
void StopSimulation(void);
void StartSimulation(void);
void UpdateMainWindowTitleBar(void);
extern int ScrollWidth;
extern int ScrollHeight;
extern BOOL NeedHoriz;
extern HWND IoList;
extern int IoListTop;
extern int IoListHeight;

// draw.cpp
int ProgCountWidestRow(void);
int ProgCountRows(void);
int CountHeightOfElement(int which, void *elem);
BOOL DrawElement(int which, void *elem, int *cx, int *cy, BOOL poweredBefore);
void DrawEndRung(int cx, int cy);
extern int ColsAvailable;
extern BOOL SelectionActive;
extern BOOL ThisHighlighted;

// draw_outputdev.cpp
extern void (*DrawChars)(int, int, char *);
void CALLBACK BlinkCursor(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);
void PaintWindow(void);
void ExportDrawingAsText(char *file);
void InitForDrawing(void);
void SetUpScrollbars(BOOL *horizShown, SCROLLINFO *horiz, SCROLLINFO *vert);
int ScreenRowsAvailable(void);
int ScreenColsAvailable(void);
extern HFONT FixedWidthFont;
extern HFONT FixedWidthFontBold;
extern int SelectedGxAfterNextPaint;
extern int SelectedGyAfterNextPaint;
extern BOOL ScrollSelectedIntoViewAfterNextPaint;
extern int ScrollXOffset;
extern int ScrollYOffset;
extern int ScrollXOffsetMax;
extern int ScrollYOffsetMax;

// schematic.cpp
void SelectElement(int gx, int gy, int state);
void MoveCursorKeyboard(int keyCode);
void MoveCursorMouseClick(int x, int y);
BOOL MoveCursorTopLeft(void);
void EditElementMouseDoubleclick(int x, int y);
void EditSelectedElement(void);
void MakeResetOnlySelected(void);
void MakeSetOnlySelected(void);
void MakeNormalSelected(void);
void NegateSelected(void);
void ForgetFromGrid(void *p);
void ForgetEverything(void);
BOOL EndOfRungElem(int Which);
BOOL CanChangeOutputElem(int Which);
BOOL StaySameElem(int Which);
void WhatCanWeDoFromCursorAndTopology(void);
BOOL FindSelected(int *gx, int *gy);
void MoveCursorNear(int gx, int gy);

#define DISPLAY_MATRIX_X_SIZE 256
#define DISPLAY_MATRIX_Y_SIZE 2048
extern ElemLeaf *DisplayMatrix[DISPLAY_MATRIX_X_SIZE][DISPLAY_MATRIX_Y_SIZE];
extern int DisplayMatrixWhich[DISPLAY_MATRIX_X_SIZE][DISPLAY_MATRIX_Y_SIZE];
extern ElemLeaf DisplayMatrixFiller;
#define PADDING_IN_DISPLAY_MATRIX (&DisplayMatrixFiller)
#define VALID_LEAF(x) ((x) != NULL && (x) != PADDING_IN_DISPLAY_MATRIX)
extern ElemLeaf *Selected;
extern int SelectedWhich;

extern PlcCursor Cursor;
extern BOOL CanInsertEnd;
extern BOOL CanInsertOther;
extern BOOL CanInsertComment;

// circuit.cpp
void AddTimer(int which);
void AddCoil(void);
void AddContact(void);
void AddEmpty(int which);
void AddMove(void);
void AddSfr(int which);
void AddMath(int which);
void AddCmp(int which);
void AddReset(void);
void AddCounter(int which);
void AddReadAdc(void);
void AddSetPwm(void);
void AddUart(int which);
void AddPersist(void);
void AddComment(char *text);
void AddShiftRegister(void);
void AddMasterRelay(void);
void AddLookUpTable(void);
void AddPiecewiseLinear(void);
void AddFormattedString(void);
void AddString(void);
void DeleteSelectedFromProgram(void);
void DeleteSelectedRung(void);
BOOL CollapseUnnecessarySubckts(int which, void *any);
void InsertRung(BOOL afterCursor);
int RungContainingSelected(void);
BOOL ItemIsLastInCircuit(ElemLeaf *item);
BOOL UartFunctionUsed(void);
BOOL PwmFunctionUsed(void);
BOOL EepromFunctionUsed(void);
void PushRungUp(void);
void PushRungDown(void);
void NewProgram(void);
ElemLeaf *AllocLeaf(void);
ElemSubcktSeries *AllocSubcktSeries(void);
ElemSubcktParallel *AllocSubcktParallel(void);
void FreeCircuit(int which, void *any);
void FreeEntireProgram(void);
void UndoUndo(void);
void UndoRedo(void);
void UndoRemember(void);
void UndoFlush(void);
BOOL CanUndo(void);

// loadsave.cpp
BOOL LoadProjectFromFile(char *filename);
BOOL SaveProjectToFile(char *filename);
void SaveElemToFile(FILE *f, int which, void *any, int depth, int rung);
ElemSubcktSeries *LoadSeriesFromFile(FILE *f);
char *strspace(char *str);
char *strspacer(char *str);
char *FrmStrToStr(char *dest, char *src);

// iolist.cpp
int GenerateIoList(int prevSel);
void SaveIoListToFile(FILE *f);
BOOL LoadIoListFromFile(FILE *f);
void ShowIoDialog(int item);
void IoListProc(NMHDR *h);
void ShowAnalogSliderPopup(char *name);

// commentdialog.cpp
void ShowCommentDialog(char *comment);
// contactsdialog.cpp
void ShowContactsDialog(BOOL *negated, char *name);
// coildialog.cpp
void ShowCoilDialog(BOOL *negated, BOOL *setOnly, BOOL *resetOnly, char *name);
// simpledialog.cpp
void CheckVarInRange(char *name, SDWORD v);
void ShowTimerDialog(int which, int *delay, char *name);
void ShowCounterDialog(int which, int *count, char *name);
void ShowMoveDialog(char *dest, char *src);
void ShowReadAdcDialog(char *name);
void ShowSetPwmDialog(char *name, int *targetFreq);
void ShowPersistDialog(char *var);
void ShowUartDialog(int which, char *name);
void ShowCmpDialog(int which, char *op1, char *op2);
void ShowSFRDialog(int which, char *op1, char *op2);
void ShowMathDialog(int which, char *dest, char *op1, char *op2);
void ShowShiftRegisterDialog(char *name, int *stages);
void ShowFormattedStringDialog(char *var, char *string);
void ShowStringDialog(char * dest, char *var, char *string);
void ShowLookUpTableDialog(ElemLeaf *l);
void ShowPiecewiseLinearDialog(ElemLeaf *l);
void ShowResetDialog(char *name);
// confdialog.cpp
void ShowConfDialog(void);
// helpdialog.cpp
void ShowHelpDialog(BOOL about);

// miscutil.cpp
#ifndef round
#define round(r)   ((r) < (LONG_MIN-0.5) || (r) > (LONG_MAX+0.5) ?\
    (r):\
    ((r) >= 0.0) ? ((r) + 0.5) : ((r) - 0.5))
#endif

#define doLOG
#ifdef doLOG
#define LOG(EXP) dbp( #EXP " at %d in %s %s %s", __LINE__, __FILE__, __TIME__, BOOST_CURRENT_FUNCTION);
#else
#define LOG(EXP)
#endif

#define stringer(exp) #exp // see C Stringizing Operator (#)

#define ooops(...) { \
        dbp("bad at %d %s\n", __LINE__, __FILE__); \
        dbp(__VA_ARGS__); \
        Error("Internal error at line %d file '%s'\n", __LINE__, __FILE__); \
        exit(1); \
    }
#define oops() { \
        dbp("bad at %d %s\n", __LINE__, __FILE__); \
        Error("Internal error at line %d file '%s'\n", __LINE__, __FILE__); \
        exit(1); \
    }
#define dodbp
#ifdef dodbp
#define WARN_IF(EXP) if (EXP) dbp("Warning: " #EXP "");

#define dbps(EXP) dbp( #EXP "='%s'", EXP);
#define dbpd(EXP) dbp( #EXP "=%d", EXP);
#define dbpx(EXP) dbp( #EXP "=0x%x", EXP);
#define dbph dbpx
#define dbpf(EXP) dbp( #EXP "=%f", EXP);
#else
#define WARN_IF(EXP)

#define dbps(EXP)
#define dbpd(EXP)
#define dbpx(EXP)
#define dbpf(EXP)
#endif

void dbp(char *str, ...);
void Error(char *str, ...);
void *CheckMalloc(size_t n);
void CheckFree(void *p);
extern HANDLE MainHeap;
void StartIhex(FILE *f);
void WriteIhex(FILE *f, BYTE b);
void FinishIhex(FILE *f);
char *IoTypeToString(int ioType);
void PinNumberForIo(char *dest, PlcProgramSingleIo *io);
HWND CreateWindowClient(DWORD exStyle, char *className, char *windowName,
    DWORD style, int x, int y, int width, int height, HWND parent,
    HMENU menu, HINSTANCE instance, void *param);
void MakeDialogBoxClass(void);
void NiceFont(HWND h);
void FixedFont(HWND h);
void CompileSuccessfulMessage(char *str);
extern BOOL RunningInBatchMode;
extern BOOL RunningInTestMode;
extern HFONT MyNiceFont;
extern HFONT MyFixedFont;
extern HWND OkButton;
extern HWND CancelButton;
extern BOOL DialogDone;
extern BOOL DialogCancel;

// lang.cpp
char *_(char *in);

// simulate.cpp
void SimulateOneCycle(BOOL forceRefresh);
void CALLBACK PlcCycleTimer(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);
void StartSimulationTimer(void);
BOOL ClearSimulationData(void);
void ClrSimulationData(void);
void CheckVariableNames(void);
void DescribeForIoList(char *name, char *out);
void SimulationToggleContact(char *name);
void SetAdcShadow(char *name, SWORD val);
SWORD GetAdcShadow(char *name);
void DestroyUartSimulationWindow(void);
void ShowUartSimulationWindow(void);
DWORD IsUsedVariable(char *name);
extern BOOL InSimulationMode;
extern BOOL SimulateRedrawAfterNextCycle;

// compilecommon.cpp
void AllocStart(void);
DWORD AllocOctetRam(void);
void AllocBitRam(DWORD *addr, int *bit);
void MemForVariable(char *name, DWORD *addrl, DWORD *addrh);
BYTE MuxForAdcVariable(char *name);
void MemForSingleBit(char *name, BOOL forRead, DWORD *addr, int *bit);
void MemCheckForErrorsPostCompile(void);
int SizeOfVar(char *name);
void BuildDirectionRegisters(BYTE *isInput, BYTE *isOutput);
void ComplainAboutBaudRateError(int divisor, double actual, double err);
void ComplainAboutBaudRateOverflow(void);
#define CompileError() longjmp(CompileErrorBuf, 1)
extern jmp_buf CompileErrorBuf;

// intcode.cpp
extern int rungNow;
void IntDumpListing(char *outFile);
BOOL GenerateIntermediateCode(void);
BOOL CheckForNumber(char *str);
int TenToThe(int x);
// pic16.cpp
void CompilePic16(char *outFile);
// avr.cpp
void CompileAvr(char *outFile);
// ansic.cpp
void CompileAnsiC(char *outFile, int compile_ISA);
void CompileAnsiC(char *outFile);
// interpreted.cpp
void CompileInterpreted(char *outFile);
// xinterpreted.cpp
void CompileXInterpreted(char *outFile);
// netzer.cpp
void CompileNetzer(char *outFile);

#endif
