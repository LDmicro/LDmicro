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

using namespace std;            // added by JG

#ifndef __LDMICRO_H
#define __LDMICRO_H

#include "stdafx.h"

#include "current_function.hpp"
#include "compilercommon.hpp"

#include "accel.h"
#include "circuit.h"
#include "plcprogram.h"

//-----------------------------------------------
#define BYTES_OF_LD_VAR 2
#define BITS_OF_LD_VAR (BYTES_OF_LD_VAR * 8)
#define PLC_CLOCK_MIN 250 //500 //
//-----------------------------------------------
// `Configuration options.'

// clang-format off

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
#define MNU_SAVE_01             0x0301
#define MNU_SAVE_02             0x0302
#define MNU_SAVE_AS             0x04
#define MNU_EXPORT              0x05
#define MNU_EXIT                0x06

#define MNU_NOTEPAD_LD          0x0700
#define MNU_NOTEPAD_PL          0x0701
#define MNU_NOTEPAD_ASM         0x0702
#define MNU_NOTEPAD_HEX         0x0703
#define MNU_NOTEPAD_C           0x0704
#define MNU_NOTEPAD_H           0x0705
#define MNU_NOTEPAD_PAS         0x0706
#define MNU_NOTEPAD_INO         0x0707
#define MNU_NOTEPAD_TXT         0x070F
#define MNU_EXPLORE_DIR         0x0780

#define MNU_UNDO                0x10
#define MNU_REDO                0x11
#define MNU_PUSH_RUNG_UP        0x12
#define MNU_PUSH_RUNG_DOWN      0x13
#define MNU_INSERT_RUNG_BEFORE  0x14
#define MNU_INSERT_RUNG_AFTER   0x15
#define MNU_DELETE_ELEMENT      0x16
#define MNU_CUT_ELEMENT         0x1601
#define MNU_DELETE_RUNG         0x17

#define MNU_SELECT_RUNG         0x1800
#define MNU_CUT_RUNG            0x1801
#define MNU_COPY_RUNG_DOWN      0x1802
#define MNU_COPY_RUNG           0x1803
#define MNU_COPY_ELEM           0x1804
#define MNU_PASTE_RUNG          0x1805
#define MNU_PASTE_INTO_RUNG     0x1806

#define MNU_REPLACE_ELEMENT     0x1807

#define MNU_SCROLL_DOWN         0x1901
#define MNU_SCROLL_UP           0x1902
#define MNU_SCROLL_PgDOWN       0x1903
#define MNU_SCROLL_PgUP         0x1904
#define MNU_ROLL_HOME           0x1905
#define MNU_ROLL_END            0x1906

#define MNU_TAB                 0x1911

#define MNU_INSERT_COMMENT      0x20
#define MNU_INSERT_CONTACTS     0x21
#define MNU_INSERT_CONT_RELAY   0x2101
#define MNU_INSERT_CONT_OUTPUT  0x2102
#define MNU_INSERT_COIL         0x22
#define MNU_INSERT_COIL_RELAY   0x2201
#define MNU_INSERT_TCY          0x2301
#define MNU_INSERT_TON          0x23
#define MNU_INSERT_TOF          0x24
#define MNU_INSERT_RTO          0x25
#define MNU_INSERT_RTL          0x2501
#define MNU_INSERT_THI          0x2502
#define MNU_INSERT_TLO          0x2503
#define MNU_INSERT_RES          0x26
#define MNU_INSERT_TIME2COUNT   0x2601
#define MNU_INSERT_OSR          0x27
#define MNU_INSERT_OSF          0x28
#define MNU_INSERT_OSL          0x2801
#define MNU_INSERT_CTU          0x29
#define MNU_INSERT_CTD          0x2a
#define MNU_INSERT_CTC          0x2b
#define MNU_INSERT_CTR          0x2b01
#define MNU_INSERT_ADD          0x2c
#define MNU_INSERT_SUB          0x2d
#define MNU_INSERT_MUL          0x2e
#define MNU_INSERT_DIV          0x2f
#define MNU_INSERT_MOD          0x2f01
#define MNU_INSERT_SET_BIT      0x2f81
#define MNU_INSERT_CLEAR_BIT    0x2f82
#define MNU_INSERT_AND          0x2f02
#define MNU_INSERT_OR           0x2f03
#define MNU_INSERT_XOR          0x2f04
#define MNU_INSERT_NOT          0x2f05
#define MNU_INSERT_NEG          0x2f06
#define MNU_INSERT_RANDOM       0x2f07
#define MNU_INSERT_SEED_RANDOM  0x2f08
#define MNU_INSERT_SHL          0x2f21
#define MNU_INSERT_SHR          0x2f22
#define MNU_INSERT_SR0          0x2f23
#define MNU_INSERT_ROL          0x2f24
#define MNU_INSERT_ROR          0x2f25
#define MNU_INSERT_MOV          0x30
#define MNU_INSERT_BIN2BCD      0x3001
#define MNU_INSERT_BCD2BIN      0x3002
#define MNU_INSERT_SWAP         0x3003
#define MNU_INSERT_OPPOSITE     0x3004
#define MNU_INSERT_READ_ADC     0x31
#define MNU_INSERT_SET_PWM      0x32
#define MNU_INSERT_SET_PWM_SOFT 0x3201
#define MNU_INSERT_UART_SEND         0x33
#define MNU_INSERT_UART_SENDn        0x3301
#define MNU_INSERT_UART_SEND_READY   0x3302
#define MNU_INSERT_UART_RECV         0x34
#define MNU_INSERT_UART_RECVn        0x3401
#define MNU_INSERT_UART_RECV_AVAIL   0x3402
#define MNU_INSERT_EQU          0x35
#define MNU_INSERT_NEQ          0x36
#define MNU_INSERT_GRT          0x37
#define MNU_INSERT_GEQ          0x38
#define MNU_INSERT_LES          0x39
#define MNU_INSERT_LEQ          0x3a
#define MNU_INSERT_IF_BIT_SET   0x3a01
#define MNU_INSERT_IF_BIT_CLEAR 0x3a02
#define MNU_INSERT_OPEN         0x3b
#define MNU_INSERT_SHORT        0x3c
#define MNU_INSERT_MASTER_RLY   0x3d
#define MNU_INSERT_SLEEP        0x3d01
#define MNU_INSERT_CLRWDT       0x3d02
#define MNU_INSERT_LOCK         0x3d03
#define MNU_INSERT_DELAY        0x3d04
#define MNU_INSERT_TIME2DELAY   0x3d05
#define MNU_INSERT_GOTO         0x3d20
#define MNU_INSERT_LABEL        0x3d21
#define MNU_INSERT_SUBPROG      0x3d22
#define MNU_INSERT_ENDSUB       0x3d23
#define MNU_INSERT_GOSUB        0x3d24
#define MNU_INSERT_RETURN       0x3d25
#define MNU_INSERT_SHIFT_REG    0x3e
#define MNU_INSERT_LUT          0x3f
#define MNU_INSERT_FMTD_STRING  0x40
#define MNU_INSERT_PERSIST      0x41
#define MNU_MAKE_NORMAL         0x42
#define MNU_NEGATE              0x43
#define MNU_MAKE_SET_ONLY       0x44
#define MNU_MAKE_RESET_ONLY     0x45
#define MNU_MAKE_TTRIGGER       0x4501
#define MNU_INSERT_PWL          0x46

