//-----------------------------------------------------------------------------
// Copyright 2017 Ihor Nehrutsa
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
// Dialog for setting the colors of LDmicro.
//-----------------------------------------------------------------------------
#include "stdafx.h"

#include "ldmicro.h"

static HWND ColorDialog;
static HWND ColorList;

static HWND ChooseButton;
static HWND UnChooseButton;
static HWND AgainButton;
static HWND RevertButton;

static LONG_PTR PrevColorDialogProc;

static DWORD                     schemeSave;
static SyntaxHighlightingColours SchemeSave;
static SyntaxHighlightingColours OrigSchemeSave;
static DWORD                     rgbResult = 0;

//-----------------------------------------------------------------------------
// Don't allow any characters other than 0-9. in the text boxes.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNumberProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    /*
    if(msg == WM_CHAR) {
        if(hwnd == ConfigBitsTextbox) {
            if(!(ishobdigit(wParam) || wParam == '\b'))
                return 0;
        } else
            if(!(isdigit(wParam) || wParam == '.' || wParam == '\b'))
                return 0;
    }

    LONG_PTR t;
    if(hwnd == CrystalTextbox)
        t = PrevCrystalProc;
    else if(hwnd == ConfigBitsTextbox)
        t = PrevConfigBitsProc;
    else if(hwnd == CycleTextbox)
        t = PrevColorDialogProc;
    else if(hwnd == BaudTextbox)
        t = PrevBaudProc;
    else
        oops();
*/
    //  return CallWindowProc((WNDPROC)t, hwnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
