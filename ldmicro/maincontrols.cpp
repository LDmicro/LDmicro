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
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
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
static HMENU        ProcessorMenu;
static HMENU        SimulateMenu;
static HMENU        TopMenu;

// listview used to maintain the list of I/O pins with symbolic names, plus
// the internal relay too
HWND                IoList;
static int          IoListSelectionPoint;
static BOOL         IoListOutOfSync;
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
        lvc.pszText = s; \
        lvc.iOrder = 0; \
        lvc.cx = w; \
        ListView_InsertColumn(hWnd, i, &lvc); \
    } while(0)
    IoList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "", WS_CHILD |
        LVS_REPORT | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS | WS_TABSTOP |
        LVS_SINGLESEL | WS_CLIPSIBLINGS, 
        12, 25, 300, 300, MainWindow, NULL, Instance, NULL);
    ListView_SetExtendedListViewStyle(IoList, LVS_EX_FULLROWSELECT);

    int typeWidth = 85;
    int pinWidth = 100;
    int portWidth = 90;
    
    LV_ADD_COLUMN(IoList, LV_IO_NAME,  250,       _("Name"));
    LV_ADD_COLUMN(IoList, LV_IO_TYPE,  typeWidth, _("Type"));
    LV_ADD_COLUMN(IoList, LV_IO_STATE, 100,       _("State"));
    LV_ADD_COLUMN(IoList, LV_IO_PIN,   pinWidth,  _("Pin on Processor"));
    LV_ADD_COLUMN(IoList, LV_IO_PORT,  portWidth, _("MCU Port"));

    HorizScrollBar = CreateWindowEx(0, WC_SCROLLBAR, "", WS_CHILD |
        SBS_HORZ | SBS_BOTTOMALIGN | WS_VISIBLE | WS_CLIPSIBLINGS, 
        100, 100, 100, 100, MainWindow, NULL, Instance, NULL);
    VertScrollBar = CreateWindowEx(0, WC_SCROLLBAR, "", WS_CHILD |
        SBS_VERT | SBS_LEFTALIGN | WS_VISIBLE | WS_CLIPSIBLINGS, 
        200, 100, 100, 100, MainWindow, NULL, Instance, NULL);
    RECT scroll;
    GetWindowRect(HorizScrollBar, &scroll);
    ScrollHeight = scroll.bottom - scroll.top;
    GetWindowRect(VertScrollBar, &scroll);
    ScrollWidth = scroll.right - scroll.left;

    StatusBar = CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 
        "LDmicro started", MainWindow, 0);
    int edges[] = { 250, 370, -1 };
    SendMessage(StatusBar, SB_SETPARTS, 3, (LPARAM)edges);

    ShowWindow(IoList, SW_SHOW);
}