#define USE_SFR
#ifdef USE_SFR
#define MNU_OPEN_SFR            0x4700
#define MNU_INSERT_SFR          0x47    // special function register read
#define MNU_INSERT_SFW          0x48    // special function register write
#define MNU_INSERT_SSFB         0x49    // set bit in special function register
#define MNU_INSERT_csFB         0x4a    // clear bit in special function register
#define MNU_INSERT_TSFB         0x4b    // test if bit is set in special function register
#define MNU_INSERT_T_C_SFB      0x4c    // test if bit is clear in special function register
#endif
/*
#define MNU_INSERT_CPRINTF      0x4c01 == ELEM_CPRINTF
#define MNU_INSERT_SPRINTF      0x4c02
#define MNU_INSERT_FPRINTF      0x4c03
#define MNU_INSERT_PRINTF       0x4c04
#define MNU_INSERT_I2C_CPRINTF  0x4c05
#define MNU_INSERT_ISP_CPRINTF  0x4c06
#define MNU_INSERT_UART_CPRINTF 0x4c07
*/
#define MNU_INSERT_STRING       0x4d
#define MNU_INSERT_OSC          0x4f01
#define MNU_INSERT_STEPPER      0x4f02
#define MNU_INSERT_PULSER       0x4f03
#define MNU_INSERT_NPULSE       0x4f04
#define MNU_INSERT_NPULSE_OFF   0x4f05
#define MNU_INSERT_PWM_OFF      0x4f06
#define MNU_INSERT_PWM_OFF_SOFT 0x4f07
#define MNU_INSERT_QUAD_ENCOD   0x4f09

#define MNU_MCU_SETTINGS        0x50
#define MNU_SPEC_FUNCTION       0x51
#define MNU_PROCESSOR_0         0xa0
#define MNU_PROCESSOR_NEW       0xa001
#define MNU_PROCESSOR_NEW_PIC12 0xa002

#define MNU_SIMULATION_MODE     0x60
#define MNU_START_SIMULATION    0x61
#define MNU_STOP_SIMULATION     0x62
#define MNU_SINGLE_CYCLE        0x63

#define MNU_INSERT_SPI          0x6401
#define MNU_INSERT_SPI_WRITE    0x6402

#define MNU_INSERT_I2C_READ     0x6451              //// Added by JG
#define MNU_INSERT_I2C_WRITE    0x6452

#define MNU_INSERT_BUS          0x6501
#define MNU_INSERT_7SEG         0x6507
#define MNU_INSERT_9SEG         0x6509
#define MNU_INSERT_14SEG        0x6514
#define MNU_INSERT_16SEG        0x6516

#define MNU_COMPILE                 0x7000          ///// All MNU_COMPILE_* modified by JG
#define MNU_COMPILE_AS              0x7010

#define MNU_COMPILE_ANSIC           0x7100          // Begin of C compiling menus   !!!
#define MNU_COMPILE_HI_TECH_C       0x7110          // Pic Hi-Tech C
#define MNU_COMPILE_CCS_PIC_C       0x7130          // Pic CCS

#define MNU_COMPILE_GNUC            0x7200          // AVR WinAVR
#define MNU_COMPILE_AVRGCC          0x7210          // AVR Gcc / Avr Studio
#define MNU_COMPILE_CODEVISIONAVR   0x7220          // AVR Codevision
#define MNU_COMPILE_IMAGECRAFT      0x7230          // AVR IccAvr
#define MNU_COMPILE_IAR             0x7240          // AVR IAR

#define MNU_COMPILE_ARMGCC          0x7300          // Added by JG
#define MNU_COMPILE_lastC           0x7399          // End of C compiling menus     !!!

#define MNU_COMPILE_ARDUINO         0x7400          // Arduino

