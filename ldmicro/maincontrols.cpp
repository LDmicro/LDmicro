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
// Common controls in the main window. The main window consists of the drawing
// area, where the ladder diagram is displayed, plus various controls for
// scrolling, I/O list, menus.
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

// scrollbars for the ladder logic area
static HWND         HorizScrollBar;
static HWND         VertScrollBar;
int                 ScrollWidth;
int                 ScrollHeight;
BOOL                NeedHoriz;

// status bar at the bottom of the screen, to display settings
static HWND         StatusBar;

// have to get back to the menus to gray/ungray, check/uncheck things
static HMENU        FileMenu;
static HMENU        EditMenu;
static HMENU        InstructionMenu;
static HMENU        CourseMenu;
static HMENU        FormatStrMenu;
static HMENU        ProcessorMenu;
static HMENU        ProcessorMenu2;
static HMENU        SimulateMenu;
static HMENU        TopMenu;
static HMENU        SpecialFunction;
static HMENU        DisplayMenu;
static HMENU        CmpMenu;
static HMENU        SignedMenu;
static HMENU        BitwiseMenu;
static HMENU        PulseMenu;
static HMENU        SchemeMenu;

// listview used to maintain the list of I/O pins with symbolic names, plus
// the internal relay too
HWND                IoList;
static int          IoListSelectionPoint = 0;
static BOOL         IoListOutOfSync = FALSE;
char                IoListSelectionName[MAX_NAME_LEN] = "";
int                 IoListHeight;
int                 IoListTop;

// whether the simulation is running in real time
static BOOL         RealTimeSimulationRunning;

//-----------------------------------------------------------------------------
// Create the standard Windows controls used in the main window: a Listview
// for the I/O list, and a status bar for settings.
//-----------------------------------------------------------------------------
void MakeMainWindowControls(void)
{
    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
#define LV_ADD_COLUMN(hWnd, i, w, s) do { \
        lvc.iSubItem = i; \
        lvc.pszText = (LPWSTR)(s); \
        lvc.iOrder = 0; \
        lvc.cx = w; \
        ListView_InsertColumn(hWnd, i, &lvc); \
    } while(0)
    // create child window for IO list
    IoList = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "", WS_CHILD |
        LVS_REPORT | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS | WS_TABSTOP |
        LVS_SINGLESEL | WS_CLIPSIBLINGS,
        12, 25, 300, 300, MainWindow, NULL, Instance, NULL);
    ListView_SetExtendedListViewStyle(IoList, LVS_EX_FULLROWSELECT);

    int typeWidth = 110;
    int pinWidth = 70;
    int portWidth = 60;
    int pinNameWidth = 140;
    int modbusWidth = 80;

    LV_ADD_COLUMN(IoList, LV_IO_NAME,        150,          _("Name"));
    LV_ADD_COLUMN(IoList, LV_IO_TYPE,        typeWidth,    _("Type"));
    LV_ADD_COLUMN(IoList, LV_IO_STATE,       100+50,       _("State"));
    LV_ADD_COLUMN(IoList, LV_IO_PIN,         pinWidth,     _("Pin on MCU"));
    LV_ADD_COLUMN(IoList, LV_IO_PORT,        portWidth,    _("MCU Port"));
    LV_ADD_COLUMN(IoList, LV_IO_PINNAME,     pinNameWidth, _("Pin Name"));
    LV_ADD_COLUMN(IoList, LV_IO_RAM_ADDRESS, 75,           _("Address"));
    LV_ADD_COLUMN(IoList, LV_IO_SISE_OF_VAR, 60,           _("Size"));
    LV_ADD_COLUMN(IoList, LV_IO_MODBUS,      modbusWidth,  _("Modbus addr"));
    // IO list horizontal scroll bar
    HorizScrollBar = CreateWindowExA(0, WC_SCROLLBARA, "", WS_CHILD |
        SBS_HORZ | SBS_BOTTOMALIGN | WS_VISIBLE | WS_CLIPSIBLINGS,
        100, 100, 100, 100, MainWindow, NULL, Instance, NULL);
    // IO list Verticle scroll bar
    VertScrollBar = CreateWindowExA(0, WC_SCROLLBARA, "", WS_CHILD |
        SBS_VERT | SBS_LEFTALIGN | WS_VISIBLE | WS_CLIPSIBLINGS,
        200, 100, 100, 100, MainWindow, NULL, Instance, NULL);
    RECT scroll;
    GetWindowRect(HorizScrollBar, &scroll);
    ScrollHeight = scroll.bottom - scroll.top;
    GetWindowRect(VertScrollBar, &scroll);
    ScrollWidth = scroll.right - scroll.left;

    StatusBar = CreateStatusWindowA(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        "LDmicro started", MainWindow, 0);
    int edges[] = { 60, 250 + 60, 370 + 130, -1 };
    SendMessage(StatusBar, SB_SETPARTS, 4, (LPARAM)edges);

    // display IO list window with white background - no actual list
    ShowWindow(IoList, SW_SHOW);
}

//-----------------------------------------------------------------------------
// Set up the title bar text for the main window; indicate whether we are in
// simulation or editing mode, and indicate the filename.
//-----------------------------------------------------------------------------
void UpdateMainWindowTitleBar(void)
{
    wchar_t line[MAX_PATH+100];
    if(InSimulationMode) {
        if(RealTimeSimulationRunning) {
            wcscpy(line, _("LDmicro - Simulation (Running)"));
        } else {
            wcscpy(line, _("LDmicro - Simulation (Stopped)"));
        }
    } else {
        wcscpy(line, _("LDmicro - Program Editor"));
    }
    if(strlen(CurrentSaveFile) > 0) {
        swprintf_s(line+wcslen(line), arraylen(line)-wcslen(line),L" - %ls", to_utf16(CurrentSaveFile).c_str());
    } else {
        wcscat(line, _(" - (not yet saved)"));
    }

    SetWindowTextW(MainWindow, line);
}

