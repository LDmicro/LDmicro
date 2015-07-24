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
// Dialog for setting the overall PLC parameters. Mostly this relates to
// timing; to set up the timers we need to know the desired cycle time,
// which is configurable, plus the MCU clock (i.e. crystal frequency).
// Jonathan Westhues, Nov 2004
//-----------------------------------------------------------------------------
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>

#include "ldmicro.h"

static HWND ConfDialog;

static HWND CrystalTextbox;
static HWND CycleTextbox;
static HWND BaudTextbox;

static LONG_PTR PrevCrystalProc;
static LONG_PTR PrevCycleProc;
static LONG_PTR PrevBaudProc;

//-----------------------------------------------------------------------------
// Don't allow any characters other than 0-9. in the text boxes.
//-----------------------------------------------------------------------------
static LRESULT CALLBACK MyNumberProc(HWND hwnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
    if(msg == WM_CHAR) {
        if(!(isdigit(wParam) || wParam == '.' || wParam == '\b')) {
            return 0;
        }
    }

    LONG_PTR t;
    if(hwnd == CrystalTextbox)
        t = PrevCrystalProc;
    else if(hwnd == CycleTextbox)
        t = PrevCycleProc;
    else if(hwnd == BaudTextbox)
        t = PrevBaudProc;
    else
        oops();

    return CallWindowProc((WNDPROC)t, hwnd, msg, wParam, lParam);
}