#define MNU_COMPILE_IHEX            0x7500
#define MNU_COMPILE_IHEXDONE        0x7510
#define MNU_COMPILE_ASM             0x7600
#define MNU_COMPILE_PASCAL          0x7700
#define MNU_COMPILE_INT             0x7800          // Interpreter
#define MNU_COMPILE_XINT            0x7810          // Extended interpreter

#define MNU_FLASH_BAT           0x7D
#define MNU_READ_BAT            0x7E
#define MNU_CLEAR_BAT           0x7F

#define MNU_MANUAL              0x80
#define MNU_ABOUT               0x81
#define MNU_HOW                 0x8100
#define MNU_FORUM               0x8101
#define MNU_WIKI                0x8102
#define MNU_LAST_RELEASE        0x8103
#define MNU_EMAIL               0x8104
#define MNU_CHANGES             0x8105
#define MNU_ISSUE               0x8106
#define MNU_RELEASE             0x82

#define MNU_SCHEME_BLACK        0x9000 // Must be a first
#define NUM_SUPPORTED_SCHEMES   6      // ...
#define MNU_SCHEME_USER         MNU_SCHEME_BLACK+NUM_SUPPORTED_SCHEMES-1 // This SCHEME number must be the largest !!!
#define MNU_SELECT_COLOR        0x9100

// Columns within the I/O etc. listview.
#define LV_IO_NAME              0x00
#define LV_IO_TYPE              0x01
#define LV_IO_STATE             0x02
#define LV_IO_PIN               0x03
#define LV_IO_PORT              0x04
#define LV_IO_PINNAME           0x05
#define LV_IO_RAM_ADDRESS       0x06
#define LV_IO_SISE_OF_VAR       0x07
#define LV_IO_MODBUS            0x08

// Timer IDs associated with the main window.
#define TIMER_BLINK_CURSOR      1
#define TIMER_SIMULATE          2

// Simulation windows           Added by JG
#define SIM_UART                1
#define SIM_SPI                 2
#define SIM_I2C                 3

#define RUNG(r)       max(min(r,Prog.numRungs-1),0)

//-----------------------------------------------
// For actually drawing the ladder logic on screen; constants that determine
// how the boxes are laid out in the window, need to know that lots of
// places for figuring out if a mouse click is in a box etc.

// dimensions, in characters, of the area reserved for 1 leaf element
#define POS_WIDTH   17
#define POS_HEIGHT  3

// offset from the top left of the window at which we start drawing, in pixels
#define X_PADDING    35 + FONT_WIDTH
#define Y_PADDING    14

typedef struct PlcCursorTag {
    int left;
    int top;
    int width;
    int height;
} PlcCursor;

//-----------------------------------------------
// The syntax highlighting style colours; a structure for the palette/scheme.

typedef struct SyntaxHighlightingColoursTag {
    char        sName[MAX_NAME_LEN];
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
extern SyntaxHighlightingColours Schemes[NUM_SUPPORTED_SCHEMES];
extern DWORD scheme;

#define IS_MCU_REG(i) ((Prog.mcu()) && (Prog.mcu()->inputRegs[i]) && (Prog.mcu()->outputRegs[i]) && (Prog.mcu()->dirRegs[i]))

//-----------------------------------------------
// Function prototypes

// ldmicro.cpp
#define CHANGING_PROGRAM(x) { \
        UndoRemember(); \
        x; \
        ProgramChanged(); \
    }
extern bool ProgramChangedNotSaved;
void ProgramChanged();
void SetMenusEnabled(bool canNegate, bool canNormal, bool canResetOnly,
    bool canSetOnly, bool canDelete, bool canInsertEnd, bool canInsertOther,
    bool canPushRungDown, bool canPushRungUp, bool canInsertComment);
void SetUndoEnabled(bool undoEnabled, bool redoEnabled);
void RefreshScrollbars();
extern HINSTANCE Instance;
extern HWND MainWindow;
extern HDC Hdc;
extern PlcProgram Prog;
extern char CurrentSaveFile[MAX_PATH];
extern char CurrentCompileFile[MAX_PATH];
extern char CurrentCompilePath[MAX_PATH];
extern ULONGLONG PrevWriteTime;
extern ULONGLONG LastWriteTime;
void ScrollDown();
void ScrollPgDown();
void ScrollUp();
void ScrollPgUp();
void RollHome();
void RollEnd();
char *ExtractFileDir(char *dest);
char *ExtractFilePath(char *dest);
const char *ExtractFileName(const char *src); // with .ext
char *GetFileName(char *dest, const char *src); // without .ext
char *SetExt(char *dest, const char *src, const char *ext);
extern char CurrentLdPath[MAX_PATH];
long int fsize(FILE *fp);
long int fsize(char filename);
const char *GetMnuName(int MNU);
int GetMnu(char *MNU_name);

// maincontrols.cpp
void MakeMainWindowControls();
HMENU MakeMainWindowMenus();
void VscrollProc(WPARAM wParam);
void HscrollProc(WPARAM wParam);
void GenerateIoListDontLoseSelection();
void RefreshStatusBar();
void RefreshControlsToSettings();
void MainWindowResized();
void ToggleSimulationMode(bool doSimulateOneRung);
void ToggleSimulationMode();
void StopSimulation();
void StartSimulation();
void UpdateMainWindowTitleBar();
extern int ScrollWidth;
extern int ScrollHeight;
extern bool NeedHoriz;
extern HWND IoList;
extern int IoListTop;
extern int IoListHeight;
extern char IoListSelectionName[MAX_NAME_LEN];