//-----------------------------------------------------------------------------
// Set the enabled state of the logic menu items to reflect where we are on
// the schematic (e.g. can't insert two coils in series).
//-----------------------------------------------------------------------------
void SetMenusEnabled(BOOL canNegate, BOOL canNormal, BOOL canResetOnly,
    BOOL canSetOnly, BOOL canDelete, BOOL canInsertEnd, BOOL canInsertOther,
    BOOL canPushDown, BOOL canPushUp, BOOL canInsertComment)
{
    EnableMenuItem(EditMenu, MNU_PUSH_RUNG_UP,
        canPushUp ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(EditMenu, MNU_PUSH_RUNG_DOWN,
        canPushDown ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(EditMenu, MNU_DELETE_RUNG,
        (Prog.numRungs > 1) ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(InstructionMenu, MNU_NEGATE,
        canNegate ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(InstructionMenu, MNU_MAKE_NORMAL,
        canNormal ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(InstructionMenu, MNU_MAKE_RESET_ONLY,
        canResetOnly ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(InstructionMenu, MNU_MAKE_TTRIGGER,
        canResetOnly ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(InstructionMenu, MNU_MAKE_SET_ONLY,
        canSetOnly ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(InstructionMenu, MNU_INSERT_COMMENT,
        canInsertComment ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(EditMenu, MNU_DELETE_ELEMENT,
        canDelete ? MF_ENABLED : MF_GRAYED);

    int t;
    t = canInsertEnd ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(InstructionMenu, MNU_INSERT_COIL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_COIL_RELAY, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_RES, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_MOV, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_ADD, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SUB, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_MUL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_DIV, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_MOD, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_PERSIST, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_READ_ADC, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SET_PWM, t);
    //EnableMenuItem(InstructionMenu, MNU_INSERT_PWM_OFF, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_NPULSE_OFF, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_MASTER_RLY, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SLEEP, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CLRWDT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_LOCK, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_GOTO, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_GOSUB, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_RETURN, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SHIFT_REG, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_LUT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_PWL, t);

    t = canInsertOther ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(InstructionMenu, MNU_INSERT_SET_BIT     , t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CLEAR_BIT   , t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_IF_BIT_SET  , t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_IF_BIT_CLEAR, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_AND, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OR , t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_XOR, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_NOT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_NEG, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_RANDOM, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SEED_RANDOM, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SHL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SHR, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SR0, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_ROL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_ROR, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_BIN2BCD, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_BCD2BIN, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SWAP, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OPPOSITE, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_TIME2COUNT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_TIME2DELAY, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_TCY, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_TON, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_TOF, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_THI, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_TLO, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OSR, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OSF, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OSC, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OSL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_STEPPER, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_PULSER, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_NPULSE, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_QUAD_ENCOD, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_RTL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_RTO, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CONTACTS, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CONT_RELAY, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CONT_OUTPUT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CTU, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CTD, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CTC, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CTR, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_EQU, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_NEQ, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_GRT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_GEQ, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_LES, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_LEQ, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SHORT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OPEN, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_DELAY, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_LABEL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SUBPROG, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_ENDSUB, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_UART_SEND, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_UART_RECV, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_UART_SENDn, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_UART_RECVn, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_UART_SEND_READY, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_UART_RECV_AVAIL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_STRING, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_FMTD_STRING, t);
    EnableMenuItem(InstructionMenu, ELEM_CPRINTF      , t);
    EnableMenuItem(InstructionMenu, ELEM_SPRINTF      , t);
    EnableMenuItem(InstructionMenu, ELEM_FPRINTF      , t);
    EnableMenuItem(InstructionMenu, ELEM_PRINTF       , t);
    EnableMenuItem(InstructionMenu, ELEM_I2C_CPRINTF  , t);
    EnableMenuItem(InstructionMenu, ELEM_ISP_CPRINTF  , t);
    EnableMenuItem(InstructionMenu, ELEM_UART_CPRINTF , t);

    #ifdef USE_SFR
    EnableMenuItem(InstructionMenu, MNU_INSERT_SFR, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SFW, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SSFB, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_csFB, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_TSFB, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_T_C_SFB, t);
    #endif

    EnableMenuItem(InstructionMenu, MNU_INSERT_SPI, t);

    EnableMenuItem(InstructionMenu, MNU_INSERT_BUS, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_7SEG, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_9SEG, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_14SEG, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_16SEG, t);
}

//-----------------------------------------------------------------------------
// Set the enabled state of the undo/redo menus.
//-----------------------------------------------------------------------------
void SetUndoEnabled(BOOL undoEnabled, BOOL redoEnabled)
{
    EnableMenuItem(EditMenu, MNU_UNDO, undoEnabled ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(EditMenu, MNU_REDO, redoEnabled ? MF_ENABLED : MF_GRAYED);
}

//-----------------------------------------------------------------------------
// Create the top-level menu bar for the main window. Mostly static, but we
// create the "select processor" menu from the list in mcutable.h dynamically.
//-----------------------------------------------------------------------------
HMENU MakeMainWindowMenus(void)
{
    HMENU settings, compile, help;
    HMENU ConfigMenu;
    int i;
    // file popup menu
    FileMenu = CreatePopupMenu();
    AppendMenuW(FileMenu, MF_STRING,   MNU_NEW,         _("&New\tCtrl+N"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_OPEN,        _("&Open...\tCtrl+O"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_NOTEPAD_LD,  _("Open ld in notepad\tF4"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_NOTEPAD_HEX, _("Open hex in notepad\tAlt+F6"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_NOTEPAD_ASM, _("Open asm in notepad\tAlt+F3"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_NOTEPAD_C,   _("Open c in notepad"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_NOTEPAD_H,   _("Open h in notepad"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_NOTEPAD_PL,  _("Open pl in notepad\tAlt+F5"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_EXPLORE_DIR, _("Explore ld directory"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_SAVE,        _("&Save\tCtrl+S or F2"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_SAVE_01,     _("Save LDmicro0.1 file format v2.3 compatible"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_SAVE_02,     _("Save LDmicro0.2 file format"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_SAVE_AS,     _("Save &As..."));

    AppendMenuW(FileMenu, MF_SEPARATOR,0,          L"");
    AppendMenuW(FileMenu, MF_STRING,   MNU_EXPORT,      _("&Export As Text...\tCtrl+E"));
    AppendMenuW(FileMenu, MF_STRING,   MNU_NOTEPAD_TXT, _("Open Text in notepad\tF3"));

    AppendMenuW(FileMenu, MF_SEPARATOR,0,          L"");
    AppendMenuW(FileMenu, MF_STRING,   MNU_EXIT,   _("E&xit\tAlt+X"));

    EditMenu = CreatePopupMenu();
    AppendMenuW(EditMenu, MF_STRING, MNU_UNDO, _("&Undo\tCtrl+Z or Alt+Backspace"));
    AppendMenuW(EditMenu, MF_STRING, MNU_REDO, _("&Redo\tCtrl+Y or Alt+Shift+Backspace"));

    AppendMenuW(EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(EditMenu, MF_STRING, MNU_INSERT_RUNG_BEFORE,
        _("Insert Rung &Before\tShift+6"));
    AppendMenuW(EditMenu, MF_STRING, MNU_INSERT_RUNG_AFTER,
        _("Insert Rung &After\tShift+V"));
    AppendMenuW(EditMenu, MF_STRING, MNU_PUSH_RUNG_UP,
        _("Move Selected Rung &Up\tAlt+Up"));
    AppendMenuW(EditMenu, MF_STRING, MNU_PUSH_RUNG_DOWN,
        _("Move Selected Rung &Down\tAlt+Down"));

    AppendMenuW(EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(EditMenu, MF_STRING, MNU_COPY_RUNG_DOWN,
        _("Dup&licate Selected Rung\tCtrl+D"));
    AppendMenuW(EditMenu, MF_STRING, MNU_SELECT_RUNG,
        _("Select Rung's\tShift+Up or Shift+Dn"));
    AppendMenuW(EditMenu, MF_STRING, MNU_SELECT_RUNG,
        _("Select Rung's\tCtrl+Left Mouse Buttton Click"));
    AppendMenuW(EditMenu, MF_STRING, MNU_CUT_RUNG,
        _("Cu&t Rung's\tCtrl+X or Shift+Del"));
    AppendMenuW(EditMenu, MF_STRING, MNU_COPY_RUNG,
        _("&Copy Rung's\tCtrl+C or Ctrl+Insert"));
    AppendMenuW(EditMenu, MF_STRING, MNU_COPY_ELEM,
        _("Copy Selected Element\tInsert"));
    AppendMenuW(EditMenu, MF_STRING, MNU_PASTE_RUNG,
        _("Paste Rung's\tCtrl+V or Shift+Insert"));
    AppendMenuW(EditMenu, MF_STRING, MNU_PASTE_INTO_RUNG,
        _("Paste Rung's/Element &Into Rung\tAlt+Insert"));

    AppendMenuW(EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(EditMenu, MF_STRING, MNU_CUT_ELEMENT,
        _("Cut Selected Element\tAlt+Del"));
    AppendMenuW(EditMenu, MF_STRING, MNU_DELETE_ELEMENT,
        _("&Delete Selected Element\tDel"));
    AppendMenuW(EditMenu, MF_STRING, MNU_DELETE_RUNG,
        _("D&elete Rung\tShift+Del"));

    AppendMenuW(EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(EditMenu, MF_STRING, MNU_REPLACE_ELEMENT,
        _("Replace Selected Element in Group\tSpace"));

    AppendMenuW(EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(EditMenu, MF_STRING, MNU_SCROLL_UP,
        _("Scroll Up\tCtrl+Up"));
    AppendMenuW(EditMenu, MF_STRING, MNU_SCROLL_DOWN,
        _("Scroll Down\tCtrl+Down"));
    AppendMenuW(EditMenu, MF_STRING, MNU_SCROLL_PgUP,
        _("Scroll PgUp\tCtrl+PgUp"));
    AppendMenuW(EditMenu, MF_STRING, MNU_SCROLL_PgDOWN,
        _("Scroll PgDown\tCtrl+PgDown"));
    AppendMenuW(EditMenu, MF_STRING, MNU_ROLL_HOME,
        _("Roll Home\tCtrl+Home"));
    AppendMenuW(EditMenu, MF_STRING, MNU_ROLL_END,
        _("Roll End\tCtrl+End"));
    AppendMenuW(EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(EditMenu, MF_STRING, MNU_TAB,
        _("Moving cursor between the main window and the I/O list\tTab"));

    // instruction popup  menu
    InstructionMenu = CreatePopupMenu();
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_COMMENT,
        _("Insert Co&mment\t;"));

    CourseMenu = CreatePopupMenu();
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_OPEN,
        _("Insert -+        +- Open-Circuit\tShift+Enter"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_SHORT,
        _("Insert -+------+- Short-Circuit\tCtrl+Enter"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_MASTER_RLY,
        _("Insert Master Control Relay"));

    AppendMenuW(CourseMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_LABEL, _("Insert LABEL declaration"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_GOTO, _("Insert GOTO Label or Rung"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_SUBPROG, _("Insert SUBPROG declaration"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_RETURN, _("Insert RETURN"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_ENDSUB, _("Insert ENDSUB declaration"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_GOSUB, _("Insert GOSUB call"));
    AppendMenuW(CourseMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_SLEEP, _("Insert SLEEP"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_CLRWDT, _("Insert CLRWDT"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_LOCK, _("Insert LOCK"));
    AppendMenuW(CourseMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_DELAY, _("Insert DELAY(us)"));
    AppendMenuW(CourseMenu, MF_STRING, MNU_INSERT_TIME2DELAY, _("Insert TIME to DELAY converter"));
    AppendMenuW(InstructionMenu, MF_STRING | MF_POPUP, (UINT_PTR)CourseMenu,_("Operations that change the course of the program"));

    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_CONTACTS,
        _("Insert &Contacts: Input Pin\tC"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_CONT_RELAY,
        _("Insert Contacts: Internal Relay\tShift+C"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_CONT_OUTPUT,
        _("Insert Contacts: Output Pin\tShift+L"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_COIL,
        _("Insert Coi&l: Output Pin\tL"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_COIL_RELAY,
        _("Insert Coil: Internal Relay\tAlt+L"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_MAKE_NORMAL,
        _("Make &Normal\tN"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_NEGATE,
        _("Make &Negated\tN"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_MAKE_SET_ONLY,
        _("Make &Set-Only\tS"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_MAKE_RESET_ONLY,
        _("Make &Reset-Only\tR"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_MAKE_TTRIGGER,
        _("Make T-trigger"));

    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_OSR,
        _("Insert _/OSR/\\_ (One Shot Rising)\t&/"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_OSF,
        _("Insert \\_OSF/\\_ (One Shot Falling)\t&\\ "));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_OSL,
        _("Insert \\_OSL\\/ (One Shot Low)"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_OSC,
        _("Insert _/OSC/\\_/\\_ (Oscillator F=1/(2*Tcycle))"));

    PulseMenu = CreatePopupMenu();
    AppendMenuW(PulseMenu, MF_STRING, MNU_INSERT_NPULSE,     _("EDIT: Insert N PULSE"));
    AppendMenuW(PulseMenu, MF_STRING, MNU_INSERT_PULSER,     _("EDIT: Insert PULSER"));
    AppendMenuW(PulseMenu, MF_STRING, MNU_INSERT_STEPPER,    _("EDIT: Insert STEPPER"));
    AppendMenuW(PulseMenu, MF_STRING, MNU_INSERT_NPULSE_OFF, _("EDIT: Insert N PULSE OFF"));
    AppendMenuW(InstructionMenu, MF_STRING | MF_POPUP, (UINT_PTR)PulseMenu,_("Pulse generators"));

    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_TON,
        _("Insert T&ON (Delayed Turn On)\tO"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_TOF,
        _("Insert TO&F (Delayed Turn Off)\tF"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_RTO,
        _("Insert R&TO (Retentive Delayed Turn On)\tT"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_RTL,
        _("Insert RTL (Retentive Delayed Turn On If Low Input)"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_TCY,
        _("Insert TCY (Cyclic On/Off)"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_THI, _("Insert THI (Hight Delay)"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_TLO, _("Insert TLO (Low Delay)"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_TIME2COUNT, _("Insert TIME to COUNTER converter"));

    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_CTU,
        _("Insert CT&U (Count Up)\tU"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_CTD,
        _("Insert CT&D (Count Down)\tI"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_CTC,
        _("Insert CT&C (Count Circular)\tJ"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_CTR,
        _("Insert CT&R (Count Circular Reversive)\tK"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_RES,
        _("Insert R&ES (Counter/RTO/RTL/PWM Reset)\tE"));

    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
/*
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_EQU,
        _("Insert EQU (Compare for Equals)\t="));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_NEQ,
        _("Insert NEQ (Compare for Not Equals)\t!"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_GRT,
        _("Insert GRT (Compare for Greater Than)\t>"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_GEQ,
        _("Insert GEQ (Compare for Greater Than or Equal)\t."));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_LES,
        _("Insert LES (Compare for Less Than)\t<"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_LEQ,
        _("Insert LEQ (Compare for Less Than or Equal)\t,"));
*/
    CmpMenu = CreatePopupMenu();
    AppendMenuW(CmpMenu, MF_STRING, MNU_INSERT_EQU,
        _("Insert EQU (Compare for Equals)\t="));
    AppendMenuW(CmpMenu, MF_STRING, MNU_INSERT_NEQ,
        _("Insert NEQ (Compare for Not Equals)\t!"));
    AppendMenuW(CmpMenu, MF_STRING, MNU_INSERT_GRT,
        _("Insert GRT (Compare for Greater Than)\t>"));
    AppendMenuW(CmpMenu, MF_STRING, MNU_INSERT_GEQ,
        _("Insert GEQ (Compare for Greater Than or Equal)\t."));
    AppendMenuW(CmpMenu, MF_STRING, MNU_INSERT_LES,
        _("Insert LES (Compare for Less Than)\t<"));
    AppendMenuW(CmpMenu, MF_STRING, MNU_INSERT_LEQ,
        _("Insert LEQ (Compare for Less Than or Equal)\t,"));
    AppendMenuW(CmpMenu, MF_STRING, MNU_INSERT_IF_BIT_SET,    _("Insert Test If Bit Set"));
    AppendMenuW(CmpMenu, MF_STRING, MNU_INSERT_IF_BIT_CLEAR,  _("Insert Test If Bit Clear"));
    AppendMenuW(InstructionMenu, MF_STRING | MF_POPUP, (UINT_PTR)CmpMenu,_("Compare variable"));

/*
    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_MOV,
        _("Insert MOV (Move)\tM"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_ADD,
        _("Insert ADD (16-bit Integer Add)\t+"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_SUB,
        _("Insert SUB (16-bit Integer Subtract)\t-"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_MUL,
        _("Insert MUL (16-bit Integer Multiply)\t*"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_DIV,
        _("Insert DIV (16-bit Integer Divide)\tD"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_MOD,_("TODO: Insert MOD (Integer Divide Remainder)"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_NEG,_("TODO: Insert NEG (Integer Negate)"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_BIN2BCD, _("TODO: Insert BIN2BCD"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_BCD2BIN, _("TODO: Insert BCD2BIN"));
*/
    SignedMenu = CreatePopupMenu();
    AppendMenuW(SignedMenu, MF_STRING, MNU_INSERT_MOV,
        _("Insert MOV (Move)\tM"));
    AppendMenuW(SignedMenu, MF_STRING, MNU_INSERT_ADD,
        _("Insert ADD (16-bit Integer Add)\t+"));
    AppendMenuW(SignedMenu, MF_STRING, MNU_INSERT_SUB,
        _("Insert SUB (16-bit Integer Subtract)\t-"));
    AppendMenuW(SignedMenu, MF_STRING, MNU_INSERT_MUL,
        _("Insert MUL (16-bit Integer Multiply)\t*"));
    AppendMenuW(SignedMenu, MF_STRING, MNU_INSERT_DIV,
        _("Insert DIV (16-bit Integer Divide)\tD"));
    AppendMenuW(SignedMenu, MF_STRING, MNU_INSERT_MOD, _("SIMUL: Insert MOD (Integer Divide Remainder)"));
    AppendMenuW(SignedMenu, MF_STRING, MNU_INSERT_NEG, _("Insert NEG (Integer Negate)"));
    AppendMenuW(SignedMenu, MF_STRING, MNU_INSERT_RANDOM, _("Insert Random"));
    AppendMenuW(SignedMenu, MF_STRING, MNU_INSERT_SEED_RANDOM, _("Insert Seed of Random"));
    AppendMenuW(InstructionMenu, MF_STRING | MF_POPUP, (UINT_PTR)SignedMenu,_("Signed variable operations"));

    BitwiseMenu = CreatePopupMenu();
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_AND,       _("SIMUL: Insert bitwise AND"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_OR ,       _("SIMUL: Insert bitwise OR     |"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_XOR,       _("Insert bitwise XOR  ^"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_NOT,       _("SIMUL: Insert bitwise NOT  ~"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_SHL,       _("SIMUL: Insert SHL << arithmetic,logic shift to the left"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_SHR,       _("SIMUL: Insert SHR >> arithmetic shift to the right"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_SR0,       _("Insert SR0 >> logic shift to the right"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_ROL,       _("SIMUL: Insert ROL cyclic shift to the left"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_ROR,       _("SIMUL: Insert ROR cyclic shift to the right"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_SWAP,      _("Insert SWAP"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_OPPOSITE,  _("SIMUL: Insert OPPOSITE"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_SET_BIT,   _("SIMUL: Insert Set Bit #"));
    AppendMenuW(BitwiseMenu, MF_STRING, MNU_INSERT_CLEAR_BIT, _("SIMUL: Insert Clear Bit #"));
    AppendMenuW(InstructionMenu, MF_STRING | MF_POPUP, (UINT_PTR)BitwiseMenu,_("Bitwise variable operations (Unsigned)"));

    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_SHIFT_REG,
        _("Insert Shift Register"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_LUT,
        _("Insert Look-Up Table"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_PWL,
        _("Insert Piecewise Linear"));

    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_FMTD_STRING,
        _("Insert Formatted String Over &UART"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_UART_SEND,
        _("Insert &UART SEND"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_UART_RECV,
        _("Insert &UART Receive"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_UART_SENDn,
        _("Insert &UART SENDn Variable"));
//  AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_UART_RECVn,
//      _("Insert &UART Receive Variable"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_UART_SEND_READY,
        _("Insert &UART Send: Is ready to send ?"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_UART_RECV_AVAIL,
        _("Insert &UART Receive: Is data available ?"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_SET_PWM,
        _("Insert Set &PWM Output\tP"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_READ_ADC,
        _("Insert &A/D Converter Read\tA"));
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_PERSIST,
        _("Insert Make Persistent"));

    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_SET_PWM_SOFT,
        _("TODO: Insert Software &PWM (AVR136 AppNote)\tP"));

    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(InstructionMenu, MF_STRING, MNU_INSERT_QUAD_ENCOD, _("EDIT: Insert QUAD ENCOD"));
    DisplayMenu = CreatePopupMenu();
    AppendMenuW(DisplayMenu, MF_STRING, MNU_INSERT_BIN2BCD,        _("SIMUL: Insert BIN2BCD"));
    AppendMenuW(DisplayMenu, MF_STRING, MNU_INSERT_BCD2BIN,        _("SIMUL: Insert BCD2BIN"));
    AppendMenuW(DisplayMenu, MF_STRING, MNU_INSERT_BUS,            _("SIMUL: Insert BUS tracer"));
    AppendMenuW(DisplayMenu, MF_STRING, MNU_INSERT_7SEG,           _("SIMUL: Insert char to 7 SEGMENT converter"));
    AppendMenuW(DisplayMenu, MF_STRING, MNU_INSERT_9SEG,           _("SIMUL: Insert char to 9 SEGMENT converter"));
    AppendMenuW(DisplayMenu, MF_STRING, MNU_INSERT_14SEG,          _("SIMUL: Insert char to 14 SEGMENT converter"));
    AppendMenuW(DisplayMenu, MF_STRING, MNU_INSERT_16SEG,          _("SIMUL: Insert char to 16 SEGMENT converter"));
    AppendMenuW(InstructionMenu, MF_STRING | MF_POPUP, (UINT_PTR)DisplayMenu,_("Displays"));

    #ifdef USE_SFR
    AppendMenuW(InstructionMenu, MF_SEPARATOR, 0, NULL);
    // Special function menu
    SpecialFunction = CreatePopupMenu();
    AppendMenuW(SpecialFunction, MF_STRING, MNU_INSERT_SFR,      _("&Insert Read From SFR"));
    AppendMenuW(SpecialFunction, MF_STRING, MNU_INSERT_SFW,      _("&Insert Write To SFR"));
    AppendMenuW(SpecialFunction, MF_STRING, MNU_INSERT_SSFB,     _("&Insert Set Bit In SFR"));
    AppendMenuW(SpecialFunction, MF_STRING, MNU_INSERT_csFB,     _("&Insert Clear Bit In SFR"));
    AppendMenuW(SpecialFunction, MF_STRING, MNU_INSERT_TSFB,     _("&Insert Test If Bit Set in SFR"));
    AppendMenuW(SpecialFunction, MF_STRING, MNU_INSERT_T_C_SFB,  _("&Insert Test If Bit Clear in SFR"));

    AppendMenuW(InstructionMenu, MF_STRING | MF_POPUP, (UINT_PTR)SpecialFunction,_("&Special Function for AVR"));
    #endif

    settings = CreatePopupMenu();
    AppendMenuW(settings, MF_STRING, MNU_MCU_SETTINGS, _("&MCU Parameters...\tCtrl+F5"));
    ProcessorMenu = CreatePopupMenu();
    Core core = SupportedMcus[0].core;
    for(i = 0; i < NUM_SUPPORTED_MCUS; i++) {
        if(core != SupportedMcus[i].core) {
            core = SupportedMcus[i].core ;
            AppendMenuW(ProcessorMenu, MF_SEPARATOR,0,L""); //separate AVR MCU core
        }
        AppendMenuA(ProcessorMenu, MF_STRING, MNU_PROCESSOR_0+i,
            SupportedMcus[i].mcuName);
    }
    AppendMenuW(ProcessorMenu, MF_SEPARATOR,0,L"");
    AppendMenuW(ProcessorMenu, MF_STRING, MNU_PROCESSOR_0+i,
        _("(no microcontroller)"));
    AppendMenuW(settings, MF_STRING | MF_POPUP, (UINT_PTR)ProcessorMenu,
        _("&Microcontroller"));

    ProcessorMenu2 = CreatePopupMenu();
    AppendMenuW(settings, MF_STRING | MF_POPUP, (UINT_PTR)ProcessorMenu2,
        _("Microcontrollers: TODO and DONE"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("DONE: Atmel AVR ATmega32U4 44-Pin packages"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("DONE: Atmel AVR ATmega32 44-Pin packages"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("DONE: Atmel AVR ATmega328 32-Pin packages"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("TODO: Atmel AVR ATtiny85"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("TODO: Atmel AVR ATtinyXXX"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("TODO: Atmel AVR AT90USB646"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("TODO: Atmel AVR AT90USB1286"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("TODO: Atmel AVR AT90USB1287"));
    AppendMenuW(ProcessorMenu2, MF_SEPARATOR,0,L"");
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW_PIC12,_("DONE: Microchip PIC10F200/202/204/206 6-SOT"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW_PIC12,_("DONE: Microchip PIC10F220/222 6-SOT"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("TODO: Microchip PIC12Fxxx"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW_PIC12,_("DONE: Microchip PIC12F629"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW_PIC12,_("DONE: Microchip PIC12F675 8-pin packages"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW_PIC12,_("DONE: Microchip PIC12F683 8-pin packages"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("DONE: Microchip PIC16F72 28-Pin PDIP, SOIC, SSOP"));
    AppendMenuW(ProcessorMenu2, MF_SEPARATOR,0,L"");
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("TODO: Microchip PIC16F1512 - PIC16F1527"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("DONE: Microchip PIC16F1512 28-Pin SPDIP, SOIC, SSOP"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("DONE: Microchip PIC16F1516 28-Pin SPDIP, SOIC, SSOP"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("DONE: Microchip PIC16F1527 64-Pin packages"));
    AppendMenuW(ProcessorMenu2, MF_STRING, MNU_PROCESSOR_NEW,_("TODO: Microchip PIC16F1933 - PIC16F1947"));

    // simulate popup menu
    SimulateMenu = CreatePopupMenu();
    AppendMenuW(SimulateMenu, MF_STRING, MNU_SIMULATION_MODE,
        _("Si&mulation Mode\tCtrl+M or F7"));
    AppendMenuW(SimulateMenu, MF_STRING | MF_GRAYED, MNU_START_SIMULATION,
        _("Start &Real-Time Simulation\tCtrl+R or F8"));
    AppendMenuW(SimulateMenu, MF_STRING | MF_GRAYED, MNU_STOP_SIMULATION,
        _("&Halt Simulation\tCtrl+H or F9"));
    AppendMenuW(SimulateMenu, MF_STRING | MF_GRAYED, MNU_SINGLE_CYCLE,
        _("Single &Cycle\tSpace"));

    compile = CreatePopupMenu();
    AppendMenuW(compile, MF_STRING, MNU_COMPILE,               _("&Compile\tF5"));
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_AS,            _("Compile &As..."));
  //AppendMenuW(compile, MF_STRING, MNU_COMPILE_IHEX,          _("Compile HEX"));
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_IHEX,          _("Compile HEX->ASM"));
    AppendMenuW(compile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_ANSIC,         _("Compile ANSIC"));
    AppendMenuW(compile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_HI_TECH_C,     _("Compile HI-TECH C for PIC"));
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_CCS_PIC_C,     _("Compile CCS C for PIC"));
    AppendMenuW(compile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_GNUC,          _("Compile AVR-GCC, Atmel AVR Toolchain, WinAVR C"));
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_CODEVISIONAVR, _("Compile CodeVisionAVR C"));
    AppendMenuW(compile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_ARDUINO,       _("Compile Sketch for ARDUINO"));
    AppendMenuW(compile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_PASCAL, _("DONE: Compile PASCAL"));
    AppendMenuW(compile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_INT,    _("Compile Interpretable Byte Code"));
    AppendMenuW(compile, MF_STRING, MNU_COMPILE_XINT,   _("Compile Interpretable Extended Byte Code"));
    AppendMenuW(compile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(compile, MF_STRING, MNU_FLASH_BAT,      _("Call flashMcu.bat\tF6"));
    AppendMenuW(compile, MF_STRING, MNU_READ_BAT,       _("Call readMcu.bat\tCtrl+F6"));
    AppendMenuW(compile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(compile, MF_STRING, MNU_CLEAR_BAT,      _("Call clear.bat"));

    ConfigMenu = CreatePopupMenu();
    SchemeMenu = CreatePopupMenu();
    for(i = 0; i < NUM_SUPPORTED_SCHEMES; i++) {
      AppendMenuA(SchemeMenu, MF_STRING, MNU_SCHEME_BLACK+i, Schemes[i].sName);
    }
    AppendMenuW(SchemeMenu, MF_SEPARATOR, 0, L"");
    AppendMenuW(SchemeMenu, MF_STRING, MNU_SELECT_COLOR, _("Select user colors"));
    AppendMenuW(ConfigMenu, MF_STRING | MF_POPUP, (UINT_PTR)SchemeMenu,_("Select color scheme"));

    help = CreatePopupMenu();
    AppendMenuW(help, MF_STRING, MNU_MANUAL, _("&Manual...\tF1"));
    AppendMenuW(help, MF_STRING, MNU_HOW, _("HOW TO:..."));
    AppendMenuW(help, MF_STRING, MNU_ABOUT, _("&About..."));
    AppendMenuW(help, MF_STRING, MNU_RELEASE, _("Releases..."));
    AppendMenuW(help, MF_STRING, MNU_CHANGES, _("Latest release changes..."));
    AppendMenuW(help, MF_STRING, MNU_FORUM, _("LDmicro Forum..."));
    AppendMenuW(help, MF_STRING, MNU_ISSUE, _("Create new issue..."));
    AppendMenuW(help, MF_STRING, MNU_EMAIL, _("E-mail..."));

    TopMenu = CreateMenu();
    AppendMenuW(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)FileMenu, _("&File"));
    AppendMenuW(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)EditMenu, _("&Edit"));
    AppendMenuW(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)settings,
        _("&Settings"));
    AppendMenuW(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)InstructionMenu,
        _("&Instruction"));
    AppendMenuW(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)SimulateMenu,
        _("Si&mulate"));
    AppendMenuW(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)compile,
        _("&Compile"));
    AppendMenuW(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)ConfigMenu, _("Config"));
    AppendMenuW(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)help, _("&Help"));

    return TopMenu;
}

//-----------------------------------------------------------------------------
// Adjust the size and visibility of the scrollbars as necessary, either due
// to a change in the size of the program or a change in the size of the
// window.
//-----------------------------------------------------------------------------
void RefreshScrollbars(void)
{
    SCROLLINFO vert, horiz;
    SetUpScrollbars(&NeedHoriz, &horiz, &vert);
    SetScrollInfo(HorizScrollBar, SB_CTL, &horiz, TRUE);
    SetScrollInfo(VertScrollBar, SB_CTL, &vert, TRUE);

    RECT main;
    GetClientRect(MainWindow, &main);

    if(NeedHoriz) {
        MoveWindow(HorizScrollBar, 0, IoListTop - ScrollHeight - 2,
            main.right - ScrollWidth - 2, ScrollHeight, TRUE);
        ShowWindow(HorizScrollBar, SW_SHOW);
        EnableWindow(HorizScrollBar, TRUE);
    } else {
        ShowWindow(HorizScrollBar, SW_HIDE);
    }
    MoveWindow(VertScrollBar, main.right - ScrollWidth - 2, 1, ScrollWidth,
        NeedHoriz ? (IoListTop - ScrollHeight - 4) : (IoListTop - 3), TRUE);

    MoveWindow(VertScrollBar, main.right - ScrollWidth - 2, 1, ScrollWidth,
        NeedHoriz ? (IoListTop - ScrollHeight - 4) : (IoListTop - 3), TRUE);

    InvalidateRect(MainWindow, NULL, FALSE);
}

//-----------------------------------------------------------------------------
// Respond to a WM_VSCROLL sent to the main window, presumably by the one and
// only vertical scrollbar that it has as a child.
//-----------------------------------------------------------------------------
void VscrollProc(WPARAM wParam)
{
    int prevY = ScrollYOffset;
    switch(LOWORD(wParam)) {
        case SB_LINEUP:
        case SB_PAGEUP:
            if(ScrollYOffset > 0) {
                ScrollYOffset--;
            }
            break;

        case SB_LINEDOWN:
        case SB_PAGEDOWN:
            if(ScrollYOffset < ScrollYOffsetMax) {
                ScrollYOffset++;
            }
            break;

        case SB_TOP:
            ScrollYOffset = 0;
            break;

        case SB_BOTTOM:
            ScrollYOffset = ScrollYOffsetMax;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            ScrollYOffset = HIWORD(wParam);
            break;
    }
    if(prevY != ScrollYOffset) {
        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = ScrollYOffset;
        SetScrollInfo(VertScrollBar, SB_CTL, &si, TRUE);

        InvalidateRect(MainWindow, NULL, FALSE);
    }
}

//-----------------------------------------------------------------------------
// Respond to a WM_HSCROLL sent to the main window, presumably by the one and
// only horizontal scrollbar that it has as a child.
//-----------------------------------------------------------------------------
void HscrollProc(WPARAM wParam)
{
    int prevX = ScrollXOffset;
    switch(LOWORD(wParam)) {
        case SB_LINEUP:
            ScrollXOffset -= FONT_WIDTH;
            break;

        case SB_PAGEUP:
            ScrollXOffset -= POS_WIDTH*FONT_WIDTH;
            break;

        case SB_LINEDOWN:
            ScrollXOffset += FONT_WIDTH;
            break;

        case SB_PAGEDOWN:
            ScrollXOffset += POS_WIDTH*FONT_WIDTH;
            break;

        case SB_TOP:
            ScrollXOffset = 0;
            break;

        case SB_BOTTOM:
            ScrollXOffset = ScrollXOffsetMax;
            break;

        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            ScrollXOffset = HIWORD(wParam);
            break;
    }

    if(ScrollXOffset > ScrollXOffsetMax) ScrollXOffset = ScrollXOffsetMax;
    if(ScrollXOffset < 0) ScrollXOffset = 0;

    if(prevX != ScrollXOffset) {
        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        si.nPos = ScrollXOffset;
        SetScrollInfo(HorizScrollBar, SB_CTL, &si, TRUE);

        InvalidateRect(MainWindow, NULL, FALSE);
    }
}

//-----------------------------------------------------------------------------
void RefreshStatusBar(void)
{
    SendMessageW(StatusBar, SB_SETTEXTW, 0, (LPARAM)_(ProgramChangedNotSaved ? "modified" : "        " ));

    if(Prog.mcu) {
        SendMessageA(StatusBar, SB_SETTEXTA, 1, (LPARAM)Prog.mcu->mcuName);
    } else {
        SendMessageW(StatusBar, SB_SETTEXTW, 1, (LPARAM)_("no MCU selected"));
    }
    wchar_t buf[1024];
    wchar_t Tunits[3];
    wchar_t Funits[3];
    wchar_t F2units[3];
    wchar_t TNunits[3];

    double T = SIprefix(1.0*Prog.cycleTime/1000000, Tunits);

    double F=0;
    double F2=0;
    if(Prog.cycleTime>0) {
        F = SIprefix(1000000.0/Prog.cycleTime, Funits);
        F2 = SIprefix(1000000.0/Prog.cycleTime/2, F2units);
    }

    double TN=SIprefix(1.0*Prog.cycleTime*CyclesCount/1000000, TNunits);

    if(Prog.cycleTime>0) {
        swprintf_s(buf,1000,_("TCycle=%.6g %lss F=%.6g %lsHz F/2=%.6g %lsHz Ncycle=%d T=%.6g %lss"),
            T,Tunits, F,Funits, F2,F2units, CyclesCount, TN,TNunits);
    } else {
        swprintf_s(buf, _("TCycle=%.6g %lss Ncycle=%d T=%.6g %lss"),
            T,Tunits, CyclesCount, TN,TNunits);
    }
    SendMessageW(StatusBar, SB_SETTEXTW, 3, (LPARAM)buf);

    if(Prog.mcu && (Prog.mcu->whichIsa == ISA_NETZER ||
                    Prog.mcu->whichIsa == ISA_PASCAL ||
                    Prog.mcu->whichIsa == ISA_INTERPRETED ||
                    Prog.mcu->whichIsa == ISA_XINTERPRETED))
    {
        wcscpy(buf, L"");
    } else {
        swprintf_s(buf, _("processor clock %.9g MHz"), (double)Prog.mcuClock/1000000.0);
    }
    SendMessageW(StatusBar, SB_SETTEXTW, 2, (LPARAM)buf);
}

//-----------------------------------------------------------------------------
// Cause the status bar and the list view to be in sync with the actual data
// structures describing the settings and the I/O configuration. Listview
// does callbacks to get the strings it displays, so it just needs to know
// how many elements to populate.
//-----------------------------------------------------------------------------
void RefreshControlsToSettings(void)
{
    int i;
    if(!IoListOutOfSync) {
        IoListSelectionPoint = -1;
        for(i = 0; i < Prog.io.count; i++) {
            if(ListView_GetItemState(IoList, i, LVIS_SELECTED)) {
                IoListSelectionPoint = i;
                break;
            }
        }
        IoListSelectionPoint = SendMessage(IoList, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
    }

    ListView_DeleteAllItems(IoList);
    for(i = 0; i < Prog.io.count; i++) {
        LVITEM lvi;
        lvi.mask        = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
        lvi.state       = lvi.stateMask = 0;
        lvi.iItem       = i;
        lvi.iSubItem    = 0;
        lvi.pszText     = LPSTR_TEXTCALLBACK;
        lvi.lParam      = i;

        if(ListView_InsertItem(IoList, &lvi) < 0) oops();
    }

    if(IoListSelectionPoint < 0)
        IoListSelectionPoint = 0;
    if(IoListSelectionPoint >= 0) {
        for(i = 0; i < Prog.io.count; i++) {
            ListView_SetItemState(IoList, i, 0, LVIS_SELECTED);
            ListView_SetItemState(IoList, i, 0, LVIS_FOCUSED);
        }
        ListView_SetItemState(IoList, IoListSelectionPoint, LVIS_SELECTED,
            LVIS_SELECTED);
        ListView_SetItemState(IoList, IoListSelectionPoint, LVIS_FOCUSED,
            LVIS_FOCUSED);
        ListView_EnsureVisible(IoList, IoListSelectionPoint, FALSE);
    }
    IoListOutOfSync = FALSE;

    RefreshStatusBar();

    for(i = 0; i < NUM_SUPPORTED_MCUS; i++) {
        if(&SupportedMcus[i] == Prog.mcu) {
            CheckMenuItem(ProcessorMenu, MNU_PROCESSOR_0+i, MF_CHECKED);
        } else {
            CheckMenuItem(ProcessorMenu, MNU_PROCESSOR_0+i, MF_UNCHECKED);
        }
    }
    // `(no microcontroller)' setting
    if(!Prog.mcu) {
        CheckMenuItem(ProcessorMenu, MNU_PROCESSOR_0+i, MF_CHECKED);
    } else {
        CheckMenuItem(ProcessorMenu, MNU_PROCESSOR_0+i, MF_UNCHECKED);
    }

    for(decltype(scheme) i = 0; i < NUM_SUPPORTED_SCHEMES; i++)
        CheckMenuItem(SchemeMenu, MNU_SCHEME_BLACK+i, (i == scheme) ? MF_CHECKED : MF_UNCHECKED);
}

//-----------------------------------------------------------------------------
// Regenerate the I/O list, keeping the selection in the same place if
// possible.
//-----------------------------------------------------------------------------
void GenerateIoListDontLoseSelection(void)
{
    int i;
    int SaveIoListSelectionPoint = IoListSelectionPoint;
    IoListSelectionPoint = -1;
    for(i = 0; i < Prog.io.count; i++) {
        if(ListView_GetItemState(IoList, i, LVIS_SELECTED)) {
            IoListSelectionPoint = i;
            break;
        }
    }
    IoListSelectionPoint = GenerateIoList(IoListSelectionPoint);

    // can't just update the listview index; if I/O has been added then the
    // new selection point might be out of range till we refill it
    if(IoListSelectionPoint >= 0) {
      if(IoListSelectionPoint != SaveIoListSelectionPoint) {
          IoListOutOfSync = TRUE;
          strcpy(IoListSelectionName, Prog.io.assignment[IoListSelectionPoint].name);
      }
    }
    RefreshControlsToSettings();
}

//-----------------------------------------------------------------------------
// Called when the main window has been resized. Adjust the size of the
// status bar and the listview to reflect the new window size.
//-----------------------------------------------------------------------------
void MainWindowResized(void)
{
    RECT main;
    GetClientRect(MainWindow, &main);

    RECT status;
    GetWindowRect(StatusBar, &status);
    int statusHeight = status.bottom - status.top;

    MoveWindow(StatusBar, 0, main.bottom - statusHeight, main.right,
        statusHeight, TRUE);

    // Make sure that the I/O list can't disappear entirely.
    if(IoListHeight < 30) {
        IoListHeight = 30;
    }
    IoListTop = main.bottom - IoListHeight - statusHeight;
    // Make sure that we can't drag the top of the I/O list above the
    // bottom of the menu bar, because it then becomes inaccessible.
    if(IoListTop < 5) {
        IoListHeight = main.bottom - statusHeight - 5;
        IoListTop = main.bottom - IoListHeight - statusHeight;
    }
    MoveWindow(IoList, 0, IoListTop, main.right, IoListHeight, TRUE);

    RefreshScrollbars();

    InvalidateRect(MainWindow, NULL, FALSE);
}

//-----------------------------------------------------------------------------
// Toggle whether we are in simulation mode. A lot of options are only
// available in one mode or the other.
//-----------------------------------------------------------------------------
void ToggleSimulationMode(BOOL doSimulateOneRung)
{
    InSimulationMode = !InSimulationMode;

    if(InSimulationMode) {
        EnableMenuItem(SimulateMenu, MNU_START_SIMULATION, MF_ENABLED);
        EnableMenuItem(SimulateMenu, MNU_SINGLE_CYCLE, MF_ENABLED);

        EnableMenuItem(FileMenu, MNU_OPEN, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_SAVE, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_SAVE_01, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_SAVE_02, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_SAVE_AS, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_NEW, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_EXPORT, MF_GRAYED);

        EnableMenuItem(TopMenu, 1, MF_GRAYED | MF_BYPOSITION);
        EnableMenuItem(TopMenu, 2, MF_GRAYED | MF_BYPOSITION);
        EnableMenuItem(TopMenu, 3, MF_GRAYED | MF_BYPOSITION);
        EnableMenuItem(TopMenu, 5, MF_GRAYED | MF_BYPOSITION);

        CheckMenuItem(SimulateMenu, MNU_SIMULATION_MODE, MF_CHECKED);

        // Recheck InSimulationMode, because there could have been a compile
        // error, which would have kicked us out of simulation mode.
        if(UartFunctionUsed() && InSimulationMode) {
            ShowUartSimulationWindow();
        }
        if(ClearSimulationData()) {
            SimulateOneCycle(TRUE); // If comment this line, then you can see initial state in ladder diagram. It is same interesting.
        }
    } else {
        RealTimeSimulationRunning = FALSE;
        KillTimer(MainWindow, TIMER_SIMULATE);

        EnableMenuItem(SimulateMenu, MNU_START_SIMULATION, MF_GRAYED);
        EnableMenuItem(SimulateMenu, MNU_STOP_SIMULATION, MF_GRAYED);
        EnableMenuItem(SimulateMenu, MNU_SINGLE_CYCLE, MF_GRAYED);

        EnableMenuItem(FileMenu, MNU_OPEN, MF_ENABLED);
        EnableMenuItem(FileMenu, MNU_SAVE, MF_ENABLED);
        EnableMenuItem(FileMenu, MNU_SAVE_01, MF_ENABLED);
        EnableMenuItem(FileMenu, MNU_SAVE_02, MF_ENABLED);
        EnableMenuItem(FileMenu, MNU_SAVE_AS, MF_ENABLED);
        EnableMenuItem(FileMenu, MNU_NEW, MF_ENABLED);
        EnableMenuItem(FileMenu, MNU_EXPORT, MF_ENABLED);

        EnableMenuItem(TopMenu, 1, MF_ENABLED | MF_BYPOSITION);
        EnableMenuItem(TopMenu, 2, MF_ENABLED | MF_BYPOSITION);
        EnableMenuItem(TopMenu, 3, MF_ENABLED | MF_BYPOSITION);
        EnableMenuItem(TopMenu, 5, MF_ENABLED | MF_BYPOSITION);

        CheckMenuItem(SimulateMenu, MNU_SIMULATION_MODE, MF_UNCHECKED);

        if(UartFunctionUsed()) {
            DestroyUartSimulationWindow();
        }
    }

    UpdateMainWindowTitleBar();

    DrawMenuBar(MainWindow);
    InvalidateRect(MainWindow, NULL, FALSE);
    ListView_RedrawItems(IoList, 0, Prog.io.count - 1);
}

void ToggleSimulationMode()
{
    ToggleSimulationMode(FALSE);
}

//-----------------------------------------------------------------------------
// Start real-time simulation. Have to update the controls grayed status
// to reflect this.
//-----------------------------------------------------------------------------
void StartSimulation(void)
{
    RealTimeSimulationRunning = TRUE;

    EnableMenuItem(SimulateMenu, MNU_START_SIMULATION, MF_GRAYED);
    EnableMenuItem(SimulateMenu, MNU_STOP_SIMULATION, MF_ENABLED);
    StartSimulationTimer();

    UpdateMainWindowTitleBar();
}

//-----------------------------------------------------------------------------
// Stop real-time simulation. Have to update the controls grayed status
// to reflect this.
//-----------------------------------------------------------------------------
void StopSimulation(void)
{
    RealTimeSimulationRunning = FALSE;

    EnableMenuItem(SimulateMenu, MNU_START_SIMULATION, MF_ENABLED);
    EnableMenuItem(SimulateMenu, MNU_STOP_SIMULATION, MF_GRAYED);
    KillTimer(MainWindow, TIMER_SIMULATE);

    UpdateMainWindowTitleBar();
}