//-----------------------------------------------------------------------------
// Set up the title bar text for the main window; indicate whether we are in
// simulation or editing mode, and indicate the filename.
//-----------------------------------------------------------------------------
void UpdateMainWindowTitleBar(void)
{
    char line[MAX_PATH+100];
    if(InSimulationMode) {
        if(RealTimeSimulationRunning) {
            strcpy(line, _("LDmicro - Simulation (Running)"));
        } else {
            strcpy(line, _("LDmicro - Simulation (Stopped)"));
        }
    } else {
        strcpy(line, _("LDmicro - Program Editor"));
    }
    if(strlen(CurrentSaveFile) > 0) {
        sprintf(line+strlen(line), " - %s", CurrentSaveFile);
    } else {
        strcat(line, _(" - (not yet saved)"));
    }

    SetWindowText(MainWindow, line);
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
    EnableMenuItem(InstructionMenu, MNU_MAKE_SET_ONLY,
        canSetOnly ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(InstructionMenu, MNU_INSERT_COMMENT,
        canInsertComment ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(EditMenu, MNU_DELETE_ELEMENT,
        canDelete ? MF_ENABLED : MF_GRAYED);

    int t;
    t = canInsertEnd ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(InstructionMenu, MNU_INSERT_COIL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_RES, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_MOV, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_ADD, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SUB, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_MUL, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_DIV, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CTC, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_PERSIST, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_READ_ADC, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SET_PWM, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_MASTER_RLY, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SHIFT_REG, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_LUT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_PWL, t);

    t = canInsertOther ? MF_ENABLED : MF_GRAYED;
    EnableMenuItem(InstructionMenu, MNU_INSERT_TON, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_TOF, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OSR, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OSF, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_RTO, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CONTACTS, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CTU, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_CTD, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_EQU, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_NEQ, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_GRT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_GEQ, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_LES, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_LEQ, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_SHORT, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_OPEN, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_UART_SEND, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_UART_RECV, t);
    EnableMenuItem(InstructionMenu, MNU_INSERT_FMTD_STR, t);
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
    int i;

    FileMenu = CreatePopupMenu();
    AppendMenu(FileMenu, MF_STRING,   MNU_NEW,    _("&New\tCtrl+N"));
    AppendMenu(FileMenu, MF_STRING,   MNU_OPEN,   _("&Open...\tCtrl+O"));
    AppendMenu(FileMenu, MF_STRING,   MNU_SAVE,   _("&Save\tCtrl+S"));
    AppendMenu(FileMenu, MF_STRING,   MNU_SAVE_AS,_("Save &As..."));
    AppendMenu(FileMenu, MF_SEPARATOR,0,          "");
    AppendMenu(FileMenu, MF_STRING,   MNU_EXPORT,
        _("&Export As Text...\tCtrl+E"));
    AppendMenu(FileMenu, MF_SEPARATOR,0,          "");
    AppendMenu(FileMenu, MF_STRING,   MNU_EXIT,   _("E&xit"));

    EditMenu = CreatePopupMenu();
    AppendMenu(EditMenu, MF_STRING, MNU_UNDO, _("&Undo\tCtrl+Z"));
    AppendMenu(EditMenu, MF_STRING, MNU_REDO, _("&Redo\tCtrl+Y"));
    AppendMenu(EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(EditMenu, MF_STRING, MNU_INSERT_RUNG_BEFORE,
        _("Insert Rung &Before\tShift+6"));
    AppendMenu(EditMenu, MF_STRING, MNU_INSERT_RUNG_AFTER,
        _("Insert Rung &After\tShift+V"));
    AppendMenu(EditMenu, MF_STRING, MNU_PUSH_RUNG_UP,
        _("Move Selected Rung &Up\tShift+Up"));
    AppendMenu(EditMenu, MF_STRING, MNU_PUSH_RUNG_DOWN,
        _("Move Selected Rung &Down\tShift+Down"));
    AppendMenu(EditMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(EditMenu, MF_STRING, MNU_DELETE_ELEMENT,
        _("&Delete Selected Element\tDel"));
    AppendMenu(EditMenu, MF_STRING, MNU_DELETE_RUNG,
        _("D&elete Rung\tShift+Del"));

    InstructionMenu = CreatePopupMenu();
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_COMMENT,
        _("Insert Co&mment\t;"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_CONTACTS,
        _("Insert &Contacts\tC"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_OSR,
        _("Insert OSR (One Shot Rising)\t&/"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_OSF,
        _("Insert OSF (One Shot Falling)\t&\\"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_TON,
        _("Insert T&ON (Delayed Turn On)\tO"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_TOF,
        _("Insert TO&F (Delayed Turn Off)\tF"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_RTO,
        _("Insert R&TO (Retentive Delayed Turn On)\tT"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_CTU,
        _("Insert CT&U (Count Up)\tU"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_CTD,
        _("Insert CT&D (Count Down)\tI"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_CTC,
        _("Insert CT&C (Count Circular)\tJ"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_EQU,
        _("Insert EQU (Compare for Equals)\t="));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_NEQ,
        _("Insert NEQ (Compare for Not Equals)"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_GRT,
        _("Insert GRT (Compare for Greater Than)\t>"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_GEQ,
        _("Insert GEQ (Compare for Greater Than or Equal)\t."));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_LES,
        _("Insert LES (Compare for Less Than)\t<"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_LEQ,
        _("Insert LEQ (Compare for Less Than or Equal)\t,"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_OPEN,
        _("Insert Open-Circuit"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_SHORT,
        _("Insert Short-Circuit"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_MASTER_RLY,
        _("Insert Master Control Relay"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_COIL,
        _("Insert Coi&l\tL"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_RES,
        _("Insert R&ES (Counter/RTO Reset)\tE"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_MOV,
        _("Insert MOV (Move)\tM"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_ADD,
        _("Insert ADD (16-bit Integer Add)\t+"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_SUB,
        _("Insert SUB (16-bit Integer Subtract)\t-"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_MUL,
        _("Insert MUL (16-bit Integer Multiply)\t*"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_DIV,
        _("Insert DIV (16-bit Integer Divide)\tD"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_SHIFT_REG,
        _("Insert Shift Register"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_LUT,
        _("Insert Look-Up Table"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_PWL,
        _("Insert Piecewise Linear"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_FMTD_STR,
        _("Insert Formatted String Over UART"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_UART_SEND,
        _("Insert &UART Send"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_UART_RECV,
        _("Insert &UART Receive"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_SET_PWM,
        _("Insert Set PWM Output"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_READ_ADC,
        _("Insert A/D Converter Read\tP"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_INSERT_PERSIST,
        _("Insert Make Persistent"));
    AppendMenu(InstructionMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(InstructionMenu, MF_STRING, MNU_MAKE_NORMAL,
        _("Make Norm&al\tA"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_NEGATE,
        _("Make &Negated\tN"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_MAKE_SET_ONLY,
        _("Make &Set-Only\tS"));
    AppendMenu(InstructionMenu, MF_STRING, MNU_MAKE_RESET_ONLY,
        _("Make &Reset-Only\tR"));

    settings = CreatePopupMenu();
    AppendMenu(settings, MF_STRING, MNU_MCU_SETTINGS, _("&MCU Parameters..."));
    ProcessorMenu = CreatePopupMenu();
    for(i = 0; i < NUM_SUPPORTED_MCUS; i++) {
        AppendMenu(ProcessorMenu, MF_STRING, MNU_PROCESSOR_0+i, 
            SupportedMcus[i].mcuName);
    }
    AppendMenu(ProcessorMenu, MF_STRING, MNU_PROCESSOR_0+i, 
        _("(no microcontroller)"));
    AppendMenu(settings, MF_STRING | MF_POPUP, (UINT_PTR)ProcessorMenu,
        _("&Microcontroller"));

    SimulateMenu = CreatePopupMenu();
    AppendMenu(SimulateMenu, MF_STRING, MNU_SIMULATION_MODE,
        _("Si&mulation Mode\tCtrl+M"));
    AppendMenu(SimulateMenu, MF_STRING | MF_GRAYED, MNU_START_SIMULATION,
        _("Start &Real-Time Simulation\tCtrl+R"));
    AppendMenu(SimulateMenu, MF_STRING | MF_GRAYED, MNU_STOP_SIMULATION,
        _("&Halt Simulation\tCtrl+H"));
    AppendMenu(SimulateMenu, MF_STRING | MF_GRAYED, MNU_SINGLE_CYCLE,
        _("Single &Cycle\tSpace"));

    compile = CreatePopupMenu();
    AppendMenu(compile, MF_STRING, MNU_COMPILE, _("&Compile\tF5"));
    AppendMenu(compile, MF_STRING, MNU_COMPILE_AS, _("Compile &As..."));

    help = CreatePopupMenu();
    AppendMenu(help, MF_STRING, MNU_MANUAL, _("&Manual...\tF1"));
    AppendMenu(help, MF_STRING, MNU_ABOUT, _("&About..."));

    TopMenu = CreateMenu();
    AppendMenu(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)FileMenu, _("&File"));
    AppendMenu(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)EditMenu, _("&Edit"));
    AppendMenu(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)settings,
        _("&Settings"));
    AppendMenu(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)InstructionMenu,
        _("&Instruction"));
    AppendMenu(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)SimulateMenu,
        _("Si&mulate"));
    AppendMenu(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)compile, 
        _("&Compile"));
    AppendMenu(TopMenu, MF_STRING | MF_POPUP, (UINT_PTR)help, _("&Help"));

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
    if(IoListSelectionPoint >= 0) {
        for(i = 0; i < Prog.io.count; i++) {
            ListView_SetItemState(IoList, i, 0, LVIS_SELECTED);
        }
        ListView_SetItemState(IoList, IoListSelectionPoint, LVIS_SELECTED,
            LVIS_SELECTED);
        ListView_EnsureVisible(IoList, IoListSelectionPoint, FALSE);
    }
    IoListOutOfSync = FALSE;

    if(Prog.mcu) {
        SendMessage(StatusBar, SB_SETTEXT, 0, (LPARAM)Prog.mcu->mcuName);
    } else {
        SendMessage(StatusBar, SB_SETTEXT, 0, (LPARAM)_("no MCU selected"));
    }
    char buf[256];
    sprintf(buf, _("cycle time %.2f ms"), (double)Prog.cycleTime/1000.0);
    SendMessage(StatusBar, SB_SETTEXT, 1, (LPARAM)buf);

    if(Prog.mcu && (Prog.mcu->whichIsa == ISA_ANSIC ||
        Prog.mcu->whichIsa == ISA_INTERPRETED))
    {
        strcpy(buf, "");
    } else {
        sprintf(buf, _("processor clock %.4f MHz"),
            (double)Prog.mcuClock/1000000.0);
    }
    SendMessage(StatusBar, SB_SETTEXT, 2, (LPARAM)buf);

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
}

//-----------------------------------------------------------------------------
// Regenerate the I/O list, keeping the selection in the same place if
// possible.
//-----------------------------------------------------------------------------
void GenerateIoListDontLoseSelection(void)
{
    int i;
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
    IoListOutOfSync = TRUE;
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
void ToggleSimulationMode(void)
{
    InSimulationMode = !InSimulationMode;

    if(InSimulationMode) {
        EnableMenuItem(SimulateMenu, MNU_START_SIMULATION, MF_ENABLED);
        EnableMenuItem(SimulateMenu, MNU_SINGLE_CYCLE, MF_ENABLED);

        EnableMenuItem(FileMenu, MNU_OPEN, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_SAVE, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_SAVE_AS, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_NEW, MF_GRAYED);
        EnableMenuItem(FileMenu, MNU_EXPORT, MF_GRAYED);

        EnableMenuItem(TopMenu, 1, MF_GRAYED | MF_BYPOSITION);
        EnableMenuItem(TopMenu, 2, MF_GRAYED | MF_BYPOSITION);
        EnableMenuItem(TopMenu, 3, MF_GRAYED | MF_BYPOSITION);
        EnableMenuItem(TopMenu, 5, MF_GRAYED | MF_BYPOSITION);
    
        CheckMenuItem(SimulateMenu, MNU_SIMULATION_MODE, MF_CHECKED);

        ClearSimulationData();
        // Recheck InSimulationMode, because there could have been a compile
        // error, which would have kicked us out of simulation mode.
        if(UartFunctionUsed() && InSimulationMode) {
            ShowUartSimulationWindow();
        }
    } else {
        RealTimeSimulationRunning = FALSE;
        KillTimer(MainWindow, TIMER_SIMULATE);

        EnableMenuItem(SimulateMenu, MNU_START_SIMULATION, MF_GRAYED);
        EnableMenuItem(SimulateMenu, MNU_STOP_SIMULATION, MF_GRAYED);
        EnableMenuItem(SimulateMenu, MNU_SINGLE_CYCLE, MF_GRAYED);

        EnableMenuItem(FileMenu, MNU_OPEN, MF_ENABLED);
        EnableMenuItem(FileMenu, MNU_SAVE, MF_ENABLED);
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