// draw.cpp
int ProgCountWidestRow();
int ProgCountRows();
extern int totalHeightScrollbars;
int CountHeightOfElement(int which, void *elem);
bool DrawElement(int which, void *elem, int *cx, int *cy, bool poweredBefore, int cols);
void DrawEndRung(int cx, int cy);
extern int ColsAvailable;
extern bool SelectionActive;
extern bool ThisHighlighted;

// draw_outputdev.cpp
void SetSyntaxHighlightingColours();                            ///// Prototype added by JG
extern void (*DrawChars)(int, int, const char *);
void CALLBACK BlinkCursor(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);
void PaintWindow();
BOOL tGetLastWriteTime(const char *CurrentSaveFile, FILETIME *sFileTime, int mode);     ///// prototype modified by JG
void ExportDrawingAsText(char *file);
void InitForDrawing();
void InitBrushesForDrawing();
void SetUpScrollbars(bool *horizShown, SCROLLINFO *horiz, SCROLLINFO *vert);
int ScreenRowsAvailable();
int ScreenColsAvailable();
extern HFONT FixedWidthFont;
extern HFONT FixedWidthFontBold;
extern int SelectedGxAfterNextPaint;
extern int SelectedGyAfterNextPaint;
extern bool ScrollSelectedIntoViewAfterNextPaint;
extern int ScrollXOffset;
extern int ScrollYOffset;
extern int ScrollXOffsetMax;
extern int ScrollYOffsetMax;

// schematic.cpp
void SelectElement(int gx, int gy, int state);
void MoveCursorKeyboard(int keyCode);
void MoveCursorMouseClick(int x, int y);
bool MoveCursorTopLeft();
void EditElementMouseDoubleclick(int x, int y);
void EditSelectedElement();
bool ReplaceSelectedElement();
void MakeResetOnlySelected();
void MakeSetOnlySelected();
void MakeNormalSelected();
void NegateSelected();
void MakeTtriggerSelected();
void ForgetFromGrid(void *p);
void ForgetEverything();
bool EndOfRungElem(int Which);
bool CanChangeOutputElem(int Which);
bool StaySameElem(int Which);
void WhatCanWeDoFromCursorAndTopology();
bool FindSelected(int *gx, int *gy);
bool MoveCursorNear(int *gx, int *gy);

#define DISPLAY_MATRIX_X_SIZE 256
#define DISPLAY_MATRIX_Y_SIZE (MAX_RUNGS*2) // 2048
extern ElemLeaf *DisplayMatrix[DISPLAY_MATRIX_X_SIZE][DISPLAY_MATRIX_Y_SIZE];
extern int DisplayMatrixWhich[DISPLAY_MATRIX_X_SIZE][DISPLAY_MATRIX_Y_SIZE];
extern ElemLeaf DisplayMatrixFiller;
#define PADDING_IN_DISPLAY_MATRIX (&DisplayMatrixFiller)
#define VALID_LEAF(x) ((x) != nullptr && (x) != PADDING_IN_DISPLAY_MATRIX)
extern ElemLeaf *Selected;
extern int SelectedWhich;

extern PlcCursor Cursor;
extern bool CanInsertEnd;
extern bool CanInsertOther;
extern bool CanInsertComment;

// undoredo.cpp
void UndoUndo();
void UndoRedo();
void UndoRemember();
void UndoFlush();
bool CanUndo();

// loadsave.cpp
bool LoadProjectFromFile(const char *filename);
bool SaveProjectToFile(char *filename, int code);
void SaveElemToFile(FILE *f, int which, void *any, int depth, int rung);
ElemSubcktSeries *LoadSeriesFromFile(FILE *f);
char *strspace(char *str);
char *strspacer(char *str);
char *FrmStrToStr(char *dest, const char *src);
void LoadWritePcPorts();

// iolist.cpp
int IsIoType(int type);
int GenerateIoList(int prevSel);
void SaveIoListToFile(FILE *f);
bool LoadIoListFromFile(FILE *f);
void ShowIoDialog(int item);
void IoListProc(NMHDR *h);
void ShowAnalogSliderPopup(char *name);

// commentdialog.cpp
void ShowCommentDialog(char *comment);
// contactsdialog.cpp
void ShowContactsDialog(bool *negated, bool *set1, char *name);
// coildialog.cpp
void ShowCoilDialog(bool *negated, bool *setOnly, bool *resetOnly, bool *ttrigger, char *name);
// simpledialog.cpp
void CheckVarInRange(char *name, char *str, SDWORD v);
void ShowTimerDialog(int which, ElemLeaf *l);
void ShowSleepDialog(int which, ElemLeaf *l);
void ShowDelayDialog(int which, ElemLeaf *l);
void ShowSpiDialog(ElemLeaf *l);
void ShowI2cDialog(ElemLeaf *l);
void ShowCounterDialog(int which, ElemLeaf *l);
void ShowVarBitDialog(int which, char *dest, char *src);
void ShowMoveDialog(int which, char *dest, char *src);
void ShowReadAdcDialog(char *name, int *refs);
void ShowGotoDialog(int which, char *name);
void ShowRandomDialog(char *name);
void ShowSetPwmDialog(void *e);
void ShowPersistDialog(char *var);
void ShowUartDialog(int which, ElemLeaf *l);
void ShowCmpDialog(int which, char *op1, char *op2);
void ShowSFRDialog(int which, char *op1, char *op2);
void ShowMathDialog(int which, char *dest, char *op1, char *op2);
void CalcSteps(ElemStepper *s, ResSteps *r);
void ShowStepperDialog(int which, void *e);
void ShowPulserDialog(int which, char *P1, char *P0, char *accel, char *counter, char *busy);
void ShowNPulseDialog(int which, char *counter, char *targetFreq, char *coil);
void ShowQuadEncodDialog(int which, ElemLeaf *l);
void ShowSegmentsDialog(int which, ElemLeaf *l);
void ShowBusDialog(ElemLeaf *l);
void ShowShiftRegisterDialog(char *name, int *stages);
void ShowFormattedStringDialog(char *var, char *string);
void ShowStringDialog(char * dest, char *var, char *string);
void ShowCprintfDialog(int which, void *e);
void ShowLookUpTableDialog(ElemLeaf *l);
void ShowPiecewiseLinearDialog(ElemLeaf *l);
void ShowResetDialog(char *name);
int ishobdigit(int c);
#define isxobdigit ishobdigit
int isalpha_(int c);
int isal_num(int c);
int isname(char *name);
double hobatof(const char *str);
long hobatoi(const char *str);
void ShowSizeOfVarDialog(PlcProgramSingleIo *io);