static void MakeControls(void)
{
    HWND textLabel = CreateWindowEx(0, WC_STATIC, _("Cycle Time (ms):"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        5, 13, 145, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(textLabel);

    CycleTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        155, 12, 85, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(CycleTextbox);

    HWND textLabel2 = CreateWindowEx(0, WC_STATIC,
        _("Crystal Frequency (MHz):"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        0, 43, 150, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(textLabel2);

    CrystalTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        155, 42, 85, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(CrystalTextbox);

    HWND textLabel3 = CreateWindowEx(0, WC_STATIC, _("UART Baud Rate (bps):"),
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE | SS_RIGHT,
        5, 73, 145, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(textLabel3);

    BaudTextbox = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
        WS_CHILD | ES_AUTOHSCROLL | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        155, 72, 85, 21, ConfDialog, NULL, Instance, NULL);
    NiceFont(BaudTextbox);

    if(!UartFunctionUsed()) {   
        EnableWindow(BaudTextbox, FALSE);
        EnableWindow(textLabel3, FALSE);
    }

    if(Prog.mcu && (Prog.mcu->whichIsa == ISA_ANSIC ||
        Prog.mcu->whichIsa == ISA_INTERPRETED)) 
    {
        EnableWindow(CrystalTextbox, FALSE);
        EnableWindow(textLabel2, FALSE);
    }

    OkButton = CreateWindowEx(0, WC_BUTTON, _("OK"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE | BS_DEFPUSHBUTTON,
        258, 11, 70, 23, ConfDialog, NULL, Instance, NULL); 
    NiceFont(OkButton);

    CancelButton = CreateWindowEx(0, WC_BUTTON, _("Cancel"),
        WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | WS_VISIBLE,
        258, 41, 70, 23, ConfDialog, NULL, Instance, NULL); 
    NiceFont(CancelButton);

    char explanation[1024] = "";

    if(UartFunctionUsed()) {
        if(Prog.mcu && Prog.mcu->uartNeeds.rxPin != 0) {
            sprintf(explanation,
                _("Serial (UART) will use pins %d and %d.\r\n\r\n"),
                Prog.mcu->uartNeeds.rxPin, Prog.mcu->uartNeeds.txPin);
        } else {
            strcpy(explanation,
                _("Please select a micro with a UART.\r\n\r\n"));
        }
    } else {
        strcpy(explanation, _("No serial instructions (UART Send/UART Receive) "
            "are in use; add one to program before setting baud rate.\r\n\r\n") 
        );
    }

    strcat(explanation,
        _("The cycle time for the 'PLC' runtime generated by LDmicro is user-"
        "configurable. Very short cycle times may not be achievable due "
        "to processor speed constraints, and very long cycle times may not "
        "be achievable due to hardware overflows. Cycle times between 10 ms "
        "and 100 ms will usually be practical.\r\n\r\n"
        "The compiler must know what speed crystal you are using with the "
        "micro to convert between timing in clock cycles and timing in "
        "seconds. A 4 MHz to 20 MHz crystal is typical; check the speed "
        "grade of the part you are using to determine the maximum allowable "
        "clock speed before choosing a crystal."));

    HWND textLabel4 = CreateWindowEx(0, WC_STATIC, explanation,
        WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        11, 104, 310, 400, ConfDialog, NULL, Instance, NULL);
    NiceFont(textLabel4);

    // Measure the explanation string, so that we know how to size our window
    RECT tr, cr;
    HDC hdc = CreateCompatibleDC(NULL);
    SelectObject(hdc, MyNiceFont);
    SetRect(&tr, 0, 0, 310, 400);
    DrawText(hdc, explanation, -1, &tr, DT_CALCRECT |
                                        DT_LEFT | DT_TOP | DT_WORDBREAK);
    DeleteDC(hdc);
    int h = 104 + tr.bottom + 10;
    SetWindowPos(ConfDialog, NULL, 0, 0, 344, h, SWP_NOMOVE);
    // h is the desired client height, but SetWindowPos includes title bar;
    // so fix it up by hand
    GetClientRect(ConfDialog, &cr);
    int nh = h + (h - (cr.bottom - cr.top));
    SetWindowPos(ConfDialog, NULL, 0, 0, 344, nh, SWP_NOMOVE);


    PrevCycleProc = SetWindowLongPtr(CycleTextbox, GWLP_WNDPROC, 
        (LONG_PTR)MyNumberProc);

    PrevCrystalProc = SetWindowLongPtr(CrystalTextbox, GWLP_WNDPROC, 
        (LONG_PTR)MyNumberProc);

    PrevBaudProc = SetWindowLongPtr(BaudTextbox, GWLP_WNDPROC, 
        (LONG_PTR)MyNumberProc);
}

void ShowConfDialog(void)
{
    // The window's height will be resized later, to fit the explanation text.
    ConfDialog = CreateWindowClient(0, "LDmicroDialog", _("PLC Configuration"),
        WS_OVERLAPPED | WS_SYSMENU,
        100, 100, 0, 0, NULL, NULL, Instance, NULL);

    MakeControls();
   
    char buf[16];
    sprintf(buf, "%.1f", (Prog.cycleTime / 1000.0));
    SendMessage(CycleTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    sprintf(buf, "%.6f", Prog.mcuClock / 1e6);
    SendMessage(CrystalTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    sprintf(buf, "%d", Prog.baudRate);
    SendMessage(BaudTextbox, WM_SETTEXT, 0, (LPARAM)buf);

    EnableWindow(MainWindow, FALSE);
    ShowWindow(ConfDialog, TRUE);
    SetFocus(CycleTextbox);

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

        if(IsDialogMessage(ConfDialog, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(!DialogCancel) {
        char buf[16];
        SendMessage(CycleTextbox, WM_GETTEXT, (WPARAM)sizeof(buf),
            (LPARAM)(buf));
        Prog.cycleTime = (int)(1000*atof(buf) + 0.5);
        if(Prog.cycleTime == 0) {
            Error(_("Zero cycle time not valid; resetting to 10 ms."));
            Prog.cycleTime = 10000;
        }

        SendMessage(CrystalTextbox, WM_GETTEXT, (WPARAM)sizeof(buf),
            (LPARAM)(buf));
        Prog.mcuClock = (int)(1e6*atof(buf) + 0.5);

        SendMessage(BaudTextbox, WM_GETTEXT, (WPARAM)sizeof(buf),
            (LPARAM)(buf));
        Prog.baudRate = atoi(buf);
    }

    EnableWindow(MainWindow, TRUE);
    DestroyWindow(ConfDialog);
    return;
}