BOOL ChooseClr(DWORD *rgbCurrent)
{
    CHOOSECOLOR cc;

    static COLORREF acrCustClr[16];
    ZeroMemory(&acrCustClr, sizeof(acrCustClr));

    acrCustClr[0] = HighlightColours.bg;
    acrCustClr[1] = HighlightColours.def;
    acrCustClr[2] = HighlightColours.selected;
    acrCustClr[3] = HighlightColours.op;
    acrCustClr[4] = HighlightColours.punct;
    acrCustClr[5] = HighlightColours.lit;
    acrCustClr[6] = HighlightColours.name;
    acrCustClr[7] = HighlightColours.rungNum;
    acrCustClr[8] = HighlightColours.comment;
    acrCustClr[9] = HighlightColours.bus;
    acrCustClr[10] = HighlightColours.simBg;
    acrCustClr[11] = HighlightColours.simRungNum;
    acrCustClr[12] = HighlightColours.simOff;
    acrCustClr[13] = HighlightColours.simOn;
    acrCustClr[14] = HighlightColours.simBusLeft;
    acrCustClr[15] = HighlightColours.simBusRight;

    ZeroMemory(&cc, sizeof(CHOOSECOLOR));
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = ColorDialog;
    cc.lpCustColors = (LPDWORD)acrCustClr;
    cc.rgbResult = *rgbCurrent;
    rgbResult = *rgbCurrent;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT /*| CC_ENABLEHOOK*/;
    // cc.lpfnHook = &CCHookProc;

    if(ChooseColor(&cc) == TRUE) {
        *rgbCurrent = cc.rgbResult;
        rgbResult = cc.rgbResult;
        return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------------
static void doChooseClr()
{
    memcpy(&SchemeSave, &HighlightColours, sizeof(HighlightColours));
    BOOL b = FALSE;
    int  sel = SendMessage(ColorList, LB_GETCURSEL, 0, 0);
    switch(sel) {
        case 0:
            b = ChooseClr(&HighlightColours.bg);
            break;
        case 1:
            b = ChooseClr(&HighlightColours.def);
            break;
        case 2:
            b = ChooseClr(&HighlightColours.selected);
            break;
        case 3:
            b = ChooseClr(&HighlightColours.op);
            break;
        case 4:
            b = ChooseClr(&HighlightColours.punct);
            break;
        case 5:
            b = ChooseClr(&HighlightColours.lit);
            break;
        case 6:
            b = ChooseClr(&HighlightColours.name);
            break;
        case 7:
            b = ChooseClr(&HighlightColours.rungNum);
            break;
        case 8:
            b = ChooseClr(&HighlightColours.comment);
            break;
        case 9:
            b = ChooseClr(&HighlightColours.bus);
            break;
        case 10:
            b = ChooseClr(&HighlightColours.simBg);
            break;
        case 11:
            b = ChooseClr(&HighlightColours.simRungNum);
            break;
        case 12:
            b = ChooseClr(&HighlightColours.simOff);
            break;
        case 13:
            b = ChooseClr(&HighlightColours.simOn);
            break;
        case 14:
            b = ChooseClr(&HighlightColours.simBusLeft);
            break;
        case 15:
            b = ChooseClr(&HighlightColours.simBusRight);
            break;
        default:
            oops();
    }
    if(b) {
        InitBrushesForDrawing();
        InvalidateRect(MainWindow, nullptr, FALSE);
    }
}
//-----------------------------------------------------------------------------
static LRESULT CALLBACK ColorDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_COMMAND: {
            if(wParam == BN_CLICKED) {
                HWND h = (HWND)lParam;
                int  sel = SendMessage(ColorList, LB_GETCURSEL, 0, 0);
                if(h == ChooseButton) {
                    doChooseClr();
                    return 1;
                } else if(h == UnChooseButton) {
                    switch(sel) {
                        case 0:
                            HighlightColours.bg = SchemeSave.bg;
                            break;
                        case 1:
                            HighlightColours.def = SchemeSave.def;
                            break;
                        case 2:
                            HighlightColours.selected = SchemeSave.selected;
                            break;
                        case 3:
                            HighlightColours.op = SchemeSave.op;
                            break;
                        case 4:
                            HighlightColours.punct = SchemeSave.punct;
                            break;
                        case 5:
                            HighlightColours.lit = SchemeSave.lit;
                            break;
                        case 6:
                            HighlightColours.name = SchemeSave.name;
                            break;
                        case 7:
                            HighlightColours.rungNum = SchemeSave.rungNum;
                            break;
                        case 8:
                            HighlightColours.comment = SchemeSave.comment;
                            break;
                        case 9:
                            HighlightColours.bus = SchemeSave.bus;
                            break;
                        case 10:
                            HighlightColours.simBg = SchemeSave.simBg;
                            break;
                        case 11:
                            HighlightColours.simRungNum = SchemeSave.simRungNum;
                            break;
                        case 12:
                            HighlightColours.simOff = SchemeSave.simOff;
                            break;
                        case 13:
                            HighlightColours.simOn = SchemeSave.simOn;
                            break;
                        case 14:
                            HighlightColours.simBusLeft = SchemeSave.simBusLeft;
                            break;
                        case 15:
                            HighlightColours.simBusRight = SchemeSave.simBusRight;
                            break;
                        default:
                            oops();
                    }
                    InitBrushesForDrawing();
                    InvalidateRect(MainWindow, nullptr, FALSE);
                    return 1;
                } else if(h == RevertButton) {
                    switch(sel) {
                        case 0:
                            HighlightColours.bg = OrigSchemeSave.bg;
                            break;
                        case 1:
                            HighlightColours.def = OrigSchemeSave.def;
                            break;
                        case 2:
                            HighlightColours.selected = OrigSchemeSave.selected;
                            break;
                        case 3:
                            HighlightColours.op = OrigSchemeSave.op;
                            break;
                        case 4:
                            HighlightColours.punct = OrigSchemeSave.punct;
                            break;
                        case 5:
                            HighlightColours.lit = OrigSchemeSave.lit;
                            break;
                        case 6:
                            HighlightColours.name = OrigSchemeSave.name;
                            break;
                        case 7:
                            HighlightColours.rungNum = OrigSchemeSave.rungNum;
                            break;
                        case 8:
                            HighlightColours.comment = OrigSchemeSave.comment;
                            break;
                        case 9:
                            HighlightColours.bus = OrigSchemeSave.bus;
                            break;
                        case 10:
                            HighlightColours.simBg = OrigSchemeSave.simBg;
                            break;
                        case 11:
                            HighlightColours.simRungNum = OrigSchemeSave.simRungNum;
                            break;
                        case 12:
                            HighlightColours.simOff = OrigSchemeSave.simOff;
                            break;
                        case 13:
                            HighlightColours.simOn = OrigSchemeSave.simOn;
                            break;
                        case 14:
                            HighlightColours.simBusLeft = OrigSchemeSave.simBusLeft;
                            break;
                        case 15:
                            HighlightColours.simBusRight = OrigSchemeSave.simBusRight;
                            break;
                        default:
                            oops();
                    }
                    InitBrushesForDrawing();
                    InvalidateRect(MainWindow, nullptr, FALSE);
                    return 1;
                } else if(h == AgainButton) {
                    switch(sel) {
                        case 0:
                            HighlightColours.bg = rgbResult;
                            break;
                        case 1:
                            HighlightColours.def = rgbResult;
                            break;
                        case 2:
                            HighlightColours.selected = rgbResult;
                            break;
                        case 3:
                            HighlightColours.op = rgbResult;
                            break;
                        case 4:
                            HighlightColours.punct = rgbResult;
                            break;
                        case 5:
                            HighlightColours.lit = rgbResult;
                            break;
                        case 6:
                            HighlightColours.name = rgbResult;
                            break;
                        case 7:
                            HighlightColours.rungNum = rgbResult;
                            break;
                        case 8:
                            HighlightColours.comment = rgbResult;
                            break;
                        case 9:
                            HighlightColours.bus = rgbResult;
                            break;
                        case 10:
                            HighlightColours.simBg = rgbResult;
                            break;
                        case 11:
                            HighlightColours.simRungNum = rgbResult;
                            break;
                        case 12:
                            HighlightColours.simOff = rgbResult;
                            break;
                        case 13:
                            HighlightColours.simOn = rgbResult;
                            break;
                        case 14:
                            HighlightColours.simBusLeft = rgbResult;
                            break;
                        case 15:
                            HighlightColours.simBusRight = rgbResult;
                            break;
                        default:
                            oops();
                    }
                    InitBrushesForDrawing();
                    InvalidateRect(MainWindow, nullptr, FALSE);
                    return 1;
                }
                break;
            }
        }
        default:
            //return CallWindowProc((WNDPROC)PrevColorDialogProc, hwnd, msg, wParam, lParam);
            //return DefWindowProc(hwnd, msg, wParam, lParam);
            break;
    }
    return CallWindowProc((WNDPROC)PrevColorDialogProc, hwnd, msg, wParam, lParam);
    //return 1;
}