// confdialog.cpp
void ShowConfDialog();

// colorDialog.cpp
void ShowColorDialog();

// helpdialog.cpp
void ShowHelpDialog(bool about);
extern const char *AboutText[];

// miscutil.cpp
#ifndef round
#define round(r)   ((r) < (LONG_MIN-0.5) || (r) > (LONG_MAX+0.5) ?\
    (r):\
    ((r) >= 0.0) ? ((r) + 0.5) : ((r) - 0.5))
#endif
extern HWND OkButton;
extern HWND CancelButton;
extern bool DialogDone;
extern bool DialogCancel;

// see C Stringizing Operator (#)
#define stringer(exp) #exp
#define useless(exp) stringer(exp)
//#define BIT0 0
// stringer(BIT0) // ==  "BIT0"
// useless(BIT0)  // == "0"

#define ooops(...) { \
        dbp("rungNow=%d", rungNow); \
        dbp("Internal error at [%d:%s]\n", __LINE__, __FILE__); \
        Error("Internal error at [%d:%s]\n", __LINE__, __FILE__); \
        Error(__VA_ARGS__); \
        doexit(EXIT_FAILURE); \
    }
#define oops() { \
        dbp("rungNow=%d", rungNow); \
        dbp("Internal error at [%d:%s]\n", __LINE__, __FILE__); \
        Error("Internal error at [%d:%s]\n", __LINE__, __FILE__); \
        doexit(EXIT_FAILURE); \
    }
#define dodbp
#ifdef dodbp
  #define WARN_IF(EXP) if (EXP) dbp("Warning: " #EXP "");
  #define dbpif(EXP)   if (EXP) dbp("Warning: " #EXP "");

  #define dbp_(EXP)   dbp( #EXP );
  #define dbps(EXP)   dbp( #EXP "='%s'", (EXP));
  #define dbpc(EXP)   dbp( #EXP "='%c'", (EXP));
  #define dbpd(EXP)   dbp( #EXP "=%d", (EXP));
  #define dbpld(EXP)  dbp( #EXP "=%ld", (EXP));
  #define dbplld(EXP) dbp( #EXP "=%lld", (EXP));

  //#define dbpb(EXP)   dbp( #EXP "=0b%b", (EXP));
  #define dbpx(EXP)   dbp( #EXP "=0x%X", (EXP));
  #define dbph dbpx
  #define dbpf(EXP)   dbp( #EXP "=%f", (EXP));
#else
  #define WARN_IF(EXP)

  #define dbps(EXP)
  #define dbpd(EXP)
  #define dbpx(EXP)
  #define dbpf(EXP)
#endif

void doexit(int status);
void dbp(const char *str, ...);
int LdMsg(UINT uType, const char *str, ...);
#define Error(str, ...) LdMsg(MB_ICONERROR, str, __VA_ARGS__)
#define Warning(str, ...) LdMsg(MB_ICONWARNING, str, __VA_ARGS__)
#define Info(str, ...) LdMsg(MB_ICONINFORMATION, str, __VA_ARGS__)
#define Question(str, ...) LdMsg(MB_ICONQUESTION, str, __VA_ARGS__)
void *CheckMalloc(size_t n);
void CheckFree(void *p);
void StartIhex(FILE *f);
void WriteIhex(FILE *f, BYTE b);
void FinishIhex(FILE *f);
const char *IoTypeToString(int ioType);
void PinNumberForIo(char *dest, PlcProgramSingleIo *io);
void PinNumberForIo(char *dest, PlcProgramSingleIo *io, char *portName, char *pinName);
char *GetPinName(int pin, char *pinName);
char *ShortPinName(McuIoPinInfo *iop, char *pinName);
char *LongPinName(McuIoPinInfo *iop, char *pinName);
const char *PinToName(int pin);
const char *ArduinoPinName(McuIoPinInfo *iop);
const char *ArduinoPinName(int pin);
//void SetMcu_(McuIoInfo *mcu);
int NameToPin(char *pinName);
McuIoPinInfo *PinInfo(int pin);
McuIoPinInfo *PinInfoForName(const char *name);
McuSpiInfo *GetMcuSpiInfo(char *name);
McuI2cInfo *GetMcuI2cInfo(char *name);          ///// Added by JG
McuPwmPinInfo *PwmPinInfo(int pin);
McuPwmPinInfo *PwmPinInfo(int pin, int timer);
McuPwmPinInfo *PwmPinInfoForName(char *name);
McuPwmPinInfo *PwmPinInfoForName(const char *name, int timer);
McuPwmPinInfo *PwmPinInfoForName(const char *name, int timer, int resolution);
McuPwmPinInfo *PwmMaxInfoForName(const char *name, int timer);                  ///// Added by JG
void getResolution(const char *s, int *resol, int *TOP);
McuAdcPinInfo *AdcPinInfo(int pin);
McuAdcPinInfo *AdcPinInfoForName(char *name);
bool IsExtIntPin(int pin);
HWND CreateWindowClient(DWORD exStyle, const char *className, const char *windowName,
    DWORD style, int x, int y, int width, int height, HWND parent,
    HMENU menu, HINSTANCE instance, void *param);
void MakeDialogBoxClass();
void NiceFont(HWND h);
void FixedFont(HWND h);
void CompileSuccessfulMessage(char *str, unsigned int uType);
void CompileSuccessfulMessage(char *str);
void CompileSuccesfullAnsiCMessage(const char *dest);
extern bool RunningInBatchMode;
extern bool RunningInTestMode;
extern HFONT MyNiceFont;
extern HFONT MyFixedFont;
bool IsNumber(const char *str);
bool IsNumber(const NameArray& name);
size_t strlenalnum(const char *str);
void CopyBit(DWORD *Dest, int bitDest, DWORD Src, int bitSrc);
char *strDelSpace(char *dest, char *src);
char *strDelSpace(char *dest);
char *strncpyn(char *s1, const char *s2, size_t n);
char *strncatn(char *s1, const char *s2, size_t n);
char *toupperstr(char *dest);
char *toupperstr(char *dest, const char *src);

// lang.cpp
const char *_(const char *in);

// simulate.cpp
void MarkInitedVariable(const char *name);
void SimulateOneCycle(bool forceRefresh);
void CALLBACK PlcCycleTimer(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);
void StartSimulationTimer();
bool ClearSimulationData();
void ClrSimulationData();
void CheckVariableNames();
void DescribeForIoList(const char *name, int type, char *out);
void SimulationToggleContact(char *name);
bool GetSingleBit(char *name);
void SetAdcShadow(char *name, SWORD val);
SWORD GetAdcShadow(const char *name);
SWORD GetAdcShadow(const NameArray& name);
void DestroySimulationWindow(HWND SimulationWindow);        ///// Prototype modified by JG
void ShowSimulationWindow(int sim);                         ///// Prototype modified by JG
extern bool InSimulationMode;
//extern bool SimulateRedrawAfterNextCycle;
extern DWORD CyclesCount;
void SetSimulationVariable(const char *name, SDWORD val);
SDWORD GetSimulationVariable(const char *name, bool forIoList);
SDWORD GetSimulationVariable(const char *name);
SDWORD GetSimulationVariable(const NameArray& name);
void SetSimulationStr(const char *name, const char *val);
char *GetSimulationStr(const char *name);
int FindOpName(int op, const NameArray& name1);
int FindOpName(int op, const NameArray& name1, const NameArray& name2);
int FindOpNameLast(int op, const NameArray& name1);
int FindOpNameLast(int op, const NameArray& name1, const NameArray& name2);
// Assignment of the `variables,' used for timers, counters, arithmetic, and
// other more general things. Allocate 2 octets (16 bits) per.
// Allocate 1 octets for  8-bits variables.
// Allocate 3 octets for  24-bits variables.
typedef  struct VariablesListTag {
    // vvv from compilecommon.cpp
    char    name[MAX_NAME_LEN];
    DWORD   addrl;
    int     Allocated;  // the number of bytes allocated in the MCU SRAM for variable
    int     SizeOfVar;  // SizeOfVar can be less than Allocated
    // ^^^ from compilecommon.cpp
    int     type;       // see PlcProgramSingleIo
    // vvv from simulate.cpp
//  SDWORD  val;        // value in simulation mode.
//  char    valstr[MAX_COMMENT_LEN]; // value in simulation mode for STRING types.
//  DWORD   usedFlags;  // in simulation mode.
//  int     initedRung; // Variable inited in rung.
//  DWORD   initedOp;   // Variable inited in Op number.
//  char    rungs[MAX_COMMENT_LEN]; // Rungs, where variable is used.
    // ^^^ from simulate.cpp
} VariablesList;