//-----------------------------------------------------------------------------
static void MakeControls()
{
    ColorList = CreateWindowEx(WS_EX_CLIENTEDGE,
                               WC_LISTBOX,
                               "",
                               WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                               10,
                               10,
                               455,
                               245,
                               ColorDialog,
                               nullptr,
                               Instance,
                               nullptr);
    FixedFont(ColorList);

    ChooseButton = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Choose a color"),
                                  WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
                                  480,
                                  10,
                                  160,
                                  23,
                                  ColorDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(ChooseButton);

    UnChooseButton = CreateWindowEx(0,
                                    WC_BUTTON,
                                    _("Un-choose a color"),
                                    WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                    480,
                                    40,
                                    160,
                                    23,
                                    ColorDialog,
                                    nullptr,
                                    Instance,
                                    nullptr);
    NiceFont(UnChooseButton);

    AgainButton = CreateWindowEx(0,
                                 WC_BUTTON,
                                 _("Try again a color"),
                                 WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                 480,
                                 70,
                                 160,
                                 23,
                                 ColorDialog,
                                 nullptr,
                                 Instance,
                                 nullptr);
    NiceFont(AgainButton);

    RevertButton = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Revert a color"),
                                  WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  480,
                                  100,
                                  160,
                                  23,
                                  ColorDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(RevertButton);

    OkButton = CreateWindowEx(0,
                              WC_BUTTON,
                              _("Applay user color scheme"),
                              WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
                              480,
                              160,
                              160,
                              23,
                              ColorDialog,
                              nullptr,
                              Instance,
                              nullptr);
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0,
                                  WC_BUTTON,
                                  _("Revert color scheme"),
                                  WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
                                  480,
                                  190,
                                  160,
                                  23,
                                  ColorDialog,
                                  nullptr,
                                  Instance,
                                  nullptr);
    NiceFont(CancelButton);
}

UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    //dbpd(uiMsg)
    return 0;
}

void ShowColorDialog()
{
    schemeSave = scheme;
    memcpy(&OrigSchemeSave, &HighlightColours, sizeof(HighlightColours));
    memcpy(&SchemeSave, &HighlightColours, sizeof(HighlightColours));

    ColorDialog = CreateWindowClient(0,
                                     "LDmicroDialog",
                                     _("Select color for user scheme:"),
                                     WS_OVERLAPPED | WS_SYSMENU,
                                     100,
                                     100,
                                     650,
                                     265,
                                     nullptr,
                                     nullptr,
                                     Instance,
                                     nullptr);

    MakeControls();

    PrevColorDialogProc = SetWindowLongPtr(ColorDialog, GWLP_WNDPROC, (LONG_PTR)ColorDialogProc);
    RECT r;
    GetWindowRect(ColorList, &r);

    int Index = 0;
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Background color"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Default foreground"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Selected element"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("OpCode (like OSR, TON, ADD, ...)"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Punctuation, like square, curly braces, etc."));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Literal number"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Name of an item"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Rung number"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Comment text"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("| The 'bus' at the right and left of screen |"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Background, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Rung number, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("De-energized element, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("Energzied element, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("| The 'bus' at the left of the screen, Simulation mode"));
    SendMessage(ColorList, LB_ADDSTRING, 0, (LPARAM)_("The 'bus' at the right of the screen, Simulation mode |"));

    EnableWindow(MainWindow, FALSE);
    ShowWindow(ColorDialog, TRUE);
    SetFocus(ColorList);

    Index = InSimulationMode ? 10 : 0;
    SendMessage(ColorList, LB_SETCURSEL, (WPARAM)Index, 0);

    MSG   msg;
    DWORD ret;
    DialogDone = FALSE;
    DialogCancel = FALSE;
    while((ret = GetMessage(&msg, nullptr, 0, 0)) && !DialogDone) {
        switch(msg.message) {
            case WM_KEYDOWN: {
                if(msg.wParam == VK_RETURN) {
                    DialogDone = TRUE;
                } else if(msg.wParam == VK_ESCAPE) {
                    DialogDone = TRUE;
                    DialogCancel = TRUE;
                }
                break;
            }
            case WM_LBUTTONDBLCLK:
            case WM_RBUTTONDBLCLK: {
                if(PtInRect(&r, msg.pt)) {
                    doChooseClr();
                }
                break;
            }
        }
        if(IsDialogMessage(ColorDialog, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        scheme = MNU_SCHEME_USER & 0xff;
        memcpy(&Schemes[MNU_SCHEME_USER & 0xff], &HighlightColours, sizeof(HighlightColours));
        RefreshControlsToSettings();
    } else {
        scheme = schemeSave;
        memcpy(&HighlightColours, &OrigSchemeSave, sizeof(HighlightColours));
    }
    InitForDrawing();
    InvalidateRect(MainWindow, nullptr, FALSE);
    EnableWindow(MainWindow, TRUE);
    SetFocus(MainWindow);
    DestroyWindow(ColorDialog);
    return;
}