#define USE_IO_REGISTERS 1 // 0-NO, 1-YES // USE IO REGISTERS in AVR
// // #define USE_LDS_STS
// not complete; just what I need
typedef enum AvrOpTag {
    OP_VACANT, // 0
    OP_NOP,
    OP_COMMENT,
    OP_COMMENTINT,
    OP_ADC,
    OP_ADD,
    OP_ADIW,
    OP_SBIW,
    OP_ASR,
    OP_BRCC,
    OP_BRCS, // 10
    OP_BREQ,
    OP_BRGE,
    OP_BRLO,
    OP_BRLT,
    OP_BRNE,
    OP_BRMI,
    OP_CBR,
    OP_CLC,
    OP_CLR,
    OP_SER, // 20
    OP_COM,
    OP_CP,
    OP_CPC,
    OP_CPI,
    OP_DEC,
    OP_EOR,
    OP_EICALL,
    OP_EIJMP,
    OP_ICALL,
    OP_IJMP,
    OP_INC,
    OP_LDI, // 30
    OP_LD_X,
    OP_LD_XP,  // post increment X+
    OP_LD_XS,  // -X pre decrement
    OP_LD_Y,
    OP_LD_YP,  // post increment Y+
    OP_LD_YS,  // -Y pre decrement
    OP_LDD_Y,  //  Y+q
    OP_LD_Z,
    OP_LD_ZP,  // post increment Z+
    OP_LD_ZS,  // -Z pre decrement // 40
    OP_LDD_Z,  // Z+q
    OP_LPM_0Z, // R0 <- (Z)
    OP_LPM_Z,  // Rd <- (Z)
    OP_LPM_ZP, // Rd <- (Z++) post incterment
    OP_DB,     // one byte in flash word, hi byte = 0! // 45
    OP_DB2,    // two bytes in flash word
    OP_DW,     // word in flash word
    OP_MOV,
    OP_MOVW,
    OP_SWAP, // 50
    OP_RCALL,
    OP_RET,
    OP_RETI,
    OP_RJMP,
    OP_ROR,
    OP_ROL,
    OP_LSL,
    OP_LSR,
    OP_SEC,
    OP_SBC,  // 60
    OP_SBCI,
    OP_SBR,
    OP_SBRC,
    OP_SBRS,
    OP_ST_X,
    OP_ST_XP, // +
    OP_ST_XS, // -
    OP_ST_Y,
    OP_ST_YP, // +
    OP_ST_YS, // - // 70
//  OP_STD_Y, // Y+q // Notes: 1. This instruction is not available in all devices. Refer to the device specific instruction set summary.
    OP_ST_Z,
    OP_ST_ZP, // +
    OP_ST_ZS, // -
//  OP_STD_Z, // Z+q // Notes: 1. This instruction is not available in all devices. Refer to the device specific instruction set summary.
    OP_SUB,
    OP_SUBI,
    OP_TST,
    OP_WDR,
    OP_AND,
    OP_ANDI,
    OP_OR,  //80
    OP_ORI,
    OP_CPSE,
    OP_BLD,
    OP_BST,
    OP_PUSH,
    OP_POP,
    OP_CLI,
    OP_SEI,
    OP_SLEEP, // 89
    #ifdef USE_MUL
    OP_MUL,
    OP_MULS,
    OP_MULSU,
    #endif
    #if USE_IO_REGISTERS == 1
    OP_IN,
    OP_OUT,
    OP_SBI,
    OP_CBI,
    OP_SBIC,
    OP_SBIS,
    #endif
} AvrOp;

// not complete; just what I need
typedef enum Pic16OpTag {
    OP_VACANT_, // 0
    OP_NOP_,
    OP_COMMENT_,
    OP_COMMENT_INT,
//  OP_ADDLW, // absent in PIC12
    OP_ADDWF,
//  OP_ANDLW,
    OP_ANDWF,
    OP_CALL,
    OP_BSF,
    OP_BCF,
    OP_BTFSC, // 10
    OP_BTFSS,
    OP_GOTO,
    OP_CLRF,
    OP_CLRWDT,
    OP_COMF,
    OP_DECF,
    OP_DECFSZ,
    OP_INCF,
    OP_INCFSZ,
    OP_IORLW, // 20
    OP_IORWF,
    OP_MOVLW,
    OP_MOVF,
    OP_MOVWF,
    OP_RETFIE, // 25
    OP_RETURN,
    OP_RETLW,
    OP_RLF,
    OP_RRF,
//  OP_SUBLW, // absent in PIC12
    OP_SUBWF, // 30
    OP_XORLW,
    OP_XORWF,
    OP_SWAPF,
    OP_MOVLB,
    OP_MOVLP, // 35
    OP_TRIS,
    OP_OPTION,
    OP_SLEEP_
} PicOp;

typedef struct PlcTimerDataTag {
    long long int ticksPerCycle; // ticks of the System Clock per PLC Cycle Time // mcuClock * PLCcycleTime
    long int tmr; //value of TMR0 or TMR1
    long int prescaler;
    int PS; // PIC
    int cs; // AVR
    long int softDivisor; // Overflow Count
    DWORD softDivisorAddr;
    double TCycle; // s,  actually
    double Fcycle; // Hz, actually
    int cycleTimeMin; //
    long long int cycleTimeMax; //
} PlcTimerData;

extern PlcTimerData plcTmr;

typedef struct PicAvrInstructionTag {
    PicOp       opPic;
    AvrOp       opAvr;
    DWORD       arg1;
    DWORD       arg2;
    DWORD       arg1orig;
    DWORD       BANK;   // this operation opPic will executed with this STATUS or BSR registers
    DWORD       PCLATH; // this operation opPic will executed with this PCLATH which now or previously selected
    int         label;
    char        commentInt[MAX_COMMENT_LEN]; // before op
    char        commentAsm[MAX_COMMENT_LEN]; // after op
    char        arg1name[MAX_NAME_LEN];
    char        arg2name[MAX_NAME_LEN];
    int         rung;  // This Instruction located in Prog.rungs[rung] LD
    uint32_t    IntPc; // This Instruction located in IntCode[IntPc]
    int         l;           // line in source file
    char        f[MAX_PATH]; // source file name
} PicAvrInstruction;

// compilecommon.cpp
int McuRAM();
int UsedRAM();
int McuROM();
int UsedROM();
int McuPWM();
int McuADC();
int McuSPI();
int McuI2C();           ///// Added by JG
int McuUART();
extern DWORD RamSection;
extern DWORD RomSection;
extern DWORD EepromAddrFree;
//extern int VariableCount;
void PrintVariables(FILE *f);
DWORD isVarUsed(const char *name);
int isVarInited(const char *name);
int isPinAssigned(const NameArray& name);
void AllocStart();
DWORD AllocOctetRam();
DWORD AllocOctetRam(int bytes);
void AllocBitRam(DWORD *addr, int *bit);
int MemForVariable(const char *name, DWORD *addrl, int sizeOfVar);
int MemForVariable(const char *name, DWORD *addr);
int MemForVariable(const NameArray& name, DWORD *addr);
int SetMemForVariable(const char *name, DWORD addr, int sizeOfVar);
int SetMemForVariable(const NameArray& name, DWORD addr, int sizeOfVar);
int MemOfVar(const char *name, DWORD *addr);
int MemOfVar(const NameArray& name, DWORD *addr);
uint8_t MuxForAdcVariable(const char *name);
uint8_t MuxForAdcVariable(const NameArray& name);
int PinsForSpiVariable(const char *name, int n, char *spipins);             ///// Added by JG
int PinsForI2cVariable(const char *name, int n, char *i2cpins);             ///// Added by JG
int SingleBitAssigned(const char *name);
int GetAssignedType(const char *name, const char *fullName);
int InputRegIndex(DWORD addr);
int OutputRegIndex(DWORD addr);
void AddrBitForPin(int pin, DWORD *addr, int *bit, bool asInput);
void MemForSingleBit(const char *name, bool forRead, DWORD *addr, int *bit);
void MemForSingleBit(const NameArray& name, bool forRead, DWORD *addr, int *bit);
void MemForSingleBit(const char *name, DWORD *addr, int *bit);
void MemCheckForErrorsPostCompile();
int SetSizeOfVar(const char *name, int sizeOfVar);
int SizeOfVar(const char *name);
int SizeOfVar(const NameArray& name);
int AllocOfVar(char *name);
int TestByteNeeded(int count, SDWORD *vals);
int byteNeeded(long long int i);
void SaveVarListToFile(FILE *f);
bool LoadVarListFromFile(FILE *f);
void BuildDirectionRegisters(BYTE *isInput, BYTE *isAnsel, BYTE *isOutput);
void BuildDirectionRegisters(WORD *isInput, WORD *isAnsel, WORD *isOutput);                     ///// Added by JG
void BuildDirectionRegisters(WORD *isInput, WORD *isAnsel, WORD *isOutput, bool raiseError);    ///// Modified by JG BYTE -> WORD
void ComplainAboutBaudRateError(int divisor, double actual, double err);
void ComplainAboutBaudRateOverflow();
double SIprefix(double val, char *prefix, int en_1_2);
double SIprefix(double val, char *prefix);
int GetVariableType(char *name);
int SetVariableType(const char *name, int type);

typedef struct LabelAddrTag {
    char  name[MAX_NAME_LEN];
    DWORD KnownAddr; // Address to jump to the start of rung abowe the current in LD
    DWORD FwdAddr;   // Address to jump to the start of rung below the current in LD
    DWORD used;
} LabelAddr;
LabelAddr *GetLabelAddr(const char *name);

// intcode.cpp
extern int int_comment_level;
extern int asm_comment_level;
extern int asm_discover_names;
extern int rungNow;
void IntDumpListing(char *outFile);
SDWORD TestTimerPeriod(char *name, SDWORD delay, int adjust); // delay in us
bool GenerateIntermediateCode();
bool CheckLeafElem(int which, void *elem);
extern DWORD addrRUartRecvErrorFlag;
extern int    bitRUartRecvErrorFlag;
extern DWORD addrRUartSendErrorFlag;
extern int    bitRUartSendErrorFlag;
bool GotoGosubUsed();
bool UartFunctionUsed();
bool UartRecvUsed();
bool UartSendUsed();
bool SpiFunctionUsed();
bool I2cFunctionUsed();                 ///// Added by JG
bool Bin32BcdRoutineUsed();
SDWORD CheckMakeNumber(const char *str);
SDWORD CheckMakeNumber(const NameArray& str);
void WipeIntMemory();
bool CheckForNumber(const char *str);
int TenToThe(int x);
int xPowerY(int x, int y);
bool MultiplyRoutineUsed();
bool DivideRoutineUsed();
void GenSymOneShot(char *dest, const char *name1, const char *name2);
int getradix(const char *str);
SDWORD CalcDelayClock(long long clocks); // in us
bool IsAddrInVar(const char *name);

// pic16.cpp
extern SDWORD PicProgLdLen;
void CompilePic16(const char* outFile);
bool McuAs(const char *str);
bool CalcPicPlcCycle(long long int cycleTimeMicroseconds, SDWORD PicProgLdLen);
// avr.cpp
extern DWORD AvrProgLdLen;
int calcAvrUsart(int *divisor, double  *actual, double  *percentErr);
int testAvrUsart(int divisor, double  actual, double  percentErr);
bool CalcAvrPlcCycle(long long int cycleTimeMicroseconds, DWORD AvrProgLdLen);
void CompileAvr(const char* outFile);
// ansic.cpp
extern int compile_MNU;
bool CompileAnsiC(const char *outFile, int MNU);
int AVR_Prediv(const char * name2, const char * name3, const char * name4);     ///// Added by JG
// interpreted.cpp
void CompileInterpreted(const char* outFile);
// xinterpreted.cpp
void CompileXInterpreted(const char* outFile);
// netzer.cpp
void CompileNetzer(const char* outFile);
// pascal.cpp
void CompilePascal(const char* outFile);

// pcports.cpp
extern McuIoPinInfo IoPc[MAX_IO];
extern int IoPcCount;
bool ParceVar(const char *str, char *prt, int *portN, int *Reg, int *Mask, int *Addr);
void FillPcPinInfo(McuIoPinInfo *pinInfo);

// translit.cpp
void Transliterate(char *dest, const char* str);

// exceptions
void abortHandler(int signum);      ///// Added by JG

#endif
